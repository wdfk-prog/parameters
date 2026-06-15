[English](./nvm-fixed-slot-no-size-manual-test.md)

# Fixed-slot-no-size scalar NVM 手动测试

## 范围

本文档覆盖 `src/nvm/scalar/layout/par_nvm_layout_fixed_slot_no_size.c` 的破坏性板级验收。测试入口是 `parameters/tests/par_test_nvm_fixed_slot_no_size.c` 提供的独立 `par_nvm_fslot_no_size` MSH helper。

该 helper 用于验证 layout-aware 地址计算，并向当前 scalar NVM backend 注入受控损坏。它不是生产命令，只能在 NVM 数据可丢弃的测试固件中启用。

## Layout 模型

每条 scalar record 存储 ID、CRC 和固定 4 字节 payload slot；不序列化自然 size。

scalar NVM 公共 header size 为 `0x0C` 字节。helper 打印的所有 offset 都是参数 NVM backend 窗口内的 backend-relative offset，不是 EEPROM 物理绝对地址。配置了 `PAR_CFG_RTT_AT24_BASE_ADDR` 时：

```text
AT24 absolute address = PAR_CFG_RTT_AT24_BASE_ADDR + backend-relative offset
```

| 字段 | 公式 / 规则 |
| --- | --- |
| record id offset | `record_offset + 0` |
| record crc offset | `record_offset + 2` |
| record payload offset | `record_offset + 3` |
| record size | `2 + 1 + 4 = 7 bytes` |
| slot offset rule | `0x0C + slot * 7` |

## 构建配置

一次只启用一个 scalar layout 和对应 helper。典型 RT-Thread 测试固件配置如下：

```text
RT_USING_FINSH=y
AUTOGEN_PM_USING_TESTS=y
AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE=y
AUTOGEN_PM_USING_NVM=y
AUTOGEN_PM_NVM_SCALAR=y
AUTOGEN_PM_USING_RTT_AT24CXX_BACKEND=y
AUTOGEN_PM_USING_TABLE_ID_CHECK=y
AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE=y
AUTOGEN_PM_TEST_NVM_FIXED_SLOT_NO_SIZE=y
```

如果修改过参数 CSV/schema，必须先重新生成 `par_table.def` 和 generated layout 文件，再重新构建固件。helper 读取的是编译后的参数表，不会替你修改 CSV 中的 persistence 标志。

## 启动后前置检查

启动日志必须能看到 scalar NVM 路径，例如：

```text
PAR: restoring persistent values from NVM
PAR_NVM: initialization started
PAR_NVM: header validated
PAR_NVM: load-all finished with status=OK
PAR_NVM: initialization finished with status=OK
PAR: initialization finished with status=OK
```

如果日志出现 `no persistent parameters configured` 或 `scalar persisted-record layout not required`，说明当前固件没有进入 scalar persistence 测试路径，应先修正 Kconfig/schema 生成结果。

然后执行：

```sh
par info
par_nvm_fslot_no_size info
```

`info` 输出应检查：

1. `layout=fixed_slot_no_size`。
2. `backend=... status=OK(0x0000)`。
3. `persistent_count` 与启动日志中的 scalar persistent count 一致。
4. `resolved_slots` 必须等于 `persistent_count`。
5. 当至少存在一个 scalar persistent 参数时，`scalar_image_size` 必须大于 `0x0C`。

## 命令说明

```sh
par_nvm_fslot_no_size info
par_nvm_fslot_no_size slot <slot>
par_nvm_fslot_no_size corrupt_slot_crc <slot> [mask]
par_nvm_fslot_no_size corrupt_payload <slot> <payload_offset> [mask]

```

| 命令 | 作用 |
| --- | --- |
| `info` | 打印 layout 名称、backend 状态、header size、record offset 定义、persistent count、scalar image size，以及可用时的 AT24 窗口。 |
| `slot <slot>` | 将一个 scalar persistent slot 解析为 `par_num`、ID、名称、record offset、CRC offset、payload offset、record size 和 payload size。 |
| `corrupt_slot_crc <slot> [mask]` | 异或损坏一个 CRC 字节，默认 mask 为 `0x01`。执行后应立即重启。 |
| `corrupt_payload <slot> <payload_offset> [mask]` | 异或损坏某个 slot 内的一个 payload 字节，默认 mask 为 `0x01`。执行后应立即重启。 |

## 详细验收流程

### 1. slot 映射与地址检查

对 `0` 到 `persistent_count - 1` 的每个 slot 执行：

```sh
par_nvm_fslot_no_size slot <slot>
```

每条命令都必须成功。输出必须包含 `slot=`、`offset=`、`crc_offset=`、`payload_offset=`、`record_size=`、`payload_size=`、`par_num=`、`id=` 和 `name=`。

对你当前这类 `par info` 示例表，只要 `persistent_count > 0`，`slot 0` 就应当是合法 scalar persistent slot。如果出现 `slot out of range or scalar persistent slot map invalid`，说明 helper 无法映射当前编译出的 scalar persistent 表，后续损坏注入测试不能继续。

### 2. save/reboot 持久化基线

从 `par info` 中选择一个可写 scalar persistent 参数，写入一个非默认且在范围内的值，保存并重启：

```sh
par set <id> <non_default_value>
par save <id>
# reboot / power-cycle
par get <id>
```

重启后的值必须等于保存值。该步骤用于证明当前 layout 确实进入了正常 NVM save/load 路径，再开始破坏性测试。

### 3. CRC 损坏恢复

选择一个已确认有效的 slot 执行：

```sh
par_nvm_fslot_no_size corrupt_slot_crc <slot>
# 立即 reboot
```

重启后的预期行为：

1. 启动日志检测到 scalar CRC corruption；
2. 按当前恢复策略从默认值重建 scalar NVM；
3. 参数模块初始化结束时不应出现不可恢复的 persistent NVM fatal error；
4. 被损坏 slot 原先保存的值不再可信，除非 rebuild 后重新写入并保存。

### 4. Payload 损坏恢复

选择一个有效 slot，并选择小于该 slot 打印出来的 `payload_size` 的 `payload_offset`：

```sh
par_nvm_fslot_no_size corrupt_payload <slot> <payload_offset>
# 立即 reboot
```

预期行为与 CRC 损坏一致，因为 payload 被修改后，存储 CRC 不再匹配。

### 5. Layout 专有损坏项

该 layout 没有额外的专有损坏命令。

### 6. 清理

测试结束后擦除或重建可丢弃 NVM image，再将板卡用于普通测试。不要让板卡停留在部分损坏的 persistence 状态。

## 排错表

| 现象 | 可能原因 | 处理 |
| --- | --- | --- |
| MSH 补全中没有该命令 | helper Kconfig 未启用、`RT_USING_FINSH` 未启用，或 `SConscript` 没有纳入源文件。 | 重新检查 `AUTOGEN_PM_TEST_NVM_FIXED_SLOT_NO_SIZE`、`AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE`、`AUTOGEN_PM_USING_TESTS` 和 `RT_USING_FINSH`，并 clean rebuild。 |
| `info` 中 `persistent_count` 非 0，但 `scalar_image_size=0x0000000C` | helper 在累加 record 前未能完成 slot 映射。 | 使用本修正版重新构建，并确认 generated table 里存在 scalar persistent 行。 |
| `persistent_count > 0` 时 `slot 0` 失败 | helper 无法映射 live scalar persistent slots。 | 确认 helper 使用 `ePAR_NUM_OF` 遍历参数表，而不是使用字节数 `par_cfg_get_table_size()`；同时检查生成表一致性。 |
| `payload_offset out of range` | 传入 offset 没有小于该 slot 的自然 `payload_size`。 | 重新执行 `slot <slot>`，选择 `[0, payload_size)` 范围内的 offset。 |
| 重启没有报 corruption | 损坏了错误的 slot/offset，或损坏后先执行了正常保存导致 record 被重写。 | 从干净 image 重做，损坏命令执行后立即 reboot。 |

## 注意与限制

- 当 `persistent_count > 0` 时，`slot 0` 是合法 slot；如果 `slot 0` 失败，优先确认 helper 已使用本修正版重新构建，且生成表内确实存在 scalar persistent 行。
- 这是手动/HIL helper，不是完整掉电耐久测试。
- helper 只验证当前 AT24CXX scalar backend 窗口，不验证无关 flash geometry 或 object NVM 行为。
