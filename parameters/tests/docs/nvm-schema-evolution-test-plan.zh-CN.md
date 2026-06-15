[English](./nvm-schema-evolution-test-plan.md)

# NVM schema 演进测试计划

本文档用于索引“生成参数表变化后，旧持久化数据如何恢复或重建”的缺失测试。

当前已补充 fixture 表、host 侧 fixture 检查、板端 MSH 验收 helper 和执行验收文档；本文档继续作为覆盖矩阵和后续扩展索引。

## 范围

Schema 演进指：固件 V1 写入有效的 scalar 和/或 object NVM 镜像后，固件 V2 使用变更后的生成参数表启动，并复用同一份 NVM 内容。

计划覆盖：

- scalar 参数追加、删除、插入、ID 变化、类型变化、仅默认值变化；
- object 参数追加、删除、插入、ID 变化、类型变化、元素大小变化、容量或长度范围变化；
- scalar 与 object 同时变化的混合升级；
- 显式修改 `PAR_CFG_TABLE_ID_SCHEMA_VER`；
- scalar 增长可能改变 object block 地址的 object placement 模式。

## 关联实现

| 范围 | 实现文件 |
| --- | --- |
| 参数表兼容 ID | `src/nvm/par_nvm_table_id.c`, `src/nvm/par_nvm_table_id.h` |
| Scalar persistence core | `src/nvm/scalar/par_nvm_scalar.c`, `src/nvm/scalar/store/par_nvm_scalar_store.c` |
| Scalar layouts | `src/nvm/scalar/layout/par_nvm_layout_*.c` |
| Object persistence core | `src/nvm/object/par_nvm_object.c`, `src/nvm/object/par_nvm_object.h` |
| Object 地址模式 | `src/nvm/object/addr/par_nvm_object_addr_*.c` |
| Object store 模式 | `src/nvm/object/store/par_nvm_object_store_*.c` |
| 生成 schema 输入 | `schema/par_table.csv`, `schema/par_id_lock.json`, `tools/pargen.py` |

## 已实现 fixture 布局

版本演进测试不建议直接修改生产 `par_table.csv`。建议使用独立 fixture 表。

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

## 通用测试流程

1. 基于 V1 fixture 构建或生成固件产物。
2. 启动 V1，并向 NVM 写入非默认 scalar 和 object 值。
3. 保留原始 NVM 镜像。
4. 基于 V2 fixture 构建或生成固件产物。
5. 使用 V2 固件读取保留的 V1 NVM 镜像。
6. 验证旧值保留、新值默认、rewrite 请求和 rebuild 行为。
7. 验证日志能区分兼容恢复、受控重建和损坏处理路径。

## Scalar 演进矩阵

| 场景 | 需要锁定的期望行为 |
| --- | --- |
| 尾部追加 persistent scalar | 旧 scalar 值保留；新 scalar 使用默认值；必要时请求重写 NVM。 |
| 中间插入 persistent scalar | 除非当前 layout 明确支持 ID-based migration，否则应视为不兼容。 |
| 删除尾部 scalar | 需要明确是允许截断还是视为不兼容；默认期望是受控重建。 |
| 删除中间 scalar | 应视为不兼容；旧值不得错位绑定到其他参数。 |
| 修改 scalar ID | 应视为不兼容；旧值不得绑定到新 ID。 |
| 修改 scalar 类型或宽度 | 应视为不兼容；旧 payload 不得按新类型解释。 |
| 仅修改 scalar 默认值 | 已有有效 NVM 值优先；只有缺失或重建时才使用新默认值。 |
| 修改 `PAR_CFG_TABLE_ID_SCHEMA_VER` | 强制 table-ID mismatch，并从默认值受控重建。 |

## Object 演进矩阵

| 场景 | 需要锁定的期望行为 |
| --- | --- |
| 尾部追加 persistent object | 旧 object 值保留；新 object 使用默认值；必要时请求重写 object block。 |
| 中间插入 object | 除非 ID-based migration 明确证明安全，否则应视为不兼容。 |
| 删除 object | 需要明确是允许垃圾回收还是视为不兼容；默认期望是受控重建。 |
| 修改 object ID | 应视为不兼容；旧 payload 不得绑定到新 object。 |
| 修改 object 类型 | 应视为不兼容。 |
| 修改 object 元素大小 | 应视为不兼容。 |
| 缩小 object capacity 或 max length | 应视为不兼容或恢复默认；旧 payload 不得越界写入新 object。 |
| 增大 object capacity 或 max length | 如果 ID、类型、元素大小和已存长度仍有效，可以保留旧 payload。 |
| 新 min length 大于旧存储长度 | 应视为不兼容或恢复默认。 |

## Scalar/Object 混合演进矩阵

| 场景 | 需要锁定的期望行为 |
| --- | --- |
| 只追加 scalar，object 表不变 | Scalar 按追加策略处理；object 值不受影响。 |
| 只追加 object，scalar 表不变 | Object 按追加策略处理；scalar 值不受影响。 |
| scalar 与 object 同时追加 | 旧 scalar/object 值保留；新增 scalar/object 使用默认值。 |
| scalar 兼容变化 + object 不兼容变化 | 需要明确 scalar 是否保留、object 是否单独重建，还是两个域一起重建。 |
| scalar 不兼容变化 + object 兼容变化 | 需要明确 object 是否保留，尤其是 shared-storage 模式下 scalar block 重建时的影响。 |
| scalar 增长 + object-after-scalar placement | 验证 object block 地址移动被显式处理，不得从旧地址误读脏数据。 |
| scalar 增长 + fixed/dedicated object placement | object 地址应保持稳定，并且与 scalar layout 增长解耦。 |

## Layout 相关注意点

不同 scalar layout 的兼容策略未必一致，因此需要分别覆盖。

| Layout | 必测演进关注点 |
| --- | --- |
| `compact-payload` | prefix-compatible append、payload size 增长、table-ID 行为。 |
| `fixed-payload-only` | prefix-compatible append、固定宽度 payload 保留。 |
| `fixed-slot-no-size` | 无 per-slot size 字段时的 prefix-compatible append。 |
| `fixed-slot-with-size` | 有 per-slot size 字段时的 prefix-compatible append 和 size 校验。 |
| `grouped-payload-only` | group count 或 object count 不匹配时的保守 rebuild 路径。 |

## 已实现测试资产

| 资产 | 路径 | 说明 |
| --- | --- | --- |
| Fixture 表 | `../fixtures/schema_evolution/*/par_table.csv` | 覆盖 V1 base、scalar/object/mixed append、不兼容变化和 schema version bump。 |
| Host 检查 | `../test_schema_evolution_fixtures.py` | 验证 fixture 完整性、pargen 可生成、兼容追加保持 V1 persistent 前缀。 |
| 板端 helper | `../par_test_schema_evolution.c` | 导出 `par_nvm_schema prepare/dump/verify`。 |
| 执行验收文档 | `./nvm-schema-evolution-acceptance.zh-CN.md` | 描述 V1/V2 固件生成、烧录和命令验收流程。 |

## 验收 checklist

- [x] V1/V2 fixture 表存在并有文档说明。
- [x] 生成器可以为每个 fixture 生成独立产物，且不修改生产 schema。
- [x] 每个 scalar layout 的 scalar-only 追加和不兼容场景已有 fixture 与执行步骤。
- [x] shared、fixed、dedicated object placement 的 object-only 追加和不兼容场景已有执行步骤。
- [x] 覆盖 scalar/object 混合变化。
- [x] 覆盖 `PAR_CFG_TABLE_ID_SCHEMA_VER` 变化。
- [x] 测试输出能区分兼容恢复、受控重建和损坏处理。
- [x] V2 启动前后可通过 `par_nvm_schema dump` 辅助定位。
