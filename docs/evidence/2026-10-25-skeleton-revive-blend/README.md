# Reverse-death blend proof: contact join rejected

Base `c26a9f6538afa621da3268c8042d6b38cdb6fb9d` passed all six hosted jobs in
[40191017940](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/40191017940).
This checkpoint exposes the loader's existing local-joint blend through the
expanded skeleton path. Requests and cache identities own their values; the
ordinary simulation adapter supplies no requests. The only caller that sets a
blend is the explicit immutable hidden Debug pose fixture. Production gameplay,
original assets, simulation/events, shader ABI and renderer capacities are
unchanged. No revival, scream, new combat rule or flask is enabled.

One 0.5-second Dead0-to-Idle0 candidate was compared with the retained direct
join, using the same oblique camera, corpse transforms and original reverse
sampling. The actual RT sequence removes the snap but visibly sweeps both feet
across floor tiles without a planted step. See the actual [early](blend-124.png)
and [late](blend-130.png) join frames. Intermediate native skin reaches -28.8 mm
local Y versus the inherited corpse's -5.25 mm; the visible sliding/contact
failure, rather than a new numerical tolerance, rejects the join.

**The candidate remains unadmitted and default-off.** Another timing/blend loop
is not justified. The next animation requirement is a contact-preserving authored
stand transition on the existing rig. It must join the reversed Dead0 stance to
Idle, retain root/facing/scale, support both feet through credible weight transfer,
and show the whole body at gameplay distance. Do not hide this issue with a root
snap, teleport, simulated death reversal or a new tolerance. Actual one-time
encounter ownership and save/lifecycle behavior remain separate implementation.

The 165-frame sequence is 5.5 authored seconds at 30 samples/s, silent and
**unpaced**. All 120 pre-blend PNG hashes and all 30 following ordinary Idle
hashes match the control. The weight-one endpoint equals ordinary Idle exactly.
This is animation/correctness evidence, not measured FPS or phone acceptance.

Focused existing CTests pass 2/2: loader smoke 40.53 s and character-slot smoke
5.76 s. They cover exact null/zero/one output, finite/mapped request rejection,
owned requests, pose-bucket identity, weight-only writes/refits, abandoned-record
refit retention and cache invalidation on recreation. The Debug application and
all four Android ABIs build. No unrelated full suite was repeated locally.

Over 15 join samples, character skin invocations increase from 9 to 15, vertex
operations from 88,686 to 147,810 and copy/upload bytes from 8,123,328 to
13,538,880. Total BLAS-refit-record invocations increase from 39 to 45, including
other actors' baseline work. These are exact work counts for different poses;
the single unpaced timing run, including an unrelated 94.66 ms control outlier,
does not establish a frame-rate gain or regression. Raw observations remain
private with the original control.

The preserved Android Debug APK is SHA256
`73a3b0cf180e5e48ebf86d5d27c8948651577fb5ba2d3be5d848413932cb2189`,
206,291,259 bytes, package `com.samfa12.hordelanternrt.debug`, code 10/name
`1.6.2-debug`. It contains the startup and teaching fixes plus the default-empty
blend capability. All 185 packaged assets are byte-identical to frozen f27982cd;
the private scream/source masters are excluded. **It has not been installed.**
The owner phone still has the frozen f27982cd candidate and is released.

Owner campaign/settings, foreground, all 151 parked owner files and application
snapshot/event sequence are unchanged. No owned renderer/build/transfer remains.
Audio/haptic revalidation: **NO** for this silent, pose-only change. Scream mix,
gameplay bridge and the normal/yellow/red rule change have their own future gates.

[Sanitized source, executable, work and artifact receipt](receipt.json). Private
logs, full raw frames/video, original failures and custody maps remain under
ignored `reports/skeleton-revive-blend-20261025/` and the prior control folder.
