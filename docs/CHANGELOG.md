# Changes for the native Linux source preview

## November 19 world-teardown roster checkpoint

Resolve soldiers by runtime ID during world teardown. Skip empty and inactive slots.
Preserve player soldier objects and remove non-player soldiers through the existing tactical removal path.

Both development game targets linked. The development suites passed 266 native and 268 strict sanitizer tests, including local multiplayer tests.
Live sector transitions, Windows, RISC-V, and full campaign validation remain open for this checkpoint.

## November 19 battle-end roster checkpoint

Skip empty team slots during battle-end updates. Continue processing later live owners.
Use the configured ID range for player updates after a victory.

Both development game targets linked. The development suites passed 265 native and 267 strict sanitizer tests, including local multiplayer tests.
Live battle-end gameplay, Windows, RISC-V, and full campaign validation remain open for this checkpoint.

## November 19 multiplayer interrupt legacy-local correction

Validate the incoming interrupt queue before changing game state.
Use the same local-team decision for queue updates and interrupt startup in both game modes.

Both development game targets linked. The development gates passed 263 native and 265 strict sanitizer tests.
Live networked multiplayer, the inactive alternate receiver, Windows, RISC-V, MSVC, and clang-cl remain unverified.

## November 18 multiplayer roster checkpoint

Skip empty roster slots when counting enemies and selecting a soldier for battle start and server override.
Check the order index, soldier ID, and runtime owner before completing a multiplayer interrupt.

Both development game targets linked. The development gates passed 250 native and 252 strict sanitizer tests.
Live networked multiplayer, Windows, RISC-V, and full campaign validation remain open.

## November 18 UpdatePublic checkpoint

Skip empty team slots when updating public opponent knowledge.
Add a production-linked regression that checks the public sighting and reported location after an empty slot.

Both development game targets linked. The development gates passed 246 native and 248 strict sanitizer tests.
Fresh Windows, RISC-V, multiplayer, and full campaign validation remain open.

## November 18 all-opponents radio checkpoint

Skip empty roster slots when reporting sightings for all opponents.
Add a production-linked regression for an empty first slot with a live opponent in a later slot.

Both development game targets linked. The development gates passed 245 native and 247 strict sanitizer tests.
These results include 14 X11 contracts in each lane. Fresh Windows, RISC-V, and full campaign validation remain open.

## November 18 panel and radio checkpoint

Protect player panel updates and radio sightings when roster slots are empty.
Add production-linked tests for both missing-owner paths.

Both development game targets linked. The development gates passed 244 native and 246 strict sanitizer tests.
These results include 14 X11 contracts in each lane. Fresh Windows, RISC-V, and full campaign validation remain open.

## November 18 laptop roster checkpoint

Skip empty slots when counting the laptop team and opening Personnel.
Add production-linked tests for roster holes and an empty Personnel list.

Both development game targets linked. The development gates passed 242 native and 244 strict sanitizer tests.
These results include 14 X11 contracts in each lane. Fresh Windows, RISC-V, and full campaign validation remain open.

## November 18 ammo crate roster checkpoint

Skip empty player roster slots when searching for a compatible ammo crate.
Add a production-linked regression that checks the merge and world-item cleanup.

Both development game targets linked. The development gates passed 240 native and 242 strict sanitizer tests.
These results include 14 X11 contracts in each lane. Fresh Windows, RISC-V, and full campaign validation remain open.

## November 18 vehicle roster checkpoint

Skip empty player roster slots when counting vehicles for admission.
Add a production-linked regression for a missing roster entry.

Both development game targets linked. The development gates passed 239 native and 241 strict sanitizer tests.
These results include 14 isolated X11 cases in each lane. Fresh Windows, RISC-V, and full campaign validation remain open.

## November 18 native checkpoint

- Skip empty soldier slots in tactical and strategic team scans.
- Correct kit point use, attachment removal, and selected save and dialogue paths.
- Add regression coverage for team slots, inventory, rendering, and strategic state.

The development builds linked both game targets and passed 238 native and 240 strict sanitizer tests.
The hosted checks cover source reconstruction and selected components.

## November 17 review corrections

- Keep save-header requests page-relative while filenames retain their absolute slot.
- Invalidate only the bad preflight slot and redraw without restarting the campaign.
- Validate negative agent selections before comparing a widened index with the roster size.
- Add matching production-linked boundary tests and source guards.

The corrected development source passed 166 native and 168 strict sanitizer tests.
The focused lanes passed four tests each, including 48 save and 16 selection assertions.
These results predate export. Exact source reconstruction and source checks remain separate.

## November 17 stream and text ownership checkpoint

- Validate animation-list and shade-table reads before publishing complete state.
- Close streams and release unpublished allocations on every rejected input.
- Validate word-wrap inputs and release complete partial lists after allocation failure.
- Retain the cumulative runtime-state, popup, credits, postal, reinforcement, and tile-resource repairs.
- Run the matching source-aware regressions in the required native check.

The latest development suites passed 161 native tests and 163 strict sanitizer tests.
Both game targets link. The 19 stream and 10 word-wrap checks failed before repair and pass afterward.
These development results predate export. Exact reconstructed-source checks remain separate.
See the [stream and text record](STREAM-TEXT-OWNERSHIP-20261117.md) for scope and limits.

## November 17 schedule, mini-event, and facility checkpoint

- Load schedule-save lists transactionally and discard serialized runtime links.
- Define the optional one-argument mini-event withdrawal policy.
- Validate facility modifier and risk entry points before table access.
- Add production-linked schedule and facility regressions plus a source-aware mini-event guard.

Development evidence: 151 native tests and 153 strict AddressSanitizer,
UndefinedBehaviorSanitizer, and LeakSanitizer tests. Both game targets link, and
fresh Clang 19 analysis finds no memory-safety or uninitialized-value issue in
the changed paths. These results belong to the development source; exact
exported-source checks are separate.
See the [runtime-boundary record](RUNTIME-BOUNDARIES-20261117.md) for scope and limits.

## November 17 runtime-boundary checkpoint

- Make save-header loading transactional and reject truncated or unterminated headers.
- Retire preserved-enemy restore state and invalid file handles on every early failure.
- Initialize move-all, Lua buddy, melee-grid, buddy-quote, and trait-probability state before use.
- Reject null quest text-entry input, bound its fixed-buffer copy, and report movement-cost output-open failures.
- Extend the production-linked world-node deletion regression without changing the validated list implementation.

Development evidence: 148 native tests and 150 strict AddressSanitizer,
UndefinedBehaviorSanitizer, and LeakSanitizer tests. Fresh Clang 19 analysis of
the changed Strategic units reports no bug. These results belong to the
development source; exact exported-source checks are separate.
See the [runtime-boundary record](RUNTIME-BOUNDARIES-20261117.md) for scope and limits.

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
