# S24 visual follow-up (bounded checkpoint captures)

- Device: Samsung `SM-S928B`, serial `R5CXC0G9GBW`, Android 16 / API 36, Adreno 750.
- Package: `com.samfa12.hordelanternrt.debug`; installed and captured exact accepted Debug APK SHA-256 `8cb976891e5719eceb7fd809ec91aac939ff3ec58eb2d6df44f15b5526f4ff87` (version 1.6.1-debug). The pre-existing installed Debug APK hash differed (`581ea466...cc066a`); exact APK was installed with `adb install -r -t`, preserving app data. Stable package untouched.
- Source commit recorded by runner: `6fa1c53d1f0e4ec3938983f2cad7bd2ece233f4a`, clean. Actual backend in all four native states: `RayQueryCompute`; not the S26 backend/evidence. Pipeline pair: mobile opaque-fast `c1e4622a...956ff58` plus generic dielectric `e1817159...7792e302`.
- Capture run: [run-20261001-221511](run-20261001-221511/validation.md). Only four authored states were captured at 75%; each reports 12 stable RT-presented frames, native RT dispatch and swapchain copy, `presented:true`. No route replay or timing samples were requested (`timing.csv` is empty; GPU timing disabled). The runner's Home/resume gate passed.

Visual/native observations:

- `opening`: native state says 1 active skinned enemy / 1 entity; screenshot shows one skeleton in the far doorway, consistent with the authored opening. No modeled hands are visible; lantern and blade are visible.
- `two-enemy-combat`: native state says 2 active skinned enemies / 2 entities and RT presentation succeeded. Screenshot does not visibly show either skeleton in the captured view; therefore this capture cannot confirm two visually distinguishable enemies, but it also does not contradict the native count. Lantern and blade remain visible; no modeled hands.
- `player-viewmodel-lantern-high` and `...-low`: both images show the held lantern and blade, but no modeled hands. These are presentation-only visual observations, not a player-feel judgment.

Evidence details and native state are in `run-20261001-221511/capture-manifest.json` and the four paired `capture-*-state.json` files. `capture-02-two-enemy-combat-75.png` SHA-256 is `2f59f7dd65953ea2dedb9417ef6b3c88e7a2baf443962491580d9f4569d731bb`; opening PNG SHA-256 is `26656bd1600be5fb78ba7c9a2115e4ea603d1324eec3b104ed91a91734a6973b`.

After capture, the device was explicitly returned to Home; launcher was the resumed activity and the game had no foreground activity. No source fixes, builds, settings changes, data clearing, benchmark/timing runs, or S26 operations were performed. This is a narrow screenshot/native-state check, not performance, gameplay, or owner-feel evidence. The app remains installed with its data preserved.

Git retains the four PNG/native-state pairs, capture manifest, summary and
validation. Private capability/device/process/package/log rosters remain local
and are not part of this archive. Runner source identity is the checkout used
for collection, not a claim that the unchanged APK was rebuilt at that hash.
