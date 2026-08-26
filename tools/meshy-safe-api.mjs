#!/usr/bin/env node

import { createHash } from "node:crypto";
import { readFile, writeFile } from "node:fs/promises";
import { basename, resolve } from "node:path";

const baseUrl = "https://api.meshy.ai";
const apiKey = process.env.MESHY_API_KEY;
if (!apiKey) throw new Error("MESHY_API_KEY is not available in the environment.");

const [operation, ...args] = process.argv.slice(2);
const headers = { Authorization: `Bearer ${apiKey}` };

async function request(path, options = {}) {
  const response = await fetch(`${baseUrl}${path}`, {
    ...options,
    headers: { ...headers, ...(options.headers ?? {}) },
  });
  if (!response.ok) {
    throw new Error(`Meshy request failed with HTTP ${response.status}.`);
  }
  return response;
}

async function createTask(requestPath) {
  const payload = JSON.parse(await readFile(resolve(requestPath), "utf8"));
  const response = await request("/openapi/v2/text-to-3d", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  const result = await response.json();
  if (typeof result.result !== "string" || result.result.length === 0) {
    throw new Error("Meshy task creation returned no task id.");
  }
  console.log(JSON.stringify({ taskId: result.result }));
}

async function status(taskId) {
  const response = await request(`/openapi/v2/text-to-3d/${encodeURIComponent(taskId)}`);
  const result = await response.json();
  console.log(JSON.stringify({
    id: result.id,
    type: result.type,
    status: result.status,
    progress: result.progress,
    createdAt: result.created_at,
    startedAt: result.started_at,
    finishedAt: result.finished_at,
    consumedCredits: result.consumed_credits,
    errorCode: result.task_error?.code ?? null,
  }));
}

async function download(taskId, outputPath) {
  const response = await request(`/openapi/v2/text-to-3d/${encodeURIComponent(taskId)}`);
  const result = await response.json();
  if (result.status !== "SUCCEEDED" || typeof result.model_urls?.glb !== "string") {
    throw new Error("Meshy task is not a successful GLB result.");
  }
  const modelResponse = await fetch(result.model_urls.glb);
  if (!modelResponse.ok) throw new Error(`Meshy GLB download failed with HTTP ${modelResponse.status}.`);
  const bytes = Buffer.from(await modelResponse.arrayBuffer());
  const resolved = resolve(outputPath);
  await writeFile(resolved, bytes);
  console.log(JSON.stringify({
    taskId,
    file: basename(resolved),
    bytes: bytes.length,
    sha256: createHash("sha256").update(bytes).digest("hex"),
  }));
}

switch (operation) {
case "balance": {
  const response = await request("/openapi/v1/balance");
  const result = await response.json();
  console.log(JSON.stringify({ balance: result.balance }));
  break;
}
case "create":
  if (args.length !== 1) throw new Error("Usage: meshy-safe-api.mjs create REQUEST.json");
  await createTask(args[0]);
  break;
case "status":
  if (args.length !== 1) throw new Error("Usage: meshy-safe-api.mjs status TASK_ID");
  await status(args[0]);
  break;
case "download":
  if (args.length !== 2) throw new Error("Usage: meshy-safe-api.mjs download TASK_ID OUTPUT.glb");
  await download(args[0], args[1]);
  break;
default:
  throw new Error("Usage: meshy-safe-api.mjs balance|create|status|download ...");
}
