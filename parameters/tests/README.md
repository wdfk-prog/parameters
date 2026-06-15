[中文](./README.zh-CN.md)

# Parameters tests

This directory is the single entry point for maintained runtime tests and test documents in the portable `Device Parameters` module.

## Test entry points

| Test area | Entry point | Build or run option | Notes |
| --- | --- | --- | --- |
| Runtime test runner | `par_test [all|list|ram|at24|flash_ee_fal]` or `par_test_run_by_name()` | `AUTOGEN_PM_USING_TESTS` | Core runner is reusable; MSH is only the RT-Thread wrapper. |
| RAM runtime coverage | `par_test ram` | `AUTOGEN_PM_TEST_USING_RAM_CONFIG` | Validates default restore, scalar APIs, ranges, metadata, IDs, access policy, callbacks, raw reset, F32, and object RAM paths. |
| AT24CXX persistence coverage | `par_test at24` | `AUTOGEN_PM_TEST_USING_AT24CXX` | Validates the AT24CXX-backed scalar persistence path through public APIs. |
| Flash EE real-FAL coverage | `par_test flash_ee_fal` | `AUTOGEN_PM_TEST_USING_FLASH_EE_FAL`, `AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE` | Destructively validates Flash EE on a dedicated real FAL partition. No fake flash suite is compiled by the RT-Thread package build. |
| Raw NVM helper | `par_nvm_raw ...` or `par_test_nvm_raw_exec()` | `AUTOGEN_PM_TEST_NVM_RAW_HELPER` | Layout-neutral dump, poke, flip, and scalar-header corruption helper. |
| Fixed-slot-with-size helper | `par_nvm_fslot ...` or `par_test_nvm_fslot_exec()` | `AUTOGEN_PM_TEST_NVM_FIXED_SLOT_WITH_SIZE` | Layout-aware scalar fixed-slot CRC/corruption acceptance helper. |
| Fixed-slot-no-size helper | `par_nvm_fslot_no_size ...` or `par_test_nvm_fslot_no_size_exec()` | `AUTOGEN_PM_TEST_NVM_FIXED_SLOT_NO_SIZE` | Layout-aware scalar fixed-slot-no-size address, CRC, and payload corruption helper. |
| Compact-payload helper | `par_nvm_compact_payload ...` or `par_test_nvm_compact_payload_exec()` | `AUTOGEN_PM_TEST_NVM_COMPACT_PAYLOAD` | Variable-record scalar compact-payload address, CRC, size, and payload corruption helper. |
| Fixed-payload-only helper | `par_nvm_fixed_payload_only ...` or `par_test_nvm_fixed_payload_only_exec()` | `AUTOGEN_PM_TEST_NVM_FIXED_PAYLOAD_ONLY` | Payload-only scalar address, CRC, and payload corruption helper. |
| Grouped-payload-only helper | `par_nvm_grouped_payload_only ...` or `par_test_nvm_grouped_payload_only_exec()` | `AUTOGEN_PM_TEST_NVM_GROUPED_PAYLOAD_ONLY` | Grouped payload-band scalar address, CRC, and payload corruption helper. |
| Object NVM helper | `par_nvm_obj ...` or `par_test_nvm_obj_exec()` | `AUTOGEN_PM_TEST_NVM_OBJECT_HELPER` | Object block inspection, payload write, and corruption injection helper. |
| NVM schema-evolution acceptance helper | `par_nvm_schema ...` | `AUTOGEN_PM_TEST_NVM_SCHEMA_EVOLUTION` | Runs V1/V2 fixture prepare, dump, append, and rebuild acceptance steps. |
| Generator unit tests | `python3 -m unittest parameters/tests/test_pargen.py` | Host Python | Validates `tools/pargen.py` behavior outside firmware. |

## Hardware-only Flash EE policy

The RT-Thread package build now keeps only the real hardware Flash EE suite:

```text
par_test flash_ee_fal
```

The previous fake-flash `par_test flash_ee` suite and `AUTOGEN_PM_TEST_USING_FLASH_EE` build option are removed from this package integration. Software flash simulation should be added later as a separate host/CI harness, not as MCU firmware code.

## Existing test code index

| Test item | Test code | Document | Status |
| --- | --- | --- | --- |
| Runtime test framework | `par_test.h`, `par_test_common.c`, `par_test_runner.c`, `par_test_msh.c` | [Runtime tests](./docs/runtime-tests.md) | Core runner and MSH wrapper are split for reuse. |
| RAM runtime tests | `par_test_ram_config.c` | [Runtime tests](./docs/runtime-tests.md) | Test code exists; detailed case list is indexed. |
| AT24CXX runtime tests | `par_test_at24cxx.c` | [Runtime tests](./docs/runtime-tests.md) | Test code exists; board-specific notes are indexed. |
| Flash EE real-FAL runtime tests | `par_test_flash_ee_fal.c` | [Flash EE real-FAL test flow](./docs/flash-ee-test-plan.md) | Real FAL partition suite exists; covers bind/init, persistence, logical erase, full-bank rollover, multi-cycle rollover, and post-full-bank writability. |
| Raw NVM manual helper | `par_test_nvm_raw.c` | [Raw NVM manual test](./docs/nvm-raw-manual-test.md) | Test code exists; manual document is a fill-in checklist. |
| Fixed-slot-with-size scalar NVM | `par_test_nvm_fixed_slot_with_size.c` | [Fixed-slot-with-size NVM manual test](./docs/nvm-fixed-slot-with-size-manual-test.md) | Test code and manual document exist. |
| Fixed-slot-no-size scalar NVM | `par_test_nvm_fixed_slot_no_size.c` | [Fixed-slot-no-size NVM manual test](./docs/nvm-fixed-slot-no-size-manual-test.md) | Dedicated helper and detailed manual document exist. |
| Compact-payload scalar NVM | `par_test_nvm_compact_payload.c` | [Compact-payload NVM manual test](./docs/nvm-compact-payload-manual-test.md) | Dedicated helper and detailed manual document exist. |
| Fixed-payload-only scalar NVM | `par_test_nvm_fixed_payload_only.c` | [Fixed-payload-only NVM manual test](./docs/nvm-fixed-payload-only-manual-test.md) | Dedicated helper and detailed manual document exist. |
| Grouped-payload-only scalar NVM | `par_test_nvm_grouped_payload_only.c` | [Grouped-payload-only NVM manual test](./docs/nvm-grouped-payload-only-manual-test.md) | Dedicated helper and detailed manual document exist. |
| Object NVM | `par_test_nvm_object.c` | [Object NVM manual test](./docs/nvm-object-manual-test.md) | Test code and manual document exist. |
| pargen generator | `test_pargen.py` | [pargen tests](./docs/pargen-tests.md) | Test code exists; host-side document is a fill-in checklist. |
| NVM schema/data evolution | `par_test_schema_evolution.c`, `schema_evolution/par_schema_evolution_core.[ch]`, `test_schema_evolution_fixtures.py`, `fixtures/schema_evolution/*/par_table.csv` | [NVM schema evolution test plan](./docs/nvm-schema-evolution-test-plan.md), [execution and acceptance](./docs/nvm-schema-evolution-acceptance.md) | Fixtures, reusable core interface, host fixture checks, and board acceptance wrapper are implemented. |

## Real FAL partition requirements

Required test-firmware configuration:

```text
AUTOGEN_PM_USING_TESTS=y
AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE=y
AUTOGEN_PM_TEST_USING_FLASH_EE_FAL=y
AUTOGEN_PM_USING_FLASH_EE_BACKEND=y
AUTOGEN_PM_FLASH_EE_PORT_FAL=y
AUTOGEN_PM_FLASH_EE_FAL_PARTITION_NAME="pm_ee_test"
```

The `flash_ee_fal` suite rewrites `pm_ee_test`. Do not share this partition with the filesystem, logs, or production parameter storage.

For a 128 KiB `pm_ee_test` partition, keep `AUTOGEN_PM_FLASH_EE_LOGICAL_SIZE` at `0x8000` or smaller so each bank can hold one compacted logical image plus rollover records.

## Document ownership rule

Keep test-related documents under this directory. Use `docs/` for detailed test plans and keep this README as the summary and index.
