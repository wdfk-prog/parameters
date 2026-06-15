[中文](./nvm-raw-manual-test.zh-CN.md)

# Raw NVM manual test

This document indexes the layout-neutral `par_nvm_raw` helper from `par_test_nvm_raw.c`.

## Scope

`par_nvm_raw` is intended for destructive board-level validation on disposable persistence data. It can dump, poke, flip, and corrupt bytes through backend-relative scalar NVM offsets without depending on a specific scalar record layout.

## Build options

| Option | Requirement |
| --- | --- |
| `AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE` | Must be enabled. |
| `AUTOGEN_PM_USING_NVM` | Must be enabled. |
| `AUTOGEN_PM_NVM_SCALAR` | Must be enabled. |
| `AUTOGEN_PM_TEST_NVM_RAW_HELPER` | Builds the `par_nvm_raw` command. |
| `RT_USING_FINSH` | Required for the MSH command. |

## Reuse interface

The helper action layer is reusable outside MSH through:

```c
int par_test_nvm_raw_exec(int argc, char **argv);
```

The RT-Thread `par_nvm_raw` command is only a shell wrapper around this function. A future host simulator may call the same function after binding test output with `par_test_set_vprint()` and providing a software NVM backend.

## Planned checklist

Record board-specific dump ranges, header-corruption steps, expected reboot behavior, and cleanup commands here.
