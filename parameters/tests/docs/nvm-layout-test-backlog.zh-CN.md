[English](./nvm-layout-test-backlog.md)

# NVM layout 测试待办

本文档显式索引尚无专项手动或自动化测试的 scalar NVM layout 和 backend adapter。


## 已覆盖的 layout 专项手动测试

| 范围 | Helper | 手动文档 |
| --- | --- | --- |
| Compact-payload scalar layout | `par_nvm_compact_payload` | [Compact-payload NVM 手动测试](./nvm-compact-payload-manual-test.zh-CN.md) |
| Fixed-payload-only scalar layout | `par_nvm_fixed_payload_only` | [Fixed-payload-only NVM 手动测试](./nvm-fixed-payload-only-manual-test.zh-CN.md) |
| Fixed-slot-no-size scalar layout | `par_nvm_fslot_no_size` | [Fixed-slot-no-size NVM 手动测试](./nvm-fixed-slot-no-size-manual-test.zh-CN.md) |
| Grouped-payload-only scalar layout | `par_nvm_grouped_payload_only` | [Grouped-payload-only NVM 手动测试](./nvm-grouped-payload-only-manual-test.zh-CN.md) |

## 待办项

| 范围 | 关联文件 | 缺失测试类型 |
| --- | --- | --- |
| GEL/NVM backend adapter | `src/nvm/backend/par_store_backend_gel_nvm.c` | 后端 open/read/write/sync/error-path 测试。 |

## 新增测试前的处理规则

新增 helper 或自动化测试后，将对应条目从本 backlog 移到 `../README.zh-CN.md` 的已有测试索引，并在本目录下创建独立测试文档。
