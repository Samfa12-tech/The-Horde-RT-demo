# Lead review disposition

The actual phone artifact is v2, based on 18616f4 plus investigation-v2.patch.
It is a temporary detached-source experiment, not a Shipping performance pair
member or accepted renderer implementation. No GPU shaders or scene work change.

Stage provenance intentionally comes from an exact seven-field join to the
completed benchmark row (which owns zone and activeStrategy after 8fbd46c), not
duplicated labels reconstructed at fence time. Reject unmatched/ambiguous rows;
raw stage logs alone cannot prove opening/OpaqueFast or presented evidence.

v2 explicitly records non-duration statuses for failed waits and identity-less
or failed record/submission paths. It never reads query results after failed
fence/idle, resets stage state only after successful idle, and destroys pools
before device destruction. Supported/valid outcomes still require actual device
evidence. Successful Android v2 build is not device acceptance.

Both endpoints use the appropriate AS-build or RT/compute stage. The default
full-frame TOP/BOTTOM endpoints are unchanged and regression-tested. Vulkan
timestamps define stage-limited execution dependencies and may be written at a
logically later stage: these are interval observations, not isolated-engine
hardware counters. Query/reset commands and host logs perturb the experiment;
do not treat its frame times as frozen A/B evidence or assume additive stages.

Reference: https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdWriteTimestamp.html
