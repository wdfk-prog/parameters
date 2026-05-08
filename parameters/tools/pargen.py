#!/usr/bin/env python3
"""
Generate parameter-manager C inputs from a CSV schema.

The tool intentionally uses only the Python standard library so it can run in
RT-Thread Env, RT-Thread Studio, CI runners, and host developer machines without
extra package installation.
"""
from __future__ import annotations

import argparse
import csv
import json
import math
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple

VERSION = "1.0.0"

REQUIRED_COLUMNS = (
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
)

SCALAR_TYPES = {"U8", "U16", "U32", "I8", "I16", "I32", "F32"}
OBJECT_TYPES = {"STR", "BYTES", "ARR_U8", "ARR_U16", "ARR_U32"}
SUPPORTED_TYPES = SCALAR_TYPES | OBJECT_TYPES
UNSIGNED_LIMITS = {
    "U8": (0, 0xFF),
    "U16": (0, 0xFFFF),
    "U32": (0, 0xFFFFFFFF),
}
SIGNED_LIMITS = {
    "I8": (-(1 << 7), (1 << 7) - 1),
    "I16": (-(1 << 15), (1 << 15) - 1),
    "I32": (-(1 << 31), (1 << 31) - 1),
}
HASH_GOLDEN_RATIO_32 = 0x61C88647
LAYOUT_SIGNATURE_MOD = 65521
LAYOUT_SIGNATURE_TYPE_CODES = {
    "U8": 1,
    "I8": 2,
    "U16": 3,
    "I16": 4,
    "U32": 5,
    "I32": 6,
    "F32": 7,
    "STR": 8,
    "BYTES": 9,
    "ARR_U8": 10,
    "ARR_U16": 11,
    "ARR_U32": 12,
}
ENUM_RE = re.compile(r"^ePAR_[A-Z0-9_]+$")


class PargenError(Exception):
    """Fatal generator error with a user-facing message."""


@dataclass
class Row:
    """One normalized row from the CSV schema."""

    line: int
    group: str
    section: str
    condition: str
    enum: str
    id_text: str
    type: str
    name: str
    min_text: str
    max_text: str
    default_text: str
    unit: str
    access: str
    read_roles: str
    write_roles: str
    persistent: int
    desc: str
    comment: str
    resolved_id: Optional[int] = None
    obj_default_len: int = 0
    obj_capacity_bytes: int = 0

    @property
    def enabled_macro(self) -> str:
        return f"PAR_LAYOUT_ROW_ENABLED_{self.enum}"

    @property
    def item_macro(self) -> str:
        return f"PAR_ITEM_{self.type}"

    def is_scalar(self) -> bool:
        return self.type in SCALAR_TYPES

    def is_object(self) -> bool:
        return self.type in OBJECT_TYPES


@dataclass
class GeneratedOutputs:
    """All generated file payloads."""

    par_table_def: str
    layout_h: str
    layout_c: str
    info_h: str
    info_c: str
    manifest_json: str
    lock_json: str


@dataclass
class GeneratorConfig:
    """Optional ID allocator configuration loaded from JSON."""

    id_ranges: Dict[str, Tuple[int, int]]
    default_id_range: Tuple[int, int]


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate parameter-manager C files from CSV.")
    parser.add_argument("--csv", default="parameters/schema/par_table.csv", help="CSV schema input path")
    parser.add_argument("--id-lock", default="parameters/schema/par_id_lock.json", help="stable enum-to-ID lock file")
    parser.add_argument("--config", default="parameters/schema/pargen.json", help="optional generator JSON config")
    parser.add_argument("--out-def", default="par_table.def", help="generated par_table.def output path")
    parser.add_argument("--out-dir", default="parameters/generated", help="generated C/header output directory")
    parser.add_argument("--manifest", default="parameters/generated/par_manifest.json", help="generated manifest JSON path")
    parser.add_argument("--check", action="store_true", help="validate only; do not write generated files")
    parser.add_argument("--verify", action="store_true", help="fail when generated files differ from existing files")
    parser.add_argument("--version", action="version", version=f"pargen.py {VERSION}")
    return parser.parse_args(argv)


def load_config(path: Path) -> GeneratorConfig:
    if not path.exists():
        return GeneratorConfig(id_ranges={}, default_id_range=(0, 65535))

    data = json.loads(path.read_text(encoding="utf-8"))
    ranges: Dict[str, Tuple[int, int]] = {}
    for key, value in data.get("id_ranges", {}).items():
        ranges[str(key)] = parse_range(value, f"id_ranges.{key}")
    default_range = parse_range(data.get("default_id_range", [0, 65535]), "default_id_range")
    return GeneratorConfig(id_ranges=ranges, default_id_range=default_range)


def parse_range(value: object, name: str) -> Tuple[int, int]:
    if not isinstance(value, list) or len(value) != 2:
        raise PargenError(f"{name} must be a two-item JSON array")
    start = int(value[0])
    end = int(value[1])
    if start < 0 or end > 65535 or start > end:
        raise PargenError(f"{name} must be inside 0..65535 and start <= end")
    return start, end


def read_lock(path: Path) -> Dict[str, int]:
    if not path.exists():
        return {}
    data = json.loads(path.read_text(encoding="utf-8"))
    if isinstance(data, dict) and "ids" in data:
        data = data["ids"]
    if not isinstance(data, dict):
        raise PargenError(f"{path}: lock file must be a JSON object")
    lock: Dict[str, int] = {}
    for enum, value in data.items():
        value_int = int(value)
        if value_int < 0 or value_int > 65535:
            raise PargenError(f"{path}: locked ID for {enum} is outside 0..65535")
        lock[str(enum)] = value_int
    return lock


def read_rows(path: Path) -> List[Row]:
    if not path.exists():
        raise PargenError(f"CSV schema not found: {path}")

    with path.open("r", encoding="utf-8", newline="") as f:
        reader = csv.DictReader(f)
        if reader.fieldnames is None:
            raise PargenError(f"{path}: missing CSV header")
        missing = [col for col in REQUIRED_COLUMNS if col not in reader.fieldnames]
        if missing:
            raise PargenError(f"{path}: missing required columns: {', '.join(missing)}")
        rows = []
        for idx, raw in enumerate(reader, start=2):
            if is_blank_csv_row(raw):
                continue
            rows.append(normalize_row(idx, raw))
    if not rows:
        raise PargenError(f"{path}: schema contains no parameter rows")
    return rows


def is_blank_csv_row(raw: Dict[str, Optional[str]]) -> bool:
    return all((value is None or str(value).strip() == "") for value in raw.values())


def normalize_row(line: int, raw: Dict[str, Optional[str]]) -> Row:
    def raw_field(name: str) -> str:
        return raw.get(name) or ""

    def field(name: str) -> str:
        return raw_field(name).strip()

    enum = field("enum")
    type_name = field("type").upper()
    persistent = parse_persistent(field("persistent"), line)
    default_text = raw_field("default") if type_name == "STR" else field("default")
    return Row(
        line=line,
        group=field("group"),
        section=field("section"),
        condition=field("condition"),
        enum=enum,
        id_text=field("id"),
        type=type_name,
        name=field("name"),
        min_text=field("min"),
        max_text=field("max"),
        default_text=default_text,
        unit=field("unit"),
        access=field("access"),
        read_roles=field("read_roles"),
        write_roles=field("write_roles"),
        persistent=persistent,
        desc=field("desc"),
        comment=field("comment"),
    )


def parse_persistent(text: str, line: int) -> int:
    normalized = text.strip().lower()
    if normalized in {"1", "true", "yes", "y"}:
        return 1
    if normalized in {"0", "false", "no", "n"}:
        return 0
    raise PargenError(f"line {line}: persistent must be 0/1/true/false")


def validate_and_resolve(rows: List[Row], lock: Dict[str, int], cfg: GeneratorConfig) -> Dict[str, object]:
    enums: Dict[str, Row] = {}
    ids: Dict[int, Row] = {}

    for row in rows:
        validate_basic(row)
        if row.enum in enums:
            raise PargenError(f"line {row.line}: enum {row.enum} duplicates line {enums[row.enum].line}")
        enums[row.enum] = row

    reserved_ids: Dict[int, Row] = {}
    for row in rows:
        if row.id_text.upper() not in {"", "AUTO"}:
            reserve_id(reserved_ids, parse_id(row.id_text, row.line), row, "explicit")

    for row in rows:
        if row.id_text.upper() in {"", "AUTO"}:
            locked_id = lock.get(row.enum)
            if locked_id is not None:
                reserve_id(reserved_ids, locked_id, row, "locked")

    used_ids: Dict[int, Row] = dict(reserved_ids)
    for row in rows:
        if row.id_text.upper() in {"", "AUTO"}:
            locked_id = lock.get(row.enum)
            if locked_id is not None:
                row.resolved_id = locked_id
            else:
                row.resolved_id = allocate_id(row, rows, used_ids, cfg)
        else:
            row.resolved_id = parse_id(row.id_text, row.line)

        assert row.resolved_id is not None
        if row.resolved_id in ids:
            other = ids[row.resolved_id]
            raise PargenError(
                f"line {row.line}: ID {row.resolved_id} for {row.enum} duplicates "
                f"{other.enum} on line {other.line}"
            )
        ids[row.resolved_id] = row
        used_ids[row.resolved_id] = row

    validate_values(rows)
    hash_bits = hash_bits_for_rows(rows)
    buckets: Dict[int, Row] = {}
    for row in rows:
        assert row.resolved_id is not None
        bucket = hash_id(row.resolved_id, hash_bits)
        if bucket in buckets:
            other = buckets[bucket]
            suggestion = find_id_suggestion(row, rows, buckets, hash_bits, cfg)
            hint = f"; suggested free ID: {suggestion}" if suggestion is not None else ""
            raise PargenError(
                f"line {row.line}: hash bucket {bucket} collision: {row.enum} ID {row.resolved_id} "
                f"conflicts with {other.enum} ID {other.resolved_id}{hint}"
            )
        buckets[bucket] = row

    count8, count16, count32, count_obj, obj_pool = compute_counts(rows)
    persistent_scalar = sum(1 for row in rows if row.is_scalar() and row.persistent)
    persistent_object = sum(1 for row in rows if row.is_object() and row.persistent)
    return {
        "param_count": len(rows),
        "count8": count8,
        "count16": count16,
        "count32": count32,
        "count_obj": count_obj,
        "obj_pool_bytes": obj_pool,
        "persistent_scalar_count": persistent_scalar,
        "persistent_object_count": persistent_object,
        "hash_bits": hash_bits,
        "hash_size": 1 << hash_bits,
    }


def validate_basic(row: Row) -> None:
    if not ENUM_RE.match(row.enum):
        raise PargenError(f"line {row.line}: enum must match ePAR_[A-Z0-9_]+")
    if row.type not in SUPPORTED_TYPES:
        raise PargenError(f"line {row.line}: unsupported type {row.type}")
    if not row.name:
        raise PargenError(f"line {row.line}: name is required")
    if not row.min_text or not row.max_text:
        raise PargenError(f"line {row.line}: min and max are required")
    if not row.default_text and row.type not in {"STR", "BYTES"}:
        raise PargenError(f"line {row.line}: default is required")
    if not row.desc:
        raise PargenError(f"line {row.line}: desc is required")
    normalize_access(row.access, row.line)
    normalize_role(row.read_roles, row.line)
    normalize_role(row.write_roles, row.line)


def parse_id(text: str, line: int) -> int:
    try:
        value = parse_c_int(text)
    except ValueError as exc:
        raise PargenError(f"line {line}: invalid id {text!r}") from exc
    if value < 0 or value > 65535:
        raise PargenError(f"line {line}: id {value} is outside 0..65535")
    return value


def reserve_id(reserved_ids: Dict[int, Row], value: int, row: Row, source: str) -> None:
    """Reserve one explicit or locked ID before AUTO allocation."""
    if value in reserved_ids:
        other = reserved_ids[value]
        raise PargenError(
            f"line {row.line}: {source} ID {value} for {row.enum} duplicates "
            f"{other.enum} on line {other.line}"
        )
    reserved_ids[value] = row


def allocate_id(row: Row, rows: List[Row], used_ids: Dict[int, Row], cfg: GeneratorConfig) -> int:
    start, end = cfg.id_ranges.get(row.group, cfg.default_id_range)
    hash_bits = hash_bits_for_rows(rows)
    used_buckets = {hash_id(value, hash_bits) for value in used_ids}
    for candidate in range(start, end + 1):
        if candidate in used_ids:
            continue
        bucket = hash_id(candidate, hash_bits)
        if bucket in used_buckets:
            continue
        return candidate
    raise PargenError(f"line {row.line}: no free AUTO ID in range {start}..{end} for group {row.group!r}")


def find_id_suggestion(row: Row, rows: List[Row], buckets: Dict[int, Row], hash_bits: int, cfg: GeneratorConfig) -> Optional[int]:
    used_ids = {int(r.resolved_id) for r in rows if r.resolved_id is not None and r is not row}
    start, end = cfg.id_ranges.get(row.group, cfg.default_id_range)
    for candidate in range(start, end + 1):
        if candidate in used_ids:
            continue
        if hash_id(candidate, hash_bits) not in buckets:
            return candidate
    return None


def validate_values(rows: List[Row]) -> None:
    for row in rows:
        if row.type in UNSIGNED_LIMITS:
            lo, hi = UNSIGNED_LIMITS[row.type]
            min_value = parse_c_int(row.min_text)
            max_value = parse_c_int(row.max_text)
            default_value = parse_c_int(row.default_text)
            validate_range(row, min_value, max_value, default_value, lo, hi)
        elif row.type in SIGNED_LIMITS:
            lo, hi = SIGNED_LIMITS[row.type]
            min_value = parse_c_int(row.min_text)
            max_value = parse_c_int(row.max_text)
            default_value = parse_c_int(row.default_text)
            validate_range(row, min_value, max_value, default_value, lo, hi)
        elif row.type == "F32":
            min_value = parse_c_float(row.min_text)
            max_value = parse_c_float(row.max_text)
            default_value = parse_c_float(row.default_text)
            if not all(math.isfinite(x) for x in (min_value, max_value, default_value)):
                raise PargenError(f"line {row.line}: F32 min/max/default must be finite")
            if not min_value < max_value:
                raise PargenError(f"line {row.line}: F32 min must be less than max")
            if not min_value <= default_value <= max_value:
                raise PargenError(f"line {row.line}: F32 default must be inside [min,max]")
        else:
            validate_object(row)


def validate_range(row: Row, min_value: int, max_value: int, default_value: int, lo: int, hi: int) -> None:
    if min_value < lo or max_value > hi:
        raise PargenError(f"line {row.line}: {row.type} range must fit {lo}..{hi}")
    if not min_value < max_value:
        raise PargenError(f"line {row.line}: min must be less than max")
    if not min_value <= default_value <= max_value:
        raise PargenError(f"line {row.line}: default must be inside [min,max]")


def validate_object(row: Row) -> None:
    min_count = parse_c_int(row.min_text)
    max_count = parse_c_int(row.max_text)
    if min_count < 0 or max_count < min_count:
        raise PargenError(f"line {row.line}: object min/max must satisfy 0 <= min <= max")
    if row.type == "STR" and max_count > 65534:
        raise PargenError(f"line {row.line}: STR max must be <= UINT16_MAX - 1")
    if row.type != "STR" and max_count > 65535:
        raise PargenError(f"line {row.line}: object max must fit uint16_t")

    if row.type == "STR":
        length = len(parse_string_default(row.default_text)) if row.default_text else 0
        capacity = max_count
    elif row.type == "BYTES":
        length = len(parse_byte_list(row.default_text)) if row.default_text else 0
        capacity = max_count
    elif row.type == "ARR_U8":
        values = parse_number_list(row.default_text)
        ensure_fixed_array(row, min_count, max_count, len(values))
        for value in values:
            if value < 0 or value > 0xFF:
                raise PargenError(f"line {row.line}: ARR_U8 default element {value} is outside 0..255")
        length = len(values)
        capacity = max_count
    elif row.type == "ARR_U16":
        values = parse_number_list(row.default_text)
        ensure_fixed_array(row, min_count, max_count, len(values))
        for value in values:
            if value < 0 or value > 0xFFFF:
                raise PargenError(f"line {row.line}: ARR_U16 default element {value} is outside 0..65535")
        length = len(values) * 2
        capacity = max_count * 2
    elif row.type == "ARR_U32":
        values = parse_number_list(row.default_text)
        ensure_fixed_array(row, min_count, max_count, len(values))
        for value in values:
            if value < 0 or value > 0xFFFFFFFF:
                raise PargenError(f"line {row.line}: ARR_U32 default element {value} is outside 0..UINT32_MAX")
        length = len(values) * 4
        capacity = max_count * 4
    else:
        raise AssertionError(row.type)

    if row.type in {"STR", "BYTES"}:
        if not min_count <= length <= max_count:
            raise PargenError(f"line {row.line}: default length {length} must be inside [{min_count},{max_count}]")
    row.obj_default_len = length
    row.obj_capacity_bytes = capacity


def ensure_fixed_array(row: Row, min_count: int, max_count: int, actual_count: int) -> None:
    if min_count != max_count:
        raise PargenError(f"line {row.line}: {row.type} requires min == max")
    if actual_count != max_count:
        raise PargenError(f"line {row.line}: {row.type} default has {actual_count} elements, expected {max_count}")


def parse_c_int(text: str) -> int:
    token = cleanup_numeric_token(text)
    macros = {
        "UINT8_MAX": 0xFF,
        "UINT16_MAX": 0xFFFF,
        "UINT32_MAX": 0xFFFFFFFF,
        "INT8_MIN": -(1 << 7),
        "INT8_MAX": (1 << 7) - 1,
        "INT16_MIN": -(1 << 15),
        "INT16_MAX": (1 << 15) - 1,
        "INT32_MIN": -(1 << 31),
        "INT32_MAX": (1 << 31) - 1,
    }
    if token in macros:
        return macros[token]
    if re.match(r"^[+-]?0[xX][0-9a-fA-F]+$", token):
        return int(token, 16)
    if re.match(r"^[+-]?\d+$", token):
        return int(token, 10)
    raise ValueError(text)


def parse_c_float(text: str) -> float:
    token = cleanup_numeric_token(text)
    token = token.replace("F", "").replace("f", "")
    return float(token)


def cleanup_numeric_token(text: str) -> str:
    """Return a C numeric token without surrounding parens or integer suffixes."""
    token = text.strip()
    while token.startswith("(") and token.endswith(")") and balanced_inner(token[1:-1]):
        token = token[1:-1].strip()
    token = re.sub(r"(?<=[0-9a-fA-F])[uUlL]+$", "", token)
    return token


def balanced_inner(text: str) -> bool:
    level = 0
    for ch in text:
        if ch == "(":
            level += 1
        elif ch == ")":
            if level == 0:
                return False
            level -= 1
    return level == 0


def parse_string_default(text: str) -> bytes:
    if text.lower().startswith("c:"):
        raise PargenError("raw C object initializers cannot be length-validated")
    if text == "":
        return b""
    return text.encode("utf-8")


def parse_byte_list(text: str) -> List[int]:
    if not text.strip():
        return []
    parts = re.split(r"[\s,;:]+", text.strip())
    values = []
    for part in parts:
        if not part:
            continue
        value = parse_c_int(part)
        if value < 0 or value > 0xFF:
            raise PargenError(f"byte value {value} is outside 0..255")
        values.append(value)
    return values


def parse_number_list(text: str) -> List[int]:
    if not text.strip():
        return []
    values = []
    for part in re.split(r"[\s,;]+", text.strip()):
        if part:
            values.append(parse_c_int(part))
    return values


def normalize_access(text: str, line: int) -> str:
    value = text.strip()
    aliases = {
        "RO": "ePAR_ACCESS_RO",
        "RW": "ePAR_ACCESS_RW",
        "ePAR_ACCESS_RO": "ePAR_ACCESS_RO",
        "ePAR_ACCESS_RW": "ePAR_ACCESS_RW",
    }
    if value not in aliases:
        raise PargenError(f"line {line}: access must be RO/RW/ePAR_ACCESS_RO/ePAR_ACCESS_RW")
    return aliases[value]


def normalize_role(text: str, line: int) -> str:
    value = text.strip()
    aliases = {
        "NONE": "ePAR_ROLE_NONE",
        "PUBLIC": "ePAR_ROLE_PUBLIC",
        "SERVICE": "ePAR_ROLE_SERVICE",
        "DEVELOPER": "ePAR_ROLE_DEVELOPER",
        "MANUFACTURING": "ePAR_ROLE_MANUFACTURING",
        "ALL": "ePAR_ROLE_ALL",
        "ePAR_ROLE_NONE": "ePAR_ROLE_NONE",
        "ePAR_ROLE_PUBLIC": "ePAR_ROLE_PUBLIC",
        "ePAR_ROLE_SERVICE": "ePAR_ROLE_SERVICE",
        "ePAR_ROLE_DEVELOPER": "ePAR_ROLE_DEVELOPER",
        "ePAR_ROLE_MANUFACTURING": "ePAR_ROLE_MANUFACTURING",
        "ePAR_ROLE_ALL": "ePAR_ROLE_ALL",
    }
    if value in aliases:
        return aliases[value]
    parts = [part.strip() for part in value.split("|")]
    if parts and all(part in aliases for part in parts):
        return "(" + " | ".join(aliases[part] for part in parts) + ")"
    raise PargenError(f"line {line}: invalid role mask {text!r}")


def hash_bits_for_rows(rows: Sequence[Row]) -> int:
    always_enabled = sum(1 for row in rows if not row.condition)
    min_rows = max(always_enabled, 1)
    min_buckets = 2 * min_rows
    bits = 1
    while (1 << bits) < min_buckets:
        bits += 1
    return bits


def hash_id(value: int, bits: int) -> int:
    return ((value * HASH_GOLDEN_RATIO_32) & 0xFFFFFFFF) >> (32 - bits)


def compute_counts(rows: List[Row]) -> Tuple[int, int, int, int, int]:
    count8 = sum(1 for row in rows if row.type in {"U8", "I8"})
    count16 = sum(1 for row in rows if row.type in {"U16", "I16"})
    count32 = sum(1 for row in rows if row.type in {"U32", "I32", "F32"})
    count_obj = sum(1 for row in rows if row.is_object())
    obj_pool = sum(row.obj_capacity_bytes for row in rows if row.is_object())
    return count8, count16, count32, count_obj, obj_pool


def generate_outputs(rows: List[Row], stats: Dict[str, object]) -> GeneratedOutputs:
    lock_data = {row.enum: int(row.resolved_id) for row in rows if row.resolved_id is not None}
    return GeneratedOutputs(
        par_table_def=generate_par_table_def(rows),
        layout_h=generate_layout_h(rows),
        layout_c=generate_layout_c(rows),
        info_h=generate_info_h(),
        info_c=generate_info_c(),
        manifest_json=generate_manifest(rows, stats),
        lock_json=json.dumps({"version": 1, "ids": lock_data}, indent=4, sort_keys=True) + "\n",
    )


def generated_banner(comment: str = "/*") -> str:
    if comment == "/*":
        return (
            "/*\n"
            " * DO NOT EDIT.\n"
            " * Generated from parameters/schema/par_table.csv by parameters/tools/pargen.py.\n"
            " */\n"
        )
    return ""


def aligned_define(name: str, value: str, width: int) -> str:
    """Return one generated #define with stable macro-name alignment."""
    return f"#define {name:<{width}} {value}\n"


def generate_par_table_def(rows: List[Row]) -> str:
    out: List[str] = []
    out.append(generated_banner())
    out.append("/*\n")
    out.append(" * Parameter single-source list for X-Macro expansion.\n")
    out.append(" *\n")
    out.append(" * This file is intentionally included multiple times. Do not add include guards.\n")
    out.append(" */\n\n")
    out.append("/* ============================================================================================================================= */\n")
    out.append("/*              enum_               id_         name_                        min_     max_     def_     unit_    access_         read_roles_      write_roles_  pers_  desc_ */\n")
    out.append("/* ============================================================================================================================= */\n\n")

    last_group = None
    last_section = None
    for row in rows:
        if row.group != last_group:
            out.append("\n")
            out.append("/* ============================================================================================================================= */\n")
            out.append(f"/*  {row.group:<124}*/\n")
            out.append("/* ============================================================================================================================= */\n\n")
            last_group = row.group
            last_section = None
        if row.section and row.section != last_section:
            out.append(f"/* {row.section} */\n")
            last_section = row.section
        if row.comment:
            for line in row.comment.splitlines():
                out.append(f"/* {line} */\n")
        if row.condition:
            out.append(f"#if {row.condition}\n")
        out.append(format_par_item(row))
        if row.condition:
            out.append(f"#endif /* {row.condition} */\n")
        out.append("\n")
    return "".join(out).rstrip() + "\n"


def format_par_item(row: Row) -> str:
    assert row.resolved_id is not None
    def_text = format_default(row)
    fields = [
        row.enum,
        str(row.resolved_id),
        c_string(row.name),
        c_unsigned(row.min_text) if row.type != "F32" and not row.type.startswith("I") else row.min_text,
        c_unsigned(row.max_text) if row.type != "F32" and not row.type.startswith("I") else row.max_text,
        def_text,
        format_unit(row.unit),
        normalize_access(row.access, row.line),
        normalize_role(row.read_roles, row.line),
        normalize_role(row.write_roles, row.line),
        str(row.persistent),
        c_string(row.desc),
    ]
    return (
        f"{row.item_macro:<16}("
        f"{fields[0]:<22}, {fields[1]:<5}, {fields[2]:<34}, "
        f"{fields[3]:<8}, {fields[4]:<10}, {fields[5]:<120}, "
        f"{fields[6]:<8}, {fields[7]:<14}, {fields[8]:<14}, {fields[9]:<15}, "
        f"{fields[10]:<5}, {fields[11]})\n"
    )


def c_unsigned(text: str) -> str:
    token = text.strip()
    if token in {"UINT8_MAX", "UINT16_MAX", "UINT32_MAX"}:
        return token
    if token.lower().startswith("0x"):
        return token if re.search(r"[uUlL]$", token) else token + "U"
    if re.match(r"^\d+$", token):
        return token + "U"
    return token


def c_string(text: str) -> str:
    return json.dumps(text, ensure_ascii=False)


def format_unit(unit: str) -> str:
    return "NULL" if unit == "" or unit.upper() == "NULL" else c_string(unit)


def format_default(row: Row) -> str:
    if row.type in UNSIGNED_LIMITS:
        return c_unsigned(row.default_text)
    if row.type in SIGNED_LIMITS:
        return row.default_text.strip()
    if row.type == "F32":
        token = row.default_text.strip()
        return token if token.lower().endswith("f") else token + "f"
    return format_object_default(row)


def format_object_default(row: Row) -> str:
    if row.default_text.lower().startswith("c:"):
        return row.default_text[2:].strip()
    if row.type == "STR":
        if row.default_text == "":
            return "((par_obj_init_t){ .p_data = NULL, .len = 0U })"
        return f"((par_obj_init_t){{ .p_data = (const uint8_t *){c_string(row.default_text)}, .len = {len(row.default_text.encode('utf-8'))}U }})"
    if row.type == "BYTES":
        values = parse_byte_list(row.default_text)
        if not values:
            return "((par_obj_init_t){ .p_data = NULL, .len = 0U })"
        elems = ", ".join(f"0x{value:02X}U" for value in values)
        return f"((par_obj_init_t){{ .p_data = (const uint8_t[]){{ {elems} }}, .len = {len(values)}U }})"
    values = parse_number_list(row.default_text)
    if row.type == "ARR_U8":
        elems = ", ".join(f"{value}U" for value in values)
        return f"((par_obj_init_t){{ .p_data = (const uint8_t[]){{ {elems} }}, .len = {len(values)}U }})"
    if row.type == "ARR_U16":
        elems = ", ".join(f"{value}U" for value in values)
        return f"((par_obj_init_t){{ .p_data = (const uint8_t *)(const uint16_t[]){{ {elems} }}, .len = (uint16_t)({len(values)}U * sizeof(uint16_t)) }})"
    if row.type == "ARR_U32":
        elems = ", ".join(f"{value}UL" for value in values)
        return f"((par_obj_init_t){{ .p_data = (const uint8_t *)(const uint32_t[]){{ {elems} }}, .len = (uint16_t)({len(values)}U * sizeof(uint32_t)) }})"
    raise AssertionError(row.type)


def generate_layout_h(rows: List[Row]) -> str:
    count8_expr = count_expr(rows, {"U8", "I8"})
    count16_expr = count_expr(rows, {"U16", "I16"})
    count32_expr = count_expr(rows, {"U32", "I32", "F32"})
    count_obj_expr = count_expr(rows, OBJECT_TYPES)
    pool_expr = object_pool_expr(rows)

    out: List[str] = []
    out.append(generated_banner())
    out.append("/**\n")
    out.append(" * @file par_layout_static.h\n")
    out.append(" * @brief Declare generated static parameter layout tables.\n")
    out.append(" */\n\n")
    out.append("#ifndef _PAR_LAYOUT_STATIC_H_\n")
    out.append("#define _PAR_LAYOUT_STATIC_H_\n\n")
    out.append("#include <stdint.h>\n")
    out.append("#include \"par_cfg.h\"\n")
    out.append("#include \"par_def.h\"\n\n")
    out.append("#ifdef __cplusplus\n")
    out.append("extern \"C\" {\n")
    out.append("#endif /* defined(__cplusplus) */\n\n")
    out.append("/**\n")
    out.append(" * @brief Generated row-enable flags used by conditional layout expressions.\n")
    out.append(" */\n")
    enabled_macro_width = max(
        (len(row.enabled_macro) for row in rows if not row.condition),
        default=0,
    )
    for row in rows:
        if row.condition:
            out.append(f"#if {row.condition}\n")
            out.append(f"#define {row.enabled_macro} (1u)\n")
            out.append("#else\n")
            out.append(f"#define {row.enabled_macro} (0u)\n")
            out.append(f"#endif /* {row.condition} */\n")
        else:
            out.append(aligned_define(row.enabled_macro, "(1u)", enabled_macro_width))
    out.append("\n")
    out.append(f"#define PAR_LAYOUT_STATIC_COUNT8         ({count8_expr})\n")
    out.append(f"#define PAR_LAYOUT_STATIC_COUNT16        ({count16_expr})\n")
    out.append(f"#define PAR_LAYOUT_STATIC_COUNT32        ({count32_expr})\n")
    out.append(f"#define PAR_LAYOUT_STATIC_COUNTOBJ       ({count_obj_expr})\n")
    out.append(f"#define PAR_LAYOUT_STATIC_OBJ_POOL_BYTES ({pool_expr})\n\n")
    out.append(generate_layout_signature_macros(rows))
    out.append(generate_layout_offset_macros(rows))
    out.append("/**\n")
    out.append(" * @brief Static scalar/object slot offset table indexed by par_num_t.\n")
    out.append(" */\n")
    out.append("extern const uint16_t g_par_layout_static_offset[ePAR_NUM_OF];\n")
    out.append("#define PAR_LAYOUT_STATIC_OFFSET_TABLE (g_par_layout_static_offset)\n\n")
    out.append("/**\n")
    out.append(" * @brief Static object-pool byte offset table indexed by par_num_t.\n")
    out.append(" */\n")
    out.append("extern const uint32_t g_par_layout_static_object_pool_offset[ePAR_NUM_OF];\n")
    out.append("#define PAR_LAYOUT_STATIC_OBJECT_POOL_OFFSET_TABLE (g_par_layout_static_object_pool_offset)\n\n")
    out.append("#ifdef __cplusplus\n")
    out.append("}\n")
    out.append("#endif /* defined(__cplusplus) */\n\n")
    out.append("#endif /* !defined(_PAR_LAYOUT_STATIC_H_) */\n")
    return "".join(out)


def compact_sum_expr(const_value: int, terms: Sequence[str]) -> str:
    expr = f"{const_value}u"
    for term in terms:
        expr += f" + ({term})"
    return expr


def count_expr(rows: List[Row], types: Iterable[str]) -> str:
    type_set = set(types)
    const_value = 0
    terms = []
    for row in rows:
        if row.type not in type_set:
            continue
        if row.condition:
            terms.append(row.enabled_macro)
        else:
            const_value += 1
    return compact_sum_expr(const_value, terms)


def object_pool_expr(rows: List[Row]) -> str:
    const_value = 0
    terms = []
    for row in rows:
        if not row.is_object():
            continue
        if row.condition:
            terms.append(f"{row.enabled_macro} * {row.obj_capacity_bytes}u")
        else:
            const_value += row.obj_capacity_bytes
    return compact_sum_expr(const_value, terms)


def prior_count_expr(rows: List[Row], row: Row, types: Iterable[str]) -> str:
    type_set = set(types)
    const_value = 0
    terms = []
    for prev in rows[:rows.index(row)]:
        if prev.type not in type_set:
            continue
        if prev.condition:
            terms.append(prev.enabled_macro)
        else:
            const_value += 1
    return compact_sum_expr(const_value, terms)


def prior_object_pool_expr(rows: List[Row], row: Row) -> str:
    const_value = 0
    terms = []
    for prev in rows[:rows.index(row)]:
        if not prev.is_object():
            continue
        if prev.condition:
            terms.append(f"{prev.enabled_macro} * {prev.obj_capacity_bytes}u")
        else:
            const_value += prev.obj_capacity_bytes
    return compact_sum_expr(const_value, terms)


def compiled_row_index_expr(rows: List[Row], row: Row) -> str:
    """Return the generated C expression for one row's compiled enum index."""
    const_value = 0
    terms = []
    for prev in rows[:rows.index(row)]:
        if prev.condition:
            terms.append(prev.enabled_macro)
        else:
            const_value += 1
    return compact_sum_expr(const_value, terms)


def layout_signature_term_a_expr(rows: List[Row], row: Row) -> str:
    """Return signature-A contribution expression for one generated row."""
    row_index_expr = compiled_row_index_expr(rows, row)
    type_code = LAYOUT_SIGNATURE_TYPE_CODES[row.type]
    obj_bytes = row.obj_capacity_bytes if row.is_object() else 0
    resolved_id = row.resolved_id or 0
    return (
        "(((((uint32_t)(" + row_index_expr + ") + 1u) * ((uint32_t)" + str(type_code) + "u + 1u) * 257u) + "
        "((((uint32_t)(" + row_index_expr + ") + 1u) * (((uint32_t)" + str(resolved_id) + "u % 257u) + 1u)) * 17u) + "
        "((uint32_t)" + str(resolved_id) + "u * 5u) + "
        "(((uint32_t)" + str(obj_bytes) + "u + 1u) * 31u)) % " + str(LAYOUT_SIGNATURE_MOD) + "u)"
    )


def layout_signature_term_b_expr(rows: List[Row], row: Row) -> str:
    """Return signature-B contribution expression for one generated row."""
    row_index_expr = compiled_row_index_expr(rows, row)
    type_code = LAYOUT_SIGNATURE_TYPE_CODES[row.type]
    obj_bytes = row.obj_capacity_bytes if row.is_object() else 0
    resolved_id = row.resolved_id or 0
    return (
        "(((((uint32_t)(" + row_index_expr + ") + 1u) * (((uint32_t)" + str(resolved_id) + "u % 257u) + 1u)) + "
        "((uint32_t)" + str(resolved_id) + "u * 3u) + "
        "((uint32_t)" + str(type_code) + "u * 389u) + "
        "(((uint32_t)" + str(obj_bytes) + "u + 1u) * 13u)) % " + str(LAYOUT_SIGNATURE_MOD) + "u)"
    )


def layout_signature_term_macros(rows: List[Row], part: str) -> str:
    """Return generated per-row layout signature term macros."""
    out: List[str] = []
    macro_width = max(
        (
            len(f"PAR_LAYOUT_STATIC_SIGNATURE_{part}_TERM_{row.enum}")
            for row in rows
            if not row.condition
        ),
        default=0,
    )
    for row in rows:
        macro = f"PAR_LAYOUT_STATIC_SIGNATURE_{part}_TERM_{row.enum}"
        term = layout_signature_term_a_expr(rows, row) if part == "A" else layout_signature_term_b_expr(rows, row)
        if row.condition:
            out.append(f"#if {row.condition}\n")
            out.append(f"#define {macro} ({term})\n")
            out.append("#else\n")
            out.append(f"#define {macro} (0u)\n")
            out.append(f"#endif /* {row.condition} */\n")
        else:
            out.append(aligned_define(macro, f"({term})", macro_width))
    return "".join(out)


LAYOUT_SIGNATURE_CHUNK_SIZE = 8


def layout_signature_expr(rows: List[Row], part: str) -> str:
    """Return a generated signature expression for enabled layout rows."""
    terms = [f"PAR_LAYOUT_STATIC_SIGNATURE_{part}_TERM_{row.enum}" for row in rows]
    return f"({compact_sum_expr(0, terms)}) % {LAYOUT_SIGNATURE_MOD}u"


def layout_signature_chunk_expr(rows: List[Row], part: str, start: int, end: int) -> str:
    """Return a bounded-length signature expression for a row slice."""
    terms = [f"PAR_LAYOUT_STATIC_SIGNATURE_{part}_TERM_{row.enum}" for row in rows[start:end]]
    return f"({compact_sum_expr(0, terms)}) % {LAYOUT_SIGNATURE_MOD}u"


def layout_signature_chunk_macros(rows: List[Row], part: str) -> str:
    """Return generated chunked layout signature macros."""
    out: List[str] = []
    chunk_macros: List[str] = []
    for chunk_index, start in enumerate(range(0, len(rows), LAYOUT_SIGNATURE_CHUNK_SIZE)):
        end = min(start + LAYOUT_SIGNATURE_CHUNK_SIZE, len(rows))
        macro = f"PAR_LAYOUT_STATIC_SIGNATURE_{part}_CHUNK_{chunk_index}"
        chunk_macros.append(macro)
        out.append(f"#define {macro} ({layout_signature_chunk_expr(rows, part, start, end)})\n")
    final_expr = f"({compact_sum_expr(0, chunk_macros)}) % {LAYOUT_SIGNATURE_MOD}u"
    out.append(f"#define PAR_LAYOUT_STATIC_SIGNATURE_{part} ({final_expr})\n")
    return "".join(out)


def generate_layout_signature_macros(rows: List[Row]) -> str:
    """Return generated layout signature macros."""
    out: List[str] = []
    out.append("/**\n")
    out.append(" * @brief Generated layout signature terms using compiled enum indexes.\n")
    out.append(" */\n")
    out.append(layout_signature_term_macros(rows, "A"))
    out.append(layout_signature_term_macros(rows, "B"))
    out.append("\n/**\n")
    out.append(" * @brief Generated layout signature used to detect stale static tables.\n")
    out.append(" */\n")
    out.append(layout_signature_chunk_macros(rows, "A"))
    out.append(layout_signature_chunk_macros(rows, "B"))
    out.append("#define PAR_LAYOUT_STATIC_SIGNATURE   (((uint32_t)PAR_LAYOUT_STATIC_SIGNATURE_A << 16u) ^ (uint32_t)PAR_LAYOUT_STATIC_SIGNATURE_B)\n\n")
    return "".join(out)


def generate_layout_offset_macros(rows: List[Row]) -> str:
    """Return generated per-row layout offset macros."""
    out: List[str] = []
    out.append("/**\n")
    out.append(" * @brief Generated static offset macros used for table freshness checks.\n")
    out.append(" */\n")
    base_offset_macro_width = max(len(f"PAR_LAYOUT_STATIC_OFFSET_{row.enum}") for row in rows)
    for row in rows:
        object_pool_macro = f"PAR_LAYOUT_STATIC_OBJECT_POOL_OFFSET_{row.enum}"
        row_offset_macro_width = base_offset_macro_width
        if row.is_object():
            row_offset_macro_width = max(row_offset_macro_width, len(object_pool_macro))
        if row.condition:
            out.append(f"#if {row.condition}\n")
        out.append(aligned_define(
            f"PAR_LAYOUT_STATIC_OFFSET_{row.enum}",
            f"({layout_offset_expr(rows, row)})",
            row_offset_macro_width,
        ))
        if row.is_object():
            out.append(aligned_define(
                object_pool_macro,
                f"({obj_pool_offset_expr(rows, row)})",
                row_offset_macro_width,
            ))
        if row.condition:
            out.append(f"#endif /* {row.condition} */\n")
    out.append("\n")
    return "".join(out)


def generate_layout_c(rows: List[Row]) -> str:
    out: List[str] = []
    out.append(generated_banner())
    out.append("/**\n")
    out.append(" * @file par_layout_static.c\n")
    out.append(" * @brief Define generated static parameter layout tables.\n")
    out.append(" */\n\n")
    out.append("#include \"par_layout_static.h\"\n\n")
    out.append("const uint16_t g_par_layout_static_offset[ePAR_NUM_OF] = {\n")
    for row in rows:
        if row.condition:
            out.append(f"#if {row.condition}\n")
        out.append(f"    [{row.enum}] = (uint16_t)(PAR_LAYOUT_STATIC_OFFSET_{row.enum}),\n")
        if row.condition:
            out.append(f"#endif /* {row.condition} */\n")
    out.append("};\n\n")
    out.append("const uint32_t g_par_layout_static_object_pool_offset[ePAR_NUM_OF] = {\n")
    out.append("    0u, /* Keep the initializer valid when all object rows are compiled out. */\n")
    for row in rows:
        if not row.is_object():
            continue
        if row.condition:
            out.append(f"#if {row.condition}\n")
        out.append(f"    [{row.enum}] = (uint32_t)(PAR_LAYOUT_STATIC_OBJECT_POOL_OFFSET_{row.enum}),\n")
        if row.condition:
            out.append(f"#endif /* {row.condition} */\n")
    out.append("};\n")
    return "".join(out)


def layout_offset_expr(rows: List[Row], row: Row) -> str:
    if row.type in {"U8", "I8"}:
        types = {"U8", "I8"}
    elif row.type in {"U16", "I16"}:
        types = {"U16", "I16"}
    elif row.type in {"U32", "I32", "F32"}:
        types = {"U32", "I32", "F32"}
    else:
        types = OBJECT_TYPES
    return prior_count_expr(rows, row, types)


def obj_pool_offset_expr(rows: List[Row], row: Row) -> str:
    return prior_object_pool_expr(rows, row)


def generate_info_h() -> str:
    return (
        generated_banner()
        + "/**\n"
        + " * @file par_generated_info.h\n"
        + " * @brief Declare generated parameter-table summary metadata.\n"
        + " */\n\n"
        + "#ifndef _PAR_GENERATED_INFO_H_\n"
        + "#define _PAR_GENERATED_INFO_H_\n\n"
        + "#include <stdint.h>\n"
        + "#include \"par_layout_static.h\"\n\n"
        + "#ifdef __cplusplus\n"
        + "extern \"C\" {\n"
        + "#endif /* defined(__cplusplus) */\n\n"
        + "/**\n"
        + " * @brief Generated parameter-table summary.\n"
        + " */\n"
        + "typedef struct\n"
        + "{\n"
        + "    uint16_t param_count;      /**< Total compiled parameter count. */\n"
        + "    uint16_t count8;           /**< Number of 8-bit storage entries. */\n"
        + "    uint16_t count16;          /**< Number of 16-bit storage entries. */\n"
        + "    uint16_t count32;          /**< Number of 32-bit storage entries. */\n"
        + "    uint16_t count_obj;        /**< Number of object storage entries. */\n"
        + "    uint32_t obj_pool_bytes;   /**< Total object-pool capacity in bytes. */\n"
        + "    uint32_t id_hash_bits;     /**< Static ID hash bit count. */\n"
        + "    uint32_t id_hash_size;     /**< Static ID hash bucket count. */\n"
        + "} par_generated_info_t;\n\n"
        + "/**\n"
        + " * @brief Generated parameter-table summary instance.\n"
        + " */\n"
        + "extern const par_generated_info_t g_par_generated_info;\n\n"
        + "#ifdef __cplusplus\n"
        + "}\n"
        + "#endif /* defined(__cplusplus) */\n\n"
        + "#endif /* !defined(_PAR_GENERATED_INFO_H_) */\n"
    )


def generate_info_c() -> str:
    return (
        generated_banner()
        + "/**\n"
        + " * @file par_generated_info.c\n"
        + " * @brief Define generated parameter-table summary metadata.\n"
        + " */\n\n"
        + "#include \"par_generated_info.h\"\n"
        + "#include \"par_id_map_static.h\"\n\n"
        + "const par_generated_info_t g_par_generated_info = {\n"
        + "    .param_count = (uint16_t)ePAR_NUM_OF,\n"
        + "    .count8 = (uint16_t)PAR_LAYOUT_STATIC_COUNT8,\n"
        + "    .count16 = (uint16_t)PAR_LAYOUT_STATIC_COUNT16,\n"
        + "    .count32 = (uint16_t)PAR_LAYOUT_STATIC_COUNT32,\n"
        + "    .count_obj = (uint16_t)PAR_LAYOUT_STATIC_COUNTOBJ,\n"
        + "    .obj_pool_bytes = (uint32_t)PAR_LAYOUT_STATIC_OBJ_POOL_BYTES,\n"
        + "#if (1 == PAR_CFG_ENABLE_ID)\n"
        + "    .id_hash_bits = (uint32_t)PAR_ID_HASH_BITS,\n"
        + "    .id_hash_size = (uint32_t)PAR_ID_HASH_SIZE,\n"
        + "#else\n"
        + "    .id_hash_bits = 0u,\n"
        + "    .id_hash_size = 0u,\n"
        + "#endif /* (1 == PAR_CFG_ENABLE_ID) */\n"
        + "};\n"
    )


def generate_manifest(rows: List[Row], stats: Dict[str, object]) -> str:
    manifest = {
        "version": 1,
        "generator": f"pargen.py {VERSION}",
        "source": "parameters/schema/par_table.csv",
        "param_count_max": stats["param_count"],
        "layout_max": {
            "count8": stats["count8"],
            "count16": stats["count16"],
            "count32": stats["count32"],
            "count_obj": stats["count_obj"],
            "obj_pool_bytes": stats["obj_pool_bytes"],
        },
        "persistent_max": {
            "scalar_count": stats["persistent_scalar_count"],
            "object_count": stats["persistent_object_count"],
        },
        "id_hash_validation": {
            "bits": stats["hash_bits"],
            "size": stats["hash_size"],
            "algorithm": "(((uint32_t)id * 0x61C88647u) >> (32u - bits))",
            "collisions": 0,
        },
        "rows": [
            {
                "enum": row.enum,
                "id": row.resolved_id,
                "type": row.type,
                "condition": row.condition,
                "group": row.group,
                "section": row.section,
            }
            for row in rows
        ],
    }
    return json.dumps(manifest, indent=4, ensure_ascii=False) + "\n"


def write_outputs(args: argparse.Namespace, outputs: GeneratedOutputs) -> None:
    out_dir = Path(args.out_dir)
    files = {
        Path(args.out_def): outputs.par_table_def,
        out_dir / "par_layout_static.h": outputs.layout_h,
        out_dir / "par_layout_static.c": outputs.layout_c,
        out_dir / "par_generated_info.h": outputs.info_h,
        out_dir / "par_generated_info.c": outputs.info_c,
        Path(args.manifest): outputs.manifest_json,
        Path(args.id_lock): outputs.lock_json,
    }
    for path, content in files.items():
        path.parent.mkdir(parents=True, exist_ok=True)
        write_text_lf(path, content)


def write_text_lf(path: Path, content: str) -> None:
    """Write generated text with LF newlines on Python 3.8 and later."""
    with path.open("w", encoding="utf-8", newline="\n") as stream:
        stream.write(content)


def verify_outputs(args: argparse.Namespace, outputs: GeneratedOutputs) -> None:
    out_dir = Path(args.out_dir)
    files = {
        Path(args.out_def): outputs.par_table_def,
        out_dir / "par_layout_static.h": outputs.layout_h,
        out_dir / "par_layout_static.c": outputs.layout_c,
        out_dir / "par_generated_info.h": outputs.info_h,
        out_dir / "par_generated_info.c": outputs.info_c,
        Path(args.manifest): outputs.manifest_json,
        Path(args.id_lock): outputs.lock_json,
    }
    diffs = []
    for path, expected in files.items():
        if not path.exists():
            diffs.append(f"missing: {path}")
            continue
        current = path.read_text(encoding="utf-8")
        if current != expected:
            diffs.append(f"out-of-date: {path}")
    if diffs:
        raise PargenError("generated outputs are not up to date:\n" + "\n".join(diffs))


def main(argv: Sequence[str]) -> int:
    args = parse_args(argv)
    try:
        rows = read_rows(Path(args.csv))
        lock = read_lock(Path(args.id_lock))
        cfg = load_config(Path(args.config))
        stats = validate_and_resolve(rows, lock, cfg)
        outputs = generate_outputs(rows, stats)
        if args.verify:
            verify_outputs(args, outputs)
        elif not args.check:
            write_outputs(args, outputs)
        print(
            "pargen: OK "
            f"rows={stats['param_count']} count8={stats['count8']} "
            f"count16={stats['count16']} count32={stats['count32']} "
            f"count_obj={stats['count_obj']} obj_pool={stats['obj_pool_bytes']} "
            f"hash_bits={stats['hash_bits']}"
        )
        return 0
    except (PargenError, OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"pargen: ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
