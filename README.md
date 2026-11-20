# Jagged Alliance 2 v1.13 for native Linux

This project ports the [JA2 v1.13 engine](https://github.com/1dot13/source) to
native Linux. The first target is Linux x86-64. RISC-V64 support is planned.

## Can I play it now?

**Development builds can run the game on Linux x86-64, but this repository does
not yet offer a ready-to-run public game download.**

The reviewed source package for the next public release is
`v0.1.0-alpha.2`. It is for developers and experienced testers. A prebuilt
player package is on hold while we resolve its redistribution terms.

| Item | Current state |
| --- | --- |
| Native Linux x86-64 | Runs in development |
| Ready-to-run public package | Not available yet |
| Single-player | Main development target |
| Multiplayer | Disabled in the current source package |
| RISC-V64 | Planned; not validated on hardware |
| Original JA2 game data | Not included |

## What do players need?

- A legally obtained copy of Jagged Alliance 2 with the required v1.13 data.
- A recent x86-64 Linux system with X11 or XWayland.
- Patience with an alpha port. Crashes and gameplay defects are still possible.

Keep your game data, profiles, and saved games private. This repository does
not contain or redistribute them.

## Where do I start?

If you want a simple installation, wait for a player-package release and watch
the [Releases](https://github.com/silasthegoldenwizard/ja2-1.13-native-linux-riscv64/releases)
page.

If you can build C++ software:

1. Open the Releases page and select `v0.1.0-alpha.2`. If it is not listed yet,
   publication is still in progress.
2. Verify the archive with the attached `SHA256SUMS` file.
3. Extract the archive.
4. Follow its `BUILD-AND-RELINK.md` instructions.
5. Supply your own JA2 v1.13 game data when you run the built engine.

The alpha.2 source packet is single-player and excludes RakNet. It pins the
tested dependency versions and includes the license, provenance, integrity, and
relinking material needed for that snapshot.

## What works today?

The native port has reached real Linux gameplay testing. Development work has
covered startup, rendering, input, audio, saves, sector transitions, tactical
combat, strategic inventory, and several crash families. This does not mean
that the full campaign is complete or stable.

Automated native and sanitizer tests cover selected engine boundaries. They do
not replace normal gameplay testing.

## Help test the port

Useful reports include:

- The exact action that caused a crash or visual defect.
- The game screen and sector.
- The selected game options.
- Whether the problem occurs again after loading the same save.
- A screenshot or local crash report, when available.

Do not upload proprietary game data. Share a minimal saved game privately only
if a maintainer requests it and you agree.

## Technical information

- [November 19 repository validation checkpoint](docs/STATUS.md)
- [Change history](docs/CHANGELOG.md)
- [Continuous-integration scope](docs/CI.md)
- [Alpha.1 component terms and distribution limits](docs/PUBLICATION-LIMITS.md)
- [Third-party notices](notices/)

The original engine and all third-party components keep their own license
terms. Read the included notices before reuse or redistribution.
