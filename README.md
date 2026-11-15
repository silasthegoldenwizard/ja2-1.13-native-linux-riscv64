# Jagged Alliance 2 v1.13 for native Linux

An experimental native Linux port of [1dot13's](https://github.com/1dot13) [JA2 v1.13](https://github.com/1dot13/source) engine.
The goal is to preserve v1.13 gameplay and support Linux x86-64, followed by RISC-V64.
RISC-V64 remains untested.

## Current release

[v0.1.0-alpha.1](https://github.com/silasthegoldenwizard/ja2-1.13-native-linux-riscv64/releases/tag/v0.1.0-alpha.1) is a source preview, released on November 15, 2026.
Full campaign reliability remains unverified, and a sanitizer shutdown leak remains open.

The release contains no game assets or prebuilt game binaries.
Original JA2 game data is required separately.

## Build

1. Download **both** source archives from the release: the engine overlay and the RakNet 3.401 companion.
2. Follow the [Linux build guide](docs/BUILD-LINUX.md) for source reconstruction, dependencies, and compilation.

## Project information

- [Release status, known issues, and validation cutoffs](docs/STATUS.md)
- [Changes](docs/CHANGELOG.md)
- [Component terms](docs/PUBLICATION-LIMITS.md) and [notices](notices/)

The engine and third-party components retain their individual terms.
Read the component terms before reuse or redistribution.
