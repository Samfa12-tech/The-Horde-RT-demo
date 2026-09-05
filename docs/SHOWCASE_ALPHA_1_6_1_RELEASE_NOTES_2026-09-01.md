# Horde Lantern RT — Showcase Alpha 1.6.1 development candidate

Package version: `1.6.1`

Android version code: `9`

## Status

This is an unpublished engineering candidate for the shared build/package
version contract. It is not a GitHub Release or itch upload, has no signed
release artifact, and establishes no fresh Android device, sustained
performance, or owner-feel evidence.

The latest published itch release remains exact package version `1.6.0` with
Android version code `8`. Its historical artifact, device-smoke, and release
evidence remain recorded separately and are not replaced by this candidate.

## Contract changes

- Root `VERSION` is the sole semantic package/display-version source.
- Android version code is resolved from the checked `version-code-map.json`.
- Windows resources and Android package metadata are generated from that same
  active source identity.
