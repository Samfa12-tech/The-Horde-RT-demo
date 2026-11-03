# Optional forest fireflies

Owner-requested restrained ambience, implemented using the existing world-space
analytic particle path inside RT frame shading. This is not a new raster pass or
screen-space image effect. Actual primary RT hit distance clips each mote,
including the first water/glass surface. Like existing dust/contact fragments,
these motes do not appear down reflected or transmitted secondary paths. No
reflection completeness is claimed.

Particles **Standard** admits three authored pockets with four fireflies each,
within 12 metres. Absolute simulation time drives gentle, bounded motion and
asynchronous brightness pulses. Actual imported terrain supplies height; missing
terrain produces no motes. They stay anchored in the world when the camera moves.
Particles **Low/Off** add no fireflies or firefly ground queries. Existing fresh
phone Low and all saved settings are unchanged. Both platforms explain the option.

The existing 64-mote upload and four-candidate tile limit are unchanged. At most
32 ambient motes plus 12 fireflies leave the [48,64) contact-fragment reservation
untouched; combat retains priority. Fireflies are self-emissive, do not light the
environment, and add no visibility ray, dynamic light, mesh or acceleration-
structure update. Pixel-footprint fading remains; no bloom or forced bright
subpixel expansion is used. Leaving the area, lowering quality and lifecycle
rebuilds clear the overlay; failed uploads never seed cache reuse.

## Source and shader checks

The extended indoor-dust host contract tests Low/Off zero work, missing ground,
distance bounds, actual support height, camera-independent positions, gentle
time-based movement, contact priority, exact tile accounting and fixed capacity.
The shared shader source was regenerated for all eight RT pipeline variants,
eight hardware RayQueryCompute variants and both compatibility raygen artifacts.
Both backends retain shared shading and shipping diagnostic isolation.

The first pipeline freeze correctly rejected the new emission early-return
branch against two diagnostic generic shader ceilings. Fresh compilation of all
eight variants established precisely one added selection merge in those two
entries: High 416 to 417 and Mobile 414 to 415. Only those two ceilings changed.
All existing byte, instruction, loop, ray-query, function, atomic and Shipping
limits remain unchanged. A subsequent full freeze passed. This is explicit
feature accounting, not removal of a failing check or measured GPU savings.

Final normal-content Windows F01 captures were inspected at Standard and Low.
Standard records four generated/projected fireflies and five tile references;
four restrained yellow-green points are visible in the right-hand woodland.
The matching Low view has none and retains the same forest composition. These
are actual frozen RT captures, not generated artwork or a motion recording.
Gentle time-dependent motion is covered by CPU tests; moving visual acceptance
is not inferred from these stills. The final native receipt binds exact artifacts.

GPU overhead remains unmeasured. CPU tests and shader builds alone do not establish
frame rate or phone acceptance; separate Android evidence is recorded in the
device compatibility record if obtained.
