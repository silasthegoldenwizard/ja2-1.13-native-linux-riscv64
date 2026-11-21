#!/usr/bin/env python3
"""Get the pinned public inputs and verify a fresh source reconstruction."""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys


def run(*args, **kwargs):
    subprocess.run([str(arg) for arg in args], check=True, **kwargs)


def git_output(repo, *args):
    return subprocess.check_output(['git', '-C', str(repo), *args], text=True).strip()


def write_pack(repo, path, object_ids):
    objects = ''.join(oid + '\n' for oid in sorted(set(object_ids)))
    with path.open('wb') as pack:
        run('git', '-C', repo, 'pack-objects', '--stdout', '--no-reuse-delta',
            input=objects.encode('ascii'), stdout=pack)


def load_pack(repo, path):
    # Index the objects in a fresh repository so a cache never supplies Git
    # configuration, hooks, refs, or reconstructed files.
    run('git', 'init', '--quiet', repo)
    with path.open('rb') as pack:
        run('git', '-C', repo, 'index-pack', '--stdin', stdin=pack)


def fetch_base(base, repository, revision, records):
    run('git', 'clone', '--filter=blob:none', '--no-checkout', '--depth=1', '--no-tags',
        repository + '.git', base)
    run('git', '-C', base, 'fetch', '--filter=blob:none', '--depth=1', '--no-tags',
        'origin', revision)
    paths = sorted({row['path'] for row in records})
    path_list = base.parent / 'base-paths'
    path_list.write_bytes(b'\0'.join(path.encode() for path in paths) + b'\0')
    # The partial clone downloads the declared blobs. The reconstruction reads
    # them without lazy fetching and verifies their SHA-256 hashes.
    run('git', '--literal-pathspecs', '-C', base, 'restore', '--source=FETCH_HEAD',
        '--worktree', '--pathspec-from-file=' + str(path_list), '--pathspec-file-nul')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument('--work', type=Path)
    action.add_argument('--cache-key', action='store_true')
    parser.add_argument('--input-cache', type=Path)
    parser.add_argument('--write-input-cache', action='store_true')
    args = parser.parse_args()
    if args.write_input_cache and (args.work is None or args.input_cache is None):
        parser.error('--write-input-cache requires --work and --input-cache.')

    payload = Path(__file__).resolve().parents[2]
    manifest = json.loads((payload / 'RECONSTRUCTION-MANIFEST.json').read_text())
    pin = json.loads((payload / 'SOURCE_PIN.json').read_text())
    repository = pin['upstream_repository']
    base_revision = manifest['base_commit']
    if repository != 'https://github.com/1dot13/source':
        parser.error('The public source route needs review for another repository.')
    if pin['upstream_commit'] != base_revision:
        parser.error('The source pin and reconstruction manifest disagree.')

    records = [row for row in manifest['records'] if row.get('upstream_blob_sha1')]
    base_objects = sorted({row['upstream_blob_sha1'] for row in records})
    sdl_pin = pin['sdl3']
    sdl_revision = sdl_pin['commit']
    if args.cache_key:
        inputs = [repository, base_revision, base_objects, sdl_pin['repository'],
                  sdl_pin['tag'], sdl_revision]
        print(hashlib.sha256(json.dumps(inputs).encode('ascii')).hexdigest())
        return

    args.work.mkdir(parents=True, exist_ok=False)
    base = args.work / 'public-base'
    sdl = args.work / 'sdl'
    base_pack = args.input_cache / 'base.pack' if args.input_cache else None
    sdl_pack = args.input_cache / 'sdl-headers.pack' if args.input_cache else None
    cached = base_pack is not None and base_pack.is_file() and sdl_pack.is_file()
    if cached:
        print('Using cached public Git objects.', flush=True)
        load_pack(base, base_pack)
        load_pack(sdl, sdl_pack)
    else:
        print('Fetching public Git objects.', flush=True)
        fetch_base(base, repository, base_revision, records)
        run('git', 'clone', '--filter=blob:none', '--no-checkout', '--depth=1', '--no-tags',
            '--branch', sdl_pin['tag'], sdl_pin['repository'] + '.git', sdl)
        if git_output(sdl, 'rev-parse', 'HEAD') != sdl_revision:
            parser.error('The SDL header tag does not match the pinned commit.')

    run(sys.executable, payload / 'reconstruct.py', '--base-repo', base / '.git',
        '--output', args.work / 'source')
    # The attachment fixture uses the SDL declarations without building SDL.
    run('git', '-C', sdl, 'restore', '--source=' + sdl_revision, '--worktree', 'include')
    if args.write_input_cache and not cached:
        args.input_cache.mkdir(parents=True, exist_ok=True)
        write_pack(base, base_pack, base_objects)
        sdl_objects = [sdl_revision, git_output(sdl, 'rev-parse', sdl_revision + '^{tree}')]
        sdl_objects.extend(git_output(sdl, 'ls-tree', '-rt', '--object-only',
                                      sdl_revision, '--', 'include').splitlines())
        write_pack(sdl, sdl_pack, sdl_objects)
    print('SDL declaration headers:', sdl_revision, flush=True)
    print('Public base:', base_revision, flush=True)
    print('Verified reconstruction:', len(manifest['records']), 'files', flush=True)


if __name__ == '__main__':
    main()
