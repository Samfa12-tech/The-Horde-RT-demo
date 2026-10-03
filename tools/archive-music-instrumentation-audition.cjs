// Preserve finite audition evidence without admitting it to runtime assets.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),zlib=require('node:zlib');
const assert=require('node:assert/strict');
const repo=path.resolve(__dirname,'..'),source='C:/Dev/tmp/horde-music-instrumentation-20261001';
const destination=path.join(repo,'docs/evidence/2026-10-01-music-instrumentation');
const manifest=path.join(destination,'SHA256SUMS.json');
if(fs.existsSync(manifest))throw new Error('Refusing to overwrite completed archive.');
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const records=[];
function transfer(from,to,gzip=false){
  const bytes=fs.readFileSync(path.join(source,from)),out=gzip?zlib.gzipSync(bytes):bytes;
  const target=path.join(destination,to);fs.mkdirSync(path.dirname(target),{recursive:true});
  fs.writeFileSync(target,out,{flag:'wx'});assert.equal(sha(fs.readFileSync(target)),sha(out));
  if(gzip)assert.equal(sha(zlib.gunzipSync(fs.readFileSync(target))),sha(bytes));
  records.push({path:to,bytes:out.length,sha256:sha(out),...(gzip?{originalBytes:bytes.length,originalSha256:sha(bytes)}:{})});
}
for(const name of ['source-variants.json','source-variants-owner-combo.json'])transfer(name,name);
for(const variant of ['whistle','reed','owner-combo']){
  for(const name of ['What_the_Dark_Keeps_Pocket_Chordsmith.json','What_the_Dark_Keeps_PCS1.txt'])
    transfer(`${variant}/${name}`,`sources/${variant}/${name}`);
  for(const cue of ['A','E'])for(const part of ['loop','tail'])
    transfer(`${variant}/render/${cue}-${part}.wav`,`audio/${variant}/${cue}-${part}.wav`);
  transfer(`${variant}/render/all-cues.json`,`receipts/${variant}.json.gz`,true);
  transfer(`${variant}/render/wrapper-config.json`,`receipts/${variant}-wrapper.json`);
}
transfer('reference/render/all-cues.json','receipts/reference-repeat.json.gz',true);
transfer('reference/render/wrapper-config.json','receipts/reference-repeat-wrapper.json');
transfer('verified-previews/verification.json','verification/initial.json');
transfer('verified-owner-combo/verification.json','verification/owner-combo.json');
transfer('verified-previews-transition-supplement/transition-verification.json','verification/initial-transition-supplement.json');
transfer('verified-owner-combo-transition-supplement/transition-verification.json','verification/owner-combo-transition-supplement.json');
transfer('combat-check/combat-verification.json','verification/combat.json');
transfer('verified-owner-combo/01-owner-combo-comparison.wav','previews/owner-combo.wav');
transfer('verified-previews/01-whistle-comparison.wav','previews/whistle-lead.wav');
transfer('combat-check/owner-combo-combat-context.wav','previews/owner-combo-combat.wav');
for(const name of ['reference-render.log','whistle-render.log','reed-render.log','owner-combo-render.log',
  'verification.log','owner-combo-verification.log','initial-transition-supplement.log','owner-combo-transition-supplement.log',
  'combat-verification.log'])transfer(name,`logs/${name}`);
for(const file of ['music-instrumentation-audition.cjs','verify-music-instrumentation-audition.cjs',
  'music-instrumentation-combat-check.cjs','archive-music-instrumentation-audition.cjs']){
  const bytes=fs.readFileSync(path.join(repo,'tools',file));
  records.push({path:`../../../tools/${file}`,bytes:bytes.length,sha256:sha(bytes),referenceOnly:true});
}
const output={schema:1,scope:'Non-runtime instrumentation audition; no bank replacement or acoustic/listening acceptance.',
  rights:'Owner-supplied; authorised for Horde use only',records,
  retainedReference:'Existing assets/audio/music/what-the-dark-keeps/runtime/A,E body/tail; current bank remains unchanged.',
  renderer:{sourceCommit:'2b87d7b1',htmlSha256:'b266814fff749bd4d7be9d8725e4d7becc2d944122bc2a302602457fa9b6e2cf',
    route:'actual app voices and live FX via retained adapter; app code not vendored'},
  note:'Reed comparison and intermediate matched pieces remain external; its source/body/tail, receipt and completed checks are archived. Earlier transition figures superseded only by named supplements.'};
fs.writeFileSync(manifest,`${JSON.stringify(output,null,2)}\n`,{flag:'wx'});
console.log(JSON.stringify({files:records.filter(x=>!x.referenceOnly).length,bytes:records.filter(x=>!x.referenceOnly).reduce((n,x)=>n+x.bytes,0),
  sha256:sha(fs.readFileSync(manifest)),runtimeBankChanged:false}));
