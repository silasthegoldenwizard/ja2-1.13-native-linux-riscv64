#!/usr/bin/env python3
"""Get the declared public base files, then use the existing verified reconstruction."""

import argparse
import json
from pathlib import Path
import subprocess
import sys


def run(*args):
    subprocess.run([str(arg) for arg in args], check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work', type=Path, required=True)
    args = parser.parse_args()
    payload = Path(__file__).resolve().parents[2]
    manifest = json.loads((payload / 'RECONSTRUCTION-MANIFEST.json').read_text())
    pin = json.loads((payload / 'SOURCE_PIN.json').read_text())
    if pin['upstream_repository'] != 'https://github.com/1dot13/source':
        parser.error('The public source route needs review for another repository.')
    if pin['upstream_commit'] != manifest['base_commit']:
        parser.error('The source pin and reconstruction manifest disagree.')
    args.work.mkdir(parents=True, exist_ok=False)
    base = args.work / 'public-base'
    run('git', 'clone', '--filter=blob:none', '--no-checkout', '--depth=1', '--no-tags',
        pin['upstream_repository'] + '.git', base)
    run('git', '-C', base, 'fetch', '--filter=blob:none', '--depth=1', '--no-tags',
        'origin', manifest['base_commit'])
    paths = sorted({row['path'] for row in manifest['records'] if row.get('upstream_blob_sha1')})
    blobs = sorted({row['upstream_blob_sha1'] for row in manifest['records'] if row.get('upstream_blob_sha1')})
    # Use Git's bulk promisor-fetch route instead of one lazy fetch per file.
    subprocess.run(['git', '-C', str(base), 'fetch', '--quiet', '--no-tags',
                    '--no-write-fetch-head', '--recurse-submodules=no', '--filter=blob:none',
                    'origin', '--stdin'], input='\n'.join(blobs) + '\n', text=True, check=True)
    path_list = args.work / 'base-paths'
    path_list.write_bytes(b'\0'.join(path.encode() for path in paths) + b'\0')
    # reconstruct.py reads the prefetched objects with lazy fetching disabled.
    run('git', '--literal-pathspecs', '-C', base, 'restore', '--source=FETCH_HEAD',
        '--worktree', '--pathspec-from-file=' + str(path_list), '--pathspec-file-nul')
    run(sys.executable, payload / 'reconstruct.py', '--base-repo', base / '.git',
        '--output', args.work / 'source')
    print('Public base:', manifest['base_commit'], flush=True)
    print('Verified reconstruction:', len(manifest['records']), 'files', flush=True)


if __name__ == '__main__':
    main()
