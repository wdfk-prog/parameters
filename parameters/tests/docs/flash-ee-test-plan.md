[中文](./flash-ee-test-plan.zh-CN.md)

# Flash EE real-FAL test flow and case reference

This document describes the maintained board-level Flash EE runtime suite. The suite runs only on a real FAL partition and intentionally rewrites that partition.

The previous fake-flash `flash_ee` suite is removed from the RT-Thread package build. Software flash simulation and deterministic power-loss injection should be implemented later as a separate host/CI harness.

## Related implementation

| Scope | File |
| --- | --- |
| Portable Flash EE core | `parameters/src/nvm/backend/par_store_backend_flash_ee.c` |
| FAL adapter | `backend/par_store_backend_flash_ee_fal.c` |
| Real hardware test suite | `parameters/tests/par_test_flash_ee_fal.c` |
| Test runner | `parameters/tests/par_test_runner.c`, `parameters/tests/par_test_msh.c` |

## Test boundary

`flash_ee_fal` covers the real FAL-backed path:

- FAL partition lookup and backend bind/init/deinit;
- empty partition format on real flash;
- write/read persistence after backend reinitialization;
- logical erase persistence after backend reinitialization;
- full-bank rollover and active-bank recovery;
- multiple rollover cycles on the same partition;
- physically full active bank scanned after reboot-like reinit and still writable.

The suite does not simulate half-programmed or half-erased flash cells. Those deterministic fault-injection cases belong in a later host/CI simulator or in manual HIL power-cut tests.

## Required board partition

Reserve a dedicated FAL partition, for example:

```text
pm_ee_test
```

The partition must not be shared with filesystem, crash logs, or production parameter storage.

For an 8 MiB external NOR flash with a 4 KiB `cmb_log` partition at the end, a 128 KiB test partition can be placed before `cmb_log`:

```text
filesystem  offset=0x000000 size=0x7DF000
pm_ee_test  offset=0x7DF000 size=0x020000
cmb_log     offset=0x7FF000 size=0x001000
```

## Build configuration

Enable these options in board-test firmware:

```text
PKG_USING_AUTOGEN_PARAMETER_MANAGER=y
AUTOGEN_PM_USING_TESTS=y
AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE=y
AUTOGEN_PM_TEST_USING_FLASH_EE_FAL=y
AUTOGEN_PM_USING_NVM=y
AUTOGEN_PM_NVM_SCALAR=y
AUTOGEN_PM_USING_FLASH_EE_BACKEND=y
AUTOGEN_PM_FLASH_EE_PORT_FAL=y
AUTOGEN_PM_FLASH_EE_FAL_PARTITION_NAME="pm_ee_test"
```

For a 128 KiB `pm_ee_test` partition split into two banks with 32-byte lines and 8-byte program units, use:

```text
AUTOGEN_PM_FLASH_EE_LOGICAL_SIZE=0x8000
AUTOGEN_PM_FLASH_EE_CACHE_SIZE=4096
AUTOGEN_PM_FLASH_EE_LINE_SIZE=32
AUTOGEN_PM_FLASH_EE_PROGRAM_SIZE=8
```

Do not enable the removed `AUTOGEN_PM_TEST_USING_FLASH_EE` option. The RT-Thread package build no longer compiles `par_test_flash_ee.c`.

## Run flow

1. Build and flash the board-test firmware.
2. Open RT-Thread MSH.
3. Confirm the suite is registered:

   ```text
   msh /> par_test list
   ```

   Expected list contains:

   ```text
   PAR_TEST_SUITE name=flash_ee_fal cases=6
   ```

4. Run the real flash suite:

   ```text
   msh /> par_test flash_ee_fal
   ```

5. Acceptance output:

   ```text
   PAR_TEST_BEGIN suite=flash_ee_fal cases=6
   PAR_TEST_CASE PASS suite=flash_ee_fal case=bind_init
   PAR_TEST_CASE PASS suite=flash_ee_fal case=write_read_persists_after_reinit
   PAR_TEST_CASE PASS suite=flash_ee_fal case=erase_persists_after_reinit
   PAR_TEST_CASE PASS suite=flash_ee_fal case=wrap_after_bank_full_preserves_latest
   PAR_TEST_CASE PASS suite=flash_ee_fal case=wrap_multiple_cycles_preserves_latest
   PAR_TEST_CASE PASS suite=flash_ee_fal case=full_bank_then_reinit_remains_writable
   PAR_TEST_SUMMARY suite=flash_ee_fal total=6 pass=6 fail=0 skip=0
   PAR_TEST_RESULT PASS suite=flash_ee_fal
   ```

If `AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE` is disabled, destructive cases are skipped by the runner. This is a protection mechanism, not a Flash EE failure.

## Case list

| Case | Main action | Core assertion |
| --- | --- | --- |
| `bind_init` | Erase `pm_ee_test`, bind FAL adapter, initialize and deinitialize backend | Real partition can be found, formatted, and opened. |
| `write_read_persists_after_reinit` | Erase logical space, write 64 bytes, sync, deinit/init, then read back | Data survives backend restart. |
| `erase_persists_after_reinit` | Write 64 bytes, logically erase them, sync, deinit/init, then read back | Erased range reads as `0xFF` after restart. |
| `wrap_after_bank_full_preserves_latest` | Repeatedly write one hot line until active bank rolls over, keep another stable line, then write a post-rollover line | Latest hot line, stable line, and post-rollover line all survive restart. |
| `wrap_multiple_cycles_preserves_latest` | Execute three rollover cycles with restart-like reinit after each cycle | Latest line image remains valid across repeated real flash bank switches. |
| `full_bank_then_reinit_remains_writable` | Fill active bank exactly, deinit/init, then perform another write | Full-bank scan after restart does not wedge the backend; next write persists. |

## Troubleshooting

| Symptom | Check first |
| --- | --- |
| `par_test list` does not show `flash_ee_fal` | `AUTOGEN_PM_USING_TESTS`, `AUTOGEN_PM_TEST_USING_FLASH_EE_FAL`, and whether `SConscript` includes `par_test_flash_ee_fal.c`. |
| All cases are skipped | `AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE` is disabled. |
| `partition_not_found=pm_ee_test` | `fal_cfg.h` does not define the partition, partition name mismatch, or FAL partition table not enabled. |
| `partition_geometry_invalid` | Partition is too small for the configured logical size, line size, and record overhead. |
| Rollover cases fail | Check bank size, program granularity, FAL erase/program return values, active-bank scan, and checkpoint/bank-switch logic. |
| Link-time RAM overflow | Ensure the removed fake-flash suite is not compiled. Only `par_test_flash_ee_fal.c` should be included for Flash EE board tests. |

## Manual HIL follow-up

Real power-loss windows still require manual or automated HIL support. Suggested later tests:

1. reset while programming record payload/metadata/commit;
2. reset while activating target bank header;
3. reset while erasing inactive bank;
4. repeated reset during multi-cycle rollover;
5. end-to-end parameter persistence through `par_save` and `par_save_by_id` on the real FAL partition.
