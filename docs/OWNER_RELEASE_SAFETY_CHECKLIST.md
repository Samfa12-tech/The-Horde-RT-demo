# Owner Release Safety Checklist

These owner-only checks protect the stable Android update identity. Codex must not inspect, copy, expose, or mark these items complete on the owner's behalf.

- [ ] Release JKS independently backed up.
- [ ] Store password independently backed up.
- [ ] Key password independently backed up.
- [ ] Recovery location verified by owner.
- [ ] No signing secret committed.
- [ ] Stable Android update identity retained.

Keep all keystores, passwords, recovery material, and signing properties outside this repository. Use the existing Horde-specific key for compatible updates; do not create a replacement identity for an update.

This development machine has a Horde-specific local-only signing handoff beside the external keystore. It is not tracked by Git and must never be printed, copied into commands, logs, reports, chat, or repository files. For an authorised release, tooling may read it only in process memory, validate the expected keystore and alias, set process-scoped Gradle signing variables, and clear them immediately after packaging. Its existence is not evidence of an independent backup, so the owner-only checks above remain unchecked.
