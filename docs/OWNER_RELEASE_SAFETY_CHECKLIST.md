# Owner Release Safety Checklist

These owner-only checks protect the stable Android update identity. Codex must not inspect, copy, expose, or mark these items complete on the owner's behalf.

- [ ] Release JKS independently backed up.
- [ ] Store password independently backed up.
- [ ] Key password independently backed up.
- [ ] Recovery location verified by owner.
- [ ] No signing secret committed.
- [ ] Stable Android update identity retained.

Keep all keystores, passwords, recovery material, and signing properties outside this repository. Use the existing Horde-specific key for compatible updates; do not create a replacement identity for an update.
