# Source update status

Updated: November 16, 2026.

The November 16 source update contains accepted native Linux development changes after the attachment guard.
The current release remains [v0.1.0-alpha.1](https://github.com/silasthegoldenwizard/ja2-1.13-native-linux-riscv64/releases/tag/v0.1.0-alpha.1).
This update does not replace its assets.

## Development result

- Face-gear palette ownership now permits a tested sanitizer game exit without the earlier shutdown leak.
- Recon, map decoding, schedules, placement, translation, SameBuilding, and targetless recoil checks have bounded repairs or regressions.
- Original development suites passed 94 native and 96 strict ASan, UBSan, and leak tests.
- A9 victory used six normal kills and two cheat-assisted kills.
  Two sector transfers conserved counts. The opposite build reloaded the saved victory and inventory state.

[Current evidence and limits](UPDATE-20261116.md) separates original-source results from exact exported-package checks.
The engine retains bfVFS. Unintegrated replacement candidates are excluded.

## Remaining limits

Minor AP redraw, surrender-text, load-flicker, and temporary top-strip reports remain open.
The current in-session reload passed twice. No post-load source patch is included.
Broader campaign reliability, complete sound/device coverage, and native RISC-V execution remain unverified.
This repository supplies source. It establishes no executable-distribution acceptance.

## Historical records

[SOURCE_PIN.json](../SOURCE_PIN.json) and [RECONSTRUCTION-MANIFEST.json](../RECONSTRUCTION-MANIFEST.json) identify the current candidate.
[TEST-EVIDENCE.json](TEST-EVIDENCE.json) retains the earlier inventory checkpoint.
[BUILD-RESULTS.json](BUILD-RESULTS.json) retains the earlier package-build and attachment evidence.
Those historical tests do not describe a new build of the current export.
The existing [CI guide](CI.md) defines the required public checks.

