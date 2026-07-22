"""Lightweight path-skippability rules shared by CI and PSDB workflows.

This module is intentionally free of TheRock submodule imports so scripts like
``psdb_pr_skippable_paths.py`` can run after a sparse checkout of
``.github/scripts`` only.
"""

from __future__ import annotations

import fnmatch
from typing import Iterable, Optional

# Paths matching any of these patterns are considered to have no influence over
# build or test workflows so any related jobs can be skipped if all paths
# modified by a commit/PR match a pattern in this list.
SKIPPABLE_PATH_PATTERNS = [
    "docs/*",
    ".gitignore",
    "*.md",
    "*.rtf",
    "*.rst",
    "*/.markdownlint-ci2.yaml",
    "*/.readthedocs.yaml",
    "*/.spellcheck.local.yaml",
    "*/.wordlist.txt",
    "projects/*/docs/*",
    "projects/*/.gitignore",
    "projects/rocr-runtime/libhsakmt/src/dxg/*",
    "shared/*/docs/*",
    "shared/*/.gitignore",
    "experimental/python/perfxpert/*",
    ".github/CODEOWNERS",
    ".github/label*.yml",
    ".github/workflows/labeler.yml",
    ".github/workflows/amdsmi-manylinux-build.yml",
    ".github/workflows/rocjitsu-corpus-tests.yml",
]


def is_path_skippable(path: str) -> bool:
    """Return True when ``path`` matches a skippable pattern."""
    return any(fnmatch.fnmatch(path, pattern) for pattern in SKIPPABLE_PATH_PATTERNS)


def check_for_non_skippable_path(paths: Optional[Iterable[str]]) -> bool:
    """Return True if at least one path is not in the skippable set."""
    if paths is None:
        return False
    return any(not is_path_skippable(p) for p in paths)
