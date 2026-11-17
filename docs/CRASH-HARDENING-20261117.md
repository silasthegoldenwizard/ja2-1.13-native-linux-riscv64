# Native crash-hardening update

This source update contains the accepted November 17 crash-hardening checkpoint.
It adds bounds, initialization, ownership, and lifecycle checks in existing engine paths.
The latest delta preserves soldier construction in optimized battle-entry builds.
It also validates battle-entry state and soldier-init list cleanup.
The review corrections preserve valid long strategic routes and propagate semantic saved-game failures to the top-level loader.

- Validate UI, item, dialogue, save/load, strategic-map, movement, projectile, and multiplayer inputs before runtime access.
- Preserve item ownership through inventory callbacks, attachment changes, transfers, and cleanup.
- Add focused production-linked regressions and bounded sanitizer fuzz targets.
- Validate AI door, roaming, targeting, movement, and NPC action boundaries.
- Retain bfVFS, the RakNet companion, pinned dependencies, and existing component terms.

## Development evidence

The latest development source passed 148 native tests and 150 strict
AddressSanitizer, UndefinedBehaviorSanitizer, and LeakSanitizer tests.
The preceding accepted crash-hardening checkpoint completed 13,122,503 sanitizer fuzz inputs.
Two refreshed vehicle and militia-movement campaigns completed 2,000,000 additional inputs for the save-compatibility corrections.
Other fuzz campaigns retain their preceding-checkpoint evidence.
The latest production-linked multiplayer RPC campaign completed 1,000,000 executions without a sanitizer diagnostic.

These results belong to the accepted development source and its test environment.
They are not fresh full-suite or gameplay results from this exact exported source.
Source reconstruction, static package checks, Owner acceptance, independent Review, and required hosted checks have separate evidence scopes.
Hosted CI covers selected native components and the published attachment-completion fixture.
It does not run the complete development suite or all fuzz campaigns.

## Remaining limits

The packet fuzzer covers malformed rejection paths, not valid multiplayer gameplay.
Broad campaign, complete sound and device coverage, and native RISC-V execution remain incomplete.
Prior physical-display and cropped-artifact limits remain unless a later accepted result resolves them.
A successful bounded test does not establish correctness outside its tested state and input scope.

The source preview contains no original game assets, private profiles, saves, or prebuilt game binaries.
This update changes no release, tag, asset, dependency, or repository setting.
