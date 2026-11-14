# Component and preparation limits

This source-only snapshot is scoped to the private repository. Publication
delivery is recorded separately; this document makes no delivery claim. Public
visibility, binary distribution and blanket legal clearance remain separate
decisions. The prior November 13 assessment applies to its precise older
payload; its qualified historical SFI/bfVFS analysis is reused for overlapping
material without pretending that it reviewed all new engine bytes.

Historical SFI-SCLA is retained verbatim as the supported original-engine
working basis. Later contribution coverage remains historical/inferred rather
than a newly authenticated express grant for every contributor. Existing
notices remain intact; current reconstructed modified files carry source-local
snapshot/change/date notices. New paths can adapt/extract engine material.
bfVFS modifications retain LGPL 2.1-or-later terms; no SFI restriction is imposed
on bfVFS itself and no engine-wide LGPL/GPL relicensing is asserted. LZMA,
libpng, Lua, libsmacker and SDL retain their component-specific notices.

RakNet is a precise, separate gap. Proposed upload contains zero paths or patch
bytes from `Multiplayer/raknet/`, but ordinary engine callers and CMake link
references remain. The accepted live source modifies `BitStream.h` and
`RakMemoryOverride.h`; those deltas are held outside the upload allowlist.
Accepted linking also depends on the separately modified RakNet 3.401 SDK from
`catacs/gameprojectx@a4c67c6c89b4e83b6d8455ac5e84719e288546f7`, component
`Libs/RakNet/Source`. Its native patch/build/probes are local-only. The earlier
assessment records historical alternative grants, Rand's separate grant and
an absent referenced AES license; no newer Facebook BSD grant is substituted.
No fresh licensing research or component clearance was performed here.

Consequently the proposed source closure intentionally omits **all** upstream
RakNet headers, including the two modified ones, plus the external library
adaptations. It reconstructs the other selected source paths exactly but cannot
build or reproduce the accepted full engine on its own. Resolve the exact held
component terms and review its bytes separately before adding it. This does not
impose a blanket hold on unrelated source preparation.

Final review must bind to the new allowlist/tree, inspect newly included source,
notices and privacy, and distinguish accepted original-source behavior from the
notice-adjusted reconstruction. No assets, images, saves, credentials, binaries,
raw logs, API/provider state or CI belong to this candidate.
