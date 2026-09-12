# Horde Lantern RT - Agent Instructions

Native Vulkan hardware-ray-tracing game/tech demo with a historical-gothic visual direction. Android is the primary, first-class target; Windows RTX is an equal validation target.

## Preserve the product and engine contracts

- **RT or nothing.** Keep real `vkCmdTraceRaysKHR` dispatch and RT-produced swapchain presentation. Do not substitute raster-only rendering, baked lighting, screen-space effects, browser WebGPU, or fake RT. Unsupported hardware gets clear diagnostics, not a silent fallback; `rtScene.presented` is true only after successful presentation of an RT-produced frame.
- Build reusable, measured engine capabilities rather than scene-specific tricks, per-object rendering branches, or an imported engine/sample dump. Do not silently reduce render scale or quality to hide a regression.
- Shared 60 Hz `GameSimulation` owns gameplay on the existing application/render thread. Preserve immutable snapshots, coherent Android input publication, and ordered semantic feedback events rather than platform-specific gameplay or direct JNI mutation.
- Preserve exact-build evidence and asset provenance. New Android device evidence belongs in `docs/ANDROID_RT_DEVICE_COMPATIBILITY_RECORD.md`, with the exact model and evidence class; vendor/SoC claims do not certify a working device. Record imported-asset licences in `ASSET_LICENSES.md` before shipping.

## Work to the requested outcome

Read the affected code and the relevant references below, not the whole documentation set before every edit. Use `PROJECT_DECISIONS.md` when a change touches an established decision and `PROJECT_MEMORY.md` when historical context is needed; dated reports and old task plans describe their own snapshots, not current implementation or permission to start unrelated work.

For an implementation request, continue through the change, affected checks, inspection of the result, and fixes for regressions caused by the change. Do not stop at the first patch or a plan unless the user requested that stopping point. Use small runnable slices for substantial work; a typo does not need a new spec, milestone, or full validation programme. Report what changed, what was actually checked, and any remaining blocker. Missing hardware is a validation gap, not a pass or a reason to abandon safe host-side work.

The lead agent, including Astra, may implement, inspect, test and fix safe local work directly. Local edits, standard local builds/tests, and rerunning affected checks are within an implementation task; do not ask for approval at each step. Preserve unrelated work and respect the execution environment's permissions. Ask only when needed to resolve a material product/architecture decision, destructive action, or external side effect outside the authorised task. Paid asset generation, device installation/data clearing, release signing/publication, credential changes, and production/account changes are not authorised merely by permission to edit code. Follow explicit authorisation when already given rather than requesting it repeatedly. Signing backup/recovery checks remain owner-only.

## Load references by task

| Change | Reference |
| --- | --- |
| Renderer, shaders, materials, water/fire/glass, held props or player geometry | [Renderer and asset contracts](docs/AGENT_ENGINE_CONTRACTS.md#renderer-and-assets) |
| Gameplay, input, combat, animation, feedback or capture state | [Simulation and feedback contracts](docs/AGENT_ENGINE_CONTRACTS.md#simulation-and-feedback) |
| Android/Windows controls, HUD, lifecycle or RT Lab | [Platform interaction contracts](docs/AGENT_ENGINE_CONTRACTS.md#platform-interaction) |
| Choosing checks, compiling shaders, device evidence or performance claims | [Validation guide](docs/AGENT_VALIDATION.md) - select the applicable section, not every gate |
| Importing or shipping an asset | `docs/ASSET_PIPELINE.md` and `ASSET_LICENSES.md`; keep source/runtime separation and commercial-safe provenance |
| Release work | [README](README.md) package summary, `tools/release-version-policy.ps1`, and [owner-only signing safeguards](docs/OWNER_RELEASE_SAFETY_CHECKLIST.md) |
| Requested campaign/roadmap work | [Phase plan](docs/PHASE_PLAN.md) and the relevant accepted plan; a future version label is not a release instruction |

## Skills and delegation

Keep guidance model-neutral: the caller chooses model and reasoning effort. Use skills only when their stated task matches. Delegate independent, bounded work when useful and available, with clear ownership and an integration check; no fixed agent roster or mandatory multi-agent ceremony. Add a repository skill only for a demonstrated recurring workflow, with a short, specific trigger and on-demand references rather than another copy of these instructions.
