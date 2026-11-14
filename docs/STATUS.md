# Accepted source status — November 14, 2026

The accepted live-source checkpoint demonstrates original native x86-64 engine
startup, menu/Preferences transitions, original campaign entry and complete
first-sector A9 victory in normal and unsuppressed sanitizer runs. Both accepted
victory lanes closed normally with exit 0. Stable victory checks found A9
player-owned, enemy counts zero, combat/busy clear, first-battle facts complete,
all four original hostiles dead/inactive and the three hired mercenaries alive.

Final complete suites passed 72/72 native and 74/74 sanitizer with zero failures
or skips. A bounded independent source/evidence review returned PASS WITH GAPS.
This is recorded acceptance evidence for the original uncommitted source bytes.
The notice-adjusted package is verified by exact reconstruction, without a new
compilation, runtime test or visual acceptance claim.

A subsequent source review identifies an unexercised lifetime issue in the
accepted snapshot: the lit-door branch of `InternalSwapStructureForPartner`
in `TileEngine/structure.cpp` reads `pBaseStructure->sGridNo` after
`DeleteStructureFromWorld` can free it, before `LightSpriteStateChangePost`.
This is source-supported; no new runtime crash was reproduced. It is preserved
in this source snapshot, with no packaging repair. Bounded local source triage
is complete; fixes and runtime reachability remain open. A separate synchronous
non-person destruction path also supports a conditional stale-alias exposure;
no new runtime crash was reproduced.

The package excludes proprietary resources, images, binaries, builds, profiles,
saves, raw logs, authentication/environment state, API/provider/accounting state
and CI. The two modified RakNet headers and separately modified RakNet 3.401
library remain local-only pending their exact component terms review. Current
engine callers/build files retain references to that dependency. Full engine
reproduction therefore remains incomplete from the proposed upload alone.

Next engineering result is a separately authorized post-A9 save/reload trial;
it has not started. Sound/devices, human visual acceptance, broad campaign
reliability, general projectile lifetime safety and native RISC-V remain open.
