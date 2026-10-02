// One authorised musical delta, using the retained v68 app renderer unchanged.
const fs = require('node:fs'), path = require('node:path'), crypto = require('node:crypto');
const assert = require('node:assert/strict'), Module = require('node:module'), zlib = require('node:zlib');
const repo = path.resolve(__dirname, '../../../..');
const music = path.join(repo, 'assets/audio/music/what-the-dark-keeps');
const work = 'C:/Dev/tmp/horde-music-drone-20261003';
const renderer = 'C:/Dev/tmp/horde-music-instrumentation-20261001/chordsmith-render-source';
const method = path.join(repo, 'docs/evidence/2026-10-01-music-render/method/render-sections.cjs');
const jsonName = 'What_the_Dark_Keeps_Pocket_Chordsmith.json', pcsName = 'What_the_Dark_Keeps_PCS1.txt';
const hash = b => crypto.createHash('sha256').update(b).digest('hex');
const json = f => JSON.parse(fs.readFileSync(f));
const put = (f, b) => { fs.mkdirSync(path.dirname(f), { recursive: true }); fs.writeFileSync(f, b, { flag: 'wx' }); };
const decode = b => JSON.parse(Buffer.from(b.toString().trim().slice(5), 'base64url'));
const changed = c => [`melodyTracks${c}`, `melodyHold${c}`];
function delta(before, after) {
  const expected = structuredClone(before);
  for (const c of ['A', 'D']) {
    const notes = expected[`melodyTracks${c}`][2], holds = expected[`melodyHold${c}`][2];
    assert.equal(notes.length, 64); assert.equal(holds.length, 64);
    assert.equal(notes[0], 9); assert.equal(notes[15], c === 'A' ? 9 : 2);
    assert.equal(holds[15], false);
    for (let i = 16; i < 64; ++i) { assert.equal(holds[i], true); assert.equal(notes[i], notes[15]); }
    for (let i = 15; i < 64; ++i) { notes[i] = null; holds[i] = false; }
  }
  assert.deepEqual(after, expected, 'Only the two step-15 held phrases may change');
}
const action = process.argv[2];
if (action === 'prepare') {
  assert(!fs.existsSync(work), 'Existing experiment: inspect it; do not restart');
  const beforeBytes = fs.readFileSync(path.join(music, 'source', jsonName));
  const before = JSON.parse(beforeBytes), manifestBytes = fs.readFileSync(path.join(music, 'asset.manifest.json'));
  assert.equal(hash(beforeBytes), 'f5d4bbc8068ae2865216304efc22337ac190ca5b7732e02a9ed5e9062b7b92f4');
  assert.equal(hash(manifestBytes), 'ea7adbc1c248fc37d705239e6bcd529fbf10206ffb77165051658bce4e438c3e');
  assert.deepEqual(decode(fs.readFileSync(path.join(music, 'source', pcsName))), before);
  const after = structuredClone(before);
  for (const c of ['A', 'D']) for (let i = 15; i < 64; ++i) {
    after[`melodyTracks${c}`][2][i] = null; after[`melodyHold${c}`][2][i] = false;
  }
  delta(before, after);
  // Keep compact arrays and all other source formatting, not a score rewrite.
  let text = beforeBytes.toString();
  for (const c of ['A', 'D']) for (const key of changed(c)) {
    const pattern = new RegExp(`("${key}"\\s*:\\s*\\[\\s*\\[[^\\n]*\\],\\s*\\[[^\\n]*\\],\\s*)(\\[[^\\n]*\\])`);
    assert.equal([...text.matchAll(new RegExp(pattern, 'g'))].length, 1);
    text = text.replace(pattern, (_, prefix) => prefix + JSON.stringify(after[key][2]));
  }
  assert.deepEqual(JSON.parse(text), after);
  const pcs = `PCS1:${Buffer.from(JSON.stringify(after)).toString('base64url')}\n`;
  assert.deepEqual(decode(pcs), after);
  put(path.join(work, 'reference-source.json'), beforeBytes);
  put(path.join(work, 'reference-manifest.json'), manifestBytes);
  put(path.join(work, 'source', jsonName), text); put(path.join(work, 'source', pcsName), pcs);
  console.log(JSON.stringify({ source: hash(Buffer.from(text)), pcs: hash(Buffer.from(pcs)), changed: ['A', 'D'], removed: 'Melody 3 step15 onset and16..63 holds; step0 preserved' }));
} else if (action === 'contract') {
  const before = json(path.join(work, 'reference-source.json')), after = json(path.join(work, 'source', jsonName));
  delta(before, after); assert.deepEqual(decode(fs.readFileSync(path.join(work, 'source', pcsName))), after);
  for (const mutate of [s => { s.bpm++; }, s => { s.melodyTracksA[2][0] = null; },
    s => { s.melodyTracksD[0][2] = null; }, s => { s.melodyHoldA[2][16] = true; }]) {
    const bad = structuredClone(after); mutate(bad); assert.throws(() => delta(before, bad));
  }
  console.log('PASS exact two-phrase contract, JSON/PCS1 parity, four forbidden-change negative checks');
} else if (action === 'render') {
  const output = path.join(work, 'render'); assert(!fs.existsSync(output), 'Do not rerender completed artifacts');
  const bytes = fs.readFileSync(path.join(work, 'source', jsonName));
  delta(json(path.join(work, 'reference-source.json')), JSON.parse(bytes));
  process.env.HORDE_MUSIC_SCORE_BASE64 = bytes.toString('base64');
  process.env.HORDE_MUSIC_SCORE_ZIP_PATH = 'C:/Users/sam_s/Downloads/What_the_Dark_Keeps_Horde_RT_Music_Pack.zip';
  let adapter = fs.readFileSync(method, 'utf8');
  function once(a, b) { assert.equal(adapter.split(a).length, 2); adapter = adapter.replace(a, b); }
  once('require("@playwright/test")', `require(${JSON.stringify(Module.createRequire('C:/Users/sam_s/Documents/Pocket Chordsmith/apps/chordsmith-web/package.json').resolve('@playwright/test'))})`);
  once('const repo = "C:/Users/sam_s/Documents/Pocket Chordsmith";', `const repo = ${JSON.stringify(renderer)};`);
  once('const output = "C:/Dev/tmp/horde-music-render-preparation-20260930/all-cue-render-corrected";', `const output = ${JSON.stringify(output)};`);
  once('score: "bc092a0f7489e52ab1e7e55e42c813ae58517ea4a73e6808de8fbbc71595d4a6"', `score: "${hash(bytes)}"`);
  const start = adapter.indexOf('const cueSpecs = ['), end = adapter.indexOf('];', start) + 2;
  assert(start >= 0 && end > start);
  adapter = adapter.slice(0, start) + 'const cueSpecs = [{id:"A",frames:576000},{id:"D",frames:576000}];' + adapter.slice(end);
  once('state.currentSection = cueId;', "const editedJsonRoundTrip = exportProject({targetSchema:17});\nconst editedPcsRoundTrip = parseShareCode('PCS1:' + utf8ToBase64Url(JSON.stringify(project)));\nstate.currentSection = cueId;");
  once('cue:cueId, errors,', 'cue:cueId, errors, editedJsonRoundTrip, editedPcsRoundTrip,');
  once('no source changes, Core renderer, built-in WAV exporter, normalization, score edits, or generation.', 'no app source changes, Core renderer, built-in WAV exporter, normalization or generation. Only the owner-authorised A/D Melody3 held phrases were removed.');
  put(path.join(output, 'wrapper-config.json'), JSON.stringify({ sourceSha256: hash(bytes), retainedAdapterSha256: hash(fs.readFileSync(method)), effectiveAdapterSha256: hash(Buffer.from(adapter)), wrapperSha256: hash(fs.readFileSync(__filename)), renderer }, null, 2) + '\n');
  const run = new Module(__filename, module); run.filename = __filename; run.paths = module.paths; run._compile(adapter, __filename);
} else if (action === 'verify') {
  const before = json(path.join(work, 'reference-source.json')), after = json(path.join(work, 'source', jsonName));
  delta(before, after); assert.deepEqual(decode(fs.readFileSync(path.join(work, 'source', pcsName))), after);
  for (const mutate of [s => { s.bpm++; }, s => { s.melodyTracksA[2][0] = null; },
    s => { s.melodyTracksD[0][2] = null; }, s => { s.melodyHoldA[2][16] = true; }]) {
    const bad = structuredClone(after); mutate(bad); assert.throws(() => delta(before, bad));
  }
  const receipt = json(path.join(work, 'render/all-cues.json')), manifest = json(path.join(work, 'reference-manifest.json'));
  assert.deepEqual(receipt.renders.map(r => r.cue), ['A', 'D']);
  const results = [];
  for (const row of receipt.renders) {
    const c = row.cue, r = row.render;
    const reference = c === 'A' ? 'docs/evidence/2026-10-01-music-instrumentation/receipts/whistle.json.gz' : 'docs/evidence/2026-10-01-music-whistle-bank/render/all-cues.json.gz';
    const old = JSON.parse(zlib.gunzipSync(fs.readFileSync(path.join(repo, reference)))).renders.find(x => x.cue === c).render;
    assert.deepEqual(row.pageErrors, []); assert.deepEqual(r.errors, []);
    assert(r.sourceMask.melodyStartsMatch && r.sourceMask.holdsMatch && r.calls.leadCallsMatch && r.calls.chordCallsMatch && r.calls.bassCallsMatch);
    assert(Object.values(r.calls.drumCountsMatch).every(Boolean)); assert.deepEqual(r.schedule.steps, old.schedule.steps);
    const expected = old.calls.expectedLeadPhraseCalls.filter(x => !(x.trackIndex === 2 && x.step === 15));
    assert.deepEqual(r.calls.expectedLeadPhraseCalls, expected);
    assert.deepEqual(r.calls.actualLeadPhraseCalls, expected.map(({midi,time,duration,instrument}) => ({midi,time,duration,instrument})));
    for (const key of ['actualChordCalls', 'actualBassPhraseCalls', 'actualDrumCounts']) assert.deepEqual(r.calls[key], old.calls[key]);
    for (const key of Object.keys(before)) if (key !== 'projectVersion' && key !== 'title') {
      assert.deepEqual(r.editedJsonRoundTrip[key], after[key]); assert.deepEqual(r.editedPcsRoundTrip[key], after[key]);
    }
    assert.equal(r.audio.metrics.nonFiniteSamples, 0); assert.equal(r.audio.metrics.clippedSamplesAtPcm16Ceiling, 0);
    const cue = manifest.cues.find(x => x.cue === c);
    for (const [kind, part] of [['loop', cue.body], ['tail', cue.tail]]) {
      const b = fs.readFileSync(path.join(work, 'render', `${c}-${kind}.wav`));
      assert.equal(b.length, part.bytes); assert.equal(b.readUInt16LE(20), 1); assert.equal(b.readUInt16LE(22), 2);
      assert.equal(b.readUInt32LE(24), 48000); assert.equal(b.readUInt16LE(34), 16); assert.equal(b.readUInt32LE(40), part.frames * 4);
      assert.equal(hash(b), row.outputs.find(x => x.path === `${c}-${kind}.wav`).sha256);
      part.sha256 = hash(b); put(path.join(work, part.path), b);
    }
    results.push({ cue: c, removed: old.calls.expectedLeadPhraseCalls.filter(x => x.trackIndex === 2 && x.step === 15), remainingEventsExact: true, metrics: r.audio.metrics });
  }
  for (const entry of manifest.sources) { const b = fs.readFileSync(path.join(work, entry.path)); entry.bytes = b.length; entry.sha256 = hash(b); }
  for (const cue of manifest.cues.filter(x => !['A', 'D'].includes(x.cue))) for (const part of [cue.body, cue.tail]) assert.equal(hash(fs.readFileSync(path.join(music, part.path))), part.sha256);
  manifest.render.evidenceManifest = 'docs/evidence/2026-10-03-music-drone/SHA256SUMS.json';
  manifest.render.processing = 'Owner-authorised removal only of Melody3 step15 held phrases in A/D. A/D bodies and tails rendered with retained v68 actual app voices/live FX; twelve unaffected WAVs retained byte-exact. All remaining events, timing, voices, FX, cue mapping, lengths and PCM playback unchanged.';
  put(path.join(work, 'asset.manifest.json'), JSON.stringify(manifest, null, 2) + '\n');
  put(path.join(work, 'verification.json'), JSON.stringify({ results, status: 'PASS source/event/PCM; packaged listening pending', manifestSha256: hash(fs.readFileSync(path.join(work, 'asset.manifest.json'))), unchangedWavs: 12, pcmBytes: 20160000 }, null, 2) + '\n');
  console.log(fs.readFileSync(path.join(work, 'verification.json'), 'utf8'));
} else if (action === 'admit') {
  assert(!fs.existsSync(path.join(work, 'admission.json')));
  const proof = json(path.join(work, 'verification.json')), manifest = json(path.join(work, 'asset.manifest.json'));
  assert.equal(hash(fs.readFileSync(path.join(work, 'asset.manifest.json'))), proof.manifestSha256);
  assert.equal(hash(fs.readFileSync(path.join(music, 'asset.manifest.json'))), 'ea7adbc1c248fc37d705239e6bcd529fbf10206ffb77165051658bce4e438c3e');
  const old = json(path.join(work, 'reference-manifest.json'));
  for (const e of [...old.sources, ...old.cues.flatMap(x => [x.body, x.tail])]) assert.equal(hash(fs.readFileSync(path.join(music, e.path))), e.sha256);
  for (const e of [...manifest.sources, ...manifest.cues.filter(x => ['A', 'D'].includes(x.cue)).flatMap(x => [x.body, x.tail])]) {
    const b = fs.readFileSync(path.join(work, e.path)); assert.equal(hash(b), e.sha256); fs.copyFileSync(path.join(work, e.path), path.join(music, e.path));
  }
  fs.copyFileSync(path.join(work, 'asset.manifest.json'), path.join(music, 'asset.manifest.json'));
  put(path.join(work, 'admission.json'), JSON.stringify({ manifestSha256: proof.manifestSha256, changedWavs: 4, unchangedWavs: 12, packagedValidation: 'pending' }, null, 2) + '\n');
  console.log(proof.manifestSha256);
} else throw new Error('Use prepare, render, verify, admit once each');
