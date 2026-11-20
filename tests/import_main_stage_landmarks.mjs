// Reproduce the approved layered world map; no image generation or repainting.
// node tests/import_main_stage_landmarks.mjs [--check]
import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {createRequire} from 'node:module';
import {fileURLToPath} from 'node:url';
const require=createRequire(import.meta.url);
let sharp;try{sharp=require('sharp');}catch{sharp=require(path.resolve(path.dirname(process.execPath),'../node_modules/sharp'));}
const repo=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const archive=path.join(repo,'Source/Concepts/MainStage/approved/landmarks-v1');
const runtime='Tower_Triumph/Tower_Triumph/Source/SMap/landmarks';
const check=process.argv.includes('--check');
const sha=b=>createHash('sha256').update(b).digest('hex');
const sourceManifest=JSON.parse(fs.readFileSync(path.join(archive,'preview-asset-manifest.json'),'utf8'));
const assets={camp:113,graveyard:114,heaven:115,default:116,sea:117,lava:118};
const names={camp:'Camp',graveyard:'Graveyard',heaven:'Heaven',default:'Hall',sea:'Sea',lava:'Lava'};
const sources=[];
for(const [id,record]of Object.entries(sourceManifest.sources)){
 const file=id+'.source.png',b=fs.readFileSync(path.join(archive,file));assert.equal(sha(b),record.sha256,file+' differs from approved generated source');
 sources.push({file,sha256:sha(b),bytes:b.length,generatedSource:record.file});
}
for(const file of ['marker-open.preview.png','marker-closed.preview.png','generation-prompts.json','preview-asset-manifest.json']){const b=fs.readFileSync(path.join(archive,file));sources.push({file,sha256:sha(b),bytes:b.length});}
function bmp(rgba,w,h,premul){
 const depth=premul?32:24,stride=Math.ceil(w*depth/8/4)*4,out=Buffer.alloc(54+stride*h);
 out.write('BM');out.writeUInt32LE(out.length,2);out.writeUInt32LE(54,10);out.writeUInt32LE(40,14);out.writeInt32LE(w,18);out.writeInt32LE(h,22);out.writeUInt16LE(1,26);out.writeUInt16LE(depth,28);out.writeUInt32LE(stride*h,34);out.writeInt32LE(3780,38);out.writeInt32LE(3780,42);
 for(let y=0;y<h;y++)for(let x=0;x<w;x++){const s=(y*w+x)*4,d=54+(h-1-y)*stride+x*depth/8,a=rgba[s+3];if(!premul)assert.equal(a,255,'Clean map must be opaque');for(let k=0;k<3;k++)out[d+2-k]=premul?Math.round(rgba[s+k]*a/255):rgba[s+k];if(premul)out[d+3]=a;}
 return out;
}
function verify(bytes,rgba,w,h,premul){
 assert.equal(bytes.readUInt32LE(14),40);assert.equal(bytes.readUInt32LE(30),0);assert.equal(bytes.readUInt16LE(28),premul?32:24);assert.equal(bytes.readInt32LE(18),w);assert.equal(bytes.readInt32LE(22),h);
 const depth=premul?4:3,stride=Math.ceil(w*depth/4)*4;let transparent=0,partial=0;
 for(let y=0;y<h;y++)for(let x=0;x<w;x++){const s=(y*w+x)*4,d=54+(h-1-y)*stride+x*depth,a=rgba[s+3];if(!a)transparent++;else if(a<255)partial++;for(let k=0;k<3;k++){assert.equal(bytes[d+2-k],premul?Math.floor((rgba[s+k]*a+127)/255):rgba[s+k]);if(premul)assert(bytes[d+2-k]<=a);}if(premul)assert.equal(bytes[d+3],a);}
 return {checkedPixels:w*h,transparentPixels:transparent,partialAlphaPixels:partial,exactSourceRGB:!premul,exactPremultipliedRGBA:premul};
}
const outputs=[];
async function emit(id,pipeline,resourceId,file,bounds,premul){
 const {data,info}=await pipeline.ensureAlpha().raw().toBuffer({resolveWithObject:true});
 const bitmap=bmp(data,info.width,info.height,premul),png=await sharp(data,{raw:{width:info.width,height:info.height,channels:4}}).png().toBuffer();
 const pngName=id+'.png',runtimeFile=runtime+'/'+file;
 if(check){assert(fs.readFileSync(path.join(archive,pngName)).equals(png),pngName+' differs');assert(fs.readFileSync(path.join(repo,runtimeFile)).equals(bitmap),runtimeFile+' differs');}
 else{fs.mkdirSync(path.dirname(path.join(repo,runtimeFile)),{recursive:true});fs.writeFileSync(path.join(archive,pngName),png);fs.writeFileSync(path.join(repo,runtimeFile),bitmap);}
 outputs.push({id,resourceId,png:pngName,pngSha256:sha(png),runtime:runtimeFile,sha256:sha(bitmap),bytes:bitmap.length,width:info.width,height:info.height,bitsPerPixel:premul?32:24,bounds:bounds||null,alphaMode:premul?'Premultiplied BGRA; BITMAPINFOHEADER40/BI_RGB, bottom-up':'Opaque BGR; BITMAPINFOHEADER40/BI_RGB, bottom-up',verification:verify(bitmap,data,info.width,info.height,premul)});
}
for(const [id,resource]of Object.entries(assets)){const bounds=sourceManifest.buildings[id].bounds,[l,t,r,b]=bounds;await emit(id,sharp(path.join(archive,'buildings.source.png')).extract({left:l,top:t,width:r-l,height:b-t}),resource,names[id]+'.bmp',bounds,true);}
for(const [id,rid]of [['open',119],['closed',120]])await emit('marker-'+id,sharp(path.join(archive,'marker-'+id+'.preview.png')),rid,id==='open'?'MarkerOpen.bmp':'MarkerClosed.bmp',null,true);
await emit('clean-map',sharp(path.join(archive,'cleanMap.source.png')),121,'CleanMap.bmp',[0,0,1672,941],false);
const manifest={schemaVersion:1,approval:'User approved the final layered building map preview for actual game integration on 2026-11-20.',mapSize:[1672,941],generator:'built-in imagegen',sources,outputs,render:{buildingPivot:'bounds bottom center',hoverScale:1.05,transitionSeconds:0.18,floatingMarkerScreenSize:44,floatingMarkerRisePixels:4,floatingMarkerPeriodSeconds:3.2,markerInput:false,hitBounds:'fixed registered building rectangle; not expanded or floating marker'},originalOverview:{png:'../world-map.png',runtime:'Tower_Triumph/Tower_Triumph/Source/SMap/background/Map_Background.bmp',resourceId:105,purpose:'Preserved approved full-map overview for region popup and maintenance background; separate from clean-map background.'},conversion:'Building sources cropped at the exact preview bounds without resampling, alpha thresholding, palette edits or repainting. Approved 96px marker PNGs reused exactly. Original source PNGs and prompts archived.',reproduce:{script:'tests/import_main_stage_landmarks.mjs',checkCommand:'node tests/import_main_stage_landmarks.mjs --check'}};
if(check){const old=JSON.parse(fs.readFileSync(path.join(archive,'landmark-assets.json'),'utf8'));assert.deepEqual(old,manifest,'Asset provenance differs from current inputs');}
else fs.writeFileSync(path.join(archive,'landmark-assets.json'),JSON.stringify(manifest,null,2)+'\n');
console.log(JSON.stringify({status:'PASS',mode:check?'check':'import',outputs:outputs.map(x=>({id:x.id,size:[x.width,x.height],resourceId:x.resourceId,pixels:x.verification.checkedPixels}))}));
