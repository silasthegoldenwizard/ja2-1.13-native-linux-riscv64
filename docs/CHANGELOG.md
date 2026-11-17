# Changes for the native Linux source preview

## November 17 crash-hardening checkpoint

- Harden native UI, inventory, dialogue, save/load, strategic, projectile, and multiplayer boundaries.
- Add production-linked regressions and bounded sanitizer fuzz targets.
- Preserve optimized-build soldier construction and validate battle-entry state and soldier-init list cleanup.
- Preserve long strategic routes and propagate semantic subsystem load failures.
- Validate AI and NPC boundaries with their matching source-aware regression.
- Retain bfVFS, the existing RakNet companion, dependencies, and release state.

Development evidence: 143 native tests and 145 strict sanitizer tests.
The preceding crash-hardening checkpoint supplied 13,122,503 accepted fuzz inputs.
Two refreshed correction campaigns supplied another 2,000,000 inputs.
These results do not establish a full build or gameplay result from this exact export.
See the [crash-hardening record](CRASH-HARDENING-20261117.md) for scope and remaining limits.

## v0.1.0-alpha.1 source packaging

The source preview pairs the engine overlay with the reviewed RakNet 3.401 source companion.
The companion includes the previously separate headers, SDK adaptations, and complete selected source notices.
Rand's supported change-date notice is a reversible packaging addition.
Publication-facing guides and a separate packaged-source build record describe this paired route.
The earlier reader changes did not change engine or SDK implementation bytes.
The attachment refresh adds one null check in `Tactical/Interface Items.cpp` after `DoAttachment`.
The complete derived pre-notice file matches Owner's supplied hash.
No other reconstructed implementation file changes.
The independently accepted completion-only regression passes 24 normal and 24 strict sanitizer checks.
Actual attachment compatibility and full UI behavior are not established by its outcome doubles.
The final source/archive review and Owner's export acknowledgment are separate from the regression result.

## November 15 development checkpoint

This November 15 checkpoint adds the completed native desktop and save/load work.
It also adds the dialogue, door, sound-cache, and inventory sorting repairs.

| Area | Change | Focused evidence |
|---|---|---|
| Display | Borderless upper-monitor output and shared UI/input scaling | Resolution and desktop probes retained |
| Saves | Bounded native count/layout and UTF16LE message handling | No-mercenary and populated cold-load results retained |
| Scrolling | Dirty-work and selected blitter-effects correction | User confirmed the repaired top edge |
| Combat | Finite recoil direction and live-node identity checks | 54 recoil and 26 node assertions per mode |
| Dialogue | Matching C++ deletion and single registry cleanup path | 56 assertions per mode |
| Doors | Use the grid captured before structure deletion | 545 checks, 24 scenarios per mode |
| Battle sounds | Cover voice sets 0–11 and reject invalid saved IDs | 147 assertions per mode |
| Sector inventory | Replace raw byte sorting with C++ object-aware sorting | 2,204 assertions per mode |

The full suites increase from 72/74 in the earlier published source checkpoint to 85/87 in this local source checkpoint.
The additional sanitizer tests are the retained intentional diagnostic canaries.
No isolated API VFS replacement candidate is added.
No game assets, binary release, dependency update, or RISC-V result is included.
