# Prepared update — November 14, 2026

This list describes the prepared source checkpoint.
A separate record describes delivery to the private repository.

- The original native engine source advances from the November 12 component harness.
  It includes original-engine linking, the menu, the campaign, and accepted A9 victory.
- The snapshot adds native input, cursor, and UI code.
  It adds bounded text and font services, scalable FreeType glyphs, and SDL presentation and audio boundaries.
  It adds FFmpeg movie and audio decoding and the remaining original rendering families.
  It keeps v1.13 mechanics.
- The snapshot keeps the Windows profile-101 resource-reader boundary.
  It includes bounded startup, VFS writable-profile, and original-engine integration changes.
- The A9 runtime repairs remain.
  They address invalid animation indexing, corpse palette copy size, and public-knowledge sentinel behavior.
  They also address the two observed projectile/structure lifetime defects.
- The snapshot includes current synthetic tests and source, dependency, and status documentation.
  The recorded original-source suites pass 72/72 and 74/74.
  The export with notice changes has source-reconstruction checks only.
- Unchanged engine and vendor files stay external.
  Component-specific notices remain.
  Actual RakNet-derived header and library changes stay on local storage.
  Their absence prevents full-engine reconstruction.
- Source review found an untested lit-door stale read in `InternalSwapStructureForPartner` after structure deletion.
  The recorded review did not reproduce a new runtime crash.
  This export includes no repair for that issue.

  The review of the listed source findings is complete.
  Repairs and runtime reachability tests are still open.
  A separate synchronous destruction alias is another conditional path supported by source review.
  The recorded review did not reproduce a crash in that path.

The preparation step made no commit or remote object.
It did not push or publish files.
It made no binary or CI configuration.
