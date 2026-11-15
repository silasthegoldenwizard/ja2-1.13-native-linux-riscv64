"""Bind the accepted completion test to the reconstructed public source."""

import argparse
import base64
import hashlib
import json
from pathlib import Path
import subprocess
import sys


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', type=Path, required=True)
    parser.add_argument('--payload', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    binding = json.loads((here / 'bindings.json').read_text())
    assert digest((here / 'attachment_cursor_completion_tests.cpp').read_bytes()) == binding['test_sha256']
    manifest = json.loads((args.payload / 'RECONSTRUCTION-MANIFEST.json').read_text())
    records = {row['path']: row for row in manifest['records']}
    sources = {}
    offsets = {}
    for name, expected in binding['normalized_sources'].items():
        data = (args.engine / name).read_bytes()
        row = records[name]
        assert digest(data) == row['reconstructed_sha256'], name
        notice = row['notice']
        insertion = base64.b64decode(notice['insertion_base64'], validate=True)
        offset = notice['insertion_offset']
        assert offset == 0 and data.startswith(insertion), name
        data = data[len(insertion):].replace(b'\r\n', b'\n')
        assert digest(data) == expected, name + ': accepted source changed'
        sources[name] = data
        offsets[name] = insertion.count(b'\n')
    assert digest((args.engine / 'Tactical/Item Types.h').read_bytes()) == binding['header_sha256']
    args.output.mkdir(parents=True, exist_ok=True)
    for fragment in binding['fragments']:
        name = fragment['name']
        source = sources[fragment['source']]
        tail = b''.join(source.splitlines(keepends=True)[fragment['first_line'] - 1:])
        if name == 'completion_before':
            fixed = next(f for f in binding['fragments'] if f['name'] == 'completion_fixed')
            raw = tail[:fixed['bytes']].replace(
                b'if(gpItemPointer && gpItemPointer->exists()){', b'if(gpItemPointer->exists()){', 1)
        else:
            raw = tail[:fragment['bytes']]
        assert len(raw) == fragment['bytes'] and digest(raw) == fragment['sha256'], name
        line = fragment['first_line'] + offsets[fragment['source']]
        include = f'#line {line} "{fragment["source"]}"\n'.encode() + raw + b'\n'
        (args.output / (name + '.inc')).write_bytes(include)
    (args.output / 'object_methods.inc').write_text(
        ''.join(f'#include "object_method_{n}.inc"\n' for n in range(8)))
    overlay_path = args.output / 'include-overlay.json'
    subprocess.run([sys.executable, args.engine / 'platform/linux/include_overlay.py',
                    args.engine, overlay_path], check=True)
    overlay = json.loads(overlay_path.read_text())
    overlay['roots'].append({'type': 'directory', 'name': '/contract/production',
        'contents': [{'type': 'file', 'name': file.name, 'external-contents': str(file)}
                     for file in sorted(args.output.glob('*.inc'))]})
    overlay_path.write_text(json.dumps(overlay, indent=2) + '\n')
    print('Bound 11 production fragments and the real OBJECTTYPE header; accepted test hash is exact.')


if __name__ == '__main__':
    main()
