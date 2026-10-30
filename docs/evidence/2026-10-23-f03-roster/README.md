# Exact F03 diagnostic roster correction

At `d014c75f2a3e2882966ef18ab79b377fc240f59f`, hosted
[run 39899227255](https://github.com/Samfa12-tech/The-Horde-RT-demo/actions/runs/39899227255)
finished with five successful jobs and one failure. The Vulkan-host lane passed
71/72 registrations; its authored-terrain journey fixture still expected the
live selection plus 32 chapter views after the two direct F03 views were added.
The retained original failure identifies that exact assertion. Route probes in
the same registration completed; this is not evidence of a gameplay route hang.

The fixture now requires exactly 35 selections (live plus 34 views) and explicitly
requires both F03 directions to request authored terrain. Existing registry,
staging, route, negative and timing assertions remain. No runtime, gate deadline
or CI structure changes are part of this correction.

Local MSVC Release: both ordinary and authored-terrain journey registrations
pass, 18.75 seconds total (authored 16.99 seconds). This does not close the
separate Debug 30-second route gate or prove a natural Keeper-to-earned-save run.

On the original hosted head, MSVC completed 169/169 selected cases plus the
isolated asset-policy registration and subsequent required offline steps.
Its earlier 15-minute job-cap failures remain in the outdoor-main record; the
latest pass does not establish their cause or authorize deadline relaxation.
Exact corrected-head hosted status is separate from these local checks.

The `prior-ci-receipt.json` identifies every original job. Failure and MSVC log
excerpts retain their original timestamps. The concurrent uncommitted masonry
candidate is outside this test-only correction and its own acceptance evidence.
Audio/haptic manual revalidation required: NO; fixture roster only.
