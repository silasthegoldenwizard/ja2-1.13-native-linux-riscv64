# P710 native component source overlay

This is a source-only private synchronization payload, held for exact payload review and coordinated execution under the existing private-update authorization. The bounded assessment supports the noncommercial overlay route with qualified historical licensing evidence. It reconstructs the accepted November 12 native component harness against `1dot13/source` commit `3394695a387a14c78e92b158cfdcf50e4068c96b`. It does not contain a playable game or current live-engine work.

The payload contains new checkpoint files, one exact patch for 38 inherited files, three replacement inherited headers, and component notices. Unchanged engine and vendor sources are obtained separately from the pinned upstream. New paths can contain extracted or adapted original logic; their names do not establish independent authorship. Exact origins and before/after hashes are in `RECONSTRUCTION-MANIFEST.json`.

This overlay reuses the earlier file-specific notice preparation for all 41 changed inherited paths and all 53 new source/build/test paths, including all 25 new `sgp/` paths. These 94 notices identify the November 12 checkpoint and retain the actual November 12 notice-insertion date; this separate overlay was prepared November 13. They do not invent every earlier contributor edit date. CRLF is explicitly canonicalized to LF where recorded. Existing source comments and notices are otherwise retained. The historical SFI text is supplied verbatim with scoped provenance; bfVFS remains separately LGPL 2.1 or later.

## Reconstruct from a separately obtained local upstream

Use an existing complete upstream Git store containing the pinned revision, an empty new output directory, and this payload. The following is an example for a future authorized recipient; preparation did not fetch, reconstruct a working tree, or build anything.

```sh
export P710_BASE_REPO=/path/to/existing/source-repository
export P710_OVERLAY=/path/to/this-payload
export P710_RECONSTRUCTED=/path/to/new-empty/source
python3 - <<'PY'
import os, pathlib, subprocess
payload = pathlib.Path(os.environ['P710_OVERLAY'])
source = pathlib.Path(os.environ['P710_RECONSTRUCTED'])
source.mkdir(parents=True, exist_ok=False)
files = (payload / 'BASE-FILES.txt').read_text().splitlines()
archive = subprocess.Popen(['git', '-C', os.environ['P710_BASE_REPO'], 'archive',
    '3394695a387a14c78e92b158cfdcf50e4068c96b', '--', *files], stdout=subprocess.PIPE)
result = subprocess.run(['tar', '-x', '-C', str(source)], stdin=archive.stdout)
archive.stdout.close()
assert archive.wait() == 0 and result.returncode == 0
for name in (payload / 'PATCHED-FILES.txt').read_text().splitlines():
    path = source / name
    path.write_bytes(path.read_bytes().replace(b'\r\n', b'\n'))
PY
git -C "$P710_RECONSTRUCTED" apply --check "$P710_OVERLAY/patches/native-base.patch"
git -C "$P710_RECONSTRUCTED" apply "$P710_OVERLAY/patches/native-base.patch"
cp -R "$P710_OVERLAY/overlay/." "$P710_RECONSTRUCTED/"
python3 - <<'PY'
import hashlib, json, os, pathlib
payload = pathlib.Path(os.environ['P710_OVERLAY'])
source = pathlib.Path(os.environ['P710_RECONSTRUCTED'])
rows = json.loads((payload / 'RECONSTRUCTION-MANIFEST.json').read_text())['records']
assert sorted(p.relative_to(source).as_posix() for p in source.rglob('*') if p.is_file()) == sorted(r['path'] for r in rows)
for row in rows:
    assert hashlib.sha256((source / row['path']).read_bytes()).hexdigest() == row['reconstructed_sha256'], row['path']
print('All 258 reconstructed source/build/test paths match.')
PY
```

The exact reconstructed tree identity is in the manifest. Run the separate [Linux build instructions](docs/BUILD-LINUX.md) from that reconstructed source root only after reviewing applicable permissions and providing the existing documented dependencies. This recipe does not install dependencies or include game assets.

The frozen predecessor passed 15 normal and 17 sanitizer tests and exact X11 demo comparisons on its preparation host. Those historical results are reused as evidence; this overlay has only deterministic source reconstruction verification. No new compilation/runtime, second-host, CI, full-game or RISC-V result is claimed.

Read [publication limits and component scope](docs/PUBLICATION-LIMITS.md) before any upload or release. Private hosting does not itself establish redistribution permission. The existing authorized private source sync remains separate from any future public-release decision.
