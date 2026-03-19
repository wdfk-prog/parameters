# API reference

This document groups the public API from `src/par.h` by responsibility.

## Conventions

- Most runtime APIs require `par_init()` to be called first.
- Some APIs are compiled only when the matching configuration option is enabled.
- `par_num_t` is the internal parameter index.
- ID-based APIs depend on `PAR_CFG_ENABLE_ID = 1`.
- NVM APIs depend on `PAR_CFG_NVM_EN = 1`.
- `F32` typed APIs and `_Generic` float dispatch depend on `PAR_CFG_ENABLE_TYPE_F32 = 1`.

## Compile-time availability notes

The module conditionally compiles parts of the API based on configuration.

- `PAR_CFG_NVM_EN = 1` enables NVM APIs
- `PAR_CFG_ENABLE_ID = 1` enables ID-dependent behavior
- `PAR_CFG_ENABLE_TYPE_F32 = 1` enables:
  - `par_set_f32()`
  - `par_get_f32()`
  - `par_set_f32_fast()`
  - `float32_t` dispatch through `PAR_SET` and `PAR_GET`

## Lifecycle

| Function | Description |
| --- | --- |
| `par_init()` | Initialize the module, validate the table, bind layout/runtime state, apply default values to live storage, and optionally load persisted values from NVM. Startup defaults are applied internally and do not use the public setter path. |
| `par_deinit()` | Deinitialize the module. |
| `par_is_init()` | Return whether the module is initialized. |

## Mutex helpers

| Function | Description |
| --- | --- |
| `par_acquire_mutex(par_num)` | Acquire the parameter lock for a specific parameter path. |
| `par_release_mutex(par_num)` | Release the parameter lock. |

These are relevant only when mutex support is enabled in the integration.

## Generic setters

| Function | Description |
| --- | --- |
| `par_set(par_num, p_val)` | Set a parameter from a typed pointer. |
| `par_set_by_id(id, p_val)` | Set a parameter using its external ID. |
| `PAR_SET(par_num, value)` | Use C11 `_Generic` to route a typed value to the matching setter. `float32_t` dispatch is available only when `PAR_CFG_ENABLE_TYPE_F32 = 1`. |


## Typed setters

| Function | Description |
| --- | --- |
| `par_set_u8()` | Set a `U8` parameter. |
| `par_set_i8()` | Set an `I8` parameter. |
| `par_set_u16()` | Set a `U16` parameter. |
| `par_set_i16()` | Set an `I16` parameter. |
| `par_set_u32()` | Set a `U32` parameter. |
| `par_set_i32()` | Set an `I32` parameter. |
| `par_set_f32()` | Set an `F32` parameter. Available only when `PAR_CFG_ENABLE_TYPE_F32 = 1`. |

## Fast setters

| Function | Description |
| --- | --- |
| `par_set_u8_fast()` | Fast set for `U8`. |
| `par_set_i8_fast()` | Fast set for `I8`. |
| `par_set_u16_fast()` | Fast set for `U16`. |
| `par_set_i16_fast()` | Fast set for `I16`. |
| `par_set_u32_fast()` | Fast set for `U32`. |
| `par_set_i32_fast()` | Fast set for `I32`. |
| `par_set_f32_fast()` | Fast set for `F32`. Available only when `PAR_CFG_ENABLE_TYPE_F32 = 1`. |

Use these only in controlled hot paths.

## Fast bitwise update helpers

| Function | Description |
| --- | --- |
| `par_bitand_set_u8_fast()` | Fast bitwise AND update for `U8`. |
| `par_bitand_set_u16_fast()` | Fast bitwise AND update for `U16`. |
| `par_bitand_set_u32_fast()` | Fast bitwise AND update for `U32`. |
| `par_bitor_set_u8_fast()` | Fast bitwise OR update for `U8`. |
| `par_bitor_set_u16_fast()` | Fast bitwise OR update for `U16`. |
| `par_bitor_set_u32_fast()` | Fast bitwise OR update for `U32`. |

## Reset and change tracking

| Function | Description |
| --- | --- |
| `par_set_to_default(par_num)` | Reset one parameter to its default value. |
| `par_set_all_to_default()` | Reset all parameters to their default values. |
| `par_has_changed(par_num, p_has_changed)` | Report whether the value differs from its default. |
| `par_is_changed(par_num)` | Return whether the value differs from its default. |

`par_set_to_default()` and `par_set_all_to_default()` are runtime reset APIs.

They are different from startup initialization:

- `par_init()` applies the default values defined in `par_table.def` directly to live storage
- `par_set_to_default()` and `par_set_all_to_default()` still use the normal runtime value path

That distinction matters if your application depends on validation callbacks, on-change callbacks, or other setter-side effects.

## Generic getters

| Function | Description |
| --- | --- |
| `par_get(par_num, p_val)` | Read a parameter into a typed destination pointer. |
| `par_get_by_id(id, p_val)` | Read a parameter using its external ID. |
| `PAR_GET(par_num, dest)` | Use C11 `_Generic` to route a destination variable to the matching getter. `float32_t` dispatch is available only when `PAR_CFG_ENABLE_TYPE_F32 = 1`. |

## Typed getters

| Function | Description |
| --- | --- |
| `par_get_u8()` | Read a `U8` parameter. |
| `par_get_i8()` | Read an `I8` parameter. |
| `par_get_u16()` | Read a `U16` parameter. |
| `par_get_i16()` | Read an `I16` parameter. |
| `par_get_u32()` | Read a `U32` parameter. |
| `par_get_i32()` | Read an `I32` parameter. |
| `par_get_f32()` | Read an `F32` parameter. Available only when `PAR_CFG_ENABLE_TYPE_F32 = 1`. |
| `par_get_default(par_num, p_val)` | Read the configured default value for a parameter. |

## Metadata access

These APIs do not follow the same runtime usage pattern as the value access APIs. They expose parameter metadata from the configuration table.

| Function | Description |
| --- | --- |
| `par_get_config(par_num)` | Return the full configuration object for one parameter. |
| `par_get_name(par_num)` | Return the display name when name metadata is enabled. |
| `par_get_range(par_num)` | Return the configured min/max range when range metadata is enabled. |
| `par_get_unit(par_num)` | Return the engineering unit when unit metadata is enabled. |
| `par_get_desc(par_num)` | Return the description string when description metadata is enabled. |
| `par_get_type(par_num)` | Return the parameter type enum. |
| `par_get_access(par_num)` | Return read-only or read-write access metadata when enabled. |
| `par_is_persistant(par_num)` | Return whether the parameter is marked persistent when enabled. |
| `par_get_num_by_id(id, p_par_num)` | Convert an external ID to `par_num_t`. |
| `par_get_id_by_num(par_num, p_id)` | Convert `par_num_t` to external ID. |

## NVM APIs

Available only when `PAR_CFG_NVM_EN = 1`.

| Function | Description |
| --- | --- |
| `par_set_n_save(par_num, p_val)` | Set one parameter and persist it immediately. |
| `par_save_all()` | Persist all persistent parameters. |
| `par_save(par_num)` | Persist one parameter. |
| `par_save_by_id(par_id)` | Persist one parameter by external ID. |
| `par_save_clean()` | Rewrite the full NVM area managed by the module. |

## Registration APIs

These APIs register behavior per parameter.

| Function | Description |
| --- | --- |
| `par_register_on_change_cb(par_num, cb)` | Register a change callback for one parameter. |
| `par_register_validation(par_num, validation)` | Register a validation callback for one parameter. |

Example:

```c
static void on_mode_change(
    const par_num_t par_num,
    const par_type_t new_val,
    const par_type_t old_val)
{
    (void)par_num;
    (void)new_val;
    (void)old_val;
}

static bool validate_mode(const par_num_t par_num, const par_type_t val)
{
    (void)par_num;
    return (val.u8 <= 3U);
}

static void app_hooks_init(void)
{
    par_register_on_change_cb(ePAR_MODE, on_mode_change);
    par_register_validation(ePAR_MODE, validate_mode);
}
```

These hooks affect runtime writes and explicit reset operations. They are not invoked during the internal startup default initialization performed by `par_init()`.

## Debug helpers

Available only when debug support is enabled.

| Function | Description |
| --- | --- |
| `par_get_status_str(status)` | Convert a status code to a debug string. |

## Status categories

`par_status_t` combines normal status, errors, and warnings.

Common values include:

- `ePAR_OK`
- `ePAR_ERROR`
- `ePAR_ERROR_INIT`
- `ePAR_ERROR_NVM`
- `ePAR_ERROR_CRC`
- `ePAR_ERROR_TYPE`
- `ePAR_ERROR_MUTEX`
- `ePAR_ERROR_VALUE`
- `ePAR_WAR_SET_TO_DEF`
- `ePAR_WAR_NVM_REWRITTEN`
- `ePAR_WAR_NO_PERSISTANT`
- `ePAR_WAR_LIMITED`
