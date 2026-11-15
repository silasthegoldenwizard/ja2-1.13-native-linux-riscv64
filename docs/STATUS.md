# Release status

Updated: November 15, 2026.
Scope: published source preview [v0.1.0-alpha.1](https://github.com/silasthegoldenwizard/ja2-1.13-native-linux-riscv64/releases/tag/v0.1.0-alpha.1), commit [0bd14100a9ac](https://github.com/silasthegoldenwizard/ja2-1.13-native-linux-riscv64/commit/0bd14100a9ac0692f767170947cc3cefcf82067c).
This page describes that release; subsequent local development has separate acceptance records.

## Current release result

- The source includes the attachment pointer guard.
  The maintainer accepted the exact export, and the user confirmed the camo-helmet sequence in the corrected sanitizer game.
- The guarded development source passed 85 native and 87 strict sanitizer tests, with no failures or skips.
  A separate completion regression passed 24 cases per mode and reproduced the original failure before the fix.
  Its attachment outcomes and UI effects use test doubles.
- The refreshed source archive passed reconstruction and focused source/archive review.
  That exact packaged source was not separately rebuilt or run.
- The release requires both the engine overlay and the RakNet source companion.
  It keeps bfVFS active; replacement-provider work is outside this release.

## Known limits at publication

A separate intentional shutdown exposed a LeakSanitizer finding: 21,267,928 retained bytes in 41,620 allocations, with exit code 134.
The attachment action completed with empty runtime stderr.
Clean normal-exit, restart, and reload acceptance for that checkpoint remains open.

Broad campaign reliability, complete sound/device coverage, and native RISC-V execution remain unverified.
The release contains source only and establishes no executable-distribution acceptance.

## Evidence dates and validation cutoffs

All records below were prepared on November 15, 2026.
Their validation cutoffs differ by stage.

| Record | Cutoff and meaning |
| --- | --- |
| [SOURCE_PIN.json](../SOURCE_PIN.json) and [TEST-EVIDENCE.json](TEST-EVIDENCE.json) | Historical inventory-transfer checkpoint, before the attachment guard and later package validation. Preparation, pending-transfer, and held-RakNet statements belong to that checkpoint. |
| [RECONSTRUCTION-MANIFEST.json](../RECONSTRUCTION-MANIFEST.json) | Attachment-fix source freeze. It identifies the exact reconstructed files and one-file guard revision. Pending acceptance fields record the state at that freeze. |
| [BUILD-RESULTS.json](BUILD-RESULTS.json) | Frozen engineering summary. It separates the earlier package build, guarded development builds, and focused completion regression. It claims no new build of the refreshed package. |
| [Release notes](https://github.com/silasthegoldenwizard/ja2-1.13-native-linux-riscv64/releases/tag/v0.1.0-alpha.1) | Published acceptance cutoff. The post-freeze section records maintainer acceptance, the user's camo-helmet result, and the separate shutdown leak. |

Historical records retain their original bytes and hashes.
Later acceptance supplements their recorded state without changing the tests they describe.
Use this page and the release notes for the published acceptance summary.
