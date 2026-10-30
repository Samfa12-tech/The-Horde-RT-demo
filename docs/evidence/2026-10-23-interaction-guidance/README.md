# Finish the chest/rope Controls guidance

The current Controls pages still described Interact as a chest-only action.
That omits the existing 1.7 ascent/return interaction, and Android's help still
referred to the old visible Raise / Lower control after the lantern became an icon.

The bounded correction:

- Android Interact now names the nearby chest **or rope** action and the Climb /
  Descend prompts, explaining that equipment stows automatically on the rope.
- Android's Lantern Icon help explains tapping to raise/lower the carried reward.
- Android controller help identifies A as the nearby chest/rope action.
- Windows left-click help names both contexts and explains automatic stowing.

This follows the actual existing input and prompt paths, rather than adding a
binding or traversal rule: Windows `WM_LBUTTONDOWN` consumes the first capture
click, then accepts a presented Climb/Descend prompt as Interact; controller A
publishes the same semantic command. `RefreshChestInteractionPrompt` already
displays the corresponding rescue prompts. Android uses the enabled contextual
button or controller A. Ordinary movement, combat, automatic crouch, lantern
ownership, accessibility labels, menus and campaign saves are unchanged.

## Proportionate checks

The changed Android resources compile. Existing focused tests pass **42/42**:
34 controller integration, six contextual layout, two lantern label/state cases;
zero skips, failures or errors. The Windows Debug application builds successfully
with the corrected string. No new test that mirrors copy wording, full local
suite, runtime RT capture, benchmark, phone installation or physical acceptance
claim was added for this text change.

[Receipt](receipt.json) binds source inputs, the compiled Windows binary and
the actual selected class counts. Raw Gradle/build logs remain in ignored
`reports/interaction-guidance-20261023/`. The initial command failed to open its
log directory before Gradle started; the corrected command's successful execution
is separate. No access restriction, data change or denied action occurred.

Compact HUD thumb reach, continuous portrait/landscape rotation and actual
natural journey/audio/warm-device acceptance remain open. Both mesh studies
stay separate: world-body rejected, Kit unadmitted. Easy Tree, smoke decisions and unfinished town 1.8
assets are untouched. No merge, release or deployment.
