# Glass capture harness correction

The September 30 baseline run `run-20260930-061828` could not install the
ABI-targeted Debug APK because ADB rejected its `testOnly` flag. The runner now
uses `install -r -t` after its existing exact Debug-package check. It retains
application data, checks installed APK SHA-256 and never authorizes Release
publication. The separate accepted viewmodel candidate package was untouched.

Run `run-20260930-062025` then installed the existing baseline APK
`14927941cc7e7596943b0a2092c01ae1ff30277d376697e170c1d44381057688`
and captured six views, but failed its expected-zone check for tinted glass.
That overall result remains **failed**, despite successful Home/resume and
usable individual native-RT captures. The fire/tinted checkpoints borrow the
opening lighting preset but stage their camera in the skylight chamber.

The fix changes only their expected zones, not simulation, camera, image gates
or runtime geometry. A new shared-simulation test stages all five glass
fixtures and confirms `SkylightChamber`. The PowerShell test independently
checks the runner's five expected zones. Both passed, alongside the existing
34 combat-state and 16 gameplay/inspection-ownership cases.

Affected-control rerun `run-20260930-062520` used the same exact baseline APK on
**SM-S948B**, 75%, and passed fire/tinted captures and Home/resume. Both report
honest RayTracingPipeline presentation, with zero transport/shadow overflows.
No timed benchmark was requested. The candidate's seven-view run is separate
evidence for the shader change, not needed to justify the harness correction.

Audio/haptic manual revalidation required: **NO**; only validation expectations
and Debug install support changed, with no event or playback changes.
