The source explicitlyHandEditedFiles field in provenance.json includes the two
generated catalog snapshots as well as the two actual implementation/test inputs.
Catalogs are generated, not hand-edited. The twoFilePatch covers only rt_lighting
and CharacterRenderSlotSmoke; subsequent changes to both shader-test compatibility
pins are dependent validation maintenance, not additional renderer changes.
Immutable APKs/catalogs retain their own exact hash identity regardless of those
later host-test updates. Do not infer the entire working-tree state from that
two-file patch. No isolated/nonphysical shaders were included.
