# Deadwood CI hydration correction

Actual run38202774273 at `2d6748e37da1413a55ccf2b7af25c50f2000332a`
compiled the shared and Vulkan targets but failed native asset intake and
package admission because the new five LFS-backed runtime GLBs were not fetched
or checked out by the workflow. The exact Vulkan job log reports `Static GLB
header magic is not glTF`; the focused retained [diagnostics](failure-excerpt.log)
also identify Android packaging and GCC/Clang failures. These are real admission
failures, not waived assertions or a renderer fallback.

All four jobs now include the exact runtime directory in both fetch and checkout
lists, including the sanitizer roster for consistent fixture custody. Local
validation verifies both occurrences in each job and all five actual GLB headers.
Exact-commit CI must validate the correction after push.

The unchanged upstream Quaternius notice has original trailing and EOF whitespace.
An exact-path whitespace attribute preserves those immutable bytes; authored
source and all other whitespace checks remain strict. There is no runtime,
shader, quality, timeout, test selection or budget change in this correction.
