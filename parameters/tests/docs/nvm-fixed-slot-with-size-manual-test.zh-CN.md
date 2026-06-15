[English](./nvm-fixed-slot-with-size-manual-test.md)

# Fixed-slot-with-size NVM 硬件手动测试说明

Object persistence 的硬件验收请使用 [Object NVM 硬件手动测试](./nvm-object-manual-test.zh-CN.md)。

本文档用于验收 `AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE` 在 RT-Thread AT24CXX 后端上的行为。测试重点是：

- scalar persistent 参数是否真正进入 NVM 初始化、加载和保存路径；
- fixed-slot-with-size 的 slot 地址计算是否符合设计；
- header 损坏、单 slot CRC 损坏后是否能在重启时被发现并恢复；
- `par set`、`par save`、`par def`、`par def_all` 和 reboot 后的语义是否清楚；
- 需要真实硬件、手动 reboot 或掉电的测试如何执行和判定。

本文档描述的是手动/HIL 验收流程，不适合作为默认自动单元测试开启。涉及 `par_nvm_raw poke`、`par_nvm_raw flip`、`par_nvm_raw corrupt_header`、`par_nvm_fslot corrupt_slot_crc` 的步骤会破坏参数 EEPROM 窗口，只能在可丢弃 NVM 数据的测试固件上执行。

## 复用接口

fixed-slot helper 动作层可不经过 MSH 直接复用：

```c
int par_test_nvm_fslot_exec(int argc, char **argv);
```

RT-Thread `par_nvm_fslot` 命令只是该函数的 shell wrapper。后续 host simulator 可在通过 `par_test_set_vprint()` 绑定输出并提供实现对应 scalar layout 的软件 NVM backend 后调用同一接口。

## 术语

| 名称 | 含义 |
| --- | --- |
| persistent 参数 | CSV `persistent` 字段为 `1`，且该参数类型被当前 Kconfig NVM 能力支持的参数。 |
| scalar persistent 参数 | `U8`、`U16`、`U32`、`I8`、`I16`、`I32`、`F32` 等标量类型的 persistent 参数。 |
| object persistent 参数 | `STR`、`BYTES`、`ARR_U8`、`ARR_U16`、`ARR_U32` 等对象类型的 persistent 参数。 |
| slot | scalar persistent 参数在 NVM 持久化表中的顺序下标，从 `0` 开始，不等于参数 ID。 |
| backend-relative offset | 相对参数 NVM 后端窗口的偏移，例如 `0x00`，不是 EEPROM 绝对地址。 |
| AT24 absolute offset | `AUTOGEN_PM_RTT_AT24_BASE_ADDR + backend-relative offset`。 |

fixed-slot-with-size scalar 记录布局：

```text
header size       = 0x0C
record size       = 8
slot offset       = 0x0C + slot * 8
slot CRC offset   = slot offset + 3
scalar image size = 0x0C + persistent_count * 8
```

示例：`slot 7` 的 CRC 偏移为 `0x0C + 7 * 8 + 3 = 0x47`。如果 `AUTOGEN_PM_RTT_AT24_BASE_ADDR = 0x2000`，则 EEPROM 绝对地址为 `0x2047`。

## 共同前置条件

### 必须重新生成参数表

CSV 是参数表的输入，Kconfig 不能在编译期把 CSV 中的 `persistent` 字段强制改成 `1`。修改 CSV 后必须重新生成 `par_table.def` 和相关生成文件，再重新构建固件。

典型流程：

```sh
python3 parameters/tools/pargen.py
scons --menuconfig
scons -j$(nproc)
```

如果工程使用外层脚本生成参数表，应按产品工程的生成入口执行。验收前必须检查 `par_table.def` diff，确认目标行的 `pers_` 参数确实是 `1`。

### 必须区分 scalar NVM 和 object NVM

本测试验证的是 `AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE`，它属于 scalar NVM record layout。只要测试 header、slot CRC、range、save/reboot，推荐只让 scalar 参数 `persistent=1`，让 object 参数 `persistent=0`。

如果把 `STR`、`BYTES`、`ARR_U8`、`ARR_U16`、`ARR_U32` 也设置为 `persistent=1`，但没有启用 object NVM，构建会失败，典型错误为：

```text
_static_assert_ePAR_TEST_STR_RW_obj_persistence_requires_PAR_CFG_NVM_OBJECT_EN
_static_assert_ePAR_TEST_BYTES_RW_obj_persistence_requires_PAR_CFG_NVM_OBJECT_EN
_static_assert_ePAR_TEST_ARR_U8_RW_obj_persistence_requires_PAR_CFG_NVM_OBJECT_EN
```

这是预期保护。解决方式二选一：

1. 本文档推荐：object 参数保持 `persistent=0`；
2. 如果专门测试 object NVM：额外启用 `AUTOGEN_PM_NVM_OBJECT`，并确认 object NVM block 地址和 scalar NVM block 不重叠。object NVM 不属于本文的 fixed-slot scalar CRC 测试范围。

### 必须确认启动进入 NVM 路径

烧录后启动日志必须出现类似：

```text
PAR: restoring persistent values from NVM
PAR_NVM: initialization started
PAR_NVM: header validated
PAR_NVM: load-all finished with status=OK
PAR: initialization finished with status=OK
```

如果出现：

```text
PAR_NVM: scalar storage backend not required
PAR_NVM: scalar persisted-record layout not required
PAR_NVM: no persistent parameters configured
PAR: initialization finished with status=NO PERSISTENT
```

说明当前编译出来的参数表没有 scalar persistent 参数。此时不能验收 header 损坏、slot CRC 损坏和 NVM 恢复路径，应先修正 CSV/生成文件/Kconfig。

## Kconfig 基础配置

除非单个用例另有说明，硬件手动测试固件应启用：

```text
RT_USING_FINSH=y
AUTOGEN_PM_USING_TESTS=y
AUTOGEN_PM_USING_NVM=y
AUTOGEN_PM_NVM_SCALAR=y
AUTOGEN_PM_ENABLE_ID=y
AUTOGEN_PM_USING_RTT_AT24CXX_BACKEND=y
AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE=y
AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE=y
AUTOGEN_PM_TEST_NVM_RAW_HELPER=y
AUTOGEN_PM_TEST_NVM_FIXED_SLOT_WITH_SIZE=y
AUTOGEN_PM_USING_MSH_TOOL=y
AUTOGEN_PM_MSH_CMD_INFO=y
AUTOGEN_PM_MSH_CMD_GET=y
AUTOGEN_PM_MSH_CMD_SET=y
AUTOGEN_PM_MSH_CMD_SAVE=y
```

AT24CXX 后端还需要按板级硬件配置：

```text
PKG_USING_AT24CXX=y
AUTOGEN_PM_RTT_AT24_I2C_BUS_NAME="hwi2c2"      # 按实际板卡修改
AUTOGEN_PM_RTT_AT24_ADDR_INPUT=1                # 按实际 A2/A1/A0 和驱动语义修改
AUTOGEN_PM_RTT_AT24_BASE_ADDR=0x2000            # 参数窗口起始地址，按验收规划修改
AUTOGEN_PM_RTT_AT24_SIZE=0x2000                 # 参数窗口大小，必须覆盖 scalar image
```


手动测试命令按职责拆分：

| 命令 | 范围 | 用途 |
| --- | --- | --- |
| `par_nvm_raw` | layout 无关的原始后端 helper | 按 backend-relative offset 执行 `info`、`lut`、`dump`、`poke`、`flip`、`corrupt_header`。 |
| `par_nvm_fslot` | fixed-slot-with-size 专用 layout helper | 按 fixed-slot 地址计算执行 `info`、`slot`、`corrupt_slot_crc`。 |

`par_nvm_raw` 可用于其他 scalar NVM layout 的原始 offset 级检查；`par_nvm_fslot` 只能在 `AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE=y` 时使用。

需要测试 `par def` 和 `par def_all` 时，额外启用：

```text
AUTOGEN_PM_MSH_CMD_DEF=y
AUTOGEN_PM_MSH_CMD_DEF_ALL=y
```

需要打印 `par_nvm_raw lut` 时，建议启用：

```text
AUTOGEN_PM_USING_DEBUG=y
```

## CSV Profile A：scalar persistent 测试表

除“负向构建测试”和“object NVM 专项测试”外，本文所有 fixed-slot-with-size 测试均使用本 CSV。关键点是：scalar 测试行 `persistent=1`，object 测试行 `persistent=0`。

将以下内容作为 `usersrc_parameter_general_par_table_pargen.csv` 或工程实际使用的参数 CSV 内容，然后重新运行生成器：

```csv
group,section,condition,enum,id,type,name,min,max,default,unit,access,read_roles,write_roles,persistent,desc,comment
TEST,runtime,,ePAR_TEST_U8_RW,AUTO,U8,Test U8 RW,10,20,12,cnt,RW,ALL,ALL,1,Runtime test U8 read-write scalar,
TEST,runtime,,ePAR_TEST_U8_RO,AUTO,U8,Test U8 RO,1,3,2,cnt,RO,ALL,NONE,1,Runtime test U8 read-only scalar,
TEST,runtime,,ePAR_TEST_U16_RW,AUTO,U16,Test U16 RW,100,200,120,ms,RW,ALL,ALL,1,Runtime test U16 scalar,
TEST,runtime,,ePAR_TEST_U32_RW,AUTO,U32,Test U32 RW,1000,2000,1200,Hz,RW,ALL,ALL,1,Runtime test U32 scalar,
TEST,runtime,,ePAR_TEST_I16_RW,AUTO,I16,Test I16 RW,-100,100,0,mV,RW,ALL,ALL,1,Runtime test I16 scalar,
TEST,runtime,,ePAR_TEST_I32_RW,AUTO,I32,Test I32 RW,-1000,1000,0,ppm,RW,ALL,ALL,1,Runtime test I32 scalar,
TEST,runtime,,ePAR_TEST_F32_RW,AUTO,F32,Test F32 RW,-10.0,10.0,1.25,V,RW,ALL,ALL,1,Runtime test F32 scalar,
TEST,runtime,,ePAR_TEST_STR_RW,AUTO,STR,Test STR RW,0,8,ap,,RW,ALL,ALL,0,Runtime test string object,
TEST,runtime,,ePAR_TEST_BYTES_RW,AUTO,BYTES,Test BYTES RW,4,4,0x00 0x11 0x22 0x33,,RW,ALL,ALL,0,Runtime test byte-array object,
TEST,runtime,,ePAR_TEST_ARR_U8_RW,AUTO,ARR_U8,Test ARR U8 RW,4,4,1 2 3 4,,RW,ALL,ALL,0,Runtime test uint8 array object,
TEST,runtime,,ePAR_TEST_ARR_U16_RW,AUTO,ARR_U16,Test ARR U16 RW,3,3,100 200 300,,RW,ALL,ALL,0,Runtime test uint16 array object,
TEST,runtime,,ePAR_TEST_ARR_U32_RW,AUTO,ARR_U32,Test ARR U32 RW,2,2,1000 2000,,RW,ALL,ALL,0,Runtime test uint32 array object,
TEST,runtime,defined(AUTOGEN_PM_TEST_USING_AT24CXX),ePAR_TEST_AT24_U16_RW,477,U16,Test AT24 U16 RW,10,100,30,nvm,RW,ALL,ALL,1,Runtime test AT24 persistent scalar,
```

如果 `AUTOGEN_PM_TEST_USING_AT24CXX=y`，本 profile 通常产生 8 个 scalar persistent slot：`slot 0..7`。如果该选项未启用，则 AT24 条件行不会编译进参数表，通常产生 7 个 scalar persistent slot：`slot 0..6`。

## CSV Profile B：object persistent 负向构建测试表

本 profile 用于证明“把 object 参数全部设为 `persistent=1` 但未启用 object NVM 时，构建必须失败”。该测试不需要烧录，不需要硬件。

```csv
group,section,condition,enum,id,type,name,min,max,default,unit,access,read_roles,write_roles,persistent,desc,comment
TEST,runtime,,ePAR_TEST_U8_RW,AUTO,U8,Test U8 RW,10,20,12,cnt,RW,ALL,ALL,1,Runtime test U8 read-write scalar,
TEST,runtime,,ePAR_TEST_U8_RO,AUTO,U8,Test U8 RO,1,3,2,cnt,RO,ALL,NONE,1,Runtime test U8 read-only scalar,
TEST,runtime,,ePAR_TEST_U16_RW,AUTO,U16,Test U16 RW,100,200,120,ms,RW,ALL,ALL,1,Runtime test U16 scalar,
TEST,runtime,,ePAR_TEST_U32_RW,AUTO,U32,Test U32 RW,1000,2000,1200,Hz,RW,ALL,ALL,1,Runtime test U32 scalar,
TEST,runtime,,ePAR_TEST_I16_RW,AUTO,I16,Test I16 RW,-100,100,0,mV,RW,ALL,ALL,1,Runtime test I16 scalar,
TEST,runtime,,ePAR_TEST_I32_RW,AUTO,I32,Test I32 RW,-1000,1000,0,ppm,RW,ALL,ALL,1,Runtime test I32 scalar,
TEST,runtime,,ePAR_TEST_F32_RW,AUTO,F32,Test F32 RW,-10.0,10.0,1.25,V,RW,ALL,ALL,1,Runtime test F32 scalar,
TEST,runtime,,ePAR_TEST_STR_RW,AUTO,STR,Test STR RW,0,8,ap,,RW,ALL,ALL,1,Runtime test string object,
TEST,runtime,,ePAR_TEST_BYTES_RW,AUTO,BYTES,Test BYTES RW,4,4,0x00 0x11 0x22 0x33,,RW,ALL,ALL,1,Runtime test byte-array object,
TEST,runtime,,ePAR_TEST_ARR_U8_RW,AUTO,ARR_U8,Test ARR U8 RW,4,4,1 2 3 4,,RW,ALL,ALL,1,Runtime test uint8 array object,
TEST,runtime,,ePAR_TEST_ARR_U16_RW,AUTO,ARR_U16,Test ARR U16 RW,3,3,100 200 300,,RW,ALL,ALL,1,Runtime test uint16 array object,
TEST,runtime,,ePAR_TEST_ARR_U32_RW,AUTO,ARR_U32,Test ARR U32 RW,2,2,1000 2000,,RW,ALL,ALL,1,Runtime test uint32 array object,
TEST,runtime,defined(AUTOGEN_PM_TEST_USING_AT24CXX),ePAR_TEST_AT24_U16_RW,477,U16,Test AT24 U16 RW,10,100,30,nvm,RW,ALL,ALL,1,Runtime test AT24 persistent scalar,
```

预期构建失败关键字：

```text
obj_persistence_requires_PAR_CFG_NVM_OBJECT_EN
```

如果启用了 `AUTOGEN_PM_NVM_OBJECT`，该负向构建测试不再适用。

## 测试用例矩阵

| 用例 | 目标 | CSV | 是否需要硬件 | 是否需要 reboot |
| --- | --- | --- | --- | --- |
| TC-00 | 构建期 object persistent 保护 | Profile B | 否 | 否 |
| TC-01 | 固件进入 scalar NVM 路径 | Profile A | 是 | 否 |
| TC-02 | fixed-slot 地址和 LUT 校验 | Profile A | 是 | 否 |
| TC-03 | header 损坏恢复 | Profile A | 是 | 是 |
| TC-04 | 单 slot CRC 损坏恢复 | Profile A | 是 | 是 |
| TC-05 | 范围限制值持久化 | Profile A | 是 | 是 |
| TC-06 | `par def` / `par def_all` 保存语义 | Profile A | 是 | 是 |
| TC-07 | 多次 reboot 稳定性 | Profile A | 是 | 是 |
| TC-08 | `par save` 后立即 reboot | Profile A | 是 | 是 |
| TC-09 | 保存中掉电风险 | Profile A | 是 | 断电上电 |

## TC-00：object persistent 保护

### Kconfig

启用 scalar NVM，但不要启用 object NVM：

```text
AUTOGEN_PM_USING_NVM=y
AUTOGEN_PM_NVM_SCALAR=y
AUTOGEN_PM_NVM_OBJECT=n
```

### CSV

使用 CSV Profile B。

### 执行

重新生成参数表并构建：

```sh
python3 parameters/tools/pargen.py
scons -j$(nproc)
```

### 预期

构建失败，日志包含：

```text
obj_persistence_requires_PAR_CFG_NVM_OBJECT_EN
```

### 判定

| 结果 | 结论 |
| --- | --- |
| 按预期失败 | object persistent 静态保护有效。 |
| 构建通过 | 需要检查 object NVM 是否被意外启用，或静态断言是否失效。 |

## TC-01：固件进入 scalar NVM 路径

### Kconfig

使用“基础配置”。不需要开启 `AUTOGEN_PM_NVM_OBJECT`。

### CSV

使用 CSV Profile A。

### 执行

烧录固件后启动，观察启动日志。然后在 MSH 执行：

```text
par_nvm_raw info
par_nvm_fslot info
```

### 预期

启动日志包含：

```text
PAR: restoring persistent values from NVM
PAR_NVM: initialization started
PAR_NVM: header validated
PAR_NVM: load-all finished with status=OK
PAR: initialization finished with status=OK
```

`par_nvm_fslot info` 包含：

```text
layout=fixed_slot_with_size
backend=rtt_at24cxx status=OK
header_size=0x0C
first_slot_offset=0x0C
record_size=8
record_crc_offset=3
persistent_count=8
scalar_image_size=0x0000004C
```

如果没有启用 `AUTOGEN_PM_TEST_USING_AT24CXX`，`persistent_count` 可以是 `7`，`scalar_image_size` 应为 `0x00000044`。

### 常见失败

| 日志 | 原因 | 处理 |
| --- | --- | --- |
| `no persistent parameters configured` | CSV 未设置 scalar `persistent=1`，或修改 CSV 后没有重新生成 `par_table.def`。 | 检查 CSV、生成产物和构建输入。 |
| `backend not required` | 没有任何需要 scalar NVM 的参数。 | 同上。 |
| `backend status=ERROR INIT` | AT24CXX 后端未初始化。 | 检查 I2C bus、AT24 地址和包配置。 |

## TC-02：fixed-slot 地址和 LUT 校验

### Kconfig

使用“基础配置”。建议开启：

```text
AUTOGEN_PM_USING_DEBUG=y
```

### CSV

使用 CSV Profile A。

### 执行

```text
par_nvm_fslot info
par_nvm_raw lut
par_nvm_fslot slot 0
par_nvm_fslot slot 7
par_nvm_raw dump 0x00 0x4C
```

如果 `persistent_count=7`，最后一个 slot 是 `slot 6`，应执行：

```text
par_nvm_fslot slot 6
par_nvm_raw dump 0x00 0x44
```

### 预期

`slot 0`：

```text
slot=0 offset=0x0000000C crc_offset=0x0000000F
```

`slot 7`：

```text
slot=7 offset=0x00000044 crc_offset=0x00000047
```

如果 `AUTOGEN_PM_RTT_AT24_BASE_ADDR=0x2000`，`slot 7` 的 `abs_crc_offset` 应为：

```text
abs_crc_offset=0x00002047
```

### 判定

| 结果 | 结论 |
| --- | --- |
| offset/CRC offset 符合公式 | fixed-slot 定位正确。 |
| slot 超范围 | 使用了不存在的 slot，先看 `persistent_count`。 |
| LUT 中参数 ID 和 slot 不符合预期 | 检查 CSV 行顺序、条件编译和生成产物。 |

## TC-03：header 损坏恢复

### Kconfig

使用“基础配置”。

### CSV

使用 CSV Profile A。

### 执行

先确认当前镜像正常：

```text
par_nvm_raw info
par_nvm_fslot info
```

再破坏 header 第 1 字节：

```text
par_nvm_raw corrupt_header
```

手动重启：

```text
reboot
```

再次重启确认 rebuild 后稳定：

```text
reboot
```

### 预期

第一次 reboot 后启动日志包含：

```text
PAR_NVM: header signature corrupted
PAR_NVM: scalar header/signature mismatch detected, rebuilding scalar NVM from defaults
PAR: initialization finished with status=OK
```

第二次 reboot 后启动日志包含：

```text
PAR_NVM: header validated
PAR_NVM: load-all finished with status=OK
PAR: initialization finished with status=OK
```

### 判定

| 结果 | 结论 |
| --- | --- |
| 第一次检测 header 损坏并 rebuild，第二次恢复 OK | header 损坏恢复路径有效。 |
| 仍显示 `NO PERSISTENT` | 当前表没有 scalar persistent 参数，不能执行该用例。 |
| 系统启动阻塞或崩溃 | 恢复路径有严重问题。 |

## TC-04：单 slot CRC 损坏恢复

### Kconfig

使用“基础配置”。

### CSV

使用 CSV Profile A。

### 执行

先选择一个有效 slot。建议优先选最后一个有效 slot，覆盖非首元素定位：

```text
par_nvm_fslot info
par_nvm_fslot slot 7
par_nvm_fslot corrupt_slot_crc 7
reboot
reboot
```

如果 `persistent_count=7`，使用 `slot 6`。不要固定使用 `slot 20`，除非 `persistent_count >= 21`。

### 预期

第一次 reboot 后启动日志包含：

```text
PAR_NVM: load-all finished with status=ERROR CRC
PAR_NVM: load error slot=7
PAR_NVM: scalar CRC corruption detected, rebuilding scalar NVM from defaults
PAR: initialization finished with status=OK
```

第二次 reboot 后启动日志包含：

```text
PAR_NVM: header validated
PAR_NVM: load-all finished with status=OK
```

### 判定

| 结果 | 结论 |
| --- | --- |
| 第一次检测 CRC 错误并 rebuild，第二次 OK | 单槽 CRC 检测和恢复路径有效。 |
| `ERR slot out of range` | 使用了不存在的 slot。先看 `persistent_count`。 |
| CRC 错误后没有 rebuild | 检查 NVM load-all 错误处理路径。 |

## TC-05：范围限制值持久化

### Kconfig

使用“基础配置”。

### CSV

使用 CSV Profile A。本文使用 `ePAR_TEST_U8_RW`，其范围为 `10..20`，默认值为 `12`。如果产品参数表使用 ID 26 或其他业务参数，应把对应 scalar 行设置为 `persistent=1`，并按实际 ID 执行。

### 执行

CSV Profile A 中 `ePAR_TEST_U8_RW` 的生成 ID 通常为 `0`。如果使用 ID lock 后 ID 不同，以 `par_nvm_raw lut` 或生成文件为准。

测试上限限制：

```text
par set 0 21
par get 0
par save
reboot
par get 0
```

测试下限限制：

```text
par set 0 9
par get 0
par save
reboot
par get 0
```

### 预期

上限限制：

```text
WAR,PAR_SET=20
WAR, value limited to configured range
par get 0 -> 20
reboot 后 par get 0 -> 20
```

下限限制：

```text
WAR,PAR_SET=10
WAR, value limited to configured range
par get 0 -> 10
reboot 后 par get 0 -> 10
```

### 判定

| 结果 | 结论 |
| --- | --- |
| reboot 后仍是限制后的值 | range 限制和 NVM 持久化正确。 |
| reboot 后变成非法原始值 | `set/save` 路径绕过 range。 |
| reboot 后变成默认值 | NVM load 未覆盖 RAM，或保存失败后被 rebuild。 |

## TC-06：`par def` / `par def_all` 保存语义

### Kconfig

除“基础配置”外，还需要：

```text
AUTOGEN_PM_MSH_CMD_DEF=y
AUTOGEN_PM_MSH_CMD_DEF_ALL=y
```

Kconfig 只能编译出命令，不能替代 `par save`。`par def` 和 `par def_all` 默认只恢复 RAM，必须执行 `par save` 才会覆盖 EEPROM。

### CSV

使用 CSV Profile A。

### 执行：单参数默认值

```text
par set 0 15
par save
reboot
par get 0

par def 0
par get 0
par save
reboot
par get 0
```

### 预期：单参数默认值

```text
set 后 reboot -> 15
def 后 get -> 12
def + save + reboot 后 get -> 12
```

### 执行：全部默认值

```text
par set 0 15
par set 2 150
par set 3 1500
par save
reboot

par def_all
par save
reboot

par get 0
par get 2
par get 3
```

### 预期：全部默认值

```text
par get 0 -> 12
par get 2 -> 120
par get 3 -> 1200
```

### 判定

| 结果 | 结论 |
| --- | --- |
| `def`/`def_all` 后 save，再 reboot 仍为默认值 | 默认值恢复和保存语义正确。 |
| `def` 后未 save，reboot 又回到旧值 | 正常，说明 `def` 只改 RAM。 |
| `def_all` 后未 save 却覆盖 EEPROM | 语义异常，应检查 shell 实现。 |

## TC-07：多次 reboot 稳定性

### Kconfig

使用“基础配置”。

### CSV

使用 CSV Profile A。

### 执行

循环执行 20 次：

```text
reboot
```

每次只观察启动日志。

### 预期

每次均出现：

```text
PAR_NVM: load-all finished with status=OK
PAR: initialization finished with status=OK
```

不应出现：

```text
ERROR CRC
header signature corrupted
table-id mismatch
scalar CRC corruption detected
```

### 判定

| 结果 | 结论 |
| --- | --- |
| 20 次均稳定 | EEPROM window 隔离、bootloader 影响和 fixed-slot 读写稳定。 |
| 偶发 CRC/header 错误 | 优先检查 EEPROM 写周期、bootloader 是否写同一地址、I2C 稳定性。 |

## TC-08：`par save` 后立即 reboot

### Kconfig

使用“基础配置”。

### CSV

使用 CSV Profile A。

### 执行

连续执行 10 轮，每轮改一个值、保存、立即重启：

```text
par set 2 121
par save
reboot
par get 2

par set 2 122
par save
reboot
par get 2
```

`ePAR_TEST_U16_RW` 范围为 `100..200`，默认值为 `120`。后续轮次可递增到 `130`。

### 预期

每次 reboot 后都能读到最后一次保存的值。

### 常见失败方向

| 现象 | 优先检查 |
| --- | --- |
| 偶发读到旧值 | AT24 写周期等待、`store->sync()`、`EE_TWR`。 |
| 偶发 CRC 错误 | `AUTOGEN_PM_NVM_WRITE_VERIFY`、`at24cxx_page_write()`、I2C ACK polling。 |
| 每次都回默认值 | `par save` 未执行成功，或启动 rebuild。 |

## TC-09：保存中掉电风险

### Kconfig

使用“基础配置”。建议启用写后校验：

```text
AUTOGEN_PM_NVM_WRITE_VERIFY=y
```

### CSV

使用 CSV Profile A。

### 执行

需要外部电源控制或 HIL 设备。执行：

```text
par set 2 150
par save
```

在 `par save` 开始后随机断电，再重新上电，观察启动日志和参数值。

### 预期和验收标准

当前 fixed-slot 单区布局没有双 header、版本号或 copy-on-write commit marker，不能承诺“保存中掉电仍保留上一个完整版本”。本用例的合格标准是：

| 场景 | 合格标准 |
| --- | --- |
| 正常保存后掉电 | reboot 后值正确。 |
| 保存中掉电 | 不死机；能发现 CRC/header 错误；能恢复默认或重建。 |
| 掉电后业务启动 | `PAR: initialization finished` 不阻塞系统启动。 |

如果产品要求“保存中掉电仍保留旧值”，当前 fixed-slot 单区布局不足，需要双区、版本号或 copy-on-write commit 设计。

## 业务参数表迁移说明

如果不是使用本文测试 CSV，而是使用产品参数，例如 `par set 26 32`、`par get 218`，则按以下规则迁移：

1. 只把需要验证 NVM 的 scalar 参数设置为 `persistent=1`；
2. 不要把 `STR`、`BYTES`、`ARR_*` 设置为 `persistent=1`，除非已经启用并规划 object NVM；
3. 修改 CSV 后必须重新生成参数表；
4. 用 `par_nvm_raw lut` 确认参数 ID 对应哪个 slot；
5. 损坏 slot CRC 时使用 slot index，不使用参数 ID。

示例：如果 `par_nvm_raw lut` 显示参数 ID 26 对应 `slot 3`，则 CRC 损坏命令应为：

```text
par_nvm_fslot corrupt_slot_crc 3
```

不是：

```text
par_nvm_fslot corrupt_slot_crc 26
```

## 问题定位速查

| 现象 | 直接原因 | 处理 |
| --- | --- | --- |
| `persistent_count=0` | 当前编译表没有 scalar persistent 参数。 | 检查 CSV、`par_table.def`、条件编译和生成流程。 |
| `ERR slot out of range` | slot index 超过 `persistent_count - 1`。 | 先执行 `par_nvm_fslot info`，选择有效 slot。 |
| object 参数 `persistent=1` 后构建失败 | 未启用 `AUTOGEN_PM_NVM_OBJECT`。 | object persistent 设回 `0`，或启用并规划 object NVM。 |
| corrupt 后没有恢复日志 | 启动未进入 NVM 路径，或 corrupt 写入未生效。 | 检查启动日志、readback、AT24 后端状态。 |
| `I2C[hwi2c2] Write error(2)` | I2C 写周期、地址、ACK polling 或 page write 问题。 | 查 AT24 地址、`EE_TWR`、`sync()`、`AUTOGEN_PM_NVM_WRITE_VERIFY`。 |
| reboot 后回默认值 | NVM 被 rebuild，或保存未成功。 | 查启动日志中的 CRC/header/table-id 错误。 |
