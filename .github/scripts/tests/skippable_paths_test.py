#!/usr/bin/env python3
"""Tests for skippable_paths (stdlib-only PSDB path classifier)."""

import importlib
import sys
import unittest
from pathlib import Path


SCRIPTS_DIR = Path(__file__).resolve().parent.parent


class SkippablePathsImportTest(unittest.TestCase):
    def test_psdb_helper_imports_without_therock_submodule(self):
        """Regression: PSDB sparse checkout has .github/scripts only."""
        scripts_dir = str(SCRIPTS_DIR)
        saved_path = sys.path[:]
        try:
            sys.path.insert(0, scripts_dir)
            for name in ("therock_configure_ci", "skippable_paths"):
                sys.modules.pop(name, None)
            importlib.import_module("skippable_paths")
            importlib.import_module("psdb_pr_skippable_paths")
        finally:
            sys.path[:] = saved_path


class SkippablePathsRulesTest(unittest.TestCase):
    def setUp(self):
        sys.path.insert(0, str(SCRIPTS_DIR))
        import skippable_paths

        self.skippable_paths = skippable_paths

    def test_docs_only_paths_are_skippable(self):
        self.assertTrue(self.skippable_paths.is_path_skippable("README.md"))
        self.assertFalse(
            self.skippable_paths.check_for_non_skippable_path(["README.md", "docs/x.rst"])
        )

    def test_source_changes_are_not_skippable(self):
        self.assertTrue(
            self.skippable_paths.check_for_non_skippable_path(
                ["README.md", "projects/rocminfo/src/main.cpp"]
            )
        )


if __name__ == "__main__":
    unittest.main()
