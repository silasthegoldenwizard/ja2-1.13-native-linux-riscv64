# Source update status

Updated: November 16, 2026.

This update contains the accepted virtual inventory/lifetime batch and the weapon-tooltip repair.
The release remains [v0.1.0-alpha.1](https://github.com/silasthegoldenwizard/ja2-1.13-native-linux-riscv64/releases/tag/v0.1.0-alpha.1).
Its assets remain unchanged.

- Release attachment-popup options, callbacks, and metadata safely, including rejected menu entries.
- Bound tooltips and stacked-item selections. Restore modal rendering and dismissed contract choices.
- Retire AI plans before soldier reset, portrait swapping, and nonowning autoresolve copies.

[Inventory batch evidence](INVENTORY-20261116.md) separates development results from export checks.
Original development suites passed 104 native and 106 strict ASan, UBSan, and leak tests.
The final development game loaded the same tactical save twice and exited 0 without a sanitizer finding.

Physical upper-monitor acceptance remains open.
The user's exact cropped scope artifact was not reproduced.
Private virtual-display results do not establish physical display, complete gameplay, or RISC-V acceptance.
Generic soldier copying outside the tested ownership paths remains unproved.

The [magazine checkpoint](MAGAZINE-20261116.md) and [earlier checkpoint](UPDATE-20261116.md) retain their historical evidence.
The engine retains bfVFS. [CI](CI.md) covers selected components and the published attachment fixture.

