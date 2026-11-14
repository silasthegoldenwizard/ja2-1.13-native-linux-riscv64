# Jagged Alliance 2 v1.13 for Linux

This experimental project ports **JA2 v1.13** to Linux.
It must keep the original gameplay and mechanics.
Linux x86-64 is the first target.
RISC-V64 is a later target.

> [!IMPORTANT]
> **Source snapshot: November 14, 2026. Private repository. Source files only.**
> Some necessary RakNet header and library changes are outside this package.
> These changes need a component license review.
> This package alone cannot reproduce the complete engine build.
> The package does not include proprietary game assets or compiled binaries.

## Progress so far

The development build reached the original menu.
It started a campaign and hired mercenaries.
It entered A9 and completed the first sector battle on native Linux x86-64.

| Area | Recorded result |
| --- | --- |
| Native test suite | 72/72 passed |
| ASan/UBSan/LSan suite | 74/74 passed, without suppressions |
| Source package | Exact source reconstruction passed. The independent package check passed with gaps. |
| Tests still necessary | Save/reload, sound and physical devices, human visual acceptance, broader campaigns, and RISC-V64 |

The engine results apply to the local development checkpoint before the package was made.
This source snapshot has no new build or runtime test.
Source review found an untested lifetime issue in the lit-door path.
The [status and known issues](docs/STATUS.md) page gives the details.

## Explore and reconstruct the source

The source overlay contains changes for a separate upstream source copy.
Reconstruction applies these changes to that copy.

1. Use an existing local upstream Git repository with the revision in [SOURCE_PIN.json](SOURCE_PIN.json).
2. Select an output path that does not exist.
3. Run this command:

```sh
python3 reconstruct.py \
  --base-repo /path/to/existing/upstream.git \
  --output /path/to/new-empty-source
```

The helper reads pinned Git objects.
It does not fetch objects.
It compares the reconstructed files with the manifest.

The [Linux build guide](docs/BUILD-LINUX.md) gives the development environment, dependencies, and full-build limitation.
The [changelog](docs/CHANGELOG.md) lists the changes in this snapshot.

## Source and licensing

Each component keeps its own license terms.
The historical SFI terms give a qualified basis for the original engine source.
bfVFS keeps LGPL 2.1-or-later terms.
Other components keep their own notices.
Private hosting does not replace these terms.
Public access and binary releases need separate decisions.

The following records give the details:

- [Publication scope and licensing](docs/PUBLICATION-LIMITS.md)
- [License notices](notices/)
- [Reconstruction manifest](RECONSTRUCTION-MANIFEST.json)
