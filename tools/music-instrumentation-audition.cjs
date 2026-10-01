// Investigation-only wrapper over the retained app-voice/live-FX render adapter.
// No synth recipes or scheduler implementation are copied or replaced here.
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const Module = require('node:module');
const assert = require('node:assert/strict');

const repo = path.resolve(__dirname, '..');
const root = 'C:/Dev/tmp/horde-music-instrumentation-20261001';
const renderRepo = path.join(root, 'chordsmith-render-source');
const dependencyRepo = 'C:/Users/sam_s/Documents/Pocket Chordsmith/apps/chordsmith-web';
const archive = 'C:/Users/sam_s/Downloads/What_the_Dark_Keeps_Horde_RT_Music_Pack.zip';
const musicRoot = path.join(repo, 'assets/audio/music/what-the-dark-keeps');
const method = path.join(repo, 'docs/evidence/2026-10-01-music-render/method/render-sections.cjs');
const sourceName = 'What_the_Dark_Keeps_Pocket_Chordsmith.json';
const sha = bytes => crypto.createHash('sha256').update(bytes).digest('hex');
const originalBytes = fs.readFileSync(path.join(musicRoot, 'source', sourceName));
const original = JSON.parse(originalBytes);
const originalPcs = fs.readFileSync(path.join(musicRoot, 'source/What_the_Dark_Keeps_PCS1.txt'), 'utf8').trim();
const decodePcs = text => JSON.parse(Buffer.from(text.trim().slice(5), 'base64url').toString('utf8'));
assert(originalPcs.startsWith('PCS1:'));
assert.deepEqual(decodePcs(originalPcs), original);
assert.equal(sha(originalBytes), 'bc092a0f7489e52ab1e7e55e42c813ae58517ea4a73e6808de8fbbc71595d4a6');
assert.equal(sha(fs.readFileSync(archive)), 'e28e5936189919fed25f6208dd7a8b97172eb5f7d25f69732cc7339c45f386fa');
assert.equal(sha(fs.readFileSync(path.join(renderRepo, 'apps/chordsmith-web/pocket_chordsmith_v68_core_bridge.html'))),
  'b266814fff749bd4d7be9d8725e4d7becc2d944122bc2a302602457fa9b6e2cf');

const mutable = new Set(['chordInstrument', 'chordVolume', 'fxChorus', 'fxDelay', 'fxMix',
  'melodyInstrumentsA', 'melodyInstrumentsE']);
function protectedValues(score) {
  return Object.fromEntries(Object.entries(score).filter(([key]) => !mutable.has(key)));
}
function candidate(lead) {
  const score = structuredClone(original);
  Object.assign(score, {chordInstrument:'felt_piano', chordVolume:0.20,
    fxChorus:0, fxDelay:0.04, fxMix:0.22});
  for (const cue of ['A', 'E']) score[`melodyInstruments${cue}`] = [lead, 'soft_pluck', 'mellow_sax'];
  assert.deepEqual(protectedValues(score), protectedValues(original));
  return score;
}
const ownerCombo = structuredClone(original);
Object.assign(ownerCombo, {chordInstrument:'felt_piano', fxChorus:0, fxDelay:0.04, fxMix:0.22});
for (const cue of ['A','E']) ownerCombo[`melodyInstruments${cue}`] = ['soft_pluck','soft_pluck','cowboy_whistle'];
assert.deepEqual(protectedValues(ownerCombo), protectedValues(original));
const definitions = {reference:original, whistle:candidate('cowboy_whistle'), reed:candidate('mellow_sax'),
  'owner-combo':ownerCombo};
const action = process.argv[2], variant = process.argv[3];
function writeNew(file, bytes) { fs.mkdirSync(path.dirname(file), {recursive:true}); fs.writeFileSync(file, bytes, {flag:'wx'}); }

if (action === 'prepare') {
  const rows = [];
  const selected = variant ? [variant] : ['reference','whistle','reed'];
  for (const id of selected) {
    assert(Object.hasOwn(definitions,id),'Unknown variant');
    const score=definitions[id];
    const bytes = id === 'reference' ? originalBytes : Buffer.from(`${JSON.stringify(score, null, 2)}\n`);
    const pcs = `PCS1:${Buffer.from(JSON.stringify(score)).toString('base64url')}\n`;
    assert.deepEqual(decodePcs(pcs), score);
    const directory = path.join(root, id);
    writeNew(path.join(directory, sourceName), bytes);
    writeNew(path.join(directory, 'What_the_Dark_Keeps_PCS1.txt'), pcs);
    rows.push({id, jsonSha256:sha(bytes), pcs1Sha256:sha(Buffer.from(pcs)),
      changedKeys:Object.keys(score).filter(key => JSON.stringify(score[key]) !== JSON.stringify(original[key])),
      protectedValuesIdentical:true, pcs1Equivalent:true});
  }
  writeNew(path.join(root, variant ? `source-variants-${variant}.json` : 'source-variants.json'), `${JSON.stringify({schema:1, runtimeBankChanged:false,
    ownerRights:'Owner-supplied; authorised for Horde use only', sourceSha256:sha(originalBytes),
    wrapperSha256:sha(fs.readFileSync(__filename)), retainedAdapterSha256:sha(fs.readFileSync(method)), rows},null,2)}\n`);
  console.log(JSON.stringify(rows, null, 2));
} else if (action === 'render') {
  if (!Object.hasOwn(definitions, variant)) throw new Error('Select reference, whistle, reed or owner-combo.');
  const directory = path.join(root, variant);
  const scoreBytes = fs.readFileSync(path.join(directory, sourceName));
  const score = JSON.parse(scoreBytes);
  assert.deepEqual(score, definitions[variant]);
  assert.deepEqual(decodePcs(fs.readFileSync(path.join(directory, 'What_the_Dark_Keeps_PCS1.txt'),'utf8')), score);
  const output = path.join(directory, 'render');
  if (fs.existsSync(output)) throw new Error(`Refusing to repeat/overwrite completed render: ${output}`);
  fs.mkdirSync(output);
  process.env.HORDE_MUSIC_SCORE_BASE64 = scoreBytes.toString('base64');
  process.env.HORDE_MUSIC_SCORE_ZIP_PATH = archive;
  let adapter = fs.readFileSync(method, 'utf8');
  function replaceOnce(search, replacement) {
    assert.equal(adapter.split(search).length, 2, `Retained adapter anchor changed: ${search.slice(0,80)}`);
    adapter = adapter.replace(search, replacement);
  }
  const dependencyRequire = Module.createRequire(path.join(dependencyRepo, 'package.json'));
  replaceOnce('require("@playwright/test")', `require(${JSON.stringify(dependencyRequire.resolve('@playwright/test'))})`);
  replaceOnce('const repo = "C:/Users/sam_s/Documents/Pocket Chordsmith";', `const repo = ${JSON.stringify(renderRepo)};`);
  replaceOnce('const output = "C:/Dev/tmp/horde-music-render-preparation-20260930/all-cue-render-corrected";',
    `const output = ${JSON.stringify(output)};`);
  replaceOnce('score: "bc092a0f7489e52ab1e7e55e42c813ae58517ea4a73e6808de8fbbc71595d4a6"', `score: "${sha(scoreBytes)}"`);
  const cueBegin = adapter.indexOf('const cueSpecs = [');
  const cueEnd = adapter.indexOf('];', cueBegin) + 2;
  assert(cueBegin >= 0 && cueEnd > cueBegin);
  adapter = adapter.slice(0,cueBegin) + 'const cueSpecs = [{id:"A",frames:576000},{id:"E",frames:576000}];' + adapter.slice(cueEnd);
  replaceOnce('state.currentSection = cueId;', `const auditionRoundTrip = exportProject({targetSchema:17});
        const auditionPcsRoundTrip = parseShareCode('PCS1:' + utf8ToBase64Url(JSON.stringify(project)));
        state.currentSection = cueId;`);
  replaceOnce('cue:cueId, errors,', 'cue:cueId, errors, auditionRoundTrip, auditionPcsRoundTrip,');
  replaceOnce('no source changes, Core renderer, built-in WAV exporter, normalization, score edits, or generation.',
    'no app source changes, Core renderer, built-in WAV exporter, normalization or generation. Owner-authorized instrumentation-only candidate; A/E audition, not a bank replacement.');
  // Writes a reproducible wrapper/config trace, never upstream app/synth code.
  writeNew(path.join(output, 'wrapper-config.json'), `${JSON.stringify({variant,renderRepo,output,
    sourceSha256:sha(scoreBytes), wrapperSha256:sha(fs.readFileSync(__filename)),
    retainedAdapterSha256:sha(fs.readFileSync(method)), effectiveAdapterSha256:sha(Buffer.from(adapter)),
    route:'retained actual app voices/live FX; no synthesis/scheduler replacement'},null,2)}\n`);
  const runModule = new Module(__filename, module);
  runModule.filename = __filename;
  runModule.paths = module.paths;
  runModule._compile(adapter, __filename);
} else {
  throw new Error('Use prepare [owner-combo] once; then render each selected variant once.');
}
