[中文](./nvm-schema-evolution-acceptance.zh-CN.md)

# NVM schema evolution execution and acceptance

This document describes how to use `tests/fixtures/schema_evolution/` and the `par_nvm_schema` MSH helper to validate persistent-data behavior across V1/V2 parameter-table changes.

## Scope

The acceptance flow covers scalar-only append/delete/insert/type-change cases, object-only append/delete/capacity-change cases, mixed scalar/object changes, and explicit `AUTOGEN_PM_TABLE_ID_SCHEMA_VER` bumps.

Run this only on disposable EEPROM or flash partitions. The helper intentionally writes and rewrites managed NVM data.

## Build options

Recommended V1 and V2 test-image options:

```text
PKG_USING_AUTOGEN_PARAMETER_MANAGER=y
AUTOGEN_PM_USING_DEBUG=y
AUTOGEN_PM_USING_NVM=y
AUTOGEN_PM_USING_TABLE_ID_CHECK=y
AUTOGEN_PM_NVM_SCALAR=y
AUTOGEN_PM_NVM_OBJECT=y
AUTOGEN_PM_NVM_OBJECT_STORE_SHARED=y or AUTOGEN_PM_NVM_OBJECT_STORE_DEDICATED=y
AUTOGEN_PM_ENABLE_ID=y
AUTOGEN_PM_ENABLE_TYPE_OBJECT=y
AUTOGEN_PM_ENABLE_TYPE_STR=y
AUTOGEN_PM_ENABLE_TYPE_BYTES=y
AUTOGEN_PM_USING_TESTS=y
AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE=y
AUTOGEN_PM_TEST_NVM_SCHEMA_EVOLUTION=y
RT_USING_FINSH=y
```

If the active scalar layout is `grouped-payload-only`, parameter-count changes take the conservative rebuild path. Validate scalar append fixtures with `verify scalar_rebuild`; use the strict object-retain/default variants only when the fixture explicitly requires one object outcome.

For `v2_schema_version_bump`, set this only in the V2 image:

```text
AUTOGEN_PM_TABLE_ID_SCHEMA_VER=2
```

Keep V1 at the default schema version `1`.

## Fixture generation

Fixtures are stored under:

```text
parameters/tests/fixtures/schema_evolution/
```

Before building one image, generate the table from the selected fixture:

```sh
python3 parameters/tools/pargen.py \
  --csv parameters/tests/fixtures/schema_evolution/v1_base/par_table.csv \
  --id-lock parameters/schema/par_id_lock.json \
  --out-def par_table.def \
  --out-dir parameters/generated \
  --manifest parameters/generated/par_manifest.json
```

A temporary fixture-specific ID lock may be used. The board helper only requires the fixed fixture IDs.

## Reusable acceptance interface

The schema-evolution checks are split into a reusable core and the RT-Thread MSH wrapper:

| Layer | File | Purpose |
| --- | --- | --- |
| Core acceptance interface | `../schema_evolution/par_schema_evolution_core.h` | Exposes `par_schema_evolution_prepare()`, `par_schema_evolution_dump()`, and `par_schema_evolution_verify()` for board and future host runners. |
| Core implementation | `../schema_evolution/par_schema_evolution_core.c` | Contains the fixture IDs, prepared values, and verification rules. It has no MSH command parsing. |
| Board wrapper | `../par_test_schema_evolution.c` | Registers `par_nvm_schema` and forwards commands to the core interface. |

Future GitHub Actions host simulation should link the same core interface with a file-backed or memory-backed NVM port. This document does not define or require that host simulator yet; it only keeps the board helper structured so those checks can be reused later.

## Board commands

```text
par_nvm_schema prepare
par_nvm_schema dump
par_nvm_schema verify base
par_nvm_schema verify scalar_append
par_nvm_schema verify object_append
par_nvm_schema verify mixed_append
par_nvm_schema verify scalar_append_object_rebuild
par_nvm_schema verify object_rebuild
par_nvm_schema verify scalar_rebuild
par_nvm_schema verify scalar_rebuild_object_retain
par_nvm_schema verify scalar_rebuild_object_default
par_nvm_schema verify full_rebuild
```

| Command | Phase | Description |
| --- | --- | --- |
| `prepare` | V1 image | Initialize the module, write known non-default values, and call `par_save_clean()`. |
| `dump` | V1/V2 image | Print fixed fixture ID presence, parameter number, type, and persistent flag. |
| `verify base` | V1 reboot | Confirm the V1 prepared values reload from NVM. |
| `verify scalar_append` | append-style V2 image | Recommended command. It selects retained-object or defaulted-object checks from the compiled object placement mode. |
| `verify object_append` | object append V2 image | Force retained V1 scalar/object values plus defaulted appended values. |
| `verify mixed_append` | scalar and object append V2 image | Use the same placement-aware checks as `scalar_append`, while also allowing appended object defaults. |
| `verify scalar_append_object_rebuild` | scalar growth or scalar-compatible/object-incompatible V2 image | Force retained scalar values and defaulted object rows after object-block relocation or object contract change. |
| `verify object_rebuild` | object deletion or object capacity/type incompatible V2 image | Confirm scalar rows retain V1 values while object rows are defaulted or removed. |
| `verify scalar_rebuild` | scalar deletion, insertion, or type-change V2 image | Confirm scalar rows are defaulted; object rows may be retained, defaulted, or absent. |
| `verify scalar_rebuild_object_retain` | scalar rebuild image with intentionally retained object storage | Strictly confirm scalar rows are defaulted while compatible object rows retain V1 values. |
| `verify scalar_rebuild_object_default` | scalar rebuild image with intentionally rebuilt object storage | Strictly confirm scalar rows are defaulted while object rows are defaulted or absent. |
| `verify full_rebuild` | schema-version bump or explicitly global rebuild V2 image | Confirm known present rows are restored to defaults after managed rebuild. |

## RT-Thread option completion

When `FINSH_USING_OPTION_COMPLETION` is enabled, the helper exports option tables for `prepare`, `dump`, `verify`, and every verify mode listed above. This enables TAB completion for the board-level MSH command path while keeping the reusable core API independent from FINSH.

## Detailed procedure: `v2_scalar_append` placement-aware

This section expands this matrix row into executable steps:

```text
v2_scalar_append    any object placement    par_nvm_schema verify scalar_append
```

This case validates that V2 only appends scalar ID `60010` at the tail. Use `par_nvm_schema verify scalar_append`; the helper selects whether object values must be retained or default-rebuilt from the compiled object placement mode.

### A. Select the object placement mode

Run these as separate acceptances when the modes are supported. They cover different risks and are not interchangeable:

| Target | Key options | Append expectation |
| --- | --- | --- |
| after-scalar shared object block | `AUTOGEN_PM_NVM_OBJECT_STORE_SHARED=y` + `AUTOGEN_PM_NVM_OBJECT_ADDR_AFTER_SCALAR=y` | scalar values retained; object values default-rebuilt because the object block moves |
| fixed shared object block | `AUTOGEN_PM_NVM_OBJECT_STORE_SHARED=y` + `AUTOGEN_PM_NVM_OBJECT_ADDR_FIXED=y` | scalar/object values retained |
| dedicated object store | `AUTOGEN_PM_NVM_OBJECT_STORE_DEDICATED=y` | scalar/object values retained |

V1 and V2 must use the same object placement/backend options. Only the schema fixture should change between V1 and V2.

If you switch back from a V2 image to a V1 image without erasing NVM, V1 boot may first see the stale V2 NVM header, such as `stored_obj_count=3` or a table-ID mismatch, and rebuild defaults once. That does not fail the V1 baseline step. The V1 baseline is established only after `par_nvm_schema prepare` is executed and `verify base` passes after reset.

### B. Build and flash the V1 image

Generate the parameter table from the V1 fixture:

```sh
python3 parameters/tools/pargen.py \
  --csv parameters/tests/fixtures/schema_evolution/v1_base/par_table.csv \
  --id-lock parameters/schema/par_id_lock.json \
  --out-def par_table.def \
  --out-dir parameters/generated \
  --manifest parameters/generated/par_manifest.json
```

Build and flash the V1 image with the selected after-scalar, fixed, or dedicated object placement configuration.

### C. Write the V1 baseline NVM image

After the V1 image boots, confirm the fixture IDs:

```text
par_nvm_schema dump
```

Expected evidence starts with the active verification branch. The following three outputs are valid.

For after-scalar placement:

```text
SCHEMA_CFG object_placement=after_scalar append_object_expect=default
```

For fixed placement:

```text
SCHEMA_CFG object_placement=fixed append_object_expect=retain
```

For dedicated placement:

```text
SCHEMA_CFG object_placement=dedicated append_object_expect=retain
```

If the output is `after_scalar/default`, you are validating the after-scalar branch and must not apply the fixed/dedicated object-retain expectation.

Then expect:

```text
SCHEMA_ID id=60006 name=base_u8 ... persistent=1
SCHEMA_ID id=60007 name=tail_u16 ... persistent=1
SCHEMA_ID id=60008 name=name_str ... persistent=1
SCHEMA_ID id=60009 name=blob_bytes ... persistent=1
SCHEMA_ID id=60010 name=new_u8 absent ...
```

Write the known V1 values:

```text
par_nvm_schema prepare
```

Expected output:

```text
PAR_SCHEMA_PREPARED base_u8=42 tail_u16=4242 name=hv1 blob=A5-5A-C3-3C
```

Reset or power-cycle the board, still with the V1 image, then verify that the values reload from NVM:

```text
par_nvm_schema verify base
```

Expected output contains at least:

```text
OK base_u8 id=60006 value=42
OK tail_u16 id=60007 value=4242
OK name_str id=60008 value=hv1
OK blob_bytes id=60009 len=4
PAR_SCHEMA_VERIFY PASS mode=base(0)
```

If this step fails, stop before V2 validation and fix the NVM backend, fixture, persistent flags, or object type configuration.

### D. Build and flash the V2 image without erasing NVM

Do not mass-erase flash, EEPROM, flash-ee, or the FAL partition. V2 validation must reuse the V1 NVM image from step C.

Generate the parameter table from the `v2_scalar_append` fixture:

```sh
python3 parameters/tools/pargen.py \
  --csv parameters/tests/fixtures/schema_evolution/v2_scalar_append/par_table.csv \
  --id-lock parameters/schema/par_id_lock.json \
  --out-def par_table.def \
  --out-dir parameters/generated \
  --manifest parameters/generated/par_manifest.json
```

Keep the same backend, scalar layout, and object placement configuration as V1, then build and flash the V2 image.

### E. Verify `v2_scalar_append` on V2

After V2 boots, first confirm the active object placement and that `60010` exists in the active table:

```text
par_nvm_schema dump
```

Expected evidence starts with the active verification branch. The following three outputs are valid.

For after-scalar placement:

```text
SCHEMA_CFG object_placement=after_scalar append_object_expect=default
```

For fixed placement:

```text
SCHEMA_CFG object_placement=fixed append_object_expect=retain
```

For dedicated placement:

```text
SCHEMA_CFG object_placement=dedicated append_object_expect=retain
```

`scalar_append` selects the object acceptance rule from this line.

Then expect:

```text
SCHEMA_ID id=60006 name=base_u8 ... persistent=1
SCHEMA_ID id=60007 name=tail_u16 ... persistent=1
SCHEMA_ID id=60008 name=name_str ... persistent=1
SCHEMA_ID id=60009 name=blob_bytes ... persistent=1
SCHEMA_ID id=60010 name=new_u8 ... persistent=1
```

Run the placement-aware append verification:

```text
par_nvm_schema verify scalar_append
```

If `SCHEMA_CFG` is `fixed/retain` or `dedicated/retain`, expected output contains at least:

```text
SCHEMA_VERIFY scalar_append placement=fixed object_expect=retain
OK base_u8 id=60006 value=42
OK tail_u16 id=60007 value=4242
OK name_str id=60008 value=hv1
OK blob_bytes id=60009 len=4
OK new_u8_default id=60010 default type=
PAR_SCHEMA_VERIFY PASS mode=scalar_append(1)
```

For dedicated placement, the first line uses `placement=dedicated`.

If `SCHEMA_CFG` is `after_scalar/default`, do not use this retain expectation; use the after-scalar expectation in the next section.

Acceptance criteria:

| ID | Check | Pass evidence |
| --- | --- | --- |
| `60006` | Retained V1 scalar | Output includes `value=42` |
| `60007` | Retained V1 scalar | Output includes `value=4242` |
| `60008` | Retained V1 STR object | Output includes `value=hv1` |
| `60009` | Retained V1 BYTES object | Output includes `len=4`; the helper internally compares `A5 5A C3 3C` |
| `60010` | Defaulted V2 appended scalar | Output includes `OK new_u8_default id=60010 default type=...`; the compiled V2 default is `55` |

The helper performs all value comparisons. Manual inspection only needs to confirm the final line:

```text
PAR_SCHEMA_VERIFY PASS mode=scalar_append(1)
```

To manually double-check `60010`, read external ID `60010` through the normal parameter read command or a temporary debug interface. It must be `55`.

## Detailed procedure: `v2_scalar_append` with after-scalar object placement

If your log contains the following line, the current run is the after-scalar branch:

```text
SCHEMA_CFG object_placement=after_scalar append_object_expect=default
```

In that case, the fixed/dedicated `append_object_expect=retain` examples do not apply.

For this configuration:

```text
AUTOGEN_PM_NVM_OBJECT_STORE_SHARED=y
AUTOGEN_PM_NVM_OBJECT_ADDR_AFTER_SCALAR=y
```

The V1 preparation commands are the same as B/C above, but `par_nvm_schema dump` must show:

```text
SCHEMA_CFG object_placement=after_scalar append_object_expect=default
```

Use the same automatic V2 verification command:

```text
par_nvm_schema verify scalar_append
```

Expected output contains at least:

```text
SCHEMA_VERIFY scalar_append placement=after_scalar object_expect=default
OK base_u8 id=60006 value=42
OK tail_u16 id=60007 value=4242
OK new_u8_default id=60010 default type=
OK name_str_default id=60008 default=v1
OK blob_bytes_default id=60009 default_len=4
PAR_SCHEMA_VERIFY PASS mode=scalar_append(1)
```

Acceptance criteria:

| ID | Check | Pass evidence |
| --- | --- | --- |
| `60006` | Retained V1 scalar | Output includes `value=42` |
| `60007` | Retained V1 scalar | Output includes `value=4242` |
| `60008` | Object block moved after scalar growth | Restored to the V2 compiled default, not `hv1` |
| `60009` | Object block moved after scalar growth | Restored to the V2 compiled default payload, not `A5 5A C3 3C` |
| `60010` | Defaulted V2 appended scalar | Default value is `55` |

Do not force after-scalar placement through `verify object_append`. Scalar-area growth changes the object-block start address, so controlled object default rebuild is the expected behavior for this placement. If boot logs show `object initialization finished with status=WARN SET TO DEF` and `par_nvm_schema dump` prints `SCHEMA_CFG object_placement=after_scalar append_object_expect=default`, use `verify scalar_append` or `verify scalar_append_object_rebuild`.

## Minimal operation template for other scenarios

Use the same flow for each V2 fixture:

1. Build and flash V1 from `v1_base`.
2. Run `par_nvm_schema prepare` on V1.
3. Reset V1 and run `par_nvm_schema verify base`; require `PASS mode=base(0)`.
4. Do not erase NVM.
5. Generate the table from the target V2 fixture, then build and flash V2.
6. Run the `par_nvm_schema verify ...` command from the matrix.
7. If the final line is not `PAR_SCHEMA_VERIFY PASS ...`, the scenario fails.

## Acceptance matrix

| V2 fixture | Extra V2 config | Command | Expected result |
| --- | --- | --- | --- |
| `v2_scalar_append` | fixed/dedicated object placement | `par_nvm_schema verify scalar_append` | V1 values retained; `60010` defaults to `55`. |
| `v2_scalar_append` | after-scalar object placement | `par_nvm_schema verify scalar_append` | Scalar V1 values retained; object rows default after relocation; `60010` defaults. |
| `v2_object_append` | none | `par_nvm_schema verify object_append` | V1 values retained; `60011` defaults to `new`. |
| `v2_mixed_append` | fixed/dedicated object placement | `par_nvm_schema verify mixed_append` | V1 scalar/object values retained; appended scalar/object defaulted. |
| `v2_mixed_append` | after-scalar object placement | `par_nvm_schema verify mixed_append` | Scalar V1 values retained; object rows default; appended rows defaulted. |
| `v2_scalar_delete_tail` | none | `par_nvm_schema verify scalar_rebuild` | Scalar rows are defaulted or removed; object rows may be retained, defaulted, or absent. Use the strict retain/default commands when that outcome is part of the case. |
| `v2_scalar_delete_middle` | none | `par_nvm_schema verify object_rebuild` | This fixture removes a middle object row; scalar rows retain V1 values, removed object rows may be absent, and remaining object rows default. |
| `v2_scalar_insert_middle` | none | `par_nvm_schema verify scalar_rebuild` | Scalar rows are defaulted; object rows may be retained, defaulted, or absent. Use a strict object command for outcome-specific checks. |
| `v2_scalar_type_change` | none | `par_nvm_schema verify scalar_rebuild` | Same ID with a different scalar type must not interpret the old payload; object rows may be retained, defaulted, or absent. For the common retained-object path, `scalar_rebuild_object_retain` may be used as the stricter check. |
| `v2_object_delete` | none | `par_nvm_schema verify object_rebuild` | Scalar rows retain V1 values; removed object rows may be absent and remaining object rows default. |
| `v2_object_capacity_change` | none | `par_nvm_schema verify object_rebuild` | Scalar rows retain V1 values; the object row with changed capacity defaults instead of reusing the old payload. |
| `v2_mixed_scalar_compatible_object_incompatible` | none | `par_nvm_schema verify scalar_append_object_rebuild` | Scalar rows retain V1 values, the appended scalar defaults, and incompatible object rows default. |
| `v2_mixed_scalar_incompatible_object_compatible` | none | `par_nvm_schema verify scalar_rebuild` | Scalar rows default; object rows may be retained, defaulted, or absent. |
| `v2_schema_version_bump` | `AUTOGEN_PM_TABLE_ID_SCHEMA_VER=2` | `par_nvm_schema verify full_rebuild` | Rows are unchanged but schema-version mismatch forces default rebuild. |

## Pass criteria

- V1 `prepare` succeeds and V1 reboot `verify base` passes.
- Append-style V2 images should use `verify scalar_append` and must finish with `PASS`.
- Object-only incompatible V2 images pass `verify object_rebuild`.
- Scalar incompatible V2 images pass `verify scalar_rebuild`; strict object-retain/default variants are optional when the expected object path is known.
- Schema-version-bump V2 images pass `verify full_rebuild`.
- Logs distinguish normal restore, table-ID mismatch, and rebuild paths.
- V2 validation reuses the V1 NVM image; do not erase NVM between V1 prepare and V2 boot.
