# Jagged Alliance 2 v1.13 for Linux

An experimental native port of **JA2 v1.13**, preserving its original gameplay and mechanics. Linux x86-64 is the first development target, with RISC-V64 planned next.

> [!IMPORTANT]
> **Source snapshot — November 14, 2026. Source-only private-repository scope.**
> Some required RakNet header and library changes remain outside this package pending component review. The package cannot reproduce the complete engine build on its own. No proprietary game assets or compiled binaries are included.

## Progress so far

The development build has reached the original menu, started a campaign, hired mercenaries, entered A9, and completed the first sector battle on native Linux x86-64.

| Area | Recorded result |
| --- | --- |
| Native test suite | 72/72 passed |
| ASan/UBSan/LSan suite | 74/74 passed, without suppressions |
| Source package | Exact reconstruction verified; independent package check passed with gaps |
| Still to validate | Save/reload, sound and physical devices, human visual acceptance, broader campaigns, and RISC-V64 |

The engine results apply to the local development checkpoint before packaging. This source snapshot has not had a fresh build or runtime test. A source-supported lifetime issue in the lit-door path remains untested; see [status and known issues](docs/STATUS.md).

## Explore and reconstruct the source

Use an existing local upstream Git repository containing the revision in [SOURCE_PIN.json](SOURCE_PIN.json), and choose an output path that does not already exist:

```sh
python3 reconstruct.py \
  --base-repo /path/to/existing/upstream.git \
  --output /path/to/new-empty-source
```

The helper reads pinned Git objects without fetching and verifies the reconstructed files against the manifest.

See the [Linux build guide](docs/BUILD-LINUX.md) for the development environment, dependencies, and current full-build blocker, and the [changelog](docs/CHANGELOG.md) for this snapshot’s changes.

## Source and licensing

Licenses apply by component. The historical SFI terms remain a qualified basis for the original engine source; bfVFS retains LGPL 2.1-or-later terms, and other components retain their own notices. Private hosting does not replace those terms. Public visibility and binary releases remain separate decisions.

See [publication scope and licensing](docs/PUBLICATION-LIMITS.md), [license notices](notices/), and the [reconstruction manifest](RECONSTRUCTION-MANIFEST.json) for the details.
