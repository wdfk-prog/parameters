[中文](./nvm-object-manual-test.zh-CN.md)

# Object NVM manual hardware test

This document describes the manual hardware acceptance flow for `AUTOGEN_PM_NVM_OBJECT`. It is intentionally separate from the scalar fixed-slot document because object persistence uses an independent object block with its own header, variable-capacity object records, payload bytes, and record CRC fields.

The test cases are destructive. Run them only on hardware with disposable parameter NVM data.

## Scope

This document validates:

- object persistent CSV configuration;
- Kconfig requirements for object persistence;
- object block base, size, slot, record CRC, and payload offsets;
- object payload persistence through public APIs;
- object header corruption recovery;
- object record CRC and payload corruption recovery;
- reboot stability after object NVM rebuild.

It does not validate the scalar fixed-slot record layout. Use [Fixed-slot-with-size NVM manual hardware test](./nvm-fixed-slot-with-size-manual-test.md) for scalar fixed-slot CRC tests.

## Reuse interface

The object helper action layer is reusable outside MSH through:

```c
int par_test_nvm_obj_exec(int argc, char **argv);
```

The RT-Thread `par_nvm_obj` command is only a shell wrapper around this function. A future host simulator may call the same function after binding test output with `par_test_set_vprint()` and providing a software object NVM backend.

## Object NVM layout terms

| Term | Meaning |
| --- | --- |
| Scalar block | The scalar NVM header and scalar persistent records. |
| Object block | The separate object persistence block managed by `AUTOGEN_PM_NVM_OBJECT`. |
| Object-block-relative offset | Offset from the object block base. `par_nvm_obj dump/poke/flip` use this offset. |
| Backend-relative offset | Offset inside the active backend window. `backend offset = object_block_base + object-block-relative offset`. |
| AT24 absolute offset | `AUTOGEN_PM_RTT_AT24_BASE_ADDR + backend-relative offset`. |

Current object serialized layout:

```text
object header size       = 0x12
object record head size  = 0x0C
record CRC offset        = record_offset + 10
record payload offset    = record_offset + 12
record size              = 0x0C + configured object capacity in bytes
object block size        = 0x12 + sum(record sizes)
```

The object block base depends on Kconfig:

| Mode | Base address rule |
| --- | --- |
| `AUTOGEN_PM_NVM_OBJECT_STORE_SHARED` + `AUTOGEN_PM_NVM_OBJECT_ADDR_AFTER_SCALAR` | Immediately after the compiled scalar NVM block. This is compact but moves when the scalar layout grows or changes. |
| `AUTOGEN_PM_NVM_OBJECT_STORE_SHARED` + `AUTOGEN_PM_NVM_OBJECT_ADDR_FIXED` | `AUTOGEN_PM_NVM_OBJECT_FIXED_ADDR` inside the same backend window. This is preferred for release validation. |
| `AUTOGEN_PM_NVM_OBJECT_STORE_DEDICATED` | `AUTOGEN_PM_NVM_OBJECT_DEDICATED_BASE_ADDR` inside the dedicated object backend. |

## Manual helper commands

Enable `AUTOGEN_PM_TEST_NVM_OBJECT_HELPER` to build:

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

`dump`, `poke`, `flip`, `corrupt_header`, `corrupt_record_crc`, and `corrupt_payload` are destructive. After a corruption command, reboot immediately and inspect startup logs. Do not run `par_nvm_obj info`, `par_nvm_obj list`, or `par get` before reboot unless the test case explicitly says so, because those commands may initialize or access the parameter module and hide the intended startup-recovery observation.

## Common Kconfig

Unless a test says otherwise, enable these options:

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

For stable object NVM placement, prefer fixed object address mode:

```text
AUTOGEN_PM_NVM_OBJECT_ADDR_FIXED=y
AUTOGEN_PM_NVM_OBJECT_FIXED_ADDR=0x100
AUTOGEN_PM_NVM_OBJECT_REGION_SIZE=0x200
```

The fixed address must not overlap the scalar NVM block. If the scalar block grows beyond `0x100`, increase `AUTOGEN_PM_NVM_OBJECT_FIXED_ADDR`.

For compact development builds, the default after-scalar mode is acceptable:

```text
AUTOGEN_PM_NVM_OBJECT_ADDR_AFTER_SCALAR=y
```

For `par get <id>` object payload display, also enable:

```text
AUTOGEN_PM_USING_MSH_TOOL=y
AUTOGEN_PM_MSH_CMD_GET=y
AUTOGEN_PM_MSH_CMD_GET_OBJECT=y
RT_USING_HEAP=y
```

For default-value tests:

```text
AUTOGEN_PM_MSH_CMD_DEF=y
AUTOGEN_PM_MSH_CMD_DEF_ALL=y
AUTOGEN_PM_MSH_CMD_SAVE=y
```

The AT24CXX backend also needs board-specific settings:

```text
PKG_USING_AT24CXX=y
AUTOGEN_PM_RTT_AT24_I2C_BUS_NAME="hwi2c2"      # adjust for the board
AUTOGEN_PM_RTT_AT24_ADDR_INPUT=1                # adjust for A2/A1/A0 and driver semantics
AUTOGEN_PM_RTT_AT24_BASE_ADDR=0x2000            # parameter backend window base
AUTOGEN_PM_RTT_AT24_SIZE=0x2000                 # parameter backend window size
```

## CSV Profile O: scalar + object persistent test table

Kconfig cannot force the CSV `persistent` field to `1`. Edit the CSV, regenerate `par_table.def` and generated layout files, then rebuild.

Use this profile for object NVM hardware tests:

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

With the object rows above, the expected object slot order is:

| Object slot | ID | Type | Capacity bytes | Record offset | CRC offset | Payload offset |
| ---: | ---: | --- | ---: | ---: | ---: | ---: |
| 0 | 7 | STR | 8 | `0x12` | `0x1C` | `0x1E` |
| 1 | 8 | BYTES | 4 | `0x26` | `0x30` | `0x32` |
| 2 | 9 | ARR_U8 | 4 | `0x36` | `0x40` | `0x42` |
| 3 | 10 | ARR_U16 | 6 | `0x46` | `0x50` | `0x52` |
| 4 | 12 | ARR_U32 | 8 | `0x58` | `0x62` | `0x64` |

The expected object block size is `0x6C` bytes.

## Test case matrix

| Case | Goal | CSV | Hardware required | Reboot required |
| --- | --- | --- | --- | --- |
| TC-O00 | Build-time object persistent protection | Object rows `persistent=1`, object NVM disabled | No | No |
| TC-O01 | Firmware enters object NVM path | Profile O | Yes | Usually two boots for blank media |
| TC-O02 | Object block address and slot map | Profile O | Yes | No |
| TC-O03 | Object payload persistence | Profile O | Yes | Yes |
| TC-O04 | Object header corruption recovery | Profile O | Yes | Yes |
| TC-O05 | Object record CRC corruption recovery | Profile O | Yes | Yes |
| TC-O06 | Object payload corruption recovery | Profile O | Yes | Yes |
| TC-O07 | Default restore and save semantics | Profile O | Yes | Yes |
| TC-O08 | Repeated reboot stability | Profile O | Yes | Yes |
| TC-O09 | Power-loss risk during object save | Profile O | Yes | Power cycle |

## TC-O00: object persistent protection

### Kconfig

Use Profile O but disable object NVM:

```text
AUTOGEN_PM_USING_NVM=y
AUTOGEN_PM_NVM_SCALAR=y
AUTOGEN_PM_NVM_OBJECT=n
```

### Expected result

Build must fail with a keyword similar to:

```text
obj_persistence_requires_PAR_CFG_NVM_OBJECT_EN
```

This proves that object rows cannot be marked persistent unless object NVM support is explicitly enabled.

## TC-O01: object NVM startup path

### Execute

Flash a firmware built with Profile O and the common object Kconfig. Reboot.

### Expected result

Startup should enter both scalar and object NVM paths. Typical stable logs after the object block has been initialized once:

```text
PAR_NVM: header validated
PAR_NVM: load-all finished with status=OK
PAR_NVM: object initialization finished with status=OK
PAR_NVM: initialization finished with status=OK
PAR: initialization finished with status=OK
```

On blank or previously corrupted media, the first boot may report object header or record validation failure and then rewrite the object block. Reboot once more. The second boot must reach `object initialization finished with status=OK`.

## TC-O02: object block address and slot map

### Execute

```text
par_nvm_obj info
par_nvm_obj list
par_nvm_obj slot 0
par_nvm_obj slot 4
par_nvm_obj dump 0x00 0x6C
```

### Expected result

`info` should show:

```text
object_nvm=1
object_count=5
object_block_base=...
object_block_size=0x0000006C
header_size=0x12
record_head_size=0x0C
record_crc_offset=10
```

`slot 0` should report `record_offset=0x00000012`, `crc_offset=0x0000001C`, and `payload_offset=0x0000001E`.

`slot 4` should report `record_offset=0x00000058`, `crc_offset=0x00000062`, and `payload_offset=0x00000064`.

## TC-O03: object payload persistence

### Execute STR persistence

```text
par_nvm_obj set_ascii 7 hello
par get 7
reboot
par get 7
```

Expected value after reboot: `hello`.

### Execute BYTES persistence

```text
par_nvm_obj set_hex 8 0xAA 0xBB 0xCC 0xDD
par get 8
reboot
par get 8
```

Expected value after reboot: `AA BB CC DD` or equivalent object display format.

### Notes

`set_hex` writes raw payload bytes through `par_set_obj_n_save_by_id()`. For `ARR_U16` and `ARR_U32`, byte order is target-native. Prefer STR and BYTES for manual shell acceptance unless a product-specific typed object setter command is available.

## TC-O04: object header corruption recovery

### Execute

```text
par_nvm_obj corrupt_header
reboot
```

### Expected first reboot logs

```text
PAR_NVM: object header signature corrupted
PAR_NVM: object store-all finished, count=5
PAR_NVM: object initialization finished with status=...
PAR: initialization finished with status=OK
```

Then reboot again:

```text
reboot
```

Expected stable logs:

```text
PAR_NVM: object initialization finished with status=OK
PAR_NVM: initialization finished with status=OK
PAR: initialization finished with status=OK
```

The second reboot must not show `object header signature corrupted`.

## TC-O05: object record CRC corruption recovery

### Execute

```text
par_nvm_obj corrupt_record_crc 0
reboot
```

### Expected first reboot logs

```text
PAR_NVM: object record CRC corrupted, par_num=...
PAR_NVM: object store-all finished, count=5
PAR_NVM: object initialization finished with status=...
PAR: initialization finished with status=OK
```

Then reboot again. The second reboot must reach `object initialization finished with status=OK` and must not report object CRC corruption.

## TC-O06: object payload corruption recovery

### Execute

```text
par_nvm_obj corrupt_payload 0 0 0x01
reboot
```

### Expected first reboot logs

Payload corruption should be detected through the record CRC check:

```text
PAR_NVM: object record CRC corrupted, par_num=...
PAR_NVM: object store-all finished, count=5
PAR: initialization finished with status=OK
```

Then reboot again. The second reboot must be clean.

## TC-O07: default restore and save semantics

### Execute

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

Expected sequence:

| Step | Expected value |
| --- | --- |
| After `set_ascii` and reboot | `hello` |
| After `par def 7` | default string, for Profile O: `ap` |
| After `par save` and reboot | default string, for Profile O: `ap` |

`par def` changes RAM only. `par save` is required to overwrite NVM.

## TC-O08: repeated reboot stability

Execute 20 reboots. Every boot should contain:

```text
PAR_NVM: object initialization finished with status=OK
PAR_NVM: initialization finished with status=OK
PAR: initialization finished with status=OK
```

No boot should contain:

```text
object header signature corrupted
object header CRC corrupted
object record CRC corrupted
object table-ID mismatch
object body-size mismatch
```

## TC-O09: power-loss risk during object save

If hardware power control is available:

1. Start an object write, for example `par_nvm_obj set_ascii 7 powercut`.
2. Randomly cut power during the write window.
3. Power on again.

Acceptance standard:

| Scenario | Acceptable result |
| --- | --- |
| Power loss after save completes | Last saved object value is restored. |
| Power loss during save | System does not deadlock; startup detects object header/record corruption or falls back to defaults and rewrites the object block. |
| Next clean reboot | Object initialization returns `status=OK`. |

The current single object block is not a copy-on-write or dual-header transactional design. It can detect and recover from corruption, but it does not guarantee that the previous complete object value survives a power loss during an in-progress object rewrite.

## Common failures

| Symptom | Cause | Fix |
| --- | --- | --- |
| Build fails with `obj_persistence_requires_PAR_CFG_NVM_OBJECT_EN` | CSV has object `persistent=1`, but object NVM is disabled. | Enable `AUTOGEN_PM_NVM_OBJECT`, or set object `persistent=0`. |
| `par_nvm_obj` command is missing | Helper Kconfig not enabled. | Enable `AUTOGEN_PM_TEST_NVM_OBJECT_HELPER` and `AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE`. |
| `object_count=0` | No object row has `persistent=1` in generated `par_table.def`. | Fix CSV, regenerate, rebuild. |
| Object base is `0` in shared mode | Object address resolution failed or no persistent object exists. | Check scalar layout, persistent object count, and object address mode. |
| Object block overlaps scalar block | Fixed object address is lower than scalar block end. | Increase `AUTOGEN_PM_NVM_OBJECT_FIXED_ADDR`. |
| Logs are missing | Async log buffer too small. | Increase `ULOG_ASYNC_OUTPUT_BUF_SIZE` for the test firmware. |
