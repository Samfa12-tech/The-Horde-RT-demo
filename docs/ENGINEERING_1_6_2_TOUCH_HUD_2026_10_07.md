# Native combat controls and hearts — 7 October 2026

Swing, Parry and the new Dodge control request the existing simulation command on press-down. Release does not publish a second action. Cancellation and menu/lifecycle cleanup clear pressed state; keyboard/accessibility clicks retain their normal single-action route. Gameplay commands are gated while paused menus, diagnostics, death, ending or benchmark UI owns input. Dodge uses the existing movement-direction mechanic, including its forward default while stationary.

Three original vector full/empty hearts replace the vitality rectangles on Android and Windows. Current/max vitality text and accessible health labels remain. Health, damage and death rules are unchanged. The action slots reserve Swing/Parry, a Dodge row, and the stable interact/lantern row; paused control previews follow the same arrangement and retain system-sized readable labels.

Host validation: 166 Java tests across 28 classes passed with no failures/errors/skips. Android lint passed with zero errors and 55 warnings; the five additional warnings over the input-regression slice are the Dodge button's four inherited API-26 autosize attributes and touch listener accessibility diagnostic. The existing click route is exercised by the regression fixture. No warnings were suppressed. A stale preview child-count assertion and one indentation error were corrected; their failed logs remain in the task evidence directory. The repeated-DOWN regression was subsequently rerun with the actual production listeners.

Windows Debug app builds with the original vector-heart control. JNI Dodge source follows the locked coherent command publisher; a fresh integrated Android native build/package is still required. These host results do not certify physical three-finger comfort, phone orientation/large-font acceptance, physical controller behavior or owner visual/haptic acceptance.

Evidence logs: `post-reset-touch-actions-02.log`, `post-reset-touch-java-full-lint-01.log`, `post-reset-touch-java-full-lint-02.log`, `post-reset-touch-java-full-lint-03.log`, and `post-reset-combat-rag-native-build-04.log`, outside Git in the existing task-4 directory.
