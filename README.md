# JA2 v1.13 — native Linux and RISC-V

Experimental port of Jagged Alliance 2 v1.13 to native Linux, with Linux x86-64 as the reference platform and Linux riscv64 as a later target. The goal is to preserve v1.13's expanded AI, inventory, weapons and combat mechanics while replacing platform-specific services with maintainable native interfaces.

This private repository currently contains project documentation only. Engine source, third-party source, game assets and binaries are withheld.

## Status — November 13, 2026

The genuine original-engine target now compiles and links as a native Linux x86-64 game executable in the local development environment. The retained full-build receipt ends with a successful `p710_native_game` target.

Focused native service and generated-resource tests have passed during development, including runtime/profile boundaries and original video-object bridges. Bounded source reviews recorded corrections and remaining gaps. These results cover their tested scopes; they do not establish complete game behavior.

The full sanitizer run for the current linked checkpoint and real gameplay acceptance are still pending. Original-engine startup, main menu, campaign, tactical battle, expanded v1.13 mechanics and new saves require end-to-end validation. RISC-V execution and real-hardware behavior remain unverified.

## Scope and next steps

1. Complete sanitizer validation against the exact linked checkpoint and address any findings.
2. Validate original-engine startup using an isolated writable profile and legitimately owned, read-only game resources.
3. Demonstrate menu, campaign and tactical battle behavior on native Linux x86-64, then verify v1.13 mechanics and new saves.
4. Build and test natively for Linux riscv64; distinguish guest results from later physical-hardware tests.

C++17 remains the primary language target. Historical save compatibility and initial multiplayer acceptance are outside the first playable milestone. Game resources, maps and configuration must still be read correctly. Original game data is not part of a source release.

## Source publication hold

Successful local builds and permission to push documentation do not resolve source redistribution requirements. Source upload remains held until all of the following are resolved for an exact reviewed payload:

- Historical RakNet 3.401 provenance, applicable licensing terms and compatibility with the combined project.
- Redistribution terms for the selected later v1.13 contributions.
- The applicable original-source grant and its combined obligations with LGPL-licensed bfVFS.
- Required prominent per-file native-change and date notices, including the recorded SFI clause 4 notice questions where that grant applies.

Any resulting source or notice changes require a new payload identity and refreshed checks before source publication.

## Upstream and references

- [JA2 v1.13 engine](https://github.com/1dot13/source) — authoritative mechanics codebase.
- [Pinned engine baseline](https://github.com/1dot13/source/tree/3394695a387a14c78e92b158cfdcf50e4068c96b).
- [JA2 Stracciatella](https://github.com/ja2-stracciatella/ja2-stracciatella) — portability reference; preserve v1.13 mechanics in this project.
