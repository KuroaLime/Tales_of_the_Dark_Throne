import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import importEnemy from './import_enemy_animation.mjs';

const require=createRequire(import.meta.url);
let sharp;
try {sharp=require('sharp');}
catch {sharp=require(path.resolve(path.dirname(process.execPath),'../node_modules/sharp'));}
if(process.argv.length!==3) throw new Error('Usage: node tests/import_enemy_animation_cli.mjs <options.json>');
const options=JSON.parse(fs.readFileSync(process.argv[2],'utf8').replace(/^\uFEFF/,''));
console.log(JSON.stringify(await importEnemy(sharp,options),null,2));
