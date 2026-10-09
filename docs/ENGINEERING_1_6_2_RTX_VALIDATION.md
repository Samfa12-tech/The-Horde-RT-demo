# 1.6.2 current RTX validation checkpoint

Source: pushed integration `3500099dc429419b0b18572efa4b68950776b9b5`, draft [PR18](https://github.com/Samfa12-tech/The-Horde-RT-demo/pull/18). This is development evidence, not release or owner acceptance.

## Exact build and platform

- Windows Debug application SHA256 `860ca70437f0068f456d8abe5ae3d6479a5282824b067e08b944651df65af523`; Diagnostic/High, 960x540 output and internal dimensions, fixed 100% scale. Native target `horde_rt_diagnostic_window`.
- NVIDIA GeForce RTX 5050 Laptop GPU, vendor4318/device11672. Preview reports Vulkan API4211029 and driver2559295488. KHR swapchain-maintenance1 presentation fences were actually enabled.
- Windows Release application SHA256 `5357a646978eca03ccdd4fe1885f556ce20ab30848e950cb727bec531de32753`; Shipping/High, 3,634,176 bytes. Fresh extracted containment check found exactly the intended Pipeline and Compute High pair, validated every module, and found no binding22 or diagnostic atomic instructions. This build is separate from the Debug captures below.

## Actual RT captures

| Run | Backend | RT views | Sync validation errors | Private local evidence directory |
|---|---|---:|---:|---|
| Compact framed preview | RayTracingPipeline | 9/9 | 0 | `reports/1.6.2-preview-pipeline-framed` |
| Compact framed preview | RayQueryCompute | 9/9 | 0 | `reports/1.6.2-preview-compute-framed` |
| Full Showcase | RayTracingPipeline | 13/13 | 0 | `reports/1.6.2-showcase-pipeline-integration` |
| Full Showcase | RayQueryCompute | 13/13 | 0 | `reports/1.6.2-showcase-compute-integration` |

Each bounded process exited0 without timeout; launch receipts, stdout/stderr and renderer-authored manifests identify its actual backend and executable. The shared frame barrier now correctly chains the TRANSFER acquire wait. The earlier audit's ten WRITE_AFTER_READ errors remain retained in `reports/1.6.2-audit-baseline`; those negative logs are not overwritten or reclassified as passes.

Preview physically allocates seven BLAS roles, one TLAS with seven instances and one active scene resource set. It uses admitted props, idle rig and material/texture subsets. The manifests record `gameplayAdvanced`, `gameplayEventsConsumed`, `audioStarted`, `fullShowcaseGpuSceneAllocated`, preference loading and preference writes all false. Actual Apply/Revert/Keep acknowledgement is exercised with an in-memory cap transition30/60, without writing user settings. Both backends produce the exact same overview PNG hash when their own timeline is paused; reset and motion use separate explicit timeline states.

The preceding Water camera cropped the pool. Only its camera changed, after a production-frustum test reproduced the crop; geometry, lighting, exposure, scale and quality stayed fixed. Current pose `(-0.65,-12.60,-0.56,+0.01)` shows the stream and pool in both real RT images. Pipeline Water PNG SHA256 `017a4b1e246f6fc02c393af61d17108435952b090b432bc5f4a0dffcb5de835f`; Compute `0381a7f7004e8865567ac69566a0f2987889b13d9bd416f0aad7b1e91794869f`. The old negative view remains in the earlier integration directory.

Full Showcase capture uses the historical frozen thirteen checkpoints and fixed animation time0. It verifies current rendering/presentation through those routes, not the live six-second reveal, combat timing or torch motion. The inherited pixel parity area-shadow pattern is visible; no claim of a new pattern defect or accepted higher shadow tier is made. Matched moving-light/contact-edge and warm cost evidence is still required.

## CI and limits

All six required jobs passed for source3500099 in [push run37106026981](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37106026981) and [PR run37106244900](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/37106244900): GCC, Clang, MSVC, selected ASan/UBSan contracts, Vulkan host, and Android Debug/build/playback/report/lint. These results belong to that exact source; later changes require current CI.

Capture timing is unpaced validation, includes settling/readback and is not sustained FPS or a matched performance comparison. Inventory measures tracked Vulkan allocations, not residency, shared phone RAM or bandwidth. No native menu/touch/accessibility, disk-interruption, lifecycle, listening or final visual acceptance follows from these images. The deeper waterfall shaft needs a specifically aimed upward view; the historical checkpoint named Skylight is the separate later corridor.

Phone allocation was withdrawn after identity verification and before any installation/test. No phone test process is active; explicit renewed allocation is required. S24 final-release coverage and S25 remain unverified. On drivers without the optional maintenance extension, formal presentation-resource retirement at shutdown remains an explicit unresolved boundary; RTX maintenance-fence success does not resolve that boundary for another driver.

Exact next action: matched baseline/candidate warm ordinary/lantern-heavy Release workloads, actual output-only resize measurement, bounded live/moving scene witnesses, and owner Form/visual/changed-audio acceptance. Preserve all negative evidence and frozen1.6.1 artifacts.

## Release fixtures and actual package follow-up

Full Windows Release CTest passed113/114 in815.45s. The sole failure, offline GitHub-preflight62, reached its final strict repository non-mutation guard during a lead checkpoint edit. No behavior assertion failed. The unchanged fixture passed1/1 in18.58s with writes paused; its guard remains intact. Logs are `../integration-release-ctest-full.log` and `../integration-release-preflight-quiet.log`. This is aggregate-plus-affected-rerun evidence, not a claim that the first aggregate run passed114/114.

Actual unpublishable validation ZIP: `../candidate-windows-release-validation-final/Horde-Lantern-RT-Windows-UNPUBLISHABLE.zip`,117128317bytes, SHA256 `a8736da989ae2c8da1e0be5ebeec83dc4aedeb2db209a38613d4437eb9600173`; exact Release5357a646 executable above. Its81 file entries match staging hashes; complete1073-byte cgltf notice, music and1.6.2 audio/environment rosters pass. Compression, asset copying and notice admission were performed without production packager entrypoint, Android operations, signing or publication. Receipt `../candidate-windows-release-validation-final/package-receipt.json`.

Real Compress-Archive output exposed legitimate zero-byte directory entries. The admission policy now accepts only canonical trailing-slash, zero-length directory ancestors of admitted files; source/foreign files, zero-byte files and nonempty disguised directories remain rejected. All24 affected policy cases pass, including real compression and negative directory/file cases. The first failed package is retained separately.

An actual-rig clearance follow-up reproduced a low-portal walking/down-look grip solve exceeding the unchanged15mm tolerance. Historical worst-bend capture is individually safe, but does not cover this moving approach. Shared reachable retraction is being corrected; the current captures and package precede that correction. New native builds, affected pose checks and current motion witnesses are required after it lands.
