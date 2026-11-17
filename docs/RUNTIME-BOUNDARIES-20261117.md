# Native runtime-boundary update

This checkpoint adds focused repairs for failures found after the preceding
crash-hardening export.

## Repairs

- Save headers now publish state only after a complete, bounded read. Fixed-size
  description and version fields require an in-range terminator, and callers
  stop when header loading fails.
- Preserved-enemy loading initializes and conditionally closes its file handle,
  routes open failure through shared cleanup, and clears restore mode on failure.
- Move-all cursor setup retains only an eligible soldier. Lua buddy lookup has a
  defined `-1` result. Same-tile melee grids start at the target tile. Buddy
  quote indices and unknown-class trait probability are bounded.
- Quest debug text entry rejects a null source and bounds its 256-character
  buffer. Strategic movement-cost export rejects null paths and failed opens.
- Schedule-save loading validates the complete serialized extent, builds a
  private list with cleared runtime links, rolls back incomplete input, and
  publishes only after a complete decode.
- One-argument mini-event money withdrawal has a defined false default for its
  optional force-to-zero policy.
- Facility modifier and risk entry points reject missing soldiers, invalid
  sectors, and out-of-range selectors before indexing production tables. Their
  accumulator and result state is initialized.

The world land-list analyzer report was reproduced with a three-node list,
duplicate tile indices, and strict sanitizers. Every node was retired exactly
once and both owners were cleared, so the implementation was retained and the
regression was expanded instead of changing production code. Two allocation
size reports were also retained because they compare signed and unsigned forms
of the same fixed-width 16-bit and 32-bit types.

## Development evidence

- 151 of 151 native tests passed.
- 153 of 153 strict AddressSanitizer, UndefinedBehaviorSanitizer, and
  LeakSanitizer tests passed with no diagnostic.
- Both native and sanitizer game targets linked.
- Fresh Clang 19 analysis found no memory-safety or uninitialized-value issue
  in the newly changed Strategic translation units. Three unrelated existing
  dead-store notes remain in `Facilities.cpp`.
- The selected production files retain their CRLF line endings.

The saved failing-before cases include a one-byte save header, missing
underground metadata during enemy restoration, six undefined-state source
boundaries, null quest text, a null movement-cost output path, a first-node
schedule append, a missing facility soldier, and an omitted Lua policy argument.

These checks cover the named boundaries, not every campaign, map, script,
interface state, device, or valid multiplayer exchange. The package contains no
game assets, player profiles, saves, or prebuilt game binary. Release assets,
tags, dependencies, and repository settings are unchanged.
