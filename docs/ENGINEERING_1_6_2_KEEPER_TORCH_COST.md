# Keeper torch cost comparison - 5 October 2026

The new flank torches have a substantial measured cost in the hot-phone
comparison. Keep that tradeoff visible during candidate review; positive
appearance feedback is not performance acceptance. No render-scale reduction,
light eviction, shader-quality cut or lighting rewrite follows from this result.

## Exact comparison and population

The control is the accepted mist runtime2bdad132, Shipping Benchmark APK
`21d1b3ba9dd469c0aa5a01d898c24b19964f399033aead2d92bd740fe23e44da`.
The candidate is Keeper-lighting runtime6468c3e4, Shipping Benchmark APK
`e42bc8cae3867ba93e5fb572b563cefa093438eac00c5698b170bfd04271994b`.
Each installed APK was actually pulled back and hashed before its course.

The order is control A1, candidate B1, candidate B2, control A2. All four native
reports complete one warmup and one measured lap:1,838 owning CPU rows and1,838
valid RT GPU rows per course, with no rejected, cancelled or outstanding rows.
The ten disjoint zone populations and weighted CPU/GPU means join the totals.
Serialized percentiles are checked for finite ordering and denominators, rather
than independently recomputed from raw samples.

Each uses genuine Pipeline,75%,1080x2235 internal/1440x2980 output, Mobile
water/dielectric/fire, Glass On, Current shadows, fire4/1/4 and saved cap30Hz.
Fresh ordinary Saved/effective UI and native configurations agree. All courses
start from ordinary MAIN/LAUNCHER without benchmark or Debug extras. Original
Debug saved50 preferences remain byte-identical1,420 bytes, SHA256
`3118148f509e127df6c607f23533bc6782ed6b504dc08aec2cff49160c68140a`.
Reports are prepared locally with hardware consent unchecked and remain unsent.

## Observed cost

| Course | Owning CPU mean ms | RT GPU mean ms | Finale RT GPU mean ms |
| --- | ---: | ---: | ---: |
| Control A1 | 71.873350 | 61.058834 | 60.621775 |
| Candidate B1 | 82.723265 | 71.927085 | 129.535840 |
| Candidate B2 | 83.082514 | 72.245400 | 130.287410 |
| Control A2 | 74.521691 | 63.742917 | 63.223970 |
| Pooled control,3,676 rows | 73.197521 | 62.400876 | 61.922873 |
| Pooled candidate,3,676 rows | 82.902890 | 72.086243 | 129.911625 |

Pooled GPU duration increases9.685367ms, about15.5%; the finale subset increases
about68.0ms, roughly doubles. Finale has101 measured frames per course. Several
earlier zones also increase, so this dataset does not isolate all added cost to
the visible torch volumes. The source change adds physical torch instances,
active fire lighting and source-occluded mist work; static shader counts do not
attribute the observed device cost to any one of these paths.

## Limits and decision

Battery headers at course start are43.5-43.7C and all available thermal-service
headers are status3. A1/B1/B2 end headers are44.0/44.0/44.1C. These are external
context observations, not ASIC temperatures, proof of equal throttling or causal
cost. The original native reports do not collect thermal data or declare cooling.
No sustained30FPS, scanout rate, separate lantern-heavy comparison or four
simultaneous real-fire result is established.

A2's host observer log stops at243 seconds, with the cause unproved. A later
single owned UI observation finds the existing complete native report timestamped
03:49:00UTC, after its03:44:26 ordinary start. The original report is recovered
through local preparation without rerunning the course or extending a deadline.
There is no continuous within-deadline host observer pass or actual A2 course-end
battery/thermal observation. Later recovery-time context does not replace that
missing endpoint. The fixed360-second method and all original observations and
negatives remain intact.

Admission receipt
`4197fc88502c4c2e5176798359694fdd5c985538fc677ed5d6547c09800b1650`
retains exact inputs, statistical joins and this qualification. The comparison
crosses the documented15% investigation threshold; it is not a blanket product
failure or acceptance. The requested lighting remains in the review candidate,
with cost and owner/reviewer tradeoff acceptance explicitly open. The separate
Lower indirect-light prototype excludes these new torches and cannot certify an
offsetting improvement for this scene. Review reveal/combat/death/extinguishing
motion and the chest transition independently.
