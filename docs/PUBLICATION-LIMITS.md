# Source-preview component scope and limits

This source preview pairs an engine overlay with a separately reviewed RakNet source companion.
Each package retains its own source identity, provenance, and component terms.
The source review and build records have distinct scopes.
Neither establishes complete legal clearance or executable-distribution acceptance.

The November 13 assessment applies to its exact older package.
Its qualified historical SFI/bfVFS analysis gives context for common material.
It did not review all new engine bytes.

The historical SFI-SCLA text stays unchanged.
It supplies the working basis for the original engine source.
Records for later contributions are historical or inferred.
They are not a new explicit grant from each contributor.
Existing notices stay unchanged.
Modified reconstructed files have source-local snapshot, change, and date notices.
New file paths can contain adapted or extracted engine material.

bfVFS modifications keep LGPL 2.1-or-later terms.
This snapshot does not impose SFI restrictions on bfVFS itself.
It does not give the complete engine a new LGPL/GPL license.
LZMA, libpng, Lua, libsmacker, and SDL keep their component-specific notices.
Repository visibility does not replace these terms.

RakNet is supplied as a separate source companion.
The package contains no source paths or patch bytes from `Multiplayer/raknet/`.
Ordinary engine callers and CMake link references remain.
The accepted live source changes `BitStream.h` and `RakMemoryOverride.h`.
Those changes are supplied in the companion's game headers.

Accepted linking also needs the separately modified RakNet 3.401 SDK.
Its source is `catacs/gameprojectx@a4c67c6c89b4e83b6d8455ac5e84719e288546f7`, component `Libs/RakNet/Source`.
The companion includes its native patch, source, build recipe, and probe sources.
The earlier assessment records historical alternative grants and Rand's separate grant.
A referenced AES license was absent from the older package.
The historical AEScrypt 0.7 license has since been recovered, with checksum and source-lineage evidence.
The exact corrected source companion passed targeted independent review after its Rand notice corrections.
The archive calls the text `LICENSE`; RakNet refers to `LICENSE-EST`.
Do not claim that the original RakNet-named file was recovered.
The companion retains all four EST conditions, the disclaimer, and the applicable advertising acknowledgment.
Its notice identifies `rijndael.cpp`, `Rijndael.h`, and `Rijndael-Boxes.h`, while preserving later adaptation notices.
The Jenkins alternatives, Rand's separate Library GPL terms, and SHA1 origins remain individually scoped.
No newer Facebook BSD grant replaces these terms.
This update reuses that bounded review and makes no complete component or executable-distribution clearance claim.

The engine overlay omits RakNet headers and external SDK implementation bytes.
The companion supplies 124 game headers, the pinned SDK source, and its native adaptations.
Use both packages for the selected complete source-input route.
The companion explicitly supplies the full LGPL 2.1 text as a permitted later version for Rand.
Its modified Rand file retains its original grant and a supported change/date notice.
The executable source/relinking and combined-distribution questions remain separate from this source preview.

The [packaged-source build record](BUILD-RESULTS.json) gives the exact technical result and its scope.
Keep accepted local-source gameplay separate from the build of source with packaging notices.

This candidate does not include assets, images, saves, credentials, or binaries.
It does not include raw logs, API/provider state, or CI configuration.
