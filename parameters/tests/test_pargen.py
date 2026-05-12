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


    def test_load_config_missing_and_invalid_range(self) -> None:
        """Missing config uses defaults and invalid ranges fail."""
        with tempfile.TemporaryDirectory() as tmp:
            tmpdir = Path(tmp)
            self.assertEqual(pargen.load_config(tmpdir / "missing.json").default_id_range, (0, 65535))
            bad_cfg = tmpdir / "bad.json"
            bad_cfg.write_text('{"default_id_range": [10, 1]}', encoding="utf-8")
            with self.assertRaises(pargen.PargenError):
                pargen.load_config(bad_cfg)

    def test_read_rows_rejects_missing_header_columns_and_empty_schema(self) -> None:
        """CSV input must include required columns and at least one data row."""
        with tempfile.TemporaryDirectory() as tmp:
            tmpdir = Path(tmp)
            missing_columns = tmpdir / "missing_columns.csv"
            missing_columns.write_text("enum,id,type\n", encoding="utf-8")
            with self.assertRaises(pargen.PargenError):
                pargen.read_rows(missing_columns)

            empty_schema = tmpdir / "empty.csv"
            write_csv(empty_schema, [])
            with self.assertRaises(pargen.PargenError):
                pargen.read_rows(empty_schema)

    def test_read_lock_rejects_non_object_and_out_of_range_ids(self) -> None:
        """Lock files must be JSON objects with uint16 external IDs."""
        with tempfile.TemporaryDirectory() as tmp:
            tmpdir = Path(tmp)
            not_object = tmpdir / "not_object.json"
            not_object.write_text("[]", encoding="utf-8")
            with self.assertRaises(pargen.PargenError):
                pargen.read_lock(not_object)

            out_of_range = tmpdir / "out_of_range.json"
            out_of_range.write_text('{"ids": {"ePAR_SYS_MODE": 65536}}', encoding="utf-8")
            with self.assertRaises(pargen.PargenError):
                pargen.read_lock(out_of_range)

            non_integer = tmpdir / "non_integer.json"
            non_integer.write_text('{"ids": {"ePAR_SYS_MODE": "not-an-id"}}', encoding="utf-8")
            with self.assertRaises(ValueError):
                pargen.read_lock(non_integer)

    def test_validate_rejects_bad_enum_type_access_role_and_missing_desc(self) -> None:
        """Basic schema validation rejects malformed row metadata."""
        cases = [
            ("enum", "BAD_ENUM"),
            ("type", "U64"),
            ("access", "WO"),
            ("read_roles", "ROOT"),
            ("desc", ""),
        ]
        for field, value in cases:
            rows = base_rows()
            rows[0][field] = value
            with self.subTest(field=field):
                with self.assertRaises(pargen.PargenError):
                    self.run_generator(rows)

    def test_invalid_id_tokens_and_duplicate_enums_fail(self) -> None:
        """Invalid external IDs and duplicate enums are rejected."""
        rows = base_rows()
        rows[0]["id"] = "not-a-number"
        with self.assertRaises(pargen.PargenError):
            self.run_generator(rows)

        rows = base_rows()
        rows[1]["enum"] = rows[0]["enum"]
        with self.assertRaises(pargen.PargenError):
            self.run_generator(rows)

    def test_object_defaults_cover_arr_u16_and_arr_u32_lengths(self) -> None:
        """ARR_U16 and ARR_U32 defaults generate byte lengths, not element counts."""
        rows = base_rows()
        rows.extend([
            {
                "group": "OBJECT",
                "section": "LUT",
                "condition": "",
                "enum": "ePAR_U16_LUT",
                "id": "50004",
                "type": "ARR_U16",
                "name": "U16 LUT",
                "min": "2",
                "max": "2",
                "default": "100,200",
                "unit": "",
                "access": "RW",
                "read_roles": "ALL",
                "write_roles": "ALL",
                "persistent": "0",
                "desc": "U16 LUT.",
                "comment": "",
            },
            {
                "group": "OBJECT",
                "section": "LUT",
                "condition": "",
                "enum": "ePAR_U32_LUT",
                "id": "50005",
                "type": "ARR_U32",
                "name": "U32 LUT",
                "min": "2",
                "max": "2",
                "default": "1000,2000",
                "unit": "",
                "access": "RW",
                "read_roles": "ALL",
                "write_roles": "ALL",
                "persistent": "0",
                "desc": "U32 LUT.",
                "comment": "",
            },
        ])
        generated_rows, generated_stats = self.run_generator(rows)
        outputs = pargen.generate_outputs(generated_rows, generated_stats)
        self.assertIn(".len = (uint16_t)(2U * sizeof(uint16_t))", outputs.par_table_def)
        self.assertIn(".len = (uint16_t)(2U * sizeof(uint32_t))", outputs.par_table_def)
        self.assertEqual(generated_rows[3].obj_default_len, 4)
        self.assertEqual(generated_rows[4].obj_default_len, 8)

    def test_verify_reports_missing_output(self) -> None:
        """Verification mode reports missing generated output files."""
        rows, stats = self.run_generator(base_rows())
        outputs = pargen.generate_outputs(rows, stats)
        with tempfile.TemporaryDirectory() as tmp:
            tmpdir = Path(tmp)
            args = pargen.parse_args([
                "--csv", str(tmpdir / "schema.csv"),
                "--id-lock", str(tmpdir / "lock.json"),
                "--config", str(tmpdir / "cfg.json"),
                "--out-def", str(tmpdir / "par_table.def"),
                "--out-dir", str(tmpdir / "generated"),
                "--manifest", str(tmpdir / "generated" / "par_manifest.json"),
            ])
            with self.assertRaises(pargen.PargenError):
                pargen.verify_outputs(args, outputs)

    def test_verify_reports_stale_output(self) -> None:
        """Verification mode reports existing generated files with stale content."""
        with tempfile.TemporaryDirectory() as tmp:
            tmpdir = Path(tmp)
            csv_path = tmpdir / "schema.csv"
            lock_path = tmpdir / "lock.json"
            cfg_path = tmpdir / "cfg.json"
            out_def = tmpdir / "par_table.def"
            out_dir = tmpdir / "generated"
            manifest = out_dir / "par_manifest.json"
            write_csv(csv_path, base_rows())
            lock_path.write_text("{}", encoding="utf-8")
            cfg_path.write_text('{"default_id_range": [0, 65535]}', encoding="utf-8")
            common_args = [
                "--csv", str(csv_path),
                "--id-lock", str(lock_path),
                "--config", str(cfg_path),
                "--out-def", str(out_def),
                "--out-dir", str(out_dir),
                "--manifest", str(manifest),
            ]
            self.assertEqual(pargen.main(common_args), 0)
            out_def.write_text(out_def.read_text(encoding="utf-8") + "/* stale */\n", encoding="utf-8")
            self.assertEqual(pargen.main(common_args + ["--verify"]), 1)

    def test_main_check_success_and_invalid_csv_failure(self) -> None:
        """CLI --check succeeds for valid CSV and returns 1 for invalid CSV."""
        with tempfile.TemporaryDirectory() as tmp:
            tmpdir = Path(tmp)
            csv_path = tmpdir / "par_table.csv"
            lock_path = tmpdir / "par_id_lock.json"
            cfg_path = tmpdir / "pargen.json"
            write_csv(csv_path, base_rows())
            lock_path.write_text("{}", encoding="utf-8")
            cfg_path.write_text('{"default_id_range": [0, 65535]}', encoding="utf-8")
            self.assertEqual(pargen.main([
                "--csv", str(csv_path),
                "--id-lock", str(lock_path),
                "--config", str(cfg_path),
                "--check",
            ]), 0)

            bad_csv = tmpdir / "bad.csv"
            bad_csv.write_text("enum,id,type\n", encoding="utf-8")
            self.assertEqual(pargen.main([
                "--csv", str(bad_csv),
                "--id-lock", str(lock_path),
                "--config", str(cfg_path),
                "--check",
            ]), 1)


    def test_csv_reader_preserves_quoted_text_for_c_string_output(self) -> None:
        """Quoted CSV fields survive parsing and remain valid C strings."""
        rows = base_rows()
        rows[0]["name"] = "Mode, Primary"
        rows[0]["desc"] = 'Line one\nquoted "mode" with comma, and slash token'
        generated_rows, generated_stats = self.run_generator(rows)
        outputs = pargen.generate_outputs(generated_rows, generated_stats)
        self.assertEqual(generated_rows[0].name, "Mode, Primary")
        self.assertEqual(generated_rows[0].desc, 'Line one\nquoted "mode" with comma, and slash token')
        self.assertIn('"Mode, Primary"', outputs.par_table_def)
        self.assertIn('Line one\\nquoted \\\"mode\\\" with comma', outputs.par_table_def)

    def test_manifest_output_is_deterministic_across_runs(self) -> None:
        """Manifest generation is deterministic for identical resolved schemas."""
        rows_a, stats_a = self.run_generator(base_rows())
        rows_b, stats_b = self.run_generator(base_rows())
        self.assertEqual(pargen.generate_outputs(rows_a, stats_a).manifest_json,
                         pargen.generate_outputs(rows_b, stats_b).manifest_json)

    def test_nested_condition_expression_is_emitted_verbatim(self) -> None:
        """Nested condition expressions remain intact in generated C guards."""
        rows = base_rows()
        rows[2]["condition"] = "((1 == PAR_CFG_ENABLE_TYPE_STR) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED))"
        generated_rows, generated_stats = self.run_generator(rows)
        outputs = pargen.generate_outputs(generated_rows, generated_stats)
        self.assertIn("#if ((1 == PAR_CFG_ENABLE_TYPE_STR) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED))",
                      outputs.par_table_def)
        self.assertIn("PAR_LAYOUT_ROW_ENABLED_ePAR_WIFI_SSID", outputs.layout_h)

    def test_auto_id_range_exhaustion_reports_group_range(self) -> None:
        """AUTO allocation fails clearly when a group ID range is exhausted."""
        rows = base_rows()[:2]
        rows[0]["id"] = "AUTO"
        rows[1]["id"] = "AUTO"
        parsed = [pargen.normalize_row(index, row) for index, row in enumerate(rows, start=2)]
        with self.assertRaisesRegex(pargen.PargenError, r"no free AUTO ID in range 0\.\.0.*SYSTEM"):
            pargen.validate_and_resolve(
                parsed,
                {},
                pargen.GeneratorConfig(id_ranges={"SYSTEM": (0, 0)}, default_id_range=(0, 0)),
            )

    def test_config_rejects_malformed_id_ranges(self) -> None:
        """Generator config rejects malformed or reversed ID ranges."""
        with tempfile.TemporaryDirectory() as tmp:
            tmpdir = Path(tmp)
            bad_group = tmpdir / "bad_group.json"
            bad_group.write_text('{"id_ranges": {"SYSTEM": [1]}}', encoding="utf-8")
            with self.assertRaisesRegex(pargen.PargenError, "id_ranges.SYSTEM"):
                pargen.load_config(bad_group)

            bad_default = tmpdir / "bad_default.json"
            bad_default.write_text('{"default_id_range": [2, 1]}', encoding="utf-8")
            with self.assertRaisesRegex(pargen.PargenError, "default_id_range"):
                pargen.load_config(bad_default)

    def test_persistent_accepts_true_false_aliases(self) -> None:
        """Persistent flag aliases normalize to the expected integer policy."""
        true_aliases = ["true", "TRUE", "yes", "Y", "1"]
        false_aliases = ["false", "FALSE", "no", "N", "0"]
        for alias in true_aliases:
            with self.subTest(alias=alias):
                self.assertEqual(pargen.parse_persistent(alias, 2), 1)
        for alias in false_aliases:
            with self.subTest(alias=alias):
                self.assertEqual(pargen.parse_persistent(alias, 2), 0)

    def test_role_mask_union_emits_parenthesized_c_expression(self) -> None:
        """Role mask unions are normalized to parenthesized C expressions."""
        rows = base_rows()
        rows[0]["read_roles"] = "PUBLIC|SERVICE"
        generated_rows, generated_stats = self.run_generator(rows)
        outputs = pargen.generate_outputs(generated_rows, generated_stats)
        self.assertIn("(ePAR_ROLE_PUBLIC | ePAR_ROLE_SERVICE)", outputs.par_table_def)

    def test_access_full_tokens_are_normalized(self) -> None:
        """Full ePAR_ACCESS_* tokens remain accepted and emitted."""
        rows = base_rows()
        rows[0]["access"] = "ePAR_ACCESS_RO"
        generated_rows, generated_stats = self.run_generator(rows)
        outputs = pargen.generate_outputs(generated_rows, generated_stats)
        self.assertIn("ePAR_ACCESS_RO", outputs.par_table_def)

    def test_utf8_string_default_uses_byte_length(self) -> None:
        """UTF-8 string defaults use encoded byte length, not character count."""
        rows = base_rows()
        rows[2]["default"] = "測試"
        rows[2]["max"] = "8"
        generated_rows, generated_stats = self.run_generator(rows)
        outputs = pargen.generate_outputs(generated_rows, generated_stats)
        self.assertEqual(len(rows[2]["default"]), 2)
        self.assertEqual(generated_rows[2].obj_default_len, 6)
        self.assertIn(".len = 6U", outputs.par_table_def)

    def test_raw_c_object_initializer_is_rejected(self) -> None:
        """Raw C object initializers are rejected before length validation."""
        rows = base_rows()
        rows[2]["default"] = "c:{ 0x00 }"
        with self.assertRaisesRegex(pargen.PargenError, "raw C object initializers"):
            self.run_generator(rows)

    def test_blank_csv_rows_are_ignored(self) -> None:
        """Blank CSV rows do not create empty parameters."""
        with tempfile.TemporaryDirectory() as tmp:
            tmpdir = Path(tmp)
            csv_path = tmpdir / "par_table.csv"
            with csv_path.open("w", encoding="utf-8", newline="") as stream:
                writer = csv.DictWriter(stream, fieldnames=COLUMNS)
                writer.writeheader()
                writer.writerow({column: "" for column in COLUMNS})
                writer.writerows(base_rows())
            loaded_rows = pargen.read_rows(csv_path)
            self.assertEqual([row.enum for row in loaded_rows],
                             ["ePAR_SYS_MODE", "ePAR_SYS_TEMP", "ePAR_WIFI_SSID"])

    def test_lock_file_wrapper_and_legacy_object_are_supported(self) -> None:
        """Lock files accept both wrapper and legacy plain-object formats."""
        with tempfile.TemporaryDirectory() as tmp:
            tmpdir = Path(tmp)
            wrapper = tmpdir / "wrapper.json"
            wrapper.write_text('{"ids": {"ePAR_SYS_MODE": 7}}', encoding="utf-8")
            legacy = tmpdir / "legacy.json"
            legacy.write_text('{"ePAR_SYS_TEMP": 8}', encoding="utf-8")
            self.assertEqual(pargen.read_lock(wrapper), {"ePAR_SYS_MODE": 7})
            self.assertEqual(pargen.read_lock(legacy), {"ePAR_SYS_TEMP": 8})

    def test_deleted_locked_id_is_not_reused_by_auto_row(self) -> None:
        """AUTO allocation skips IDs reserved by deleted lock entries."""
        rows = base_rows()[:2]
        rows[0]["id"] = "AUTO"
        rows[1]["id"] = "AUTO"
        parsed = [pargen.normalize_row(index, row) for index, row in enumerate(rows, start=2)]
        pargen.validate_and_resolve(
            parsed,
            {"ePAR_DELETED_ROW": 0},
            pargen.GeneratorConfig(id_ranges={"SYSTEM": (0, 10)}, default_id_range=(0, 10)),
        )
        self.assertEqual(parsed[0].resolved_id, 1)
        self.assertEqual(parsed[1].resolved_id, 2)

    def test_id_suggestion_skips_deleted_locked_ids(self) -> None:
        """Hash-collision ID suggestions skip deleted lock entries."""
        rows = base_rows()[:2]
        rows[1]["id"] = "3"
        parsed = [pargen.normalize_row(index, row) for index, row in enumerate(rows, start=2)]
        with self.assertRaisesRegex(pargen.PargenError, r"suggested free ID: 2"):
            pargen.validate_and_resolve(
                parsed,
                {"ePAR_DELETED_ROW": 1},
                pargen.GeneratorConfig(id_ranges={"SYSTEM": (0, 4)}, default_id_range=(0, 4)),
            )

    def test_condition_with_string_literal_is_emitted_verbatim(self) -> None:
        """Conditions containing quoted tokens are preserved in generated guards."""
        rows = base_rows()
        rows[2]["condition"] = '(defined(PAR_TARGET_NAME) && (PAR_TARGET_NAME[0] == "x"[0]))'
        generated_rows, generated_stats = self.run_generator(rows)
        outputs = pargen.generate_outputs(generated_rows, generated_stats)
        self.assertIn('#if (defined(PAR_TARGET_NAME) && (PAR_TARGET_NAME[0] == "x"[0]))',
                      outputs.par_table_def)
        self.assertIn('#endif /* (defined(PAR_TARGET_NAME) && (PAR_TARGET_NAME[0] == "x"[0])) */',
                      outputs.par_table_def)

    def test_string_default_backslash_and_control_chars_are_escaped(self) -> None:
        """String object defaults escape control characters in generated C."""
        rows = base_rows()
        rows[2]["default"] = 'line\\one\n"ssid"'
        rows[2]["max"] = "16"
        generated_rows, generated_stats = self.run_generator(rows)
        outputs = pargen.generate_outputs(generated_rows, generated_stats)
        self.assertIn('(const uint8_t *)"line\\\\one\\n\\\"ssid\\\""', outputs.par_table_def)
        self.assertEqual(generated_rows[2].obj_default_len, len('line\\one\n"ssid"'.encode("utf-8")))

    # Keep generated negative-corpus tests focused on pargen behavior. CI
    # harness wrapper, workflow, and log-checker self-tests are out of scope.
    def test_fixed_seed_mutations_reject_invalid_scalars_and_metadata(self) -> None:
        """Fixed-seed negative corpus rejects malformed scalar and metadata fields."""
        cases = [
            (0, "id", "0x"),
            (0, "persistent", "maybe"),
            (0, "min", "11"),
            (0, "default", "99"),
            (1, "default", "nan"),
            (1, "access", "WO"),
            (1, "write_roles", "ROOT"),
        ]
        for row_index, field, value in cases:
            rows = base_rows()
            rows[row_index][field] = value
            with self.subTest(row=row_index, field=field, value=value):
                with self.assertRaises((pargen.PargenError, ValueError)):
                    self.run_generator(rows)

    def test_fixed_seed_mutations_reject_invalid_object_defaults(self) -> None:
        """Fixed-seed negative corpus rejects malformed object defaults."""
        cases = [
            ("BYTES", "0", "2", "0x100"),
            ("ARR_U8", "2", "2", "1,300"),
            ("ARR_U16", "2", "2", "1,70000"),
            ("ARR_U32", "2", "2", "1,-1"),
            ("ARR_U8", "1", "2", "1,2"),
        ]
        for type_name, min_text, max_text, default_text in cases:
            rows = base_rows()
            rows[2].update({
                "type": type_name,
                "min": min_text,
                "max": max_text,
                "default": default_text,
            })
            with self.subTest(type=type_name, default=default_text):
                with self.assertRaises(pargen.PargenError):
                    self.run_generator(rows)


if __name__ == "__main__":
    unittest.main()
