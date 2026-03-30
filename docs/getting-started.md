# Getting started

This guide shows how to integrate the `Device Parameters` module into a firmware project, which files you must provide, and which configuration choices matter first.

## Integration checklist

1. Add `src/par.c` and the needed sources from `src/def`, `src/layout`, `src/persist`, and `src/port` to your project. Add `src/persist/backend` only when you use the packaged backend adapter.
2. Provide `par_table.def` at the package root.
3. Provide `port/par_cfg_port.h`.
4. Decide whether you want:
   - NVM persistence and which storage backend will implement it
   - a platform-specific interface backend
   - a platform-specific atomic backend
   - compile-scan or script-provided layout
   - raw reset-all support
   - `F32` parameter support
5. Call `par_init()` before runtime access.
6. Use the typed APIs. Getter calls take an explicit output pointer and return `par_status_t`.

## Required files

### `par_table.def`

`par_table.def` is the single source of truth for parameter definitions.

Each row defines one parameter and is reused to build:

- `par_num_t`
- the parameter configuration table
- compile-time integer validation checks
- compile-time storage counts

A minimal example:

```c
PAR_ITEM_U8 (
    ePAR_MODE,
    10,
    "Mode",
    0U,
    3U,
    0U,
    NULL,
    ePAR_ACCESS_RW,
    true,
    "Application operating mode"
)

PAR_ITEM_F32(
    ePAR_TARGET_TEMP,
    20,
    "Target temperature",
    -40.0f,
    125.0f,
    25.0f,
    "degC",
    ePAR_ACCESS_RW,
    true,
    "Requested control target temperature"
)
```

Use `template/par_table.deftmp` as the starting point.

This example requires `PAR_CFG_ENABLE_TYPE_F32 = 1`. If `F32` support is disabled, remove all `PAR_ITEM_F32(...)` rows from `par_table.def`.

### `port/par_cfg_port.h`

`src/par_cfg.h` includes `par_cfg_port.h` unconditionally.

If you do not need platform overrides yet, start with a minimal stub:

```c
#ifndef _PAR_CFG_PORT_H_
#define _PAR_CFG_PORT_H_
/* Optional platform overrides */
#endif
```

Use `template/par_cfg_port.htmp` as the starting point.

Keep `parameters/src` on the compiler include path so application code can include `par.h`. Also add the directory that contains your integration-owned `par_cfg_port.h` (and optionally `par_atomic_port.h`) to the compiler include path.

## Optional integration files

### `port/par_if_port.c`

Provide this file only when `PAR_CFG_IF_PORT_EN = 1` **and** your target needs to override the core weak defaults.
Compile it as a normal source file (do not `#include` the `.c` file from core code).

Use it to integrate platform-specific services such as:

- initialization hooks
- mutex handling
- optional platform hooks unrelated to the core ID lookup hash map

The ID lookup hash used by `par_get_by_id()` / `par_set_by_id()` is part of the core module and is generated at compile time, not supplied by `port/par_if_port.c`.
Do not confuse it with optional table-hash support used by NVM compatibility features.

### `port/par_atomic_port.h`

Provide this file only when the default C11 atomic backend is not suitable.

Use it when:

- your compiler does not support `<stdatomic.h>` well enough
- your RTOS already provides atomic primitives
- you want all atomic operations mapped to a platform-native implementation

Enable it with:

```c
#define PAR_ATOMIC_BACKEND PAR_ATOMIC_BACKEND_PORT
```

### Static layout header

Provide a generated static layout header only when:

```c
#define PAR_CFG_LAYOUT_SOURCE PAR_CFG_LAYOUT_SCRIPT
```

Use `template/par_layout_static.htmp` as the contract for the generated file.

The template includes the required count macros and offset-table declaration. It can also serve as the starting point for a project-specific file header banner.

## Configuration decisions that matter first

### NVM support

Enable NVM only when you actually need persistent parameters.

Relevant options in `par_cfg.h`:

- `PAR_CFG_NVM_EN`
- `PAR_CFG_NVM_REGION`
- `PAR_CFG_ENABLE_ID`
- `PAR_CFG_ENABLE_PERSIST`

ID-based lookup is generated statically when `PAR_CFG_ENABLE_ID = 1`. Optional startup diagnostics can be enabled with:

- `PAR_CFG_ENABLE_RUNTIME_ID_DUP_CHECK`
- `PAR_CFG_ENABLE_RUNTIME_ID_HASH_COLLISION_CHECK`

When NVM is enabled, the parameters module requires a concrete storage backend implementation. `src/persist/par_nvm.c` resolves and validates the backend API once during initialization, then uses the mounted callbacks directly for later reads, writes, erases, and sync operations. The package can build the `GeneralEmbeddedCLibraries/nvm` adapter from `parameters/src/persist/backend/`, or the application can provide `par_store_backend_get_api()` itself. The module can reuse an already-initialized backend or initialize it on demand and later deinitialize it only when it owns that initialization. Module deinit is conservative: it attempts backend and interface cleanup, and it clears the top-level module init state only after the owned child deinit steps succeed.

Backend choices:

- enable the packaged `GeneralEmbeddedCLibraries/nvm` adapter
- provide exactly one application-owned `par_store_backend_get_api()` implementation

If `PAR_CFG_NVM_EN = 1` and no backend implementation is linked, the build fails at link time by design.

### Layout source

Choose one of these two modes:

```c
#define PAR_CFG_LAYOUT_COMPILE_SCAN   (0u)
#define PAR_CFG_LAYOUT_SCRIPT         (1u)
```

Use **compile scan** when parameter definitions live in source and you want the module to derive offsets at initialization.

Use **script layout** when your build or tooling already generates fixed layout data before compilation.

### Atomic backend

The default backend is C11 atomics. Switch to the port backend only when the default is not a good fit for the target.

### Runtime hooks

Use these options to control whether normal setters include runtime validation and on-change notifications:

```c
#define PAR_CFG_ENABLE_RUNTIME_VALIDATION ( 1 )
#define PAR_CFG_ENABLE_CHANGE_CALLBACK    ( 1 )
```

`PAR_CFG_ENABLE_RUNTIME_VALIDATION` controls whether normal setters call per-parameter validation callbacks registered through `par_register_validation()`.

`PAR_CFG_ENABLE_CHANGE_CALLBACK` controls whether normal setters raise per-parameter change callbacks registered through `par_register_on_change_cb()`.

These options are independent:

- set both to `1` to keep the full normal setter behavior
- set either one to `0` to compile out that part of the runtime hook path
- fast setters still skip these hooks regardless of configuration

When `PAR_CFG_ENABLE_RUNTIME_VALIDATION = 0`, `par_register_validation()` is not available.

When `PAR_CFG_ENABLE_CHANGE_CALLBACK = 0`, `par_register_on_change_cb()` is not available.


### Raw reset-all API
Use `PAR_CFG_ENABLE_RESET_ALL_RAW` to control whether the raw reset-all API and default mirror storage are compiled in.

```c
#define PAR_CFG_ENABLE_RESET_ALL_RAW ( 1 )
```

When `PAR_CFG_ENABLE_RESET_ALL_RAW = 1`:

* `par_reset_all_to_default_raw()` is available
* a grouped default mirror snapshot is kept in RAM for raw restore
* the grouped snapshot preserves the internal `U8/U16/U32` width-group storage layout
* `F32` defaults are mirrored into the 32-bit storage group as bit patterns after layout is known

`par_reset_all_to_default_raw()` restores live values by copying the grouped storage snapshot and intentionally bypasses runtime validation callbacks, on-change callbacks, and range logic.

The grouped default mirror snapshot is built before optional NVM load, so `par_reset_all_to_default_raw()` restores configured defaults, not persisted runtime values loaded from NVM.

### F32 type support
Use `PAR_CFG_ENABLE_TYPE_F32` to control whether `F32` parameters are compiled into the module.

```c
#define PAR_CFG_ENABLE_TYPE_F32 ( 1 )
```

Set it to `0` only when your integration does not need floating-point parameters.

When `PAR_CFG_ENABLE_TYPE_F32 = 0`:

* `PAR_ITEM_F32(...)` entries are not allowed in `par_table.def`
* `par_set_f32()`, `par_get_f32()`, and `par_set_f32_fast()` are not available
* startup F32 default patching is skipped

## Initialization

Call `par_init()` before any runtime parameter access.

```c
if (par_init() != ePAR_OK)
{
    /* Handle error */
}
```

If `PAR_CFG_NVM_EN = 1`, NVM loading happens after the module applies default values from `par_table.def`, so persisted values can overwrite the startup defaults.
When raw reset-all is enabled, its grouped default mirror snapshot is built before that optional NVM load, so raw reset returns live storage to the configured defaults rather than to persisted NVM-loaded values.

### How `par_init()` applies default values

`par_init()` validates the table, binds the storage layout, initializes the interface layer, applies default values to live storage, and then optionally loads persisted values from NVM.

During startup:

- integer default values defined in `par_table.def` are already present in the grouped live storage object at definition time
- when `PAR_CFG_ENABLE_TYPE_F32 = 1`, `F32` default values are written into the grouped 32-bit storage member after layout offsets are known
- if NVM support is enabled, persisted values may then overwrite those default values

Do not rely on startup initialization to trigger application callbacks or runtime validation hooks.

## Reading and writing values

The `F32` examples in this section require `PAR_CFG_ENABLE_TYPE_F32 = 1`.

### Use the typed APIs in normal application code
```c
(void)par_set_f32(ePAR_TARGET_TEMP, (float32_t)42.5f);

float32_t target_temp = 0.0f;
(void)par_get_f32(ePAR_TARGET_TEMP, &target_temp);
```

### Use typed APIs when explicitness matters

```c
(void)par_set_u16(ePAR_PWM_LIMIT, 1024U);
uint16_t pwm_limit = 0U;
(void)par_get_u16(ePAR_PWM_LIMIT, &pwm_limit);
```

### Use pointer-based generic APIs only when needed

```c
float32_t value = 12.0f;
(void)par_set(ePAR_TARGET_TEMP, &value);

float32_t readback = 0.0f;
(void)par_get(ePAR_TARGET_TEMP, &readback);
```

## Registering callbacks and validation

The registration APIs work per parameter and take the parameter number directly.

`par_register_on_change_cb()` is available only when `PAR_CFG_ENABLE_CHANGE_CALLBACK = 1`.

`par_register_validation()` is available only when `PAR_CFG_ENABLE_RUNTIME_VALIDATION = 1`.

### On-change callback

Use this only when `PAR_CFG_ENABLE_CHANGE_CALLBACK = 1`. Keep the callback synchronous, short, and non-blocking. Avoid long-running I/O, waits, sleeps, or other operations that may extend parameter-module lock hold time.

```c
#if (1 == PAR_CFG_ENABLE_CHANGE_CALLBACK)
static void on_mode_changed(
    const par_num_t par_num,
    const par_type_t new_val,
    const par_type_t old_val)
{
    (void)par_num;
    /* React to the change */
}

static void app_register_callbacks(void)
{
    par_register_on_change_cb(ePAR_MODE, on_mode_changed);
}
#endif
```

### Validation callback

Use this only when `PAR_CFG_ENABLE_RUNTIME_VALIDATION = 1`. Keep validation logic synchronous, short, and non-blocking. Avoid long-running I/O, waits, sleeps, or other operations that may extend parameter-module lock hold time.

```c
#if (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION)
static bool validate_target_temp(const par_num_t par_num, const par_type_t val)
{
    (void)par_num;

    return (val.f32 >= -20.0f) && (val.f32 <= 100.0f);
}

static void app_register_validation(void)
{
    par_register_validation(ePAR_TARGET_TEMP, validate_target_temp);
}
#endif
```

## Normal vs fast setters

Use the normal setters unless you have a measured reason not to.

### Normal setters

Normal setters go through the normal runtime path.

Depending on build-time configuration, that path can include runtime validation callbacks and on-change callbacks:

- runtime validation callbacks are used only when `PAR_CFG_ENABLE_RUNTIME_VALIDATION = 1`
- on-change callbacks are raised only when `PAR_CFG_ENABLE_CHANGE_CALLBACK = 1`

```c
(void)par_set_f32(ePAR_TARGET_TEMP, 25.0f);
```

### Fast setters

Fast setters are meant for controlled hot paths where you accept reduced safety or observability in exchange for lower overhead. They do not run runtime validation callbacks or on-change callbacks.
```c
(void)par_set_u16_fast(ePAR_PWM_LIMIT, 1200U);
```

When you only have a typed value pointer and still want the unchecked path, use the generic fast dispatcher:
```c
float32_t value = 12.0f;
(void)par_set_fast(ePAR_TARGET_TEMP, &value);
```

Do not use fast setters as the default API for ordinary application code.

### Bitwise fast setters

Bitwise fast setters are the flags-only variant of the fast path. Use them only for `U8` / `U16` / `U32` parameters that represent bitmasks or status flags. They intentionally bypass runtime validation callbacks, on-change callbacks, and normal setter range semantics.

```c
(void)par_bitor_set_u32_fast(ePAR_STATUS_FLAGS, STATUS_FLAG_READY);
(void)par_bitand_set_u32_fast(ePAR_STATUS_FLAGS, (uint32_t)(~STATUS_FLAG_ERROR));
```

Good fits are enable masks, fault flags, and mode bits. Do not use bitwise fast setters as a substitute for ordinary numeric writes such as temperature, current limits, or thresholds.

## Persistence to NVM

When NVM support is enabled and a storage backend is linked, use the NVM APIs for storing current values.

```c
if (par_save_all() != ePAR_OK)
{
    /* Handle storage error */
}
```

Or update and store one parameter in one step:

```c
uint32_t baud = 115200U;
(void)par_set_n_save(ePAR_UART_BAUD, &baud);
```

## Common mistakes to avoid

- Forgetting to provide `par_cfg_port.h`
- Treating `par_num_t` as a stable external interface
- Using fast setters before understanding their tradeoffs
- Enabling NVM without linking any concrete parameter-storage backend
- Writing `par_table.def` entries with duplicate IDs
- Assigning different external IDs that still resolve to the same ID hash bucket
- Changing external IDs without rebuilding and checking the compile-time ID-map validation output
- Assuming the repository already ships a ready-to-build `par_table.def` for your project
- Disabling `PAR_CFG_ENABLE_TYPE_F32` while keeping `PAR_ITEM_F32(...)` entries in `par_table.def`
- Assuming `par_set_f32()` and `par_get_f32()` are still available after F32 support is disabled
- Registering validation or change callbacks without enabling the matching configuration macro

### Compile-time error example when F32 support is disabled

If `PAR_CFG_ENABLE_TYPE_F32 = 0` and `par_table.def` still contains `PAR_ITEM_F32(...)`, the build fails with a static assertion.

Example:

```log
def.h:124:51: error: size of array '_static_assert_ePAR_SYS_CPU_LOAD_MAX_f32_type_is_disabled__remove_PAR_ITEM_F32' is negative
  124 | #define _STATIC_ASSERT(name, expn) typedef char _static_assert_##name[(expn)?1:-1]
      |                                                   ^~~~~~~~~~~~~~~
port/par_cfg_port.h:130:44: note: in expansion of macro '_STATIC_ASSERT'
  130 | #define PAR_PORT_STATIC_ASSERT(name, expn) _STATIC_ASSERT(name, expn)
      |                                            ^~~~~~~~~~~~~~~~
src/par_cfg.h:160:53: note: in expansion of macro 'PAR_PORT_STATIC_ASSERT'
  160 | #define PAR_STATIC_ASSERT(name, expn)               PAR_PORT_STATIC_ASSERT(name, expn);
      |                                                     ^~~~~~~~~~~~~~~~~~~~~~
src/def/par_def.c:73:94: note: in expansion of macro 'PAR_STATIC_ASSERT'
   73 |     #define PAR_CHECK_F32(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_) PAR_STATIC_ASSERT(enum_##_f32_type_is_disabled__remove_PAR_ITEM_F32, 0)
      |                                                                                              ^~~~~~~~~~~~~~~~~
src/def/par_def.c:85:23: note: in expansion of macro 'PAR_CHECK_F32'
   85 | #define PAR_ITEM_F32  PAR_CHECK_F32
      |                       ^~~~~~~~~~~~~
par_table.def:189:1: note: in expansion of macro 'PAR_ITEM_F32'
  189 | PAR_ITEM_F32(ePAR_SYS_CPU_LOAD_MAX, 10011,  "CPU Max. load",               0.0f,        100.0f,     0.0f,       "%",    ePAR_ACCESS_RO,  false,  "Maximum CPU load in %")
      | ^~~~~~~~~~~~
```

Fix the table first: remove the `PAR_ITEM_F32(...)` entry or re-enable `PAR_CFG_ENABLE_TYPE_F32`.

### Compile-time error example when ID hash buckets collide

The build also fails when two different external IDs resolve to the same hash bucket, because the static ID map requires a collision-free table under the configured hash geometry.

This does not mean the IDs are equal.
It means the current one-entry-per-bucket ID map cannot represent both rows at the same time.

Example:

```log
par_table.def: In function 'par_compile_check_hash_bucket_collision':
src/def/par_def.c:156:105: error: duplicate case value
  156 | #define PAR_CHECK_ID_BUCKET_CASE(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_) case PAR_HASH_ID_CONST(id_): break;
      |                                                                                                         ^~~~
src/def/par_def.c:162:31: note: in expansion of macro 'PAR_CHECK_ID_BUCKET_CASE'
  162 |         #define PAR_ITEM_U16  PAR_CHECK_ID_BUCKET_CASE
      |                               ^~~~~~~~~~~~~~~~~~~~~~~~
par_table.def:141:1: note: in expansion of macro 'PAR_ITEM_U16'
  141 | PAR_ITEM_U16(ePAR_CH3_VOL_RAW, 253, "Ch3 Raw Vout", ...)
      | ^~~~~~~~~~~~
src/def/par_def.c:156:105: note: previously used here
  156 | #define PAR_CHECK_ID_BUCKET_CASE(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_) case PAR_HASH_ID_CONST(id_): break;
      |                                                                                                         ^~~~
src/def/par_def.c:167:31: note: in expansion of macro 'PAR_CHECK_ID_BUCKET_CASE'
  167 |         #define PAR_ITEM_F32  PAR_CHECK_ID_BUCKET_CASE
      |                               ^~~~~~~~~~~~~~~~~~~~~~~~
par_table.def:54:1: note: in expansion of macro 'PAR_ITEM_F32'
   54 | PAR_ITEM_F32(ePAR_CH1_TSIM, 20, "Ch1 Ref Temperature", ...)
      | ^~~~~~~~~~~~
```

Read this error as:

* the two IDs are different
* but `PAR_HASH_ID_CONST(253)` and `PAR_HASH_ID_CONST(20)` produced the same bucket index
* the current ID map cannot accept both rows

Fix the table first:

1. keep IDs unique
2. change one of the conflicting external IDs in `par_table.def`
3. rebuild until the compile-time collision check no longer fails

If collisions become frequent, reconsider the ID assignment policy or replace the one-entry-per-bucket map with a probing-based implementation.
