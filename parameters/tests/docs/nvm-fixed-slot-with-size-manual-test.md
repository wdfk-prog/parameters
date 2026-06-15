[中文](./nvm-fixed-slot-with-size-manual-test.zh-CN.md)

# Fixed-slot-with-size NVM manual hardware test guide

For object persistence acceptance, use [Object NVM manual hardware test](./nvm-object-manual-test.md).

This document defines the manual and hardware-in-the-loop acceptance tests for `AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE` on the RT-Thread AT24CXX backend. The tests focus on:

- whether scalar persistent parameters really enter the NVM initialization, load, and save paths;
- whether fixed-slot-with-size slot address calculation matches the layout contract;
- whether header corruption and single-slot CRC corruption are detected and recovered during reboot;
- whether `par set`, `par save`, `par def`, `par def_all`, and reboot have clear persistence semantics;
- how to execute and judge tests that require real hardware, manual `reboot`, or power interruption.

This is a manual/HIL acceptance guide. It is not intended to be enabled as a default automatic unit test. Steps that use `par_nvm_raw poke`, `par_nvm_raw flip`, `par_nvm_raw corrupt_header`, or `par_nvm_fslot corrupt_slot_crc` intentionally damage the parameter EEPROM window and must only be run on a test firmware whose NVM contents may be discarded.

## Reuse interface

The fixed-slot helper action layer is reusable outside MSH through:

```c
int par_test_nvm_fslot_exec(int argc, char **argv);
```

The RT-Thread `par_nvm_fslot` command is only a shell wrapper around this function. A future host simulator may call the same function after binding test output with `par_test_set_vprint()` and providing a software NVM backend that implements the selected scalar layout.

## Terms

| Term | Meaning |
| --- | --- |
| persistent parameter | A parameter whose CSV `persistent` field is `1` and whose type is supported by the enabled Kconfig NVM capability. |
| scalar persistent parameter | A persistent scalar parameter, such as `U8`, `U16`, `U32`, `I8`, `I16`, `I32`, or `F32`. |
| object persistent parameter | A persistent object parameter, such as `STR`, `BYTES`, `ARR_U8`, `ARR_U16`, or `ARR_U32`. |
| slot | The sequential index of a scalar persistent parameter in the NVM persistence table. It starts from `0` and is not the parameter ID. |
| backend-relative offset | Offset relative to the parameter NVM backend window, for example `0x00`. It is not the EEPROM absolute address. |
| AT24 absolute offset | `AUTOGEN_PM_RTT_AT24_BASE_ADDR + backend-relative offset`. |

The fixed-slot-with-size scalar record layout is:

```text
header size       = 0x0C
record size       = 8
slot offset       = 0x0C + slot * 8
slot CRC offset   = slot offset + 3
scalar image size = 0x0C + persistent_count * 8
```

Example: the CRC offset of `slot 7` is `0x0C + 7 * 8 + 3 = 0x47`. If `AUTOGEN_PM_RTT_AT24_BASE_ADDR = 0x2000`, the EEPROM absolute offset is `0x2047`.

## Common prerequisites

### Regenerate the parameter table after CSV changes

The CSV file is the parameter-table input. Kconfig cannot force the CSV `persistent` field to become `1` at compile time. After editing the CSV, regenerate `par_table.def` and the related generated files, then rebuild the firmware.

Typical flow:

```sh
python3 parameters/tools/pargen.py
scons --menuconfig
scons -j$(nproc)
```

If the product project has an outer table-generation script, use that project-owned generation entry. Before acceptance, inspect the `par_table.def` diff and verify that the target rows really pass `1` as the `pers_` argument.

### Distinguish scalar NVM from object NVM

This guide validates `AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE`, which is a scalar NVM record layout. For header, slot CRC, range, and save/reboot tests, keep scalar parameters at `persistent=1` and object parameters at `persistent=0`.

If `STR`, `BYTES`, `ARR_U8`, `ARR_U16`, or `ARR_U32` are also set to `persistent=1` without enabling object NVM, the build must fail. Typical error keywords are:

```text
_static_assert_ePAR_TEST_STR_RW_obj_persistence_requires_PAR_CFG_NVM_OBJECT_EN
_static_assert_ePAR_TEST_BYTES_RW_obj_persistence_requires_PAR_CFG_NVM_OBJECT_EN
_static_assert_ePAR_TEST_ARR_U8_RW_obj_persistence_requires_PAR_CFG_NVM_OBJECT_EN
```

This is expected protection. Use one of the following fixes:

1. Recommended for this guide: keep object parameters at `persistent=0`.
2. If object NVM is being tested separately: enable `AUTOGEN_PM_NVM_OBJECT` and confirm that the object NVM block does not overlap the scalar NVM block. Object NVM is outside the fixed-slot scalar CRC scope of this document.

### Confirm that startup enters the NVM path

After flashing, startup logs must contain lines similar to:

```text
PAR: restoring persistent values from NVM
PAR_NVM: initialization started
PAR_NVM: header validated
PAR_NVM: load-all finished with status=OK
PAR: initialization finished with status=OK
```

If startup shows:

```text
PAR_NVM: scalar storage backend not required
PAR_NVM: scalar persisted-record layout not required
PAR_NVM: no persistent parameters configured
PAR: initialization finished with status=NO PERSISTENT
```

then the compiled parameter table has no scalar persistent parameter. Header corruption, slot CRC corruption, and NVM recovery acceptance cannot be run in that firmware. Fix the CSV, generated files, and Kconfig first.

## Base Kconfig configuration

Unless a test case says otherwise, enable these options in the manual hardware test firmware:

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

The AT24CXX backend also needs board-specific settings:

```text
PKG_USING_AT24CXX=y
AUTOGEN_PM_RTT_AT24_I2C_BUS_NAME="hwi2c2"      # adjust for the board
AUTOGEN_PM_RTT_AT24_ADDR_INPUT=1                # adjust for A2/A1/A0 and driver semantics
AUTOGEN_PM_RTT_AT24_BASE_ADDR=0x2000            # parameter window base address
AUTOGEN_PM_RTT_AT24_SIZE=0x2000                 # parameter window size; must cover the scalar image
```


The manual helpers are split by responsibility:

| Command | Scope | Use |
| --- | --- | --- |
| `par_nvm_raw` | Layout-neutral raw backend helper | `info`, `lut`, `dump`, `poke`, `flip`, and `corrupt_header` by backend-relative offset. |
| `par_nvm_fslot` | Fixed-slot-with-size layout helper | `info`, `slot`, and `corrupt_slot_crc` using fixed-slot address calculation. |

`par_nvm_raw` can be used with other scalar NVM layouts for raw offset-level checks. `par_nvm_fslot` must only be used when `AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE=y`.

For `par def` and `par def_all` tests, additionally enable:

```text
AUTOGEN_PM_MSH_CMD_DEF=y
AUTOGEN_PM_MSH_CMD_DEF_ALL=y
```

For `par_nvm_raw lut`, enabling debug output is recommended:

```text
AUTOGEN_PM_USING_DEBUG=y
```

## CSV Profile A: scalar persistent test table

Use this CSV for all fixed-slot-with-size tests except the negative build test and any separate object NVM test. The key rule is: scalar test rows use `persistent=1`; object test rows use `persistent=0`.

Save the following content as `usersrc_parameter_general_par_table_pargen.csv`, or as the actual parameter CSV used by the product project, then rerun the generator:

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

If `AUTOGEN_PM_TEST_USING_AT24CXX=y`, this profile usually produces 8 scalar persistent slots: `slot 0..7`. If that option is not enabled, the AT24 conditional row is not compiled into the table and the profile usually produces 7 scalar persistent slots: `slot 0..6`.

## CSV Profile B: object persistent negative build test table

Use this profile to prove that object parameters set to `persistent=1` fail to build when object NVM is not enabled. This test does not require flashing or hardware.

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

Expected build-failure keyword:

```text
obj_persistence_requires_PAR_CFG_NVM_OBJECT_EN
```

If `AUTOGEN_PM_NVM_OBJECT` is enabled, this negative build test no longer applies.

## Test case matrix

| Case | Goal | CSV | Hardware required | Reboot required |
| --- | --- | --- | --- | --- |
| TC-00 | Build-time object persistent protection | Profile B | No | No |
| TC-01 | Firmware enters scalar NVM path | Profile A | Yes | No |
| TC-02 | Fixed-slot address and LUT check | Profile A | Yes | No |
| TC-03 | Header corruption recovery | Profile A | Yes | Yes |
| TC-04 | Single-slot CRC corruption recovery | Profile A | Yes | Yes |
| TC-05 | Range-limited value persistence | Profile A | Yes | Yes |
| TC-06 | `par def` / `par def_all` save semantics | Profile A | Yes | Yes |
| TC-07 | Repeated reboot stability | Profile A | Yes | Yes |
| TC-08 | Immediate reboot after `par save` | Profile A | Yes | Yes |
| TC-09 | Power-loss risk during save | Profile A | Yes | Power cycle |

## TC-00: object persistent protection

### Kconfig

Enable scalar NVM, but do not enable object NVM:

```text
AUTOGEN_PM_USING_NVM=y
AUTOGEN_PM_NVM_SCALAR=y
AUTOGEN_PM_NVM_OBJECT=n
```

### CSV

Use CSV Profile B.

### Steps

Regenerate the parameter table and build:

```sh
python3 parameters/tools/pargen.py
scons -j$(nproc)
```

### Expected result

The build fails and logs contain:

```text
obj_persistence_requires_PAR_CFG_NVM_OBJECT_EN
```

### Judgement

| Result | Conclusion |
| --- | --- |
| Build fails as expected | Object persistent static protection is effective. |
| Build passes | Check whether object NVM was unintentionally enabled or whether the static assertion is broken. |

## TC-01: firmware enters scalar NVM path

### Kconfig

Use the base configuration. `AUTOGEN_PM_NVM_OBJECT` is not required.

### CSV

Use CSV Profile A.

### Steps

Flash the firmware, boot it, and inspect startup logs. Then run in MSH:

```text
par_nvm_raw info
par_nvm_fslot info
```

### Expected result

Startup logs contain:

```text
PAR: restoring persistent values from NVM
PAR_NVM: initialization started
PAR_NVM: header validated
PAR_NVM: load-all finished with status=OK
PAR: initialization finished with status=OK
```

`par_nvm_fslot info` contains:

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

If `AUTOGEN_PM_TEST_USING_AT24CXX` is not enabled, `persistent_count` may be `7` and `scalar_image_size` should be `0x00000044`.

### Common failures

| Log | Cause | Action |
| --- | --- | --- |
| `no persistent parameters configured` | The CSV did not set scalar `persistent=1`, or `par_table.def` was not regenerated after editing CSV. | Check CSV, generated artifacts, and build input. |
| `backend not required` | No parameter requires scalar NVM. | Same as above. |
| `backend status=ERROR INIT` | AT24CXX backend did not initialize. | Check I2C bus, AT24 address, and package configuration. |

## TC-02: fixed-slot address and LUT check

### Kconfig

Use the base configuration. Recommended:

```text
AUTOGEN_PM_USING_DEBUG=y
```

### CSV

Use CSV Profile A.

### Steps

```text
par_nvm_fslot info
par_nvm_raw lut
par_nvm_fslot slot 0
par_nvm_fslot slot 7
par_nvm_raw dump 0x00 0x4C
```

If `persistent_count=7`, the last slot is `slot 6`; run:

```text
par_nvm_fslot slot 6
par_nvm_raw dump 0x00 0x44
```

### Expected result

`slot 0`:

```text
slot=0 offset=0x0000000C crc_offset=0x0000000F
```

`slot 7`:

```text
slot=7 offset=0x00000044 crc_offset=0x00000047
```

If `AUTOGEN_PM_RTT_AT24_BASE_ADDR=0x2000`, the `slot 7` `abs_crc_offset` should be:

```text
abs_crc_offset=0x00002047
```

### Judgement

| Result | Conclusion |
| --- | --- |
| Offset and CRC offset match the formula | Fixed-slot addressing is correct. |
| Slot is out of range | A non-existing slot was used; inspect `persistent_count`. |
| Parameter ID to slot mapping in LUT is unexpected | Check CSV row order, conditional compilation, and generated artifacts. |

## TC-03: header corruption recovery

### Kconfig

Use the base configuration.

### CSV

Use CSV Profile A.

### Steps

Confirm the current image is normal:

```text
par_nvm_raw info
par_nvm_fslot info
```

Corrupt the first byte of the header through the layout-neutral raw helper:

```text
par_nvm_raw corrupt_header
```

Manually reboot:

```text
reboot
```

Reboot again to confirm that the rebuilt image is stable:

```text
reboot
```

### Expected result

After the first reboot, startup logs contain:

```text
PAR_NVM: header signature corrupted
PAR_NVM: scalar header/signature mismatch detected, rebuilding scalar NVM from defaults
PAR: initialization finished with status=OK
```

After the second reboot, startup logs contain:

```text
PAR_NVM: header validated
PAR_NVM: load-all finished with status=OK
PAR: initialization finished with status=OK
```

### Judgement

| Result | Conclusion |
| --- | --- |
| First reboot detects header corruption and rebuilds; second reboot is OK | Header corruption recovery is effective. |
| Still shows `NO PERSISTENT` | The current table has no scalar persistent parameter; this case cannot be executed. |
| System blocks or crashes during startup | Recovery path has a severe defect. |

## TC-04: single-slot CRC corruption recovery

### Kconfig

Use the base configuration.

### CSV

Use CSV Profile A.

### Steps

Select a valid slot first. Prefer the last valid slot to cover non-first-element addressing:

```text
par_nvm_fslot info
par_nvm_fslot slot 7
par_nvm_fslot corrupt_slot_crc 7
reboot
reboot
```

If `persistent_count=7`, use `slot 6`. Do not hard-code `slot 20` unless `persistent_count >= 21`.

### Expected result

After the first reboot, startup logs contain:

```text
PAR_NVM: load-all finished with status=ERROR CRC
PAR_NVM: load error slot=7
PAR_NVM: scalar CRC corruption detected, rebuilding scalar NVM from defaults
PAR: initialization finished with status=OK
```

After the second reboot, startup logs contain:

```text
PAR_NVM: header validated
PAR_NVM: load-all finished with status=OK
```

### Judgement

| Result | Conclusion |
| --- | --- |
| First reboot detects CRC error and rebuilds; second reboot is OK | Single-slot CRC detection and recovery are effective. |
| `ERR slot out of range` | A non-existing slot was used; inspect `persistent_count` first. |
| CRC error does not trigger rebuild | Check the NVM load-all error handling path. |

## TC-05: range-limited value persistence

### Kconfig

Use the base configuration.

### CSV

Use CSV Profile A. This guide uses `ePAR_TEST_U8_RW`, whose range is `10..20` and default value is `12`. If the product table uses ID 26 or another business parameter, set that scalar row to `persistent=1` and run commands using the actual ID.

### Steps

In CSV Profile A, the generated ID of `ePAR_TEST_U8_RW` is usually `0`. If an ID lock changes it, use `par_nvm_raw lut` or the generated files as the source of truth.

Upper-limit test:

```text
par set 0 21
par get 0
par save
reboot
par get 0
```

Lower-limit test:

```text
par set 0 9
par get 0
par save
reboot
par get 0
```

### Expected result

Upper-limit test:

```text
WAR,PAR_SET=20
WAR, value limited to configured range
par get 0 -> 20
After reboot, par get 0 -> 20
```

Lower-limit test:

```text
WAR,PAR_SET=10
WAR, value limited to configured range
par get 0 -> 10
After reboot, par get 0 -> 10
```

### Judgement

| Result | Conclusion |
| --- | --- |
| The value after reboot is still the limited value | Range limiting and NVM persistence are correct. |
| The value after reboot is the illegal raw input | The `set/save` path bypassed range validation. |
| The value after reboot is the default | NVM load did not override RAM, or save failed and startup rebuilt defaults. |

## TC-06: `par def` / `par def_all` save semantics

### Kconfig

In addition to the base configuration, enable:

```text
AUTOGEN_PM_MSH_CMD_DEF=y
AUTOGEN_PM_MSH_CMD_DEF_ALL=y
```

Kconfig only compiles these commands. It does not replace `par save`. `par def` and `par def_all` normally restore RAM only; `par save` is required to overwrite EEPROM.

### CSV

Use CSV Profile A.

### Steps: single-parameter default

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

### Expected result: single-parameter default

```text
After set + reboot -> 15
After def + get -> 12
After def + save + reboot -> 12
```

### Steps: all defaults

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

### Expected result: all defaults

```text
par get 0 -> 12
par get 2 -> 120
par get 3 -> 1200
```

### Judgement

| Result | Conclusion |
| --- | --- |
| After `def`/`def_all` + save, reboot still returns defaults | Default restore and save semantics are correct. |
| After `def` without save, reboot returns the old value | Normal; `def` modified RAM only. |
| `def_all` without save overwrites EEPROM | Unexpected semantics; inspect the shell implementation. |

## TC-07: repeated reboot stability

### Kconfig

Use the base configuration.

### CSV

Use CSV Profile A.

### Steps

Run 20 reboot cycles:

```text
reboot
```

Inspect startup logs every time.

### Expected result

Every boot shows:

```text
PAR_NVM: load-all finished with status=OK
PAR: initialization finished with status=OK
```

The following must not appear:

```text
ERROR CRC
header signature corrupted
table-id mismatch
scalar CRC corruption detected
```

### Judgement

| Result | Conclusion |
| --- | --- |
| All 20 cycles are stable | EEPROM window isolation, bootloader interaction, and fixed-slot read/write are stable. |
| Occasional CRC/header errors | First check EEPROM write cycle timing, whether the bootloader writes the same address range, and I2C stability. |

## TC-08: immediate reboot after `par save`

### Kconfig

Use the base configuration.

### CSV

Use CSV Profile A.

### Steps

Run 10 rounds. In each round, change a value, save it, and reboot immediately:

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

`ePAR_TEST_U16_RW` range is `100..200`, and its default is `120`. Later rounds may increment up to `130`.

### Expected result

After each reboot, the last saved value is readable.

### Common failure directions

| Symptom | First check |
| --- | --- |
| Occasionally reads the old value | AT24 write-cycle wait, `store->sync()`, `EE_TWR`. |
| Occasional CRC error | `AUTOGEN_PM_NVM_WRITE_VERIFY`, `at24cxx_page_write()`, I2C ACK polling. |
| Always returns defaults | `par save` did not complete successfully, or startup rebuilt defaults. |

## TC-09: power-loss risk during save

### Kconfig

Use the base configuration. Write verification is recommended:

```text
AUTOGEN_PM_NVM_WRITE_VERIFY=y
```

### CSV

Use CSV Profile A.

### Steps

This test requires an external power controller or HIL device. Run:

```text
par set 2 150
par save
```

Cut power at a random point after `par save` starts. Restore power and inspect startup logs and parameter values.

### Expected result and acceptance criteria

The current fixed-slot single-area layout has no dual header, version number, or copy-on-write commit marker. It cannot promise that a power loss during save preserves the previous complete version. Acceptance criteria for this test are:

| Scenario | Passing criteria |
| --- | --- |
| Power loss after a normal save completed | Value after reboot is correct. |
| Power loss during save | The system does not hang; CRC/header errors can be detected; defaults can be restored or the area can be rebuilt. |
| Business startup after power loss | `PAR: initialization finished` does not block system startup. |

If the product requires preserving the old value even when power is lost during save, the current fixed-slot single-area layout is insufficient. Use a dual-area, versioned, or copy-on-write commit design.

## Migrating this guide to a product parameter table

If a product parameter table is used instead of this guide's test CSV, for example `par set 26 32` or `par get 218`, migrate the tests with these rules:

1. Set only the scalar parameters that need NVM verification to `persistent=1`.
2. Do not set `STR`, `BYTES`, or `ARR_*` to `persistent=1` unless object NVM is enabled and planned.
3. Regenerate the parameter table after editing the CSV.
4. Use `par_nvm_raw lut` to confirm which slot corresponds to a parameter ID.
5. When corrupting a slot CRC, use the slot index, not the parameter ID.

Example: if `par_nvm_raw lut` shows that parameter ID 26 maps to `slot 3`, use:

```text
par_nvm_fslot corrupt_slot_crc 3
```

Do not use:

```text
par_nvm_fslot corrupt_slot_crc 26
```

## Troubleshooting quick reference

| Symptom | Direct cause | Action |
| --- | --- | --- |
| `persistent_count=0` | The compiled table has no scalar persistent parameter. | Check CSV, `par_table.def`, conditional compilation, and generation flow. |
| `ERR slot out of range` | Slot index is greater than `persistent_count - 1`. | Run `par_nvm_fslot info` first and choose a valid slot. |
| Build fails after object parameters are set to `persistent=1` | `AUTOGEN_PM_NVM_OBJECT` is not enabled. | Set object persistent back to `0`, or enable and plan object NVM. |
| No recovery logs after corruption | Startup did not enter the NVM path, or the corruption write did not take effect. | Check startup logs, readback, and AT24 backend status. |
| `I2C[hwi2c2] Write error(2)` | I2C write-cycle, address, ACK polling, or page-write issue. | Check AT24 address, `EE_TWR`, `sync()`, and `AUTOGEN_PM_NVM_WRITE_VERIFY`. |
| Reboot returns defaults | NVM was rebuilt, or save did not complete. | Check startup logs for CRC/header/table-id errors. |
