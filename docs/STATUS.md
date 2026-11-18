# Source update status

Updated: November 18, 2026.

The current native Linux source preview includes the [November 18 checkpoint](CHANGELOG.md#november-18-native-checkpoint).
The release remains [v0.1.0-alpha.1](https://github.com/silasthegoldenwizard/ja2-1.13-native-linux-riscv64/releases/tag/v0.1.0-alpha.1).
Its assets remain unchanged.

- Decode schedule-save lists transactionally and validate mini-event, facility, mission, and agent inputs.
- Initialize selected UI, Lua, combat, loyalty, and empty-input states before use.
- Release unpublished popup, credits, postal, tile, and word-wrap resources on failure.
- Restore the 20- and 30-militia reinforcement choices.
- Validate animation-list and shade-table input before publishing complete state.
- Preserve later-page save discovery and keep header-preflight errors non-destructive.
- Validate Rebel Command selections without narrowing roster sizes.
- Retain the preceding crash-hardening, AI, save-compatibility, and attachment repairs.

The [stream and text record](STREAM-TEXT-OWNERSHIP-20261117.md), [runtime-boundary record](RUNTIME-BOUNDARIES-20261117.md), and [crash-hardening record](CRASH-HARDENING-20261117.md) describe scope and limits.
The latest development suites passed 238 native tests and 240 strict sanitizer tests.
Both development game targets link.
These results predate export. They do not establish a full exported-game build or complete gameplay acceptance.
Earlier fuzz campaigns retain their recorded checkpoint scopes.

The engine retains bfVFS and the pinned dependencies.
[CI](CI.md) covers selected components, source-aware guards, and the published attachment fixture.
Broad campaign, full sound and device coverage, valid multiplayer gameplay, and native RISC-V execution remain incomplete.
Earlier physical-display and cropped-artifact limits retain their stated scope.

The [inventory batch](INVENTORY-20261116.md), [magazine checkpoint](MAGAZINE-20261116.md), and [earlier checkpoint](UPDATE-20261116.md) retain their historical evidence.
