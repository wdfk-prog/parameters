[English](./nvm-object-manual-test.md)

# Object NVM 硬件手动测试

本文档说明 `AUTOGEN_PM_NVM_OBJECT` 的硬件手动验收流程。它和 scalar fixed-slot 文档分开维护，因为 object persistence 使用独立 object block，包含独立 header、变长容量 object record、payload 区和 record CRC 字段。

本文档中的测试用例具有破坏性，只能在 NVM 数据可丢弃的测试固件和测试硬件上执行。

## 覆盖范围

本文档验证：

- object persistent CSV 配置；
- object persistence 所需 Kconfig；
- object block base、size、slot、record CRC 和 payload offset；
- 通过公共 API 保存 object payload；
- object header 损坏恢复；
- object record CRC 和 payload 损坏恢复；
- object NVM rebuild 后的 reboot 稳定性。

它不验证 scalar fixed-slot record layout。scalar fixed-slot CRC 验收请使用 [Fixed-slot-with-size NVM 硬件手动测试](./nvm-fixed-slot-with-size-manual-test.zh-CN.md)。

## 复用接口

object helper 动作层可不经过 MSH 直接复用：

```c
int par_test_nvm_obj_exec(int argc, char **argv);
```

RT-Thread `par_nvm_obj` 命令只是该函数的 shell wrapper。后续 host simulator 可在通过 `par_test_set_vprint()` 绑定输出并提供软件 object NVM backend 后调用同一接口。

## Object NVM 布局术语

| 术语 | 含义 |
| --- | --- |
| Scalar block | scalar NVM header 和 scalar persistent records。 |
| Object block | `AUTOGEN_PM_NVM_OBJECT` 管理的独立 object persistence block。 |
| Object-block-relative offset | 从 object block base 开始计算的偏移；`par_nvm_obj dump/poke/flip` 使用该偏移。 |
| Backend-relative offset | active backend window 内的偏移；`backend offset = object_block_base + object-block-relative offset`。 |
| AT24 absolute offset | `AUTOGEN_PM_RTT_AT24_BASE_ADDR + backend-relative offset`。 |

当前 object 序列化布局：

```text
object header size       = 0x12
object record head size  = 0x0C
record CRC offset        = record_offset + 10
record payload offset    = record_offset + 12
record size              = 0x0C + object 配置容量字节数
object block size        = 0x12 + sum(record sizes)
```

Object block base 由 Kconfig 决定：

| 模式 | Base 地址规则 |
| --- | --- |
| `AUTOGEN_PM_NVM_OBJECT_STORE_SHARED` + `AUTOGEN_PM_NVM_OBJECT_ADDR_AFTER_SCALAR` | 紧跟当前编译出的 scalar NVM block。紧凑，但 scalar layout 增长或变化时 object base 会移动。 |
| `AUTOGEN_PM_NVM_OBJECT_STORE_SHARED` + `AUTOGEN_PM_NVM_OBJECT_ADDR_FIXED` | 使用同一 backend window 内的 `AUTOGEN_PM_NVM_OBJECT_FIXED_ADDR`。发布前验收更推荐。 |
| `AUTOGEN_PM_NVM_OBJECT_STORE_DEDICATED` | 使用 dedicated object backend 内的 `AUTOGEN_PM_NVM_OBJECT_DEDICATED_BASE_ADDR`。 |

## 手动 helper 命令

启用 `AUTOGEN_PM_TEST_NVM_OBJECT_HELPER` 后会编译：

```text
par_nvm_obj info
par_nvm_obj list
par_nvm_obj slot <slot>
par_nvm_obj dump <block_offset> <len>
par_nvm_obj poke <block_offset> <value>
par_nvm_obj flip <block_offset> <mask>
par_nvm_obj corrupt_header
par_nvm_obj corrupt_record_crc <slot>
par_nvm_obj corrupt_payload <slot> <payload_offset> [mask]
par_nvm_obj set_ascii <id> <text>
par_nvm_obj set_hex <id> <byte0> [byte1 ...]
```

`dump`、`poke`、`flip`、`corrupt_header`、`corrupt_record_crc`、`corrupt_payload` 都是破坏性命令。执行损坏命令后，应立即 reboot 并观察启动日志。除非用例明确要求，否则不要在损坏后先执行 `par_nvm_obj info`、`par_nvm_obj list` 或 `par get`，否则可能提前初始化或访问参数模块，掩盖本来要观察的启动恢复路径。

## 通用 Kconfig

除非用例另有说明，测试固件启用：

```text
RT_USING_FINSH=y
AUTOGEN_PM_USING_TESTS=y
AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE=y
AUTOGEN_PM_TEST_NVM_OBJECT_HELPER=y

AUTOGEN_PM_USING_NVM=y
AUTOGEN_PM_NVM_SCALAR=y
AUTOGEN_PM_NVM_OBJECT=y
AUTOGEN_PM_ENABLE_ID=y
AUTOGEN_PM_ENABLE_TYPE_OBJECT=y
AUTOGEN_PM_ENABLE_TYPE_STR=y
AUTOGEN_PM_ENABLE_TYPE_BYTES=y
AUTOGEN_PM_ENABLE_TYPE_ARR_U8=y
AUTOGEN_PM_ENABLE_TYPE_ARR_U16=y
AUTOGEN_PM_ENABLE_TYPE_ARR_U32=y

AUTOGEN_PM_USING_RTT_AT24CXX_BACKEND=y
AUTOGEN_PM_NVM_OBJECT_STORE_SHARED=y
```

为了让 object NVM 地址稳定，推荐使用 fixed object address：

```text
AUTOGEN_PM_NVM_OBJECT_ADDR_FIXED=y
AUTOGEN_PM_NVM_OBJECT_FIXED_ADDR=0x100
AUTOGEN_PM_NVM_OBJECT_REGION_SIZE=0x200
```

fixed address 不能和 scalar NVM block 重叠。如果 scalar block 增长超过 `0x100`，需要调大 `AUTOGEN_PM_NVM_OBJECT_FIXED_ADDR`。

开发阶段也可以使用默认 after-scalar 模式：

```text
AUTOGEN_PM_NVM_OBJECT_ADDR_AFTER_SCALAR=y
```

如果需要 `par get <id>` 显示 object payload，还要启用：

```text
AUTOGEN_PM_USING_MSH_TOOL=y
AUTOGEN_PM_MSH_CMD_GET=y
AUTOGEN_PM_MSH_CMD_GET_OBJECT=y
RT_USING_HEAP=y
```

如果需要测试默认值恢复：

```text
AUTOGEN_PM_MSH_CMD_DEF=y
AUTOGEN_PM_MSH_CMD_DEF_ALL=y
AUTOGEN_PM_MSH_CMD_SAVE=y
```

AT24CXX backend 还需要板级配置：

```text
PKG_USING_AT24CXX=y
AUTOGEN_PM_RTT_AT24_I2C_BUS_NAME="hwi2c2"      # 按板子修改
AUTOGEN_PM_RTT_AT24_ADDR_INPUT=1                # 按 A2/A1/A0 和驱动语义修改
AUTOGEN_PM_RTT_AT24_BASE_ADDR=0x2000            # 参数 backend window base
AUTOGEN_PM_RTT_AT24_SIZE=0x2000                 # 参数 backend window size
```

## CSV Profile O：scalar + object persistent 测试表

Kconfig 无法强制 CSV 的 `persistent` 字段变为 `1`。必须修改 CSV，重新生成 `par_table.def` 和 generated layout 文件，然后重新构建。

Object NVM 硬件测试使用此 profile：

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

使用上面的 object rows 时，预期 object slot 顺序为：

| Object slot | ID | Type | Capacity bytes | Record offset | CRC offset | Payload offset |
| ---: | ---: | --- | ---: | ---: | ---: | ---: |
| 0 | 7 | STR | 8 | `0x12` | `0x1C` | `0x1E` |
| 1 | 8 | BYTES | 4 | `0x26` | `0x30` | `0x32` |
| 2 | 9 | ARR_U8 | 4 | `0x36` | `0x40` | `0x42` |
| 3 | 10 | ARR_U16 | 6 | `0x46` | `0x50` | `0x52` |
| 4 | 12 | ARR_U32 | 8 | `0x58` | `0x62` | `0x64` |

预期 object block size 为 `0x6C` 字节。

## 测试用例矩阵

| 用例 | 目标 | CSV | 需要硬件 | 需要 reboot |
| --- | --- | --- | --- | --- |
| TC-O00 | object persistent 编译期保护 | object rows `persistent=1`，object NVM 关闭 | 否 | 否 |
| TC-O01 | 固件进入 object NVM 路径 | Profile O | 是 | 空白介质通常需要两次启动确认 |
| TC-O02 | object block 地址和 slot map | Profile O | 是 | 否 |
| TC-O03 | object payload 持久化 | Profile O | 是 | 是 |
| TC-O04 | object header 损坏恢复 | Profile O | 是 | 是 |
| TC-O05 | object record CRC 损坏恢复 | Profile O | 是 | 是 |
| TC-O06 | object payload 损坏恢复 | Profile O | 是 | 是 |
| TC-O07 | 默认值恢复与保存语义 | Profile O | 是 | 是 |
| TC-O08 | 多次 reboot 稳定性 | Profile O | 是 | 是 |
| TC-O09 | object save 期间掉电风险 | Profile O | 是 | 掉电/上电 |

## TC-O00：object persistent 编译期保护

### Kconfig

使用 Profile O，但关闭 object NVM：

```text
AUTOGEN_PM_USING_NVM=y
AUTOGEN_PM_NVM_SCALAR=y
AUTOGEN_PM_NVM_OBJECT=n
```

### 预期结果

构建必须失败，关键字类似：

```text
obj_persistence_requires_PAR_CFG_NVM_OBJECT_EN
```

这证明 object row 不能在未启用 object NVM 时被标记为 persistent。

## TC-O01：object NVM 启动路径

### 执行

烧录使用 Profile O 和通用 object Kconfig 的固件，然后 reboot。

### 预期结果

启动应进入 scalar 和 object NVM 两条路径。Object block 已初始化后的稳定日志通常类似：

```text
PAR_NVM: header validated
PAR_NVM: load-all finished with status=OK
PAR_NVM: object initialization finished with status=OK
PAR_NVM: initialization finished with status=OK
PAR: initialization finished with status=OK
```

如果介质为空或已有脏数据，第一次启动可能报告 object header 或 record 校验失败，然后重写 object block。再 reboot 一次，第二次启动必须达到 `object initialization finished with status=OK`。

## TC-O02：object block 地址和 slot map

### 执行

```text
par_nvm_obj info
par_nvm_obj list
par_nvm_obj slot 0
par_nvm_obj slot 4
par_nvm_obj dump 0x00 0x6C
```

### 预期结果

`info` 应显示：

```text
object_nvm=1
object_count=5
object_block_base=...
object_block_size=0x0000006C
header_size=0x12
record_head_size=0x0C
record_crc_offset=10
```

`slot 0` 应显示 `record_offset=0x00000012`、`crc_offset=0x0000001C`、`payload_offset=0x0000001E`。

`slot 4` 应显示 `record_offset=0x00000058`、`crc_offset=0x00000062`、`payload_offset=0x00000064`。

## TC-O03：object payload 持久化

### 执行 STR 持久化

```text
par_nvm_obj set_ascii 7 hello
par get 7
reboot
par get 7
```

reboot 后预期值：`hello`。

### 执行 BYTES 持久化

```text
par_nvm_obj set_hex 8 0xAA 0xBB 0xCC 0xDD
par get 8
reboot
par get 8
```

reboot 后预期值：`AA BB CC DD`，或等价 object 显示格式。

### 注意

`set_hex` 通过 `par_set_obj_n_save_by_id()` 写入原始 payload bytes。对 `ARR_U16` 和 `ARR_U32`，字节序是目标平台 native endian。手动 shell 验收建议优先使用 STR 和 BYTES，除非产品另有 typed object setter 命令。

## TC-O04：object header 损坏恢复

### 执行

```text
par_nvm_obj corrupt_header
reboot
```

### 第一次 reboot 预期日志

```text
PAR_NVM: object header signature corrupted
PAR_NVM: object store-all finished, count=5
PAR_NVM: object initialization finished with status=...
PAR: initialization finished with status=OK
```

然后再次 reboot：

```text
reboot
```

预期稳定日志：

```text
PAR_NVM: object initialization finished with status=OK
PAR_NVM: initialization finished with status=OK
PAR: initialization finished with status=OK
```

第二次 reboot 不应再出现 `object header signature corrupted`。

## TC-O05：object record CRC 损坏恢复

### 执行

```text
par_nvm_obj corrupt_record_crc 0
reboot
```

### 第一次 reboot 预期日志

```text
PAR_NVM: object record CRC corrupted, par_num=...
PAR_NVM: object store-all finished, count=5
PAR_NVM: object initialization finished with status=...
PAR: initialization finished with status=OK
```

然后再次 reboot。第二次 reboot 必须达到 `object initialization finished with status=OK`，且不再报告 object CRC corruption。

## TC-O06：object payload 损坏恢复

### 执行

```text
par_nvm_obj corrupt_payload 0 0 0x01
reboot
```

### 第一次 reboot 预期日志

Payload 损坏应通过 record CRC 被检测：

```text
PAR_NVM: object record CRC corrupted, par_num=...
PAR_NVM: object store-all finished, count=5
PAR: initialization finished with status=OK
```

然后再次 reboot。第二次 reboot 必须干净启动。

## TC-O07：默认值恢复与保存语义

### 执行

```text
par_nvm_obj set_ascii 7 hello
par get 7
reboot
par get 7

par def 7
par get 7
par save
reboot
par get 7
```

预期：

| 步骤 | 预期值 |
| --- | --- |
| `set_ascii` 后 reboot | `hello` |
| `par def 7` 后 | 默认字符串，Profile O 中为 `ap` |
| `par save` 后 reboot | 默认字符串，Profile O 中为 `ap` |

`par def` 只改 RAM；必须执行 `par save` 才会覆盖 NVM。

## TC-O08：多次 reboot 稳定性

执行 20 次 reboot。每次启动应包含：

```text
PAR_NVM: object initialization finished with status=OK
PAR_NVM: initialization finished with status=OK
PAR: initialization finished with status=OK
```

不应出现：

```text
object header signature corrupted
object header CRC corrupted
object record CRC corrupted
object table-ID mismatch
object body-size mismatch
```

## TC-O09：object save 期间掉电风险

如果具备电源控制条件：

1. 开始一次 object 写入，例如 `par_nvm_obj set_ascii 7 powercut`。
2. 在写入窗口内随机断电。
3. 再次上电。

验收标准：

| 场景 | 合格标准 |
| --- | --- |
| save 完成后掉电 | 上电后恢复最后保存的 object 值。 |
| save 过程中掉电 | 系统不死锁；启动能发现 object header/record 损坏，或回默认值并重写 object block。 |
| 再次 clean reboot | object initialization 返回 `status=OK`。 |

当前单 object block 不是 copy-on-write 或双 header 事务设计。它能检测并恢复损坏，但不保证 save 过程中掉电后仍保留上一个完整 object 值。

## 常见失败

| 现象 | 原因 | 修复 |
| --- | --- | --- |
| 构建失败并出现 `obj_persistence_requires_PAR_CFG_NVM_OBJECT_EN` | CSV 有 object `persistent=1`，但 object NVM 未启用。 | 启用 `AUTOGEN_PM_NVM_OBJECT`，或把 object `persistent` 改回 `0`。 |
| 没有 `par_nvm_obj` 命令 | helper Kconfig 未启用。 | 启用 `AUTOGEN_PM_TEST_NVM_OBJECT_HELPER` 和 `AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE`。 |
| `object_count=0` | 生成后的 `par_table.def` 中没有 object row `persistent=1`。 | 修正 CSV，重新生成，重新构建。 |
| shared 模式下 object base 为 `0` | object 地址解析失败，或没有 persistent object。 | 检查 scalar layout、persistent object count 和 object address mode。 |
| object block 与 scalar block 重叠 | fixed object address 小于 scalar block 结束地址。 | 增大 `AUTOGEN_PM_NVM_OBJECT_FIXED_ADDR`。 |
| 启动日志缺失 | 异步日志缓冲区太小。 | 测试固件中增大 `ULOG_ASYNC_OUTPUT_BUF_SIZE`。 |
