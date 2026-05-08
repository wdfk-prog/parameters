#!/usr/bin/env python3
"""Unit tests for the CSV parameter generator."""

from __future__ import annotations

import csv
import tempfile
import unittest
from pathlib import Path

import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "parameters" / "tools"))

import pargen  # noqa: E402


COLUMNS = [
    "group",
    "section",
    "condition",
    "enum",
    "id",
    "type",
    "name",
    "min",
    "max",
    "default",
    "unit",
    "access",
    "read_roles",
    "write_roles",
    "persistent",
    "desc",
    "comment",
]


def write_csv(path: Path, rows: list[dict[str, str]]) -> None:
    """Write a temporary pargen CSV schema."""
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=COLUMNS)
        writer.writeheader()
        writer.writerows(rows)


def base_rows() -> list[dict[str, str]]:
    """Return a valid minimal schema covering scalar and object rows."""
    return [
        {
            "group": "SYSTEM",
            "section": "Control",
            "condition": "",
            "enum": "ePAR_SYS_MODE",
            "id": "0",
            "type": "U8",
            "name": "System Mode",
            "min": "0",
            "max": "3",
            "default": "1",
            "unit": "",
            "access": "RW",
            "read_roles": "ALL",
            "write_roles": "ALL",
            "persistent": "1",
            "desc": "System mode.",
            "comment": "",
        },
        {
            "group": "SYSTEM",
            "section": "Control",
            "condition": "",
            "enum": "ePAR_SYS_TEMP",
            "id": "AUTO",
            "type": "F32",
            "name": "System Temp",
            "min": "-40.0",
            "max": "125.0",
            "default": "25.0",
            "unit": "degC",
            "access": "RO",
            "read_roles": "ALL",
            "write_roles": "NONE",
            "persistent": "0",
            "desc": "System temperature.",
            "comment": "",
        },
        {
            "group": "OBJECT",
            "section": "Network",
            "condition": "(1 == PAR_CFG_ENABLE_TYPE_STR)",
            "enum": "ePAR_WIFI_SSID",
            "id": "50001",
            "type": "STR",
            "name": "WiFi SSID",
            "min": "0",
            "max": "32",
            "default": "ap",
            "unit": "",
            "access": "RW",
            "read_roles": "ALL",
            "write_roles": "ALL",
            "persistent": "0",
            "desc": "WiFi SSID.",
            "comment": "",
        },
    ]


class PargenTests(unittest.TestCase):
    """Validate pargen schema checks and generated outputs."""

    def run_generator(self, rows: list[dict[str, str]]) -> tuple[list[pargen.Row], dict[str, object]]:
        """Load, validate, and resolve a temporary schema."""
        with tempfile.TemporaryDirectory() as tmp:
            tmpdir = Path(tmp)
            csv_path = tmpdir / "par_table.csv"
            write_csv(csv_path, rows)
            loaded_rows = pargen.read_rows(csv_path)
            stats = pargen.validate_and_resolve(
                loaded_rows,
                {},
                pargen.GeneratorConfig(id_ranges={"SYSTEM": (0, 99), "OBJECT": (50000, 50999)}, default_id_range=(0, 65535)),
            )
            return loaded_rows, stats

    def test_valid_schema_generates_outputs(self) -> None:
        """A valid schema produces table, layout, info, manifest, and lock output."""
        rows, stats = self.run_generator(base_rows())
        outputs = pargen.generate_outputs(rows, stats)
        self.assertIn("PAR_ITEM_U8", outputs.par_table_def)
        self.assertNotIn("PAR_LAYOUT_TABLE_OFFSET_", outputs.par_table_def)
        self.assertNotIn("PAR_LAYOUT_TABLE_OBJECT_POOL_OFFSET_", outputs.par_table_def)
        self.assertIn("#if (1 == PAR_CFG_ENABLE_TYPE_STR)", outputs.par_table_def)
        self.assertIn("PAR_LAYOUT_STATIC_COUNT8", outputs.layout_h)
        self.assertIn("PAR_LAYOUT_STATIC_SIGNATURE", outputs.layout_h)
        self.assertIn("PAR_LAYOUT_STATIC_OFFSET_ePAR_SYS_MODE", outputs.layout_h)
        self.assertIn("g_par_generated_info", outputs.info_c)
        self.assertIn('"collisions": 0', outputs.manifest_json)
        self.assertIn("ePAR_SYS_TEMP", outputs.lock_json)

    def test_checked_in_outputs_are_synchronized(self) -> None:
        """Checked-in generated outputs match the current generator implementation."""
        args = pargen.parse_args([
            "--csv", str(ROOT / "parameters" / "schema" / "par_table.csv"),
            "--id-lock", str(ROOT / "parameters" / "schema" / "par_id_lock.json"),
            "--config", str(ROOT / "parameters" / "schema" / "pargen.json"),
            "--out-def", str(ROOT / "par_table.def"),
            "--out-dir", str(ROOT / "parameters" / "generated"),
            "--manifest", str(ROOT / "parameters" / "generated" / "par_manifest.json"),
        ])
        rows = pargen.read_rows(Path(args.csv))
        lock = pargen.read_lock(Path(args.id_lock))
        cfg = pargen.load_config(Path(args.config))
        stats = pargen.validate_and_resolve(rows, lock, cfg)
        outputs = pargen.generate_outputs(rows, stats)
        pargen.verify_outputs(args, outputs)

    def test_all_conditional_rows_generate_layout_header(self) -> None:
        """Layout generation works when every schema row is conditionally compiled."""
        rows = base_rows()[:1]
        rows[0]["condition"] = "(1 == PAR_CFG_ENABLE_TYPE_STR)"
        generated_rows, generated_stats = self.run_generator(rows)
        outputs = pargen.generate_outputs(generated_rows, generated_stats)
        self.assertIn("PAR_LAYOUT_ROW_ENABLED_ePAR_SYS_MODE", outputs.layout_h)
        self.assertIn("PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_SYS_MODE", outputs.layout_h)

    def test_str_default_preserves_surrounding_whitespace(self) -> None:
        """String defaults preserve surrounding spaces as payload bytes."""
        rows = base_rows()
        rows[2]["default"] = " ap "
        generated_rows, generated_stats = self.run_generator(rows)
        outputs = pargen.generate_outputs(generated_rows, generated_stats)
        self.assertEqual(generated_rows[2].default_text, " ap ")
        self.assertIn('(const uint8_t *)" ap "', outputs.par_table_def)
        self.assertIn(".len = 4U", outputs.par_table_def)

    def test_object_pool_offset_table_keeps_standard_initializer(self) -> None:
        """Object-pool offset table remains valid when object rows compile out."""
        rows, stats = self.run_generator(base_rows())
        outputs = pargen.generate_outputs(rows, stats)
        object_table = outputs.layout_c.split("const uint32_t g_par_layout_static_object_pool_offset[ePAR_NUM_OF] = {", 1)[1]
        self.assertIn("0u, /* Keep the initializer valid", object_table)
        self.assertIn("[ePAR_WIFI_SSID]", object_table)

    def test_default_value_out_of_range_fails(self) -> None:
        """Scalar defaults outside [min,max] are rejected before C generation."""
        rows = base_rows()
        rows[0]["default"] = "4"
        with self.assertRaises(pargen.PargenError):
            self.run_generator(rows)

    def test_c_integer_suffixes_accept_hex_tokens(self) -> None:
        """C integer suffix cleanup accepts decimal and hexadecimal tokens."""
        self.assertEqual(pargen.parse_c_int("123UL"), 123)
        self.assertEqual(pargen.parse_c_int("0xAAU"), 0xAA)
        self.assertEqual(pargen.parse_c_int("0xFFUL"), 0xFF)
        self.assertEqual(pargen.parse_c_int("(-0x1FUL)"), -0x1F)

    def test_byte_defaults_accept_hex_integer_suffixes(self) -> None:
        """Byte-array defaults accept C hexadecimal integer suffixes."""
        rows = base_rows()
        rows.append(
            {
                "group": "OBJECT",
                "section": "Key",
                "condition": "",
                "enum": "ePAR_AES_KEY",
                "id": "50000",
                "type": "BYTES",
                "name": "AES Key",
                "min": "3",
                "max": "3",
                "default": "0x00U 0xAAU 0xFFUL",
                "unit": "",
                "access": "RW",
                "read_roles": "ALL",
                "write_roles": "ALL",
                "persistent": "0",
                "desc": "AES key fragment.",
                "comment": "",
            }
        )
        generated_rows, generated_stats = self.run_generator(rows)
        outputs = pargen.generate_outputs(generated_rows, generated_stats)
        self.assertIn("0x00U, 0xAAU, 0xFFU", outputs.par_table_def)

    def test_duplicate_id_fails(self) -> None:
        """Explicit duplicated external IDs are rejected."""
        rows = base_rows()
        rows[1]["id"] = "0"
        with self.assertRaises(pargen.PargenError):
            self.run_generator(rows)


    def test_auto_id_skips_reserved_explicit_hash_buckets(self) -> None:
        """AUTO allocation must avoid buckets already reserved by later explicit rows."""
        rows = base_rows()[:2]
        rows[0]["id"] = "AUTO"
        rows[1]["id"] = "3"
        with tempfile.TemporaryDirectory() as tmp:
            tmpdir = Path(tmp)
            csv_path = tmpdir / "par_table.csv"
            write_csv(csv_path, rows)
            parsed = pargen.read_rows(csv_path)
            stats = pargen.validate_and_resolve(
                parsed,
                {},
                pargen.GeneratorConfig(id_ranges={"SYSTEM": (0, 10)}, default_id_range=(0, 10)),
            )
            self.assertEqual(stats["hash_bits"], 2)
            self.assertEqual(parsed[0].resolved_id, 1)

    def test_hash_bucket_collision_fails(self) -> None:
        """IDs that map to the same static hash bucket are rejected."""
        rows = base_rows()
        rows[1]["id"] = "3"
        with self.assertRaises(pargen.PargenError):
            self.run_generator(rows)

    def test_auto_id_reserves_later_locked_ids(self) -> None:
        """AUTO allocation must not consume IDs locked by later AUTO rows."""
        rows = base_rows()[:2]
        rows[0]["id"] = "AUTO"
        rows[1]["id"] = "AUTO"
        parsed = [pargen.normalize_row(index, row) for index, row in enumerate(rows, start=2)]
        pargen.validate_and_resolve(
            parsed,
            {"ePAR_SYS_TEMP": 1},
            pargen.GeneratorConfig(id_ranges={"SYSTEM": (0, 10)}, default_id_range=(0, 10)),
        )
        self.assertEqual(parsed[0].resolved_id, 0)
        self.assertEqual(parsed[1].resolved_id, 1)

    def test_layout_signature_changes_when_rows_reorder(self) -> None:
        """Layout signature must detect same-type row reordering."""
        rows = [base_rows()[0], dict(base_rows()[0])]
        rows[1]["enum"] = "ePAR_SYS_MODE_ALT"
        rows[1]["id"] = "1"
        rows[1]["name"] = "System Mode Alt"
        generated_rows, generated_stats = self.run_generator(rows)
        reversed_rows, reversed_stats = self.run_generator(list(reversed(rows)))
        generated_outputs = pargen.generate_outputs(generated_rows, generated_stats)
        reversed_outputs = pargen.generate_outputs(reversed_rows, reversed_stats)
        self.assertNotEqual(
            pargen.layout_signature_expr(generated_rows, "A"),
            pargen.layout_signature_expr(reversed_rows, "A"),
        )
        self.assertIn("PAR_LAYOUT_STATIC_SIGNATURE_A", generated_outputs.layout_h)
        self.assertIn("PAR_LAYOUT_STATIC_SIGNATURE_A", reversed_outputs.layout_h)

    def test_layout_signature_uses_compiled_index_for_conditional_middle_rows(self) -> None:
        """Conditional rows in the middle must shift later generated indexes."""
        rows = base_rows()
        rows = [rows[0], rows[2], rows[1]]
        generated_rows, generated_stats = self.run_generator(rows)
        generated_outputs = pargen.generate_outputs(generated_rows, generated_stats)
        term_line = next(
            line
            for line in generated_outputs.layout_h.splitlines()
            if "PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_SYS_TEMP" in line
        )
        self.assertIn("PAR_LAYOUT_ROW_ENABLED_ePAR_WIFI_SSID", term_line)
        self.assertIn("1u + (PAR_LAYOUT_ROW_ENABLED_ePAR_WIFI_SSID)", term_line)


    def test_generated_layout_header_lines_are_bounded(self) -> None:
        """Bundled layout header lines remain within portable tool limits."""
        args = pargen.parse_args([
            "--csv", str(ROOT / "parameters" / "schema" / "par_table.csv"),
            "--id-lock", str(ROOT / "parameters" / "schema" / "par_id_lock.json"),
            "--config", str(ROOT / "parameters" / "schema" / "pargen.json"),
        ])
        rows = pargen.read_rows(Path(args.csv))
        lock = pargen.read_lock(Path(args.id_lock))
        cfg = pargen.load_config(Path(args.config))
        stats = pargen.validate_and_resolve(rows, lock, cfg)
        outputs = pargen.generate_outputs(rows, stats)
        longest_line = max(len(line) for line in outputs.layout_h.splitlines())
        self.assertLess(longest_line, 1000)
        self.assertIn("PAR_LAYOUT_STATIC_SIGNATURE_A_CHUNK_0", outputs.layout_h)

    def test_array_default_count_fails(self) -> None:
        """Fixed-size object arrays must provide exactly max elements."""
        rows = base_rows()
        rows.append(
            {
                "group": "OBJECT",
                "section": "LUT",
                "condition": "",
                "enum": "ePAR_U8_LUT",
                "id": "50003",
                "type": "ARR_U8",
                "name": "U8 LUT",
                "min": "4",
                "max": "4",
                "default": "1,2,3",
                "unit": "",
                "access": "RW",
                "read_roles": "ALL",
                "write_roles": "ALL",
                "persistent": "0",
                "desc": "U8 LUT.",
                "comment": "",
            }
        )
        with self.assertRaises(pargen.PargenError):
            self.run_generator(rows)


if __name__ == "__main__":
    unittest.main()
