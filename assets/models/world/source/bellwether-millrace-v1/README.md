# Bellwether millrace terrain and water

The chapter now has a graded channel from the river intake to the wheel bay and the tailrace upstream of the original practice landing. Its cross-section has a submerged bed and raised banks. The deeper wheel bay clears the full wheel rim during a turn, with 15 cm of terrain clearance checked across its width. The narrow, steep section at the wheel follows the surveyed 2.19 m drop. The channel lips fade into the existing river terrain.

The renderer and native water contact both use a ribbon generated from the [surveyed layout](../bellwether-waterwheel-v1/layout.json). The 18 new ray-traced water faces occupy the existing main-river shader bounds. They extend the shared water buffer by 2,016 bytes. The original lake, river and outflow stay in their established positions in that buffer. The bed and its shoulders use the original terrain material images and existing collision intake. The [wheel, stone supports and raised sluice](../bellwether-millworks-v1/README.md) now use this bay. The walk to the wheel and the optional dry crossing remain open.

## Regenerate

Use Python 3 with NumPy and the exact r23 runtime GLB from Git checkpoint 7fe375c5 or the archived source. The command writes a derivative to a new directory. The terrain file stays within 39,618 triangles and 21,134 vertices. The recipe verifies exact reuse of the core primitive and embedded material bytes, consistency of shared seam edges, and upward winding of new faces after float32 quantization.

    python tools/author-bellwether-millrace.py --root . --input PATH_TO_R23_GLB --output NEW_DIRECTORY
    python tools/author-bellwether-millrace.py --root . --emit-native-header

The receipt identifies the terrain source, surveyed layout and resulting runtime GLB. The derivative has 39,266 triangles and 20,196 vertices. The first 42 triangle and GPU profile records were compared byte for byte with the previous checkpoint. The native Chapter Regional Water and Bellwether Shell suites check the derived course against the same water faces used for rendering and the same terrain contact used for movement. The Windows Debug environment survey uses --capture-environment-survey millrace with the existing hidden capture flags. It samples the real terrain for camera support. Both Windows RT backends presented all 120 frames of the hidden survey and the existing walk-to-practice-landing checkpoint. This verifies the channel render and the existing landing route. The walk to the wheel bay and the dry crossing still need authoring.
