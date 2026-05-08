[中文](./api-reference.zh-CN.md)

# API reference

This page groups the public API by responsibility. Always include `par.h` unless a narrow internal integration intentionally includes a lower-level header.

## Lifecycle

| API | Purpose |
| --- | --- |
| `par_init()` | Initialize table checks, storage, defaults, and optional restore path. |
| `par_deinit()` | Deinitialize the module and release platform resources where supported. |
| `par_is_init()` | Query whether the module is initialized. |

## Mutex helpers

| API | Purpose |
| --- | --- |
| `par_acquire_mutex(par_num)` | Acquire the parameter mutex when mutex support is enabled. |
| `par_release_mutex(par_num)` | Release the parameter mutex. |

Use these only for integration code that must compose multiple operations under the same lock.

## Scalar setters

| API family | Purpose |
| --- | --- |
| `par_set_scalar()` | Pointer-based scalar setter when the caller already knows the type. |
| `par_set_scalar_by_id()` | ID-based pointer scalar setter when ID support is enabled. |
| `par_set_u8()` / `par_set_i8()` | 8-bit typed setters. |
| `par_set_u16()` / `par_set_i16()` | 16-bit typed setters. |
| `par_set_u32()` / `par_set_i32()` | 32-bit typed setters. |
| `par_set_f32()` | Floating-point setter when `PAR_CFG_ENABLE_TYPE_F32` is enabled. |

## Fast scalar setters

Fast setters such as `par_set_u8_fast()` and `par_set_u32_fast()` are intended for trusted internal code paths. They should not be used as the default API for external commands or remote interfaces.

Bitwise helpers are available for unsigned scalar values:

- `par_bitand_set_u8_fast()` / `par_bitor_set_u8_fast()`
- `par_bitand_set_u16_fast()` / `par_bitor_set_u16_fast()`
- `par_bitand_set_u32_fast()` / `par_bitor_set_u32_fast()`

## Scalar getters

| API family | Purpose |
| --- | --- |
| `par_get_scalar()` | Pointer-based scalar getter. |
| `par_get_scalar_by_id()` | ID-based scalar getter when ID support is enabled. |
| `par_get_u8()` / `par_get_i8()` | 8-bit typed getters. |
| `par_get_u16()` / `par_get_i16()` | 16-bit typed getters. |
| `par_get_u32()` / `par_get_i32()` | 32-bit typed getters. |
| `par_get_f32()` | Floating-point getter when `PAR_CFG_ENABLE_TYPE_F32` is enabled. |
| `par_get_scalar_default()` | Read scalar default value from the generated table. |

All public getters use output pointers and return `par_status_t`.

## Object APIs

Object APIs are compiled only when object type support is enabled.

| Type | Set API | Get API | Default API |
| --- | --- | --- | --- |
| Bytes | `par_set_bytes()` | `par_get_bytes()` | `par_get_default_bytes()` |
| String | `par_set_str()` | `par_get_str()` | `par_get_default_str()` |
| `U8` array | `par_set_arr_u8()` | `par_get_arr_u8()` | `par_get_default_arr_u8()` |
| `U16` array | `par_set_arr_u16()` | `par_get_arr_u16()` | `par_get_default_arr_u16()` |
| `U32` array | `par_set_arr_u32()` | `par_get_arr_u32()` | `par_get_default_arr_u32()` |

Additional object metadata APIs:

- `par_get_obj_len()`
- `par_get_obj_capacity()`
- ID-based variants when `PAR_CFG_ENABLE_ID` is enabled.

## Metadata APIs

| API | Availability |
| --- | --- |
| `par_get_config()` | Always available. |
| `par_get_name()` | `PAR_CFG_ENABLE_NAME` |
| `par_get_range()` | `PAR_CFG_ENABLE_RANGE` |
| `par_get_unit()` | `PAR_CFG_ENABLE_UNIT` |
| `par_get_desc()` | `PAR_CFG_ENABLE_DESC` |
| `par_get_type()` | Always available. |
| `par_get_access()` | `PAR_CFG_ENABLE_ACCESS` |
| `par_is_persistent()` | `PAR_CFG_NVM_EN` |
| `par_get_num_by_id()` / `par_get_id_by_num()` | `PAR_CFG_ENABLE_ID` |

## Role policy APIs

When `PAR_CFG_ENABLE_ROLE_POLICY` is enabled:

- `par_get_read_roles()`
- `par_get_write_roles()`
- `par_can_read()`
- `par_can_write()`

These APIs evaluate metadata. The integration layer still owns the current caller identity and enforcement point.

## Registration APIs

| API | Availability | Purpose |
| --- | --- | --- |
| `par_register_on_change_cb()` | `PAR_CFG_ENABLE_CHANGE_CALLBACK` | Register a callback for value changes. |
| `par_register_validation()` | `PAR_CFG_ENABLE_RUNTIME_VALIDATION` | Register scalar runtime validation. |
| `par_register_obj_validation()` | `PAR_CFG_ENABLE_RUNTIME_VALIDATION` and `PAR_CFG_OBJECT_TYPES_ENABLED` | Register object runtime validation. |

Registration changes are not serialized against concurrent callback dispatch by the core. Register hooks during single-threaded setup or under application-level synchronization.

## Reset and change tracking

Default restore APIs are maintenance/recovery paths, not external write paths. They intentionally bypass external write-access checks and role-policy checks. `par_set_to_default()` and the non-raw `par_set_all_to_default()` path also bypass runtime validation callbacks and on-change callbacks by using fast default-restore semantics.

| API | Purpose |
| --- | --- |
| `par_set_to_default()` | Restore one parameter to its configured default through the fast default-restore path. |
| `par_set_all_to_default()` | Restore all parameters; uses `par_reset_all_to_default_raw()` when raw reset is enabled, otherwise iterates through `par_set_to_default()`. |
| `par_reset_all_to_default_raw()` | Restore all defaults through the raw grouped-storage maintenance path when enabled; bypasses setter-side range behavior as well. |
| `par_has_changed()` | Query whether a parameter has changed. |

## NVM APIs

Compiled only when `PAR_CFG_NVM_EN` is enabled.

| API | Purpose |
| --- | --- |
| `par_set_scalar_n_save()` | Set a scalar and save it. |
| `par_save()` | Save one parameter by internal number. |
| `par_save_by_id()` | Save one parameter by external ID. |
| `par_save_all()` | Save all persistent parameters. |
| `par_save_clean()` | Clean or compact backend data when supported. |

Backend APIs are intentionally separated from the parameter API. See [Flash-ee backend design](./flash-ee-backend-design.md) and [RT-Thread package integration](./rt-thread-package.md).
