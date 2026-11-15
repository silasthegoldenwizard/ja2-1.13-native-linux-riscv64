# November 15 experimental source status

## Accepted local source

The historical development checkpoint passed 85 native tests and 87 strict sanitizer tests, with no failures or skips.
Sanitizer checks keep fatal ASan/UBSan behavior, leak detection, and no suppressions.
The source includes these completed repairs:

- Borderless upper-monitor presentation and aspect-preserving UI/input scaling.
- Explosion-count, soldier-layout, animation-cache, and UTF16LE save/load boundaries.
- Dirty-region scrolling and the stale top strip confirmed by the user.
- Zero-direction recoil and retired bullet-graphics node handling.
- Dynamic-dialogue allocation and registry ownership.
- Lit-door redraw after structure retirement.
- Twelve valid NPC voice sets and invalid saved voice rejection.
- C++ ownership during sector-inventory sorting and equal empty-slot comparison.

The inventory regression reproduces the original list failure before correction.
It checks permutations, nested attachments, empty entries, partial ranges, exact identity multiplicities, and a 64-entry partition case.
Independent source review found no blocking issue, with bounded coverage gaps retained.

## Gameplay limits

A protected earlier campaign completed A9 through ordinary controls in native and sanitizer runs.
The later all-feature campaign has not yet completed the same acceptance.
Separate live development trials passed transfers, stack merging, and cross-build reload checks after the stash-close repair.
These are bounded development observations, separate from packaged-source runtime acceptance.
The final sanitizer build cold-loads the preserved A9 entry and passes 71 saved-state checks.
Observed cold loads take approximately 71–77 seconds and can trigger a temporary desktop not-responding prompt.
No loading-performance repair is included.
Sound-enabled play, physical devices, broader campaigns, post-victory saves, and RISC-V remain open.

A later live sanitizer trial found a null-pointer abort while sunglasses were attached to a camo helmet.
This revision includes the one-line pointer guard after `DoAttachment`.
Its complete pre-notice file matches Owner's supplied hash.
Saved guarded-source build receipts and JUnit reports pass 85 native and 87 strict sanitizer tests without skips.
Owner separately reports a 75-check cold load.
The focused completion regression passed 24 checks in both normal and strict sanitizer lanes.
Its independent review accepted the bound production fragments and replay evidence.
Attachment outcomes, UI effects, and map pointer clearing remain test doubles.
At this freeze, Owner's acknowledgment of the mechanical export and the targeted in-game retest are unconfirmed.
The final source/archive review is recorded separately.

## VFS status

The live engine continues to use bfVFS.
The independent API replacement and its property/startup compatibility candidates are not integrated into this source.
Owner's isolated current-caller admission passes 49 FileMan, 85 INIReader, and 41 NativeText assertions per mode.
That result does not establish full provider compatibility or a live replacement.
The separately scoped startup candidate reaches a further bootstrap-path runtime gap.
Its task acceptance and future integration remain separate from this export.

## Paired source packages

The main package excludes both modified game RakNet headers and the external SDK implementation.
The separately reviewed companion supplies the game headers, SDK source, native adaptations, and component notices.
Download both source archives from the release.
The [packaged-source build record](BUILD-RESULTS.json) identifies the clean-build result and tested inputs.
The prior exact package passed both SDK and full-engine build modes.
Its fresh fixture suites passed 85/85 native and 87/87 strict sanitizer tests, with no failures or skips.
Those full game binaries were not executed by package validation.
The new attachment guard changes one reconstructed implementation file.
No new build or game execution of this refreshed package is claimed.
The earlier 85/87 results above remain evidence for local source before packaging.
Source reconstruction and clean compilation do not establish complete gameplay or executable-distribution acceptance.
