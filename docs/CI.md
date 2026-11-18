# Documentation checks

The `docs` check runs for each pull request and each push to `main`.
It checks `README.md` and Markdown files under `docs/`.
Relative inline links, image links, and reference definitions must point to existing repository files or directories.
Merge conflict markers also cause a failure.
The check excludes code examples and comments.
It does not check external URLs, heading anchors, or all Markdown syntax.

Run the same check locally:

```sh
python3 .github/scripts/check_docs.py
```

The [workflow](../.github/workflows/docs.yml) uses a GitHub-hosted runner and a read-only repository token.
It uses no game assets, private secrets, or installed Python packages.

## Merge process

Open a pull request with a short title that describes the main change.
Aim for 50 characters.
Keep the description brief and link detailed evidence when necessary.

The repository supports automatic squash merging after required checks pass.
Enable auto-merge only for a change that has the applicable review and acceptance.
The short pull request title becomes the squash commit title.
This documentation check does not validate engine behavior or workflow security.
Code and workflow changes still need their applicable technical review and checks.
Release tags, release notes, and release assets use a separate publication step.

## Native component checks

The `native` job reconstructs the 1625 declared source files from the pinned public base and this overlay.
The existing CMake/CTest component lane builds production code with C++17 and Clang on Ubuntu 24.04.
It runs seven normal tests and nine sanitizer tests, including the existing sanitizer diagnostic probes.
VFS, image, SDL, FFmpeg, FreeType, RakNet, and full-game integration are outside this component lane.

The [attachment regression](../tests/attachment-completion/README.md) adds 24 controlled completion cases in normal and strict sanitizer modes.
It binds its production fragments to the reconstructed source and uses real engine headers.
Those declarations also need headers from the project's pinned SDL source; this lane does not build or link SDL.
It does not add a new production guard; the guard is already in `v0.1.0-alpha.1`.
Its controlled outcomes do not establish full callback, attachment-policy, UI, or campaign behavior.

The jobs run for every PR without path filters.
Only checks with a demonstrated failing control and corrected pass become required.
Future source changes that affect the frozen regression bindings need a reviewed binding update.

## Source-aware state and ownership checks

The required native job checks cumulative strategic state, allocation ownership, reinforcement choices, tile construction, stream loading, and word wrapping.
The correction adds a save-preflight guard and updates the Rebel Command bounds guard.
The lane runs 90 source invariants across 11 Python checkers against reconstructed source.
These checks inspect source invariants. They do not execute rare allocation failures or establish complete gameplay behavior.
The development suites and static-analyzer results retain their separate source-checkpoint scopes.
