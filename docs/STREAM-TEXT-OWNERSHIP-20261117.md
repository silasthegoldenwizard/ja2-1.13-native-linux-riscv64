# Stream and text ownership checkpoint

This cumulative update extends the native Linux source preview with accepted runtime-state and resource-ownership repairs.
It retains the earlier unpublished schedule, facility, mission, loyalty, autoresolve, AIM, popup, credits, postal, reinforcement, and tile-resource changes.

## Stream loading

Animation-list loading counts successful reads, checks line endings and counts, and rejects invalid names or incomplete reads.
The temporary animation array stays local until validation and stream closure succeed.
A shared cleanup routine clears stale state on rebuild, escape, and shutdown.
Loaded-order input and rendering require a nonempty published list.
Shade-table loading validates all 48 values in a local table before copying the complete result to active state.
Every rejected read closes its stream and releases temporary ownership.

## Word wrapping

Both word-wrap builders validate their caller inputs.
One checked helper allocates each node and its string before linking the node.
An allocation failure releases the unpublished node and the complete earlier partial list.
The successful wrapping and list-ownership contracts remain unchanged.

## Evidence and limits

The stream regression passes 19 source checks after all 19 failed before repair.
The word-wrap regression passes 10 source checks after all 10 failed before repair.
The matching checks run against reconstructed source in the required native workflow.
They establish source invariant presence. They do not dynamically exercise every failure path.

The latest development suites passed 161 native and 163 strict AddressSanitizer, UndefinedBehaviorSanitizer, and LeakSanitizer tests.
Both development game targets link. Targeted Clang 19 stream and allocation findings are gone; unrelated dead stores remain.
These results predate export. They are separate from exact-export component tests and hosted CI.

No game assets, saves, profiles, build products, or dependency updates are included.
Broad campaign, complete sound and device coverage, valid multiplayer gameplay, and native RISC-V acceptance remain incomplete.
The [publication limits](PUBLICATION-LIMITS.md) and [current status](STATUS.md) retain the wider scope.

## Review corrections

Save-list discovery now supplies a page-relative index to the header loader.
A preflight header failure invalidates only the selected absolute slot, clears selection, and redraws the list.
Recovery after an actual load failure retains its existing restart behavior.
Rebel Command rejects negative selection before widening the index and comparing it with the full roster size.
The new production-linked checks cover later-page saves, callback effects, roster sizes 127 and 128, invalid indices, and representable generic selection.

The corrected development checkpoint passed 166 native and 168 strict sanitizer tests.
Its focused lanes passed four tests each, including 48 save and 16 selection assertions.
Those results predate export. No destructive active-game, profile, or save reproduction was used.
