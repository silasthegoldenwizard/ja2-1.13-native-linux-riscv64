# Accepted source status — November 14, 2026

This page describes the accepted live-source checkpoint.
It does not describe later live-source changes.

The original engine starts on native x86-64.
It completes menu and Preferences transitions and enters an original campaign.
Normal and unsuppressed sanitizer runs both completed first-sector A9 victory.
Both accepted victory runs closed normally with exit 0.

The stable victory checks recorded these results:

- A9 was player-owned.
- Enemy counts were zero.
- Combat/busy state was clear.
- First-battle facts were complete.
- All four original hostiles had dead/inactive status.
- The three hired mercenaries were alive.

The final complete suites passed 72/72 native tests and 74/74 sanitizer tests.
No test failed or was skipped.
A bounded independent source/evidence review gave the result PASS WITH GAPS.
These records apply to the original source bytes, which remained uncommitted.
The package with notice changes has exact source-reconstruction checks.
It has no new compilation, runtime test, or human visual acceptance.

A later source review found an untested lifetime issue in the accepted snapshot.
It is in the lit-door branch of `InternalSwapStructureForPartner` in `TileEngine/structure.cpp`.
The routine calls `DeleteStructureFromWorld`.
This call can free the old structure.
The lit-door path can then read `pBaseStructure->sGridNo` before `LightSpriteStateChangePost`.

Source review supports this issue.
The recorded review did not reproduce a new runtime crash.
This snapshot keeps the issue and includes no packaging repair.

The review of the listed source findings is complete.
Repairs and runtime reachability tests are still open.
A separate synchronous non-person destruction path can also expose a stale alias.
Source review supports this conditional path.
The recorded review did not reproduce a new runtime crash in that path.

The package does not include these items:

- Proprietary resources, images, binaries, or builds
- Profiles or saves
- Raw logs
- Authentication or environment state
- API, provider, or accounting state
- CI configuration

The two modified RakNet headers stay on local storage.
The separately modified RakNet 3.401 library also stays there.
Their exact component terms need a review.
Current engine callers and build files keep references to this dependency.
This package alone cannot reproduce the complete engine.

At this checkpoint, the next engineering result was a separate post-A9 save/reload trial.
That trial needed separate authorization and had not started.
Sound and physical devices, human visual acceptance, and broad campaign reliability remained unproved.
General projectile lifetime safety and native RISC-V also remained unproved.
