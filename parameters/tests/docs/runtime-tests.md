[中文](./runtime-tests.zh-CN.md)

# Runtime tests

This document indexes the RT-Thread runtime tests compiled from `parameters/tests/*.c`.

## Build options

| Option | Purpose |
| --- | --- |
| `AUTOGEN_PM_USING_TESTS` | Enables the common runtime-test framework and `par_test` MSH command. |
| `AUTOGEN_PM_TEST_USING_RAM_CONFIG` | Enables RAM-only parameter API coverage from `par_test_ram_config.c`. |
| `AUTOGEN_PM_TEST_FORCE_RAM_FEATURES` | Selects optional features needed by the full RAM test matrix. |
| `AUTOGEN_PM_TEST_TABLE_ROWS` | Enables dedicated scalar RAM test rows in `par_table.def`. |
| `AUTOGEN_PM_TEST_OBJECT_ROWS` | Enables dedicated object rows used by RAM object tests. |
| `AUTOGEN_PM_TEST_USING_AT24CXX` | Enables AT24CXX persistence tests from `par_test_at24cxx.c`. |
| `AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE` | Allows destructive tests and helpers to modify disposable persistent storage. |
| `AUTOGEN_PM_TEST_AUTO_RUN` | Runs enabled suites after application initialization. |
| `AUTOGEN_PM_TEST_FAIL_FAST` | Stops the active suite at the first failed case. |

## Test code map

| File | Coverage |
| --- | --- |
| `par_test.h` | Shared test framework declarations, suite/case descriptors, and assertion helpers. |
| `par_test_common.c` | Common formatting, status, and helper routines used by runtime tests. |
| `par_test_runner.c` | Hardware/host reusable suite registry, output callback, and suite dispatch core. |
| `par_test_msh.c` | RT-Thread MSH wrapper and optional auto-run entry point for `par_test_runner.c`. |
| `par_test_ram_config.c` | Non-persistent RAM parameter API behavior and feature coverage. |
| `par_test_at24cxx.c` | AT24CXX-backed scalar persistence behavior through public APIs. |

## Reuse model

The runtime test framework is split so that test logic is not tied to RT-Thread shell commands:

| Interface | Intended user | Notes |
| --- | --- | --- |
| `par_test_set_vprint()` | Hardware and host harnesses | Binds stable test output to RT-Thread, stdout, or a capture buffer. |
| `par_test_run_by_name()` | Hardware and host harnesses | Runs `all` or a named suite without using MSH. |
| `par_test_run_suite()` / `par_test_get_suite()` | Host harnesses | Allows a future simulator runner to enumerate and schedule suites directly. |
| `par_test` MSH command | Hardware HIL | Thin wrapper only; it must not own test pass/fail logic. |

Host simulator code is not implemented here yet. A future runner should provide a software parameter/NVM backend, bind a printer with `par_test_set_vprint()`, then call the core interfaces above.

## Manual checklist placeholder

Keep detailed case-by-case expected output here when a board profile is fixed for CI or HIL.
