[English](./README.md)

# Parameters 测试总览

本目录是可移植 `Device Parameters` 模块所有运行时测试代码和测试文档的统一入口。

## 测试入口

| 测试范围 | 入口 | 构建或运行选项 | 说明 |
| --- | --- | --- | --- |
| 运行时测试框架 | `par_test [all|list|ram|at24|flash_ee_fal]` 或 `par_test_run_by_name()` | `AUTOGEN_PM_USING_TESTS` | core runner 可复用；MSH 只是 RT-Thread 包装层。 |
| RAM 运行时覆盖 | `par_test ram` | `AUTOGEN_PM_TEST_USING_RAM_CONFIG` | 验证默认值恢复、scalar API、范围限制、metadata、ID、访问策略、callback、raw reset、F32 和 object RAM 路径。 |
| AT24CXX 持久化覆盖 | `par_test at24` | `AUTOGEN_PM_TEST_USING_AT24CXX` | 通过公共 API 验证 AT24CXX 后端上的 scalar persistence 路径。 |
| Flash EE 真实 FAL 覆盖 | `par_test flash_ee_fal` | `AUTOGEN_PM_TEST_USING_FLASH_EE_FAL`, `AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE` | 在专用真实 FAL 分区上破坏性验证 Flash EE；RT-Thread 包构建不再编译 fake flash suite。 |
| Raw NVM helper | `par_nvm_raw ...` 或 `par_test_nvm_raw_exec()` | `AUTOGEN_PM_TEST_NVM_RAW_HELPER` | 与布局无关的 dump、poke、flip、scalar header 损坏 helper。 |
| Fixed-slot-with-size helper | `par_nvm_fslot ...` 或 `par_test_nvm_fslot_exec()` | `AUTOGEN_PM_TEST_NVM_FIXED_SLOT_WITH_SIZE` | 面向 scalar fixed-slot-with-size 布局的 CRC/损坏验收 helper。 |
| Fixed-slot-no-size helper | `par_nvm_fslot_no_size ...` 或 `par_test_nvm_fslot_no_size_exec()` | `AUTOGEN_PM_TEST_NVM_FIXED_SLOT_NO_SIZE` | 面向 scalar fixed-slot-no-size 布局的地址检查、CRC 和 payload 损坏 helper。 |
| Compact-payload helper | `par_nvm_compact_payload ...` 或 `par_test_nvm_compact_payload_exec()` | `AUTOGEN_PM_TEST_NVM_COMPACT_PAYLOAD` | 面向 scalar compact-payload 布局的地址、CRC、size 和 payload 损坏 helper。 |
| Fixed-payload-only helper | `par_nvm_fixed_payload_only ...` 或 `par_test_nvm_fixed_payload_only_exec()` | `AUTOGEN_PM_TEST_NVM_FIXED_PAYLOAD_ONLY` | 面向 payload-only scalar 布局的地址、CRC 和 payload 损坏 helper。 |
| Grouped-payload-only helper | `par_nvm_grouped_payload_only ...` 或 `par_test_nvm_grouped_payload_only_exec()` | `AUTOGEN_PM_TEST_NVM_GROUPED_PAYLOAD_ONLY` | 面向 grouped payload-band scalar 布局的地址、CRC 和 payload 损坏 helper。 |
| Object NVM helper | `par_nvm_obj ...` 或 `par_test_nvm_obj_exec()` | `AUTOGEN_PM_TEST_NVM_OBJECT_HELPER` | object block 检查、payload 写入和损坏注入 helper。 |
| NVM schema-evolution 验收 helper | `par_nvm_schema ...` | `AUTOGEN_PM_TEST_NVM_SCHEMA_EVOLUTION` | 执行 V1/V2 fixture prepare、dump、append 和 rebuild 验收步骤。 |
| Generator 单元测试 | `python3 -m unittest parameters/tests/test_pargen.py` | Host Python | 在固件外验证 `tools/pargen.py` 行为。 |

## Flash EE 只保留真实硬件测试

RT-Thread 包构建现在只保留真实硬件 Flash EE suite：

```text
par_test flash_ee_fal
```

旧的 fake-flash `par_test flash_ee` suite 和 `AUTOGEN_PM_TEST_USING_FLASH_EE` 构建选项已从本包集成中移除。软件 flash 模拟后续应作为独立 host/CI harness 增加，不应进入 MCU 固件。

## 现有测试代码索引

| 测试项 | 测试代码 | 文档 | 状态 |
| --- | --- | --- | --- |
| 运行时测试框架 | `par_test.h`, `par_test_common.c`, `par_test_runner.c`, `par_test_msh.c` | [Runtime tests](./docs/runtime-tests.md) | core runner 和 MSH wrapper 已拆分。 |
| RAM 运行时测试 | `par_test_ram_config.c` | [Runtime tests](./docs/runtime-tests.md) | 测试代码已存在；详细 case 列表已索引。 |
| AT24CXX 运行时测试 | `par_test_at24cxx.c` | [Runtime tests](./docs/runtime-tests.md) | 测试代码已存在；板级注意事项已索引。 |
| Flash EE 真实 FAL 运行时测试 | `par_test_flash_ee_fal.c` | [Flash EE 真实 FAL 测试流程](./docs/flash-ee-test-plan.zh-CN.md) | 真实 FAL 分区 suite 已存在；覆盖 bind/init、持久化、逻辑擦除、满 bank 回绕、多轮回绕和 full-bank 后继续写入。 |
| Raw NVM 手工 helper | `par_test_nvm_raw.c` | [Raw NVM manual test](./docs/nvm-raw-manual-test.md) | 测试代码已存在；手工文档是检查清单。 |
| Fixed-slot-with-size scalar NVM | `par_test_nvm_fixed_slot_with_size.c` | [Fixed-slot-with-size NVM manual test](./docs/nvm-fixed-slot-with-size-manual-test.md) | 测试代码和手工文档已存在。 |
| Fixed-slot-no-size scalar NVM | `par_test_nvm_fixed_slot_no_size.c` | [Fixed-slot-no-size NVM manual test](./docs/nvm-fixed-slot-no-size-manual-test.md) | 专用 helper 和详细手工文档已存在。 |
| Compact-payload scalar NVM | `par_test_nvm_compact_payload.c` | [Compact-payload NVM manual test](./docs/nvm-compact-payload-manual-test.md) | 专用 helper 和详细手工文档已存在。 |
| Fixed-payload-only scalar NVM | `par_test_nvm_fixed_payload_only.c` | [Fixed-payload-only NVM manual test](./docs/nvm-fixed-payload-only-manual-test.md) | 专用 helper 和详细手工文档已存在。 |
| Grouped-payload-only scalar NVM | `par_test_nvm_grouped_payload_only.c` | [Grouped-payload-only NVM manual test](./docs/nvm-grouped-payload-only-manual-test.md) | 专用 helper 和详细手工文档已存在。 |
| Object NVM | `par_test_nvm_object.c` | [Object NVM manual test](./docs/nvm-object-manual-test.md) | 测试代码和文档已存在。 |
| pargen generator | `test_pargen.py` | [pargen tests](./docs/pargen-tests.md) | 测试代码已存在；host 侧文档是检查清单。 |
| NVM schema/data evolution | `par_test_schema_evolution.c`, `schema_evolution/par_schema_evolution_core.[ch]`, `test_schema_evolution_fixtures.py`, `fixtures/schema_evolution/*/par_table.csv` | [NVM schema evolution test plan](./docs/nvm-schema-evolution-test-plan.md), [execution and acceptance](./docs/nvm-schema-evolution-acceptance.md) | fixture、可复用 core interface、host fixture 检查和板级验收 wrapper 已实现。 |

## 真实 FAL 分区要求

测试固件需要启用：

```text
AUTOGEN_PM_USING_TESTS=y
AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE=y
AUTOGEN_PM_TEST_USING_FLASH_EE_FAL=y
AUTOGEN_PM_USING_FLASH_EE_BACKEND=y
AUTOGEN_PM_FLASH_EE_PORT_FAL=y
AUTOGEN_PM_FLASH_EE_FAL_PARTITION_NAME="pm_ee_test"
```

`flash_ee_fal` suite 会擦写 `pm_ee_test`。不要把该分区与文件系统、日志或生产参数存储混用。

如果 `pm_ee_test` 分区为 128 KiB，建议 `AUTOGEN_PM_FLASH_EE_LOGICAL_SIZE` 不超过 `0x8000`，保证每个 bank 能容纳一份 compacted logical image 和回绕记录。

## 文档归属规则

测试相关文档保持在本目录下。详细测试计划放在 `docs/`，本 README 只作为总览和索引。
