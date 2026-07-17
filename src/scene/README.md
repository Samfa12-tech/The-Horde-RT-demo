# Scene

Scene code provides the narrow skeleton loader plus a configurable UV-bearing skinned-character path. The skeleton evaluates `Idle_5`, `Walking`, `Attack`, and `Dead`; the lich presentation uses `Idle_02` and `Dead` while whole-instance hover/orbit avoids the distorted walking clip. Unique vertices are skinned on CPU and supplied to the selected dynamic RT BLAS at a bounded cadence.

Keep import paths narrow and measured on Android before adding more models, clips, or general-purpose asset machinery.
