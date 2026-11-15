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
