/** Mechanical import of generated enemy poses. No poses are drawn or warped.
 * Invoke the default export with an installed sharp instance and the options
 * below. Explicit roots are source-cell-local; a single uniform scale applies
 * to every pose. PNG alpha is preserved; only the game's BMP uses a binary key.
 */
import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';

const COLS = 8, ROWS = 6, CELL = 128, WIDTH = COLS * CELL, HEIGHT = ROWS * CELL;
const ROOT = [64, 112], KEY = [255, 0, 255], ALPHA = 128;
const hash = bytes => createHash('sha256').update(bytes).digest('hex');
const stateFor = i => i < 8 ? 'Idle' : i < 16 ? 'Walk' : i < 24 ? 'Attack' :
  i < 32 ? 'Hit' : i < 40 ? 'Dead' : i < 44 ? 'WalkStop' : 'IdleReturn';
const states = [
  ['Idle', 0, 8, true], ['Walk', 8, 8, true], ['Attack', 16, 8, false],
  ['Hit', 24, 8, false], ['Dead', 32, 8, false],
  ['WalkStop', 40, 4, false], ['IdleReturn', 44, 4, false],
].map(([name, first, count, loop]) => ({name, frames: Array.from({length:count}, (_, i) => first+i), loop}));

function boundsOf(data, width, rect) {
  let l=rect[2], t=rect[3], r=rect[0], b=rect[1], count=0, alphaCount=0;
  for(let y=rect[1];y<rect[3];y++) for(let x=rect[0];x<rect[2];x++) {
    const a=data[(y*width+x)*4+3];
    if(a>0) alphaCount++;
    if(a<ALPHA) continue;
    count++;l=Math.min(l,x);t=Math.min(t,y);r=Math.max(r,x+1);b=Math.max(b,y+1);
  }
  return {bounds:count?[l-rect[0],t-rect[1],r-rect[0],b-rect[1]]:[0,0,0,0], opaquePixels:count, alphaPixels:alphaCount};
}

export function bitmap24(rgba, width, height) {
  const stride=(width*3+3)&~3, out=Buffer.alloc(54+stride*height);
  out.write('BM');out.writeUInt32LE(out.length,2);out.writeUInt32LE(54,10);
  out.writeUInt32LE(40,14);out.writeInt32LE(width,18);out.writeInt32LE(height,22);
  out.writeUInt16LE(1,26);out.writeUInt16LE(24,28);out.writeUInt32LE(stride*height,34);
  for(let y=0;y<height;y++) for(let x=0;x<width;x++) {
    const i=(y*width+x)*4, o=54+(height-1-y)*stride+x*3;
    const c=rgba[i+3]>=ALPHA ? [rgba[i],rgba[i+1],rgba[i+2]] : KEY;
    if(rgba[i+3]>=ALPHA) assert(!(c[0]===255&&c[1]===0&&c[2]===255),'Opaque magenta conflicts with transparency key');
    out[o]=c[2];out[o+1]=c[1];out[o+2]=c[0];
  }
  return out;
}

async function contactSheet(sharp, raw, stem, outputPath) {
  // Diagnostic labels and neutral backdrop only; the character pixels are
  // unchanged apart from a nearest-neighbor display enlargement.
  const tileW=192,tileH=216,layers=[];
  for(let i=0;i<48;i++) {
    const pixels=Buffer.alloc(CELL*CELL*4);
    for(let y=0;y<CELL;y++) raw.copy(pixels,y*CELL*4,
      (((Math.floor(i/COLS)*CELL+y)*WIDTH)+(i%COLS)*CELL)*4,
      (((Math.floor(i/COLS)*CELL+y)*WIDTH)+(i%COLS+1)*CELL)*4);
    // The diagnostic reproduces the BMP renderer's binary transparency, while
    // the saved production PNG above retains the original sampled alpha.
    for(let p=3;p<pixels.length;p+=4) pixels[p]=pixels[p]>=ALPHA?255:0;
    layers.push({input:await sharp(pixels,{raw:{width:CELL,height:CELL,channels:4}})
      .resize(tileW,tileW,{kernel:'nearest'}).png().toBuffer(),left:(i%COLS)*tileW,top:Math.floor(i/COLS)*tileH});
    const label=Buffer.from(`<svg width="192" height="24"><text x="8" y="17" font-family="sans-serif" font-size="13" fill="#e3e6ed">${i.toString().padStart(2,'0')} ${stateFor(i)}</text></svg>`);
    layers.push({input:label,left:(i%COLS)*tileW,top:Math.floor(i/COLS)*tileH+tileW});
  }
  await sharp({create:{width:COLS*tileW,height:ROWS*tileH,channels:4,background:'#20232b'}})
    .composite(layers).png().toFile(outputPath);
}

/** options: inputPath, stem, outputDirectory, prompt or promptPath,
 * alignment or alignmentPath, inspectOnly=false, contactPath (optional).
 * Alignment: {scale, frames:[{rootX,groundY}*48], cellRects?:[[l,t,r,b]*48]}.
 */
export default async function importEnemy(sharp, options) {
  assert(typeof sharp==='function','An installed sharp instance is required');
  const {inputPath,stem,outputDirectory}=options;
  assert(/^S[1245]_[A-Za-z]+$/.test(stem),'Use the existing species stem');
  assert(inputPath&&outputDirectory,'inputPath and outputDirectory are required');
  const sourcePath=path.resolve(inputPath),outDir=path.resolve(outputDirectory);
  const sourceBytes=fs.readFileSync(sourcePath);
  assert(sourceBytes.subarray(0,8).equals(Buffer.from([137,80,78,71,13,10,26,10])),'Use the original generated PNG');
  const decoded=await sharp(sourceBytes).ensureAlpha().raw().toBuffer({resolveWithObject:true});
  const {width:sw,height:sh,channels}=decoded.info; assert.equal(channels,4);
  const alignment=options.alignment || (options.alignmentPath ? JSON.parse(fs.readFileSync(options.alignmentPath,'utf8').replace(/^\uFEFF/,'')) : {});
  const cells=alignment.cellRects || Array.from({length:48},(_,i)=>[
    Math.floor((i%COLS)*sw/COLS),Math.floor(Math.floor(i/COLS)*sh/ROWS),
    Math.floor((i%COLS+1)*sw/COLS),Math.floor((Math.floor(i/COLS)+1)*sh/ROWS)]);
  assert.equal(cells.length,48,'Exactly 48 source cells are required');
  const inspections=cells.map((cell,index)=>{
    assert(cell.length===4&&cell.every(Number.isInteger)&&cell[0]>=0&&cell[1]>=0&&cell[2]<=sw&&cell[3]<=sh&&cell[2]>cell[0]&&cell[3]>cell[1],`Bad cell ${index}`);
    const found=boundsOf(decoded.data,sw,cell),cw=cell[2]-cell[0],ch=cell[3]-cell[1];
    return {index,state:stateFor(index),sourceCell:cell,...found,
      touchesCellEdge:found.opaquePixels>0&&(found.bounds[0]===0||found.bounds[1]===0||found.bounds[2]===cw||found.bounds[3]===ch)};
  });
  fs.mkdirSync(outDir,{recursive:true});
  const name=`${stem}_Animated`;
  const inspection={source:sourcePath,sourceSheetSize:[sw,sh],columns:COLS,rows:ROWS,alphaThreshold:ALPHA,frames:inspections};
  fs.writeFileSync(path.join(outDir,`${name}.inspection.json`),JSON.stringify(inspection,null,2)+'\n');
  if(options.inspectOnly) return inspection;
  const scale=alignment.scale ?? Math.min(CELL/(sw/COLS),CELL/(sh/ROWS));
  assert(Number.isFinite(scale)&&scale>0&&scale<=8,'Bad common scale');
  if(alignment.frames) assert.equal(alignment.frames.length,48,'Exactly 48 explicit source roots required');
  const output=Buffer.alloc(WIDTH*HEIGHT*4),frames=[];
  for(let i=0;i<48;i++) {
    const insp=inspections[i],cell=cells[i],cw=cell[2]-cell[0],ch=cell[3]-cell[1];
    assert(insp.opaquePixels>0,`Empty generated pose ${i}`);
    assert(insp.alphaPixels<cw*ch,`Pose ${i} has no transparent background`);
    assert(!insp.touchesCellEdge,`Generated pose ${i} touches its cell edge; review/regenerate it`);
    const sourceRoot=alignment.frames ? [alignment.frames[i].rootX,alignment.frames[i].groundY] : [cw*.5,ch*.875];
    assert(sourceRoot.every(Number.isFinite),`Invalid root ${i}`);
    const projected=[ROOT[0]+(insp.bounds[0]-sourceRoot[0])*scale,ROOT[1]+(insp.bounds[1]-sourceRoot[1])*scale,
      ROOT[0]+(insp.bounds[2]-sourceRoot[0])*scale,ROOT[1]+(insp.bounds[3]-sourceRoot[1])*scale];
    assert(projected[0]>=1&&projected[1]>=1&&projected[2]<=127&&projected[3]<=127,
      `Pose ${i} would clip: ${projected.join(',')}; correct the common scale or reviewed roots`);
    const tx=(i%COLS)*CELL,ty=Math.floor(i/COLS)*CELL;
    for(let y=0;y<CELL;y++) for(let x=0;x<CELL;x++) {
      const sx=Math.floor(sourceRoot[0]+(x+.5-ROOT[0])/scale),sy=Math.floor(sourceRoot[1]+(y+.5-ROOT[1])/scale);
      if(sx<0||sy<0||sx>=cw||sy>=ch) continue;
      const from=((cell[1]+sy)*sw+cell[0]+sx)*4,to=((ty+y)*WIDTH+tx+x)*4;
      decoded.data.copy(output,to,from,from+4);
    }
    const final=boundsOf(output,WIDTH,[tx,ty,tx+CELL,ty+CELL]);
    assert(final.opaquePixels>0,`Pose ${i} vanished at game scale`);
    assert(final.bounds[0]>0&&final.bounds[1]>0&&final.bounds[2]<CELL&&final.bounds[3]<CELL,`Pose ${i} has no output gutter`);
    frames.push({index:i,state:stateFor(i),sourceCell:cell,sourceBounds:insp.bounds,sourceRoot,outputRoot:ROOT,
      projectedSourceBounds:projected,outputBounds:final.bounds,opaquePixels:final.opaquePixels});
  }
  const png=await sharp(output,{raw:{width:WIDTH,height:HEIGHT,channels:4}}).png().toBuffer();
  const bmp=bitmap24(output,WIDTH,HEIGHT),portrait=Buffer.alloc(CELL*CELL*4);
  for(let y=0;y<CELL;y++) output.copy(portrait,y*CELL*4,y*WIDTH*4,(y*WIDTH+CELL)*4);
  const portraitPNG=await sharp(portrait,{raw:{width:CELL,height:CELL,channels:4}}).png().toBuffer();
  const portraitBMP=bitmap24(portrait,CELL,CELL);
  const prompt=options.prompt ?? (options.promptPath ? fs.readFileSync(options.promptPath,'utf8') : '');
  assert(prompt.trim().length>0,'Save the exact generation prompt with the art');
  const layout={schemaVersion:1,species:stem,source:`${name}.source.png`,png:`${name}.png`,bitmap:`${name}.bmp`,
    sourceSha256:hash(sourceBytes),pngSha256:hash(png),bitmapSha256:hash(bmp),sourceSheetSize:[sw,sh],
    columns:COLS,rows:ROWS,frameCount:48,cellSize:CELL,sheetSize:[WIDTH,HEIGHT],nominalFrameHeight:96,
    root:ROOT,sourceFacesRight:true,uniformScale:scale,alphaThreshold:ALPHA,transparencyKey:'#ff00ff',
    impactFrame:options.impactFrame ?? 4,
    generation:{tool:'image_gen',transparentBackground:true,returnedImagePath:options.generationSourcePath ?? sourcePath,
      styleReference:'Source/Concepts/CharacterIdle/approved/master.png',prompt:`${name}.prompt.txt`,promptSha256:hash(Buffer.from(prompt))},
    frameOrder:'row-major, left to right, top to bottom',states,frames,
    portrait:{png:`${stem}_Portrait.png`,bitmap:`${stem}_Portrait.bmp`,pngSha256:hash(portraitPNG),bitmapSha256:hash(portraitBMP),
      sourceFrame:0,sheetSize:[CELL,CELL],method:'Exact normalized frame 0 pixels; no redraw or recolor.'},
    method:'One uniform nearest-neighbor scale with explicit source-cell-local root translation. PNG retains sampled source RGBA; BMP retains RGB at alpha >= 128 with exact magenta elsewhere. No pose synthesis, repainting, or per-frame fitting.'};
  const files=[[layout.source,sourceBytes],[layout.png,png],[layout.bitmap,bmp],
    [layout.portrait.png,portraitPNG],[layout.portrait.bitmap,portraitBMP],
    [`${name}.prompt.txt`,prompt],[`${name}.layout.json`,JSON.stringify(layout,null,2)+'\n'],
    [`${name}.alignment.json`,JSON.stringify({scale,frames:frames.map(f=>({rootX:f.sourceRoot[0],groundY:f.sourceRoot[1]})),cellRects:cells},null,2)+'\n']];
  for(const [file,bytes] of files) fs.writeFileSync(path.join(outDir,file),bytes);
  if(options.contactPath) {
    fs.mkdirSync(path.dirname(path.resolve(options.contactPath)),{recursive:true});
    await contactSheet(sharp,output,stem,path.resolve(options.contactPath));
  }
  return {species:stem,outputDirectory:outDir,frames:48,uniformScale:scale,
    sourceSize:[sw,sh],sheetSize:[WIDTH,HEIGHT],layout:path.join(outDir,`${name}.layout.json`),
    contactPath:options.contactPath?path.resolve(options.contactPath):null};
}
