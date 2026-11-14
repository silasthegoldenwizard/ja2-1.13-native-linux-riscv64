"""P710 reconstruction helper, prepared 2026-11-14.

Reconstruct and hash-check only the declared source closure from local Git objects.
This package-specific helper contains no copied engine implementation.
"""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess


def sha(data):
    return hashlib.sha256(data).hexdigest()


def apply_unified(files, patch):
    lines = patch.splitlines(keepends=True)
    cursor = 0
    touched = []
    while cursor < len(lines):
        assert lines[cursor].startswith(b'--- a/')
        name = lines[cursor][6:].rstrip(b'\n').decode()
        cursor += 1
        assert lines[cursor] == ('+++ b/' + name + '\n').encode()
        cursor += 1
        original = files[name].splitlines(keepends=True)
        output = []
        source_pos = 0
        while cursor < len(lines) and lines[cursor].startswith(b'@@ '):
            hunk = re.fullmatch(rb'@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@\n', lines[cursor])
            assert hunk, (name, lines[cursor])
            start = int(hunk[1]) - 1
            old_count = int(hunk[2] or b'1')
            new_count = int(hunk[4] or b'1')
            assert start >= source_pos
            output.extend(original[source_pos:start])
            source_pos = start
            cursor += 1
            consumed = produced = 0
            while consumed < old_count or produced < new_count:
                line = lines[cursor]
                cursor += 1
                kind, value = line[:1], line[1:]
                assert kind in (b' ', b'-', b'+'), (name, line)
                if kind in (b' ', b'-'):
                    assert original[source_pos] == value, (name, source_pos)
                    source_pos += 1
                    consumed += 1
                if kind in (b' ', b'+'):
                    output.append(value)
                    produced += 1
            assert (consumed, produced) == (old_count, new_count)
        output.extend(original[source_pos:])
        files[name] = b''.join(output)
        touched.append(name)
    return touched


def reconstruct(payload, reader):
    manifest = json.loads((payload / 'RECONSTRUCTION-MANIFEST.json').read_text())
    files = {}
    for row in manifest['records']:
        if row.get('upstream_blob_sha1'):
            data = reader(row['upstream_blob_sha1'], row['path'])
            assert sha(data) == row['base_sha256'], row['path']
            if row.get('payload_path', '').startswith('patches/'):
                data = data.replace(b'\r\n', b'\n')
                if row.get('base_append_final_newline'):
                    data += b'\n'
                assert sha(data) == row['canonical_base_sha256'], row['path']
            files[row['path']] = data
    touched = apply_unified(files, (payload / 'patches/native-base.patch').read_bytes())
    for item in json.loads((payload / 'patches/native-byte-context.json').read_text())['patches']:
        patch = base64.b64decode(item['unified_diff_base64'], validate=True)
        assert sha(patch) == item['decoded_sha256']
        touched.extend(apply_unified(files, patch))
    expected = (payload / 'PATCHED-FILES.txt').read_text().splitlines()
    assert sorted(touched) == expected and len(touched) == len(set(touched))
    for row in manifest['records']:
        if row.get('payload_path', '').startswith('overlay/'):
            files[row['path']] = (payload / row['payload_path']).read_bytes()
        assert sha(files[row['path']]) == row['reconstructed_sha256'], row['path']
    assert sorted(files) == sorted(row['path'] for row in manifest['records'])
    return files


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--base-repo', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    if args.output.exists():
        parser.error('output must be a new, absent directory')
    environment = dict(os.environ, GIT_NO_LAZY_FETCH='1', GIT_OPTIONAL_LOCKS='0')
    def reader(oid, path):
        return subprocess.check_output(['git', '--git-dir=' + str(args.base_repo), 'cat-file', 'blob', oid], env=environment)
    files = reconstruct(Path(__file__).resolve().parent, reader)
    args.output.mkdir(parents=True, exist_ok=False)
    for name, data in files.items():
        path = args.output / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    print('Verified and reconstructed', len(files), 'source/build/license files; RakNet remains excluded.')
