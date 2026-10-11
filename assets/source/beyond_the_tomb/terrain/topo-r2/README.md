# Owner-supplied r2 planning heightfield

Recovered byte-for-byte from the supplied `Horde-connected-reference-maps-r2.zip` (archive SHA-256 `5b3d1eadb7606e0348f19c9959aa34f901b3c1986350d69f4ebc6a21bc7cba8b`). Original Horde planning reference; not runtime terrain or an admitted production mesh.

`proposed_terrain_heightfield.npz`: 4,354,922 bytes; SHA-256 `251126186fa4923f9902524ec4ac2cc76d0f7a1c82cb1feac9901cc7832bb0ba`. Grid 1002 x 612 at 0.5 m. East/North/Up to engine: X=East+78.3, Y=Up-31.55, Z=126.2-North.

Coordinate layout and reviewed sheets remain under `docs/references/horde-1.7/d37f1638/docs/design/world/connected/`. Derivative authoring must preserve their overall route intent while documenting smoothing and exact tomb/shaft interfaces. This original stays outside runtime packages.
