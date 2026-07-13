# Scene

Scene code loads the merged-animation skeleton GLB, evaluates exactly `Idle_5`, `Walking`, `Attack`, and `Dead`, skins unique vertices on CPU, and supplies the dynamic RT vertex stream. Idle/walking loop; attack/death clamp at their final pose.

Keep import paths narrow and measured on Android before adding more models, clips, or general-purpose asset machinery.
