import fs from 'node:fs';
import path from 'node:path';
import { createRequire } from 'node:module';

const [inputPath, outputPath, packageRoot] = process.argv.slice(2);
if (!inputPath || !outputPath || !packageRoot) {
  throw new Error('usage: run-gltf-validator.mjs <input.glb> <report.json> <package-root>');
}
const require = createRequire(import.meta.url);
const validator = require(path.resolve(packageRoot, 'node_modules/gltf-validator'));
const bytes = new Uint8Array(fs.readFileSync(inputPath));
const report = await validator.validateBytes(bytes, { uri: path.basename(inputPath) });
fs.writeFileSync(outputPath, `${JSON.stringify(report, null, 2)}\n`, 'utf8');
console.log(`glTF Validator ${report.validatorVersion}: ${report.issues.numErrors} error(s), ${report.issues.numWarnings} warning(s)`);
