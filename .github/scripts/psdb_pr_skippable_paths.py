#!/usr/bin/env python3
"""GitHub Actions helper: PR path skippability for PSDB trigger.

Lists changed files on a pull request via ``gh(1)`` and writes step outputs
``any_file`` and ``non_skippable`` using the same rules as
``therock_configure_ci.check_for_non_skippable_path`` (SKIPPABLE_PATH_PATTERNS).

See https://github.com/ROCm/rocm-systems/blob/develop/.github/scripts/therock_configure_ci.py
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path


def _set_github_output(name: str, value: str) -> None:
    path = os.environ.get("GITHUB_OUTPUT", "")
    if not path:
        print("GITHUB_OUTPUT is not set", file=sys.stderr)
        sys.exit(1)
    with open(path, "a", encoding="utf-8") as f:
        f.write(f"{name}={value}\n")


def _list_pr_filenames(repo: str, pr_number: int) -> list[str]:
    proc = subprocess.run(
        [
            "gh",
            "api",
            f"repos/{repo}/pulls/{pr_number}/files",
            "--paginate",
            "-q",
            ".[].filename",
        ],
        capture_output=True,
        text=True,
        check=True,
    )
    return [ln.strip() for ln in proc.stdout.splitlines() if ln.strip()]


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Classify PR file paths for PSDB skippable-only detection."
    )
    parser.add_argument(
        "--pr-number",
        type=int,
        default=int(os.environ.get("PR_NUMBER", "0")),
        metavar="N",
        help="Pull request number (default: PR_NUMBER env).",
    )
    args = parser.parse_args()

    if args.pr_number <= 0:
        print("Invalid or missing PR number; set --pr-number or PR_NUMBER.", file=sys.stderr)
        return 1

    repo = os.environ.get("GITHUB_REPOSITORY", "")
    if not repo:
        print("GITHUB_REPOSITORY must be set.", file=sys.stderr)
        return 1

    scripts_dir = Path(__file__).resolve().parent
    if str(scripts_dir) not in sys.path:
        sys.path.insert(0, str(scripts_dir))

    from skippable_paths import check_for_non_skippable_path

    try:
        files = _list_pr_filenames(repo, args.pr_number)
    except subprocess.CalledProcessError as exc:
        print(exc.stderr or exc.stdout or str(exc), file=sys.stderr)
        return 1

    if not files:
        _set_github_output("any_file", "false")
        _set_github_output("non_skippable", "false")
        return 0

    _set_github_output("any_file", "true")
    non_skip = check_for_non_skippable_path(files)
    _set_github_output("non_skippable", "true" if non_skip else "false")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
