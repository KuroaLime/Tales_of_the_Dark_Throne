// Mechanically crop the crest already approved in the popup preview.
// No artwork is generated, recolored or repainted here.
import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {fileURLToPath} from 'node:url';
import {createHash} from 'node:crypto';
const require=createRequire(import.meta.url);
let sharp;try{sharp=require('sharp');}catch{sharp=require(path.resolve(path.dirname(process.execPath),'../node_modules/sharp'));}
const repo=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const archive=path.join(repo,'Source/Concepts/MainStage/approved');
const input=path.join(archive,'popup-panel.png');
const check=process.argv.includes('--check');
const png=await sharp(input).resize({width:320}).extract({left:93,top:0,width:134,height:51}).png().toBuffer();
const {data,info}=await sharp(png).ensureAlpha().raw().toBuffer({resolveWithObject:true});
const bmp=Buffer.alloc(54+info.width*info.height*4);
bmp.write('BM');bmp.writeUInt32LE(bmp.length,2);bmp.writeUInt32LE(54,10);bmp.writeUInt32LE(40,14);
bmp.writeInt32LE(info.width,18);bmp.writeInt32LE(-info.height,22);bmp.writeUInt16LE(1,26);bmp.writeUInt16LE(32,28);bmp.writeUInt32LE(data.length,34);
for(let i=0;i<data.length;i+=4){const a=data[i+3];bmp[54+i]=Math.round(data[i+2]*a/255);bmp[55+i]=Math.round(data[i+1]*a/255);bmp[56+i]=Math.round(data[i]*a/255);bmp[57+i]=a;}
const runtime=path.join(repo,'Tower_Triumph/Tower_Triumph/Source/UI/StarlightCrest.bmp');
if(check){if(!fs.readFileSync(runtime).equals(bmp))throw Error('Crest differs from approved crop');}
else{fs.mkdirSync(path.dirname(runtime),{recursive:true});fs.writeFileSync(runtime,bmp);fs.writeFileSync(path.join(archive,'popup-crest.png'),png);fs.writeFileSync(path.join(archive,'popup-ui-assets.json'),JSON.stringify({source:'popup-panel.png',crop:{resizedWidth:320,left:93,top:0,width:134,height:51},runtime:path.relative(repo,runtime).replaceAll('\\','/'),format:'32-bit premultiplied BGRA, top-down, BI_RGB',sourceSha256:createHash('sha256').update(fs.readFileSync(input)).digest('hex'),sha256:createHash('sha256').update(bmp).digest('hex')},null,2)+'\n');}
console.log('PASS: approved 134x51 premultiplied crest '+(check?'verified':'imported'));
