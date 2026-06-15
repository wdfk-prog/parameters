[中文](./nvm-layout-test-backlog.zh-CN.md)

# NVM layout test backlog

This document keeps an explicit index of scalar NVM layouts and backend adapters that do not yet have dedicated manual or automated tests.


## Covered layout-specific manual tests

| Scope | Helper | Manual document |
| --- | --- | --- |
| Compact-payload scalar layout | `par_nvm_compact_payload` | [Compact-payload NVM manual test](./nvm-compact-payload-manual-test.md) |
| Fixed-payload-only scalar layout | `par_nvm_fixed_payload_only` | [Fixed-payload-only NVM manual test](./nvm-fixed-payload-only-manual-test.md) |
| Fixed-slot-no-size scalar layout | `par_nvm_fslot_no_size` | [Fixed-slot-no-size NVM manual test](./nvm-fixed-slot-no-size-manual-test.md) |
| Grouped-payload-only scalar layout | `par_nvm_grouped_payload_only` | [Grouped-payload-only NVM manual test](./nvm-grouped-payload-only-manual-test.md) |

## Backlog

| Area | Related file | Missing test type |
| --- | --- | --- |
| GEL/NVM backend adapter | `src/nvm/backend/par_store_backend_gel_nvm.c` | Backend open/read/write/sync/error-path tests. |

## Use before adding tests

When a new helper or automated test is added, move its item from this backlog into `../README.md` as an existing test entry and create a focused document under this directory.
