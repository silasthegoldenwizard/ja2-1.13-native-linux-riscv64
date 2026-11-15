#!/usr/bin/env python3
"""Check local link targets in the public Markdown documentation."""

import re
import sys
from pathlib import Path
from urllib.parse import unquote, urlsplit


ROOT = Path(__file__).resolve().parents[2]
INLINE_LINK = re.compile(
    r'!?\[[^\]\n]*\]\(\s*(?:<([^>\n]+)>|([^\s)]+))'
    r'(?:\s+["\'][^\n]*?["\'])?\s*\)'
)
REFERENCE = re.compile(
    r'^ {0,3}\[[^\]\n]+\]:\s*(?:<([^>\n]+)>|(\S+))', re.MULTILINE
)


def prose(text):
    """Keep line numbers while excluding comments and fenced/inline code."""
    text = re.sub(r'<!--.*?-->', lambda m: '\n' * m[0].count('\n'), text, flags=re.DOTALL)
    lines = []
    fence = None
    for line in text.splitlines(keepends=True):
        marker = re.match(r'^ {0,3}(`{3,}|~{3,})', line)
        if fence:
            if re.match(r'^ {0,3}' + re.escape(fence[0]) + r'{'+str(len(fence))+r',}\s*$', line):
                fence = None
            lines.append('\n' if line.endswith('\n') else '')
        elif marker:
            fence = marker[1]
            lines.append('\n' if line.endswith('\n') else '')
        else:
            lines.append(re.sub(r'(`+).*?\1', '', line))
    return ''.join(lines)


def main():
    files = [ROOT / 'README.md', *sorted((ROOT / 'docs').rglob('*.md'))]
    errors = []
    checked = 0
    for path in files:
        text = path.read_text(encoding='utf-8')
        for number, line in enumerate(text.splitlines(), 1):
            if re.match(r'^(<<<<<<< |=======\s*$|>>>>>>> )', line):
                errors.append(f'{path.relative_to(ROOT)}:{number}: merge conflict marker')
        body = prose(text)
        for pattern in (INLINE_LINK, REFERENCE):
            for match in pattern.finditer(body):
                target = match[1] or match[2]
                number = body.count('\n', 0, match.start()) + 1
                try:
                    url = urlsplit(target)
                except ValueError:
                    errors.append(f'{path.relative_to(ROOT)}:{number}: invalid target: {target}')
                    continue
                if url.scheme or url.netloc or not url.path or url.path.startswith('/'):
                    continue
                checked += 1
                destination = (path.parent / unquote(url.path)).resolve()
                if not destination.is_relative_to(ROOT) or not destination.exists():
                    errors.append(f'{path.relative_to(ROOT)}:{number}: missing local target: {target}')
    if errors:
        print('\n'.join(errors), file=sys.stderr)
        return 1
    print(f'PASS: {len(files)} Markdown files; {checked} local link targets.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
