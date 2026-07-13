# Gameplay

The gameplay layer contains the shared corridor/arch collision helper and the deliberately narrow one-enemy sword-combat state used by Android and Windows.

`SwordCombat.h` owns attack latching, the player sword hit window/cone, enemy approach/attack/death/respawn timing, and damage-pulse state. Rendering receives an immutable snapshot; it does not decide hits or combat phases.

Combat, attacks, block, dodge, and enemy AI remain deferred until the visual RT scene and phone asset path are stable.
