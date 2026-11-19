import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';

const repo=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const directory=path.join(repo,'Tower_Triumph/Tower_Triumph/Resource/Monster/Approved');
const hash=p=>createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const definitions=[
  ['S1_Skeleton','Skeleton',1],['S1_Leach','Lich',1],
  ['S2_Angel','Angel Guard',2],['S2_BAngel','Baby Angel',2],
  ['S2_Falcon','Sky Falcon',2],['S2_WAngel','Wing Angel',2],
  ['S4_Slime','Sea Slime',4],['S4_Snale','Mermaid',4],
  ['S4_Spear','Spear Crab',4],['S4_WFly','Jellyfish',4],
  ['S5_Dragon','Fire Demon',5],['S5_Golem','Lava Golem',5],
  ['S5_Hound','Lava Hound',5],['S5_Larva','Lava Larva',5],
];
const assets=[];
for(const [species,name,stage] of definitions){
  const layoutFile=`${species}_Animated.layout.json`;
  const layout=JSON.parse(fs.readFileSync(path.join(directory,layoutFile),'utf8').replace(/^\uFEFF/,''));
  assert.equal(layout.species,species);assert.equal(layout.frameCount,48);
  assert.equal(layout.generation.tool,'image_gen');
  assert.equal(layout.impactFrame,species==='S2_BAngel'?5:4);
  for(const [file,expected] of [[layout.source,layout.sourceSha256],[layout.png,layout.pngSha256],
    [layout.bitmap,layout.bitmapSha256],[layout.portrait.png,layout.portrait.pngSha256],
    [layout.portrait.bitmap,layout.portrait.bitmapSha256],[layout.generation.prompt,layout.generation.promptSha256]])
    assert.equal(hash(path.join(directory,file)),expected,`${species}: ${file}`);
  const prompts=fs.readdirSync(directory).filter(p=>p.startsWith(species+'.')&&p.endsWith('.prompt.txt'))
    .map(file=>({file,sha256:hash(path.join(directory,file))}));
  assets.push({species,name,stage,usedInMixedStage3:true,legacyFile:`../${species}.bmp`,
    legacySha256:hash(path.join(directory,'..',`${species}.bmp`)),
    layout:layoutFile,layoutSha256:hash(path.join(directory,layoutFile)),
    source:layout.source,sourceSha256:layout.sourceSha256,
    png:layout.png,pngSha256:layout.pngSha256,bitmap:layout.bitmap,bitmapSha256:layout.bitmapSha256,
    portrait:layout.portrait,generation:layout.generation,prompts,
    alignment:`${species}_Animated.alignment.json`,alignmentSha256:hash(path.join(directory,`${species}_Animated.alignment.json`)),
    impactFrame:layout.impactFrame,uniformScale:layout.uniformScale,neutralBounds:layout.frames[0].outputBounds});
}
const reference='Source/Concepts/CharacterIdle/approved/master.png';
const manifest={schemaVersion:1,title:'Enemy character style and animation pack',
  request:'Match the user-approved allied pixel-art style and add distinct enemy animation states.',
  generationTool:'image_gen',styleReference:{file:reference,sha256:hash(path.join(repo,reference))},
  speciesCount:assets.length,frameCount:assets.length*48,portraitCount:assets.length,
  sheet:{columns:8,rows:6,cellSize:128,width:1024,height:768,root:[64,112],nominalFrameHeight:96,sourceFacesRight:true},
  states:[{name:'Idle',first:0,count:8},{name:'Walk',first:8,count:8},{name:'Attack',first:16,count:8},
    {name:'Hit',first:24,count:8},{name:'Dead',first:32,count:8},{name:'WalkStop',first:40,count:4},
    {name:'IdleReturn',first:44,count:4}],
  transparency:{png:'Original sampled source RGBA preserved.',bitmap:'Uncompressed 24-bit RGB; source alpha >= 128 is opaque, other pixels are exactly #ff00ff.'},
  importMethod:'Uniform nearest-neighbor scaling and reviewed source-cell-local root translations only. Reviewed source cell boundaries may shift into empty gutters to preserve extended weapons. No character repainting, pose synthesis, or per-frame resizing.',
  portraitMethod:'Exact normalized first idle cell, 128 x 128 pixels. Same design and palette as the animated atlas.',
  preserved:'Legacy enemy atlases, allied characters, skill art, map art, AI, skill values, damage, and spawn data are not replaced by this asset pack.',
  assets};
const output=path.join(directory,'Enemy_Animations.manifest.json');
fs.writeFileSync(output,JSON.stringify(manifest,null,2)+'\n');
console.log(JSON.stringify({output,species:manifest.speciesCount,frames:manifest.frameCount,portraits:manifest.portraitCount}));
