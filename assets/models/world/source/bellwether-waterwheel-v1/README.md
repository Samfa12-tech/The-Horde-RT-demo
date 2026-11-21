# Bellwether waterwheel and millrace

The editable wheel uses a 3 m diameter, sixteen cupped paddles, oak rims and spokes, and round iron journals. The GLB has one rigid node at the centre of the axle with local +X as the rotation axis. The face is 1.76 m wide and has 0.27 m clearance per side in the 2.3 m race. The [native assembly](../bellwether-millworks-v1/README.md) rotates at the axle while the sluice and piers stay fixed.

## Fit against the chapter river

The original connected-map profile passed farther west. At its mapped upstream bend, the current terrain stands at world Y -9.87 m, approximately 9.4 m above the intake at world Y -18.87 m. This study puts the race between the real west riverbank and the mill's east wall. It retains the 2.3 m channel, gravity feed, return to the same river, and an optional crossing north of the building. The planned return finishes upstream of the existing seal-trial landing.

| Feature | Engine position or measured result |
| --- | --- |
| Intake | X 143.60, Y -18.90, Z 74.00 m |
| Optional crossing centre | X 138.80, Z 94.00 m, with the planned 7 by 3 m deck envelope reserved |
| Axle centre | X 137.20, Y -21.10, Z 109.35 m |
| Headwater to tailwater | Y -20.16 to -22.35 m, a 2.19 m fall |
| Return to main river | X 139.50, Y -23.045, Z 119.00 m |

Both race profiles descend. The surveyed lips sit 24–51 mm below the existing river across the stations. The axle height gives 0.25 m of nominal paddle immersion in the tailwater. The largest sampled cut to a 0.4 m-deep bed is 3.75 m. The headrace also needs up to 0.69 m of sampled fill to the bed. These are station samples from the pinned runtime GLB. The designed bed will need local cut and masonry retention, including a contained gate and drop at the wheel. The bridge still needs dry approaches and abutments on both banks. Author the bed and its water faces together so they share their edges. Preserve the practice landing and keep the crossing optional.

## Rebuild and validation

Open the packed Blender master for further authoring. Use the terrain and authoring script at Git checkpoint 7fe375c5 to reproduce this survey. The [native terrain derivative](../bellwether-millrace-v1/README.md) now supplies the millrace bed. The source texture set is the existing Horde chest wood. Run Blender 5.2 in background mode with the repository's author-bellwether-waterwheel.py and pass the absolute repository and a new output directory after the argument separator. The script validates the atlas and terrain hashes, surveys the race and emits the editable master, portable GLB and receipt.

The candidate exports 2,180 triangles, 4,252 vertices, two opaque materials, and the existing wood base, normal and roughness/metallic maps. Two independent exports matched byte for byte. Khronos glTF Validator reported zero errors and warnings. An isolated Blender render was reviewed. The current Bellwether shell, regional water and water contact host suites passed. Engine admission, native player-route inspection, real-time RT and phone checks remain open. The original 1.7 water and 1.8 mill campaign remain the active runtime for this source checkpoint.
