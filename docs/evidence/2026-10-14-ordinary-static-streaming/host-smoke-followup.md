# Host sweep completion and ownership-smoke correction

The full194-test Debug registry has now executed: the interrupted first segment
completed tests1-106; continuation107-194 covered every remaining registration.
193 passed, and CharacterRenderSlotSmoke failed one source-wiring assertion.
The [full logs and coverage receipt](checks/host-coverage-result.json) preserve
that failure, the two segments and the exact baseline source/test hashes.

The failing literal `record.primaryTorchPixels != 0u` was already absent at
parent d8882c1b, while its test still required it. The real capture now invokes
`ClaimedTorchPrimaryOwnershipValid`, which rejects both hand-torch masks and
permits raw legacy torch pixels only when an actual Keeper world-torch alias is
present. The source check now requires this exact call and its raw count/mask
input; seven direct semantic cases also cover both aliases, unexplained pixels,
either leaked hand owner, and incorrect alias masks. Diagnostics availability,
player/sword masks and final GripRing-contact assertions remain required.

Final Debug2/2 and optimized Release2/2 smoke/controller checks pass. Only test
code and additive CI selection change; the measured Release EXE, Debug EXE,
Android APK, shader identities, quality and runtime bytes in the preceding
receipt remain unchanged. The character smoke executable and CTest now join
the Vulkan-host CI lane, alongside the exact face-certificate test. Fresh
exact-commit CI is required after this follow-up push.

This corrects the preceding report's pending aggregate host status. It does not
close physical-phone acceptance or the reusable independent-region architecture
milestone. That subsequent owner direction is engine loading/streaming for
continuous connected paths, persistent coarse/distant views and reliable return
travel; no new world art is requested.
