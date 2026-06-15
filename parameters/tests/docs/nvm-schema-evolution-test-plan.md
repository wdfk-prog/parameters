[中文](./nvm-schema-evolution-test-plan.zh-CN.md)

# NVM schema evolution test plan

This document tracks missing tests for persistent-data compatibility when the generated parameter schema changes across firmware versions.

Fixture tables, host fixture checks, a board-side MSH acceptance helper, and the execution document are now implemented. Keep this file as the coverage matrix and extension index.

## Scope

Schema evolution means that firmware V1 writes a valid scalar and/or object NVM image, then firmware V2 boots with a changed generated table while reusing the same NVM contents.

The planned coverage includes:

- scalar parameter append, deletion, insertion, ID change, type change, and default-only change;
- object parameter append, deletion, insertion, ID change, type change, element-size change, and capacity/range change;
- mixed scalar and object changes in the same firmware upgrade;
- explicit `PAR_CFG_TABLE_ID_SCHEMA_VER` changes;
- object placement modes where scalar growth can change the object block address.

## Related implementation

| Area | Implementation |
| --- | --- |
| Table compatibility ID | `src/nvm/par_nvm_table_id.c`, `src/nvm/par_nvm_table_id.h` |
| Scalar persistence core | `src/nvm/scalar/par_nvm_scalar.c`, `src/nvm/scalar/store/par_nvm_scalar_store.c` |
| Scalar layouts | `src/nvm/scalar/layout/par_nvm_layout_*.c` |
| Object persistence core | `src/nvm/object/par_nvm_object.c`, `src/nvm/object/par_nvm_object.h` |
| Object address modes | `src/nvm/object/addr/par_nvm_object_addr_*.c` |
| Object store modes | `src/nvm/object/store/par_nvm_object_store_*.c` |
| Generated schema inputs | `schema/par_table.csv`, `schema/par_id_lock.json`, `tools/pargen.py` |

## Implemented fixture layout

Use separate generated-table fixtures instead of mutating the production table in place.

```text
parameters/tests/fixtures/schema_evolution/
├── v1_base/
│   └── par_table.csv
├── v2_scalar_append/
│   └── par_table.csv
├── v2_scalar_delete_tail/
│   └── par_table.csv
├── v2_scalar_delete_middle/
│   └── par_table.csv
├── v2_scalar_insert_middle/
│   └── par_table.csv
├── v2_scalar_type_change/
│   └── par_table.csv
├── v2_object_append/
│   └── par_table.csv
├── v2_object_delete/
│   └── par_table.csv
├── v2_object_capacity_change/
│   └── par_table.csv
├── v2_mixed_append/
│   └── par_table.csv
├── v2_mixed_scalar_compatible_object_incompatible/
│   └── par_table.csv
├── v2_mixed_scalar_incompatible_object_compatible/
│   └── par_table.csv
└── v2_schema_version_bump/
    └── par_table.csv
```

## General test flow

1. Build or generate firmware artifacts from a V1 fixture.
2. Boot V1 and write non-default scalar and object values to NVM.
3. Preserve the raw NVM image.
4. Build or generate firmware artifacts from a V2 fixture.
5. Boot V2 against the preserved V1 NVM image.
6. Verify restored values, defaulted values, rewrite requests, and rebuild behavior.
7. Verify that logs identify whether the path was compatible recovery, managed rebuild, or corruption handling.

## Scalar evolution matrix

| Case | Expected behavior to lock down |
| --- | --- |
| Append persistent scalar at the end | Existing scalar values are retained; new scalar uses its default; NVM may request rewrite. |
| Insert persistent scalar in the middle | Treat as incompatible unless the selected layout explicitly supports ID-based migration. |
| Delete tail scalar | Define whether deletion is accepted by truncation or treated as incompatible; default expectation is managed rebuild. |
| Delete middle scalar | Treat as incompatible; old values must not shift into the wrong parameter. |
| Change scalar ID | Treat as incompatible; old value must not bind to the new ID. |
| Change scalar type or width | Treat as incompatible; old payload must not be reinterpreted with a new type. |
| Change scalar default only | Existing valid NVM value wins; the new default is used only when the value is absent or rebuilt. |
| Bump `PAR_CFG_TABLE_ID_SCHEMA_VER` | Force table-ID mismatch and managed rebuild from defaults. |

## Object evolution matrix

| Case | Expected behavior to lock down |
| --- | --- |
| Append persistent object at the end | Existing object values are retained; new object is initialized from default; object block may request rewrite. |
| Insert object in the middle | Treat as incompatible unless ID-based migration explicitly proves safe. |
| Delete object | Define whether deletion is accepted by garbage collection or treated as incompatible; default expectation is managed rebuild. |
| Change object ID | Treat as incompatible; old payload must not bind to the new object. |
| Change object type | Treat as incompatible. |
| Change object element size | Treat as incompatible. |
| Decrease object capacity or max length | Treat as incompatible or restore default; old payload must not overflow the new object. |
| Increase object capacity or max length | Existing payload may be retained if ID, type, element size, and stored length remain valid. |
| Raise object min length above stored length | Treat as incompatible or restore default. |

## Mixed scalar/object matrix

| Case | Expected behavior to lock down |
| --- | --- |
| Append scalar only; object table unchanged | Scalar follows append policy; object values remain unaffected. |
| Append object only; scalar table unchanged | Object follows append policy; scalar values remain unaffected. |
| Append scalar and object together | Old scalar/object values are retained; new scalar/object entries use defaults. |
| Scalar-compatible change plus object-incompatible change | Decide whether scalar values survive while object block rebuilds, or whether both domains rebuild together. |
| Scalar-incompatible change plus object-compatible change | Decide whether object values survive while scalar block rebuilds, especially in shared-storage modes. |
| Scalar growth with object-after-scalar placement | Verify that moved object block addresses are handled explicitly and do not read stale data from the old address. |
| Scalar growth with fixed or dedicated object placement | Object values should remain address-stable and independent from scalar-layout growth. |

## Layout-specific notes

The scalar layout families should be tested separately because their compatibility policies are not necessarily identical.

| Layout | Required evolution focus |
| --- | --- |
| `compact-payload` | Prefix-compatible append, payload-size growth, table-ID behavior. |
| `fixed-payload-only` | Prefix-compatible append, fixed-width payload retention. |
| `fixed-slot-no-size` | Prefix-compatible append without per-slot size field. |
| `fixed-slot-with-size` | Prefix-compatible append with per-slot size validation. |
| `grouped-payload-only` | Conservative rebuild path when group counts or object counts do not match. |

## Acceptance checklist

- [ ] V1/V2 fixture tables exist and are documented.
- [ ] The generator can produce separate artifacts for each fixture without modifying the production schema.
- [ ] Scalar-only append/incompatible cases are covered for every scalar layout.
- [ ] Object-only append/incompatible cases are covered for shared, fixed, and dedicated object placement where applicable.
- [ ] Mixed scalar/object changes are covered.
- [ ] `PAR_CFG_TABLE_ID_SCHEMA_VER` bump is covered.
- [ ] Test output distinguishes compatible recovery, managed rebuild, and corruption handling.
- [ ] NVM images are preserved or dumpable before and after V2 boot for diagnosis.
