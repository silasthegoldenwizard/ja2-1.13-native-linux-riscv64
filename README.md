# Jagged Alliance 2 v1.13 for native Linux

This experimental project ports JA2 v1.13 to Linux while retaining its gameplay and mechanics.
Linux x86-64 is the current development target. RISC-V64 remains untested.

This is the `v0.1.0-alpha.1` experimental source preview.
Use both the engine overlay and the RakNet 3.401 source companion from the [release](https://github.com/silasthegoldenwizard/ja2-1.13-native-linux-riscv64/releases/tag/v0.1.0-alpha.1).
Together they supply the selected engine and native SDK source inputs.
The engine overlay alone does not include the RakNet headers or library source.
Original game data is required separately.
The source packages contain no proprietary game assets, profiles, saves, credentials, or compiled binaries.

## Tested development source

| Check | Saved result |
|---|---|
| Native regression suite | 85/85 pass |
| Strict ASan/UBSan/LSan suite | 87/87 pass, with leak detection and no suppressions |
| Sector-inventory sorting | 2,204 assertions per mode |
| Dynamic dialogue | 56 lifecycle assertions per mode |
| Lit-door lifetime | 545 checks across 24 scenarios per mode |
| NPC battle-sound cache | 147 assertions per mode |
| Real upper-monitor cold load | 71/71 saved-state checks on the final sanitizer build |

The local build previously completed A9 in a protected ordinary campaign.
The later all-feature campaign exposed additional defects, now covered by focused regressions.
Broad all-feature gameplay acceptance remains open.
Separate live development trials exercised transfers, stack merging, and cross-build saves after the inventory-sort repair.
Those trials are separate from the packaged-source fixture runs.

**Attachment guard included:** the item-attachment callback now checks the held-item pointer after `DoAttachment`.
That operation can clear the pointer before the refresh call.
The exact pre-notice file matches Owner's supplied SHA-256.
An independently accepted completion-only regression reproduces the earlier null member call.
The fixed normal and strict sanitizer lanes each pass 24 checks with empty runtime stderr.
Actual production fragments execute, but attachment outcomes and UI effects are test doubles.
At this freeze, Owner's acknowledgment of the mechanical export and the targeted in-game retest are unconfirmed.
The final source/archive review is separate from these engineering results.

The table above describes the accepted development checkpoint before the attachment guard and package notices.
The [packaged-source build record](docs/BUILD-RESULTS.json) identifies the separate clean-build result and its exact inputs.
The earlier package built both native and sanitizer game targets and passed 85/85 and 87/87 fixture tests without skips.
Those package results precede the attachment guard.
Saved Owner build receipts show successful guarded native and sanitizer game builds.
Their JUnit reports contain 85 and 87 passing tests, with no failures or skips.
Owner separately reports a 75-check cold load after the guard.
The refreshed packaged source has not had a new build or game execution.
The build record keeps the prior package evidence separate from current reported results.
Source pins and reconstruction records retain their snapshot-preparation scope.
Build verification does not establish broad campaign reliability or native RISC-V support.
The two extra sanitizer tests are intentional diagnostic canaries.

## Reconstruct the source

Use an existing local upstream Git object store with the pinned revision.
Select an output directory that does not exist.

```sh
python3 reconstruct.py \
  --base-repo /path/to/existing/upstream.git \
  --output /path/to/new-empty-source
```

The helper reads pinned objects and verifies every declared reconstructed file.
It does not fetch source, install dependencies, compile, or launch the game.

- [Status and remaining checks](docs/STATUS.md)
- [Linux build context and RakNet closure](docs/BUILD-LINUX.md)
- [Packaged-source build record](docs/BUILD-RESULTS.json)
- [Changes since the previous source update](docs/CHANGELOG.md)
- [Saved test identities](docs/TEST-EVIDENCE.json)
- [Source and dependency pins](SOURCE_PIN.json)
- [Component limits](docs/PUBLICATION-LIMITS.md)
- [Notices](notices/)

The engine and libraries retain their individual terms.
No whole-project relicensing or complete legal clearance is asserted.
