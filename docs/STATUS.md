# Source update status

Updated: November 17, 2026.

This update contains the accepted native crash-hardening checkpoint.
The release remains [v0.1.0-alpha.1](https://github.com/silasthegoldenwizard/ja2-1.13-native-linux-riscv64/releases/tag/v0.1.0-alpha.1).
Its assets remain unchanged.

- Validate UI, item, dialogue, save/load, strategic, projectile, and multiplayer boundaries.
- Preserve ownership and lifecycle rules through inventory callbacks and cleanup.
- Add production-linked regressions and sanitizer fuzz targets for repaired paths.
- Preserve soldier construction in optimized builds and validate battle-entry state and soldier-init list cleanup.
- Preserve long strategic routes and stop the top-level load on semantic subsystem failure.
- Include the accepted AI runtime guards and their matching source-aware regression.

The [crash-hardening record](CRASH-HARDENING-20261117.md) separates development results from exact-export checks.
Original development suites passed 143 native tests and 145 strict sanitizer tests.
The preceding crash-hardening checkpoint completed 13,122,503 accepted fuzz inputs without a sanitizer diagnostic.
The save-compatibility corrections add 2,000,000 refreshed vehicle and militia-movement fuzz inputs.
Other campaigns retain their preceding-checkpoint evidence.
The latest malformed multiplayer RPC campaign accounts for 1,000,000 of those executions.
These are development-source results, not a full exported-game build or complete gameplay result.

Broad campaign, full sound and device coverage, valid multiplayer gameplay, and native RISC-V execution remain incomplete.
Earlier physical-display and cropped-artifact limits retain their stated scope.
The engine retains bfVFS. [CI](CI.md) covers selected components and the published attachment fixture.

The [inventory batch](INVENTORY-20261116.md), [magazine checkpoint](MAGAZINE-20261116.md), and [earlier checkpoint](UPDATE-20261116.md) retain their historical evidence.
