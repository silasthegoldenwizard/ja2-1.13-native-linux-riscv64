# Source update status

Updated: November 16, 2026.

The current source update repairs the strategic-inventory magazine callback and UI image cleanup.
The user confirmed that magazine reloading works in the patched desktop game.
The existing release remains [v0.1.0-alpha.1](https://github.com/silasthegoldenwizard/ja2-1.13-native-linux-riscv64/releases/tag/v0.1.0-alpha.1).
This update does not replace its assets.

## Current development result

- Guard the cursor pointer after the ammunition callback consumes a magazine.
- Complete deferred button-image retirement after callback deletion.
- Remove inventory and description controls before releasing their images.

The original development suites passed 96 native and 98 strict ASan, UBSan, and leak tests.
[Magazine update evidence](MAGAZINE-20261116.md) states the focused checks and remaining limits.
These results belong to the development source, not a complete game build of this export.

## Remaining limits

Raven's earlier portrait failure has no confirmed causal link to the repaired resource leak.
A fresh portrait check remains open.
The earlier fast-forward closure remains unresolved.
Enhanced-description and money cleanup have source review but no direct new fixture coverage.

The [prior checkpoint](UPDATE-20261116.md) retains its A9 and cross-build evidence.
Broad campaign, sound/device, visual, and RISC-V acceptance remain incomplete.
The engine retains bfVFS. This update adds no replacement provider.
The existing [CI guide](CI.md) defines the required public checks.

