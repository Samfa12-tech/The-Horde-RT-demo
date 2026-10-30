# Android capture-registry correction

The original proof checkpoint `f51aa8d5` failed the Android job in
[40112326112](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/40112326112).
The completed job log reports 447 tests, one failure:
`ChapterDebugCheckpointTest.allAuthoredChapterCapturesResolveWithTheirNativeCamera`,
line29. This was a checkpoint regression, not a phone or RT-driver failure.

The two new C++ chapter views230/231 were absent from `MainActivity.checkpointId`
and `developmentCheckpointViewPose`. Added their names/IDs and exact yaw/pitch to
the existing Android mapping. The native registry, test assertions and unknown-ID
rejection stay unchanged. No Android retention-mode selection, controls or gameplay
mechanic was added. These remain explicit frozen Debug views.

Focused local command, from `android/`:

```text
gradlew.bat :app:testDebugUnitTest --tests com.samfa12.hordelanternrt.ChapterDebugCheckpointTest --no-daemon --max-workers=2
```

Result: BUILD SUCCESSFUL in21s; existing test XML records zero failures/errors.
Full local log is retained at
`reports/forest-retention-20261024/android-registry-focused.log`.
No phone was accessed or APK installed. The native renderer/source and captured
proof hashes are unchanged by these four Java entries; new native captures are
not required solely for this registry correction. Exact corrected-head hosted
validation remains separate and is reported in the final checkpoint.
