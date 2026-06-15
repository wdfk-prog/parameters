[English](./flash-ee-test-plan.md)

# Flash EE 真实 FAL 测试流程与用例说明

本文档说明当前维护的板级 Flash EE runtime suite。该 suite 只运行在真实 FAL 分区上，并会破坏性擦写该分区。

旧的 fake-flash `flash_ee` suite 已从 RT-Thread 包构建中移除。软件 flash 模拟和确定性掉电注入后续应作为独立 host/CI harness 实现。

## 关联实现

| 范围 | 文件 |
| --- | --- |
| 可移植 Flash EE core | `parameters/src/nvm/backend/par_store_backend_flash_ee.c` |
| FAL 适配 | `backend/par_store_backend_flash_ee_fal.c` |
| 真实硬件测试 suite | `parameters/tests/par_test_flash_ee_fal.c` |
| 测试 runner | `parameters/tests/par_test_runner.c`, `parameters/tests/par_test_msh.c` |

## 测试边界

`flash_ee_fal` 覆盖真实 FAL 后端路径：

- FAL 分区查找和 backend bind/init/deinit；
- 真实 flash 空分区首次格式化；
- 写入/读取在 backend reinit 后保持；
- 逻辑擦除在 backend reinit 后保持；
- 满 bank 回绕和 active-bank 恢复；
- 同一分区上多轮回绕；
- 物理 active bank 写满后重启式扫描，随后仍可继续写入。

该 suite 不模拟半写入或半擦除 flash cell。这类确定性 fault-injection 用例应放到后续 host/CI simulator，或通过手工 HIL 断电测试覆盖。

## 必需板级分区

预留一个专用 FAL 分区，例如：

```text
pm_ee_test
```

该分区不得与文件系统、崩溃日志或生产参数存储共用。

对于 8 MiB 外部 NOR flash，且最后 4 KiB 保留给 `cmb_log` 的布局，可在 `cmb_log` 前划出 128 KiB 测试分区：

```text
filesystem  offset=0x000000 size=0x7DF000
pm_ee_test  offset=0x7DF000 size=0x020000
cmb_log     offset=0x7FF000 size=0x001000
```

## 构建配置

测试固件启用：

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

如果 `pm_ee_test` 是 128 KiB，并拆成两个 bank，line size 为 32 byte，program unit 为 8 byte，建议使用：

```text
AUTOGEN_PM_FLASH_EE_LOGICAL_SIZE=0x8000
AUTOGEN_PM_FLASH_EE_CACHE_SIZE=4096
AUTOGEN_PM_FLASH_EE_LINE_SIZE=32
AUTOGEN_PM_FLASH_EE_PROGRAM_SIZE=8
```

不要启用已移除的 `AUTOGEN_PM_TEST_USING_FLASH_EE` 选项。RT-Thread 包构建不再编译 `par_test_flash_ee.c`。

## 运行流程

1. 编译并烧录测试固件。
2. 打开 RT-Thread MSH。
3. 确认 suite 已注册：

   ```text
   msh /> par_test list
   ```

   期望包含：

   ```text
   PAR_TEST_SUITE name=flash_ee_fal cases=6
   ```

4. 运行真实 flash suite：

   ```text
   msh /> par_test flash_ee_fal
   ```

5. 验收输出：

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

如果没有启用 `AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE`，runner 会跳过破坏性 case。这是保护机制，不是 Flash EE 失败。

## 用例清单

| Case | 主要动作 | 核心断言 |
| --- | --- | --- |
| `bind_init` | 擦除 `pm_ee_test`，绑定 FAL adapter，初始化并反初始化 backend | 能找到真实分区，完成格式化并打开。 |
| `write_read_persists_after_reinit` | 逻辑擦除，写入 64 byte，sync，deinit/init 后读回 | 数据在 backend 重启后保持。 |
| `erase_persists_after_reinit` | 写入 64 byte，逻辑擦除，sync，deinit/init 后读回 | 擦除范围在重启后读回 `0xFF`。 |
| `wrap_after_bank_full_preserves_latest` | 反复写一个热点 line 直到 active bank 回绕，同时保留另一个稳定 line，再写入回绕后的新 line | 热点 line 最新值、稳定 line 和回绕后新写入都能在重启后保持。 |
| `wrap_multiple_cycles_preserves_latest` | 连续执行 3 轮回绕，并在每轮后做重启式 reinit | 多次真实 flash bank switch 后最新 logical image 仍有效。 |
| `full_bank_then_reinit_remains_writable` | 刚好填满 active bank，deinit/init 后再次写入 | 满 bank 重启扫描不会卡死；下一次写入可持久化。 |

## 故障定位

| 现象 | 优先检查 |
| --- | --- |
| `par_test list` 没有 `flash_ee_fal` | `AUTOGEN_PM_USING_TESTS`、`AUTOGEN_PM_TEST_USING_FLASH_EE_FAL` 是否启用；`SConscript` 是否包含 `par_test_flash_ee_fal.c`。 |
| 全部 case 被跳过 | `AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE` 未启用。 |
| `partition_not_found=pm_ee_test` | `fal_cfg.h` 没有定义该分区、分区名不一致，或 FAL 分区表未启用。 |
| `partition_geometry_invalid` | 分区对当前 logical size、line size 和 record overhead 来说太小。 |
| 回绕用例失败 | 检查 bank size、program granularity、FAL erase/program 返回值、active-bank scan 和 checkpoint/bank-switch 逻辑。 |
| 链接时报 RAM 溢出 | 确认已移除 fake-flash suite；Flash EE 板级测试只应编译 `par_test_flash_ee_fal.c`。 |

## 后续手工 HIL 补充

真实掉电窗口仍需要手工或自动 HIL 支持。后续建议覆盖：

1. record payload/metadata/commit 编程中复位；
2. target bank header 激活中复位；
3. inactive bank 擦除中复位；
4. 多轮回绕过程中反复复位；
5. 通过真实 FAL 分区验证 `par_save` 和 `par_save_by_id` 的端到端参数持久化。
