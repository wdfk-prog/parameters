[中文](./nvm-fixed-slot-no-size-manual-test.zh-CN.md)

# Fixed-slot-no-size scalar NVM manual test

## Scope

This document covers destructive board-level validation for `src/nvm/scalar/layout/par_nvm_layout_fixed_slot_no_size.c`. The test entry is the dedicated `par_nvm_fslot_no_size` MSH helper implemented in `parameters/tests/par_test_nvm_fixed_slot_no_size.c`.

The helper validates layout-aware address calculation and injects controlled corruption into the active scalar NVM backend. It is not a production command and must only be enabled in test firmware with disposable persistent data.

## Layout model

Each scalar record stores ID, CRC, and a fixed 4-byte payload slot; the natural size is not serialized.

Common scalar header size is `0x0C` bytes. All offsets printed by the helper are backend-relative offsets inside the parameter NVM window, not raw EEPROM absolute addresses. When `PAR_CFG_RTT_AT24_BASE_ADDR` is configured, the AT24 absolute address is:

```text
AT24 absolute address = PAR_CFG_RTT_AT24_BASE_ADDR + backend-relative offset
```

| Field | Formula / rule |
| --- | --- |
| record id offset | `record_offset + 0` |
| record crc offset | `record_offset + 2` |
| record payload offset | `record_offset + 3` |
| record size | `2 + 1 + 4 = 7 bytes` |
| slot offset rule | `0x0C + slot * 7` |

## Build configuration

Enable exactly one scalar layout and its matching helper. A typical RT-Thread test firmware configuration is:

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

If the parameter CSV/schema is changed, regenerate `par_table.def` and generated layout files before building the firmware. The helper reads the compiled table; it does not modify CSV persistence flags.

## Pre-checks after boot

Startup logs must show that scalar NVM is active, for example:

```text
PAR: restoring persistent values from NVM
PAR_NVM: initialization started
PAR_NVM: header validated
PAR_NVM: load-all finished with status=OK
PAR_NVM: initialization finished with status=OK
PAR: initialization finished with status=OK
```

If logs say `no persistent parameters configured` or `scalar persisted-record layout not required`, stop the test and fix Kconfig/schema generation first.

Also run:

```sh
par info
par_nvm_fslot_no_size info
```

Expected `info` checks:

1. `layout=fixed_slot_no_size`.
2. `backend=... status=OK(0x0000)`.
3. `persistent_count` equals the scalar persistent count reported by startup logs.
4. `resolved_slots` equals `persistent_count`.
5. `scalar_image_size` is greater than `0x0C` when at least one scalar persistent parameter exists.

## Command reference

```sh
par_nvm_fslot_no_size info
par_nvm_fslot_no_size slot <slot>
par_nvm_fslot_no_size corrupt_slot_crc <slot> [mask]
par_nvm_fslot_no_size corrupt_payload <slot> <payload_offset> [mask]

```

| Command | Purpose |
| --- | --- |
| `info` | Print layout name, backend status, header size, record offsets, persistent count, scalar image size, and AT24 window when available. |
| `slot <slot>` | Resolve one persistent scalar slot to `par_num`, ID, name, record offset, CRC offset, payload offset, record size, and payload size. |
| `corrupt_slot_crc <slot> [mask]` | XOR one CRC byte. Default mask is `0x01`. Reboot immediately after this command. |
| `corrupt_payload <slot> <payload_offset> [mask]` | XOR one payload byte inside one slot. Default mask is `0x01`. Reboot immediately after this command. |

## Detailed acceptance flow

### 1. Slot-map and address validation

For every slot from `0` to `persistent_count - 1`, run:

```sh
par_nvm_fslot_no_size slot <slot>
```

Each command must succeed. The output must include `slot=`, `offset=`, `crc_offset=`, `payload_offset=`, `record_size=`, `payload_size=`, `par_num=`, `id=`, and `name=`.

For the sample table shown by `par info`, slot `0` should be a valid scalar persistent slot. A failure such as `slot out of range or scalar persistent slot map invalid` means the compiled helper cannot map the live scalar persistent table and the corruption tests must not continue.

### 2. Save/reboot persistence sanity

Pick a writable scalar persistent parameter from `par info`, set it to a non-default in-range value, save it, and reboot:

```sh
par set <id> <non_default_value>
par save <id>
# reboot / power-cycle
par get <id>
```

The value after reboot must match the saved value. This proves the selected layout is actually participating in the normal NVM save/load path before destructive tests begin.

### 3. CRC corruption recovery

Pick a known valid slot and run:

```sh
par_nvm_fslot_no_size corrupt_slot_crc <slot>
# reboot immediately
```

Expected behavior after reboot:

1. startup detects a scalar CRC corruption;
2. scalar NVM is rebuilt from defaults according to the current recovery policy;
3. parameter initialization finishes without a persistent NVM fatal error;
4. the previously corrupted scalar value is no longer trusted unless rewritten after rebuild.

### 4. Payload corruption recovery

Pick a valid slot and a `payload_offset` lower than the printed `payload_size`:

```sh
par_nvm_fslot_no_size corrupt_payload <slot> <payload_offset>
# reboot immediately
```

Expected behavior is the same as CRC corruption, because the stored CRC no longer matches the mutated payload.

### 5. Layout-specific corruption

No additional layout-specific corruption command is defined for this layout.

### 6. Cleanup

Erase or rebuild the disposable NVM image before reusing the board for normal testing. Do not leave a board in a partially corrupted persistence state.

## Troubleshooting

| Symptom | Likely cause | Action |
| --- | --- | --- |
| Command is not listed in MSH completion | Helper Kconfig is disabled, `RT_USING_FINSH` is disabled, or `SConscript` did not include the helper source. | Recheck `AUTOGEN_PM_TEST_NVM_FIXED_SLOT_NO_SIZE`, `AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE`, `AUTOGEN_PM_USING_TESTS`, and `RT_USING_FINSH`; rebuild from a clean tree. |
| `info` shows `scalar_image_size=0x0000000C` while `persistent_count` is non-zero | Slot mapping failed before records were summed. | Rebuild with this revision and verify the generated table contains scalar persistent rows. |
| `slot 0` fails although `persistent_count > 0` | The helper cannot map live scalar persistent slots. | Confirm the helper loops over `ePAR_NUM_OF`, not byte-size `par_cfg_get_table_size()`, and confirm generated table consistency. |
| `payload_offset out of range` | Offset is not lower than the printed natural `payload_size`. | Re-run `slot <slot>` and choose an offset in `[0, payload_size)`. |
| Reboot does not report corruption | Wrong slot/offset was corrupted, or a normal save rewrote the record before reboot. | Repeat from a clean image and reboot immediately after the corruption command. |

## Notes and limits

- Slot `0` is valid when `persistent_count > 0`; if `slot 0` fails, first check that the helper was built from this revision and that the generated table contains scalar persistent rows.
- This is a manual/HIL helper, not a complete power-loss endurance test.
- The helper validates the active AT24CXX scalar backend window only; it does not validate unrelated flash geometry or object NVM behavior.
