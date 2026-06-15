#!/usr/bin/env python3
"""Validate NVM schema-evolution fixture tables."""

from __future__ import annotations

import unittest
from pathlib import Path

import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "parameters" / "tools"))

import pargen  # noqa: E402

FIXTURE_ROOT = ROOT / "parameters" / "tests" / "fixtures" / "schema_evolution"
EXPECTED_FIXTURES = {
    "v1_base",
    "v2_scalar_append",
    "v2_scalar_delete_tail",
    "v2_scalar_delete_middle",
    "v2_scalar_insert_middle",
    "v2_scalar_type_change",
    "v2_object_append",
    "v2_object_delete",
    "v2_object_capacity_change",
    "v2_mixed_append",
    "v2_mixed_scalar_compatible_object_incompatible",
    "v2_mixed_scalar_incompatible_object_compatible",
    "v2_schema_version_bump",
}


class SchemaEvolutionFixtureTests(unittest.TestCase):
    """Check fixture completeness and generator-level validity."""

    def load_fixture(self, name: str) -> list[pargen.Row]:
        """Load, validate, and ID-resolve one fixture table."""
        csv_path = FIXTURE_ROOT / name / "par_table.csv"
        rows = pargen.read_rows(csv_path)
        pargen.validate_and_resolve(rows, {}, pargen.GeneratorConfig(id_ranges={}, default_id_range=(0, 65535)))
        return rows

    def test_fixture_set_is_complete(self) -> None:
        """All planned schema-evolution fixture directories are present."""
        actual = {path.name for path in FIXTURE_ROOT.iterdir() if path.is_dir()}
        self.assertEqual(EXPECTED_FIXTURES, actual)

    def test_all_fixtures_generate_outputs(self) -> None:
        """Every fixture can produce par_table.def, layout, info, manifest, and lock content."""
        for name in sorted(EXPECTED_FIXTURES):
            with self.subTest(name=name):
                rows = self.load_fixture(name)
                stats = pargen.validate_and_resolve(rows, {}, pargen.GeneratorConfig(id_ranges={}, default_id_range=(0, 65535)))
                outputs = pargen.generate_outputs(rows, stats)
                self.assertIn("PAR_ITEM_", outputs.par_table_def)
                self.assertIn("PAR_LAYOUT_STATIC_SIGNATURE", outputs.layout_h)
                self.assertIn('"persistent_max"', outputs.manifest_json)
                self.assertIn('"ePAR_SCHEMA_BASE_U8"', outputs.lock_json)

    def test_compatible_append_fixtures_keep_v1_prefix(self) -> None:
        """Compatible append fixtures preserve the same first four persistent rows as V1."""
        v1_prefix = [row.enum for row in self.load_fixture("v1_base")]
        for name in ["v2_scalar_append", "v2_object_append", "v2_mixed_append"]:
            with self.subTest(name=name):
                rows = self.load_fixture(name)
                self.assertEqual(v1_prefix, [row.enum for row in rows[: len(v1_prefix)]])
                self.assertGreater(len(rows), len(v1_prefix))

    def test_incompatible_fixtures_change_or_remove_v1_prefix(self) -> None:
        """Incompatible fixtures intentionally remove or alter V1 rows."""
        v1_signature = [(row.resolved_id, row.type, row.min_text, row.max_text) for row in self.load_fixture("v1_base")]
        incompatible = EXPECTED_FIXTURES - {
            "v1_base",
            "v2_scalar_append",
            "v2_object_append",
            "v2_mixed_append",
            "v2_schema_version_bump",
        }
        for name in sorted(incompatible):
            with self.subTest(name=name):
                rows = self.load_fixture(name)
                signature = [(row.resolved_id, row.type, row.min_text, row.max_text) for row in rows[: len(v1_signature)]]
                self.assertNotEqual(v1_signature, signature)

    def test_schema_version_bump_fixture_keeps_table_rows_unchanged(self) -> None:
        """The schema-version-bump fixture reuses V1 rows; Kconfig provides the incompatibility trigger."""
        v1_rows = [(row.enum, row.resolved_id, row.type, row.default_text) for row in self.load_fixture("v1_base")]
        v2_rows = [(row.enum, row.resolved_id, row.type, row.default_text) for row in self.load_fixture("v2_schema_version_bump")]
        self.assertEqual(v1_rows, v2_rows)


if __name__ == "__main__":
    unittest.main()
