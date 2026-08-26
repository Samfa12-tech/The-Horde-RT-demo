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

async function taskMetadata(taskId) {
  const response = await request(`/openapi/v2/text-to-3d/${encodeURIComponent(taskId)}`);
  const result = await response.json();
  console.log(JSON.stringify({
    id: result.id,
    type: result.type,
    status: result.status,
    prompt: result.prompt,
    negativePrompt: result.negative_prompt,
    artStyle: result.art_style,
    aiModel: result.ai_model,
    topology: result.topology,
    targetPolycount: result.target_polycount,
    shouldRemesh: result.should_remesh,
    symmetryMode: result.symmetry_mode,
    moderation: result.moderation,
    texturePrompt: result.texture_prompt,
    enablePbr: result.enable_pbr,
    textureImageResolution: result.texture_image_resolution,
    createdAt: result.created_at,
    consumedCredits: result.consumed_credits,
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

async function createRemesh(requestPath) {
  const payload = JSON.parse(await readFile(resolve(requestPath), "utf8"));
  const response = await request("/openapi/v1/remesh", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  const result = await response.json();
  if (typeof result.result !== "string" || result.result.length === 0) {
    throw new Error("Meshy remesh creation returned no task id.");
  }
  console.log(JSON.stringify({ taskId: result.result }));
}

async function remeshStatus(taskId) {
  const response = await request(`/openapi/v1/remesh/${encodeURIComponent(taskId)}`);
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
    hasGlb: typeof result.model_urls?.glb === "string",
    errorCode: result.task_error?.code ?? null,
  }));
}

async function remeshMetadata(taskId) {
  const response = await request(`/openapi/v1/remesh/${encodeURIComponent(taskId)}`);
  const result = await response.json();
  console.log(JSON.stringify({
    id: result.id,
    type: result.type,
    status: result.status,
    targetPolycount: result.target_polycount,
    topology: result.topology,
    resizeHeight: result.resize_height,
    originAt: result.origin_at,
    createdAt: result.created_at,
    consumedCredits: result.consumed_credits,
  }));
}

async function downloadRemesh(taskId, outputPath) {
  const response = await request(`/openapi/v1/remesh/${encodeURIComponent(taskId)}`);
  const result = await response.json();
  if (result.status !== "SUCCEEDED" || typeof result.model_urls?.glb !== "string") {
    throw new Error("Meshy remesh task is not a successful GLB result.");
  }
  const modelResponse = await fetch(result.model_urls.glb);
  if (!modelResponse.ok) throw new Error(`Meshy remesh GLB download failed with HTTP ${modelResponse.status}.`);
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

async function createRig(modelPath, heightMeters) {
  const bytes = await readFile(resolve(modelPath));
  const response = await request("/openapi/v1/rigging", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({
      model_url: `data:model/gltf-binary;base64,${bytes.toString("base64")}`,
      height_meters: Number(heightMeters),
    }),
  });
  const result = await response.json();
  if (typeof result.result !== "string" || result.result.length === 0) {
    throw new Error("Meshy rig creation returned no task id.");
  }
  console.log(JSON.stringify({ taskId: result.result }));
}

async function rigStatus(taskId) {
  const response = await request(`/openapi/v1/rigging/${encodeURIComponent(taskId)}`);
  const result = await response.json();
  console.log(JSON.stringify({
    id: result.id,
    status: result.status,
    progress: result.progress,
    createdAt: result.created_at,
    startedAt: result.started_at,
    finishedAt: result.finished_at,
    consumedCredits: result.consumed_credits,
    hasRiggedGlb: typeof result.result?.rigged_character_glb_url === "string",
    hasWalkingGlb: typeof result.result?.basic_animations?.walking_glb_url === "string",
    hasRunningGlb: typeof result.result?.basic_animations?.running_glb_url === "string",
    errorCode: result.task_error?.code ?? null,
  }));
}

async function downloadRig(taskId, variant, outputPath) {
  const response = await request(`/openapi/v1/rigging/${encodeURIComponent(taskId)}`);
  const result = await response.json();
  const modelUrl = variant === "rigged" ? result.result?.rigged_character_glb_url :
    variant === "walking" ? result.result?.basic_animations?.walking_glb_url : null;
  if (result.status !== "SUCCEEDED" || typeof modelUrl !== "string") {
    throw new Error("Meshy rig task is not a successful requested GLB result.");
  }
  const modelResponse = await fetch(modelUrl);
  if (!modelResponse.ok) throw new Error(`Meshy rig GLB download failed with HTTP ${modelResponse.status}.`);
  const bytes = Buffer.from(await modelResponse.arrayBuffer());
  const resolved = resolve(outputPath);
  await writeFile(resolved, bytes);
  console.log(JSON.stringify({
    taskId,
    variant,
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
case "task-metadata":
  if (args.length !== 1) throw new Error("Usage: meshy-safe-api.mjs task-metadata TASK_ID");
  await taskMetadata(args[0]);
  break;
case "download":
  if (args.length !== 2) throw new Error("Usage: meshy-safe-api.mjs download TASK_ID OUTPUT.glb");
  await download(args[0], args[1]);
  break;
case "create-remesh":
  if (args.length !== 1) throw new Error("Usage: meshy-safe-api.mjs create-remesh REQUEST.json");
  await createRemesh(args[0]);
  break;
case "remesh-status":
  if (args.length !== 1) throw new Error("Usage: meshy-safe-api.mjs remesh-status TASK_ID");
  await remeshStatus(args[0]);
  break;
case "remesh-metadata":
  if (args.length !== 1) throw new Error("Usage: meshy-safe-api.mjs remesh-metadata TASK_ID");
  await remeshMetadata(args[0]);
  break;
case "download-remesh":
  if (args.length !== 2) throw new Error("Usage: meshy-safe-api.mjs download-remesh TASK_ID OUTPUT.glb");
  await downloadRemesh(args[0], args[1]);
  break;
case "create-rig":
  if (args.length !== 2) throw new Error("Usage: meshy-safe-api.mjs create-rig MODEL.glb HEIGHT_METRES");
  await createRig(args[0], args[1]);
  break;
case "rig-status":
  if (args.length !== 1) throw new Error("Usage: meshy-safe-api.mjs rig-status TASK_ID");
  await rigStatus(args[0]);
  break;
case "download-rig":
  if (args.length !== 3) throw new Error("Usage: meshy-safe-api.mjs download-rig TASK_ID rigged|walking OUTPUT.glb");
  await downloadRig(args[0], args[1], args[2]);
  break;
default:
  throw new Error("Usage: meshy-safe-api.mjs balance|create|status|task-metadata|download|create-remesh|remesh-status|remesh-metadata|download-remesh|create-rig|rig-status|download-rig ...");
}
