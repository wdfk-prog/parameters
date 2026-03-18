# Getting started

This guide shows how to integrate the `Device Parameters` module into a firmware project, which files you must provide, and which configuration choices matter first.

## Integration checklist

1. Add `src/*.c` and `src/*.h` to your project.
2. Provide `par_table.def` at the package root.
3. Provide `port/par_cfg_port.h`.
4. Decide whether you want:
   - NVM persistence
   - a platform-specific interface backend
   - a platform-specific atomic backend
   - compile-scan or script-provided layout
5. Call `par_init()` before runtime access.
6. Use typed APIs or the generic `PAR_SET` and `PAR_GET` macros.

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

## Optional integration files

### `port/par_if_port.c`

Provide this file only when `PAR_CFG_IF_PORT_EN = 1`.

Use it to integrate platform-specific services such as:

- initialization hooks
- mutex handling
- optional table hash support

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

## Configuration decisions that matter first

### NVM support

Enable NVM only when you actually need persistent parameters.

Relevant options in `par_cfg.h`:

- `PAR_CFG_NVM_EN`
- `PAR_CFG_NVM_REGION`
- `PAR_CFG_ENABLE_ID`
- `PAR_CFG_ENABLE_PERSIST`

When NVM is enabled, the external NVM module must be present in the project.

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

## Initialization

Call `par_init()` before any runtime parameter access.

```c
if (par_init() != ePAR_OK)
{
    /* Handle error */
}
```

If `PAR_CFG_NVM_EN = 1`, NVM loading happens after the module applies default values from `par_table.def`, so persisted values can overwrite the startup defaults.

### How `par_init()` applies default values

`par_init()` validates the table, binds the storage layout, initializes the interface layer, applies default values to live storage, and then optionally loads persisted values from NVM.

During startup:

- integer default values defined in `par_table.def` are already present in the shared storage arrays at definition time
- `F32` default values are written after layout offsets are known
- if NVM support is enabled, persisted values may then overwrite those default values

Do not rely on startup initialization to trigger application callbacks or runtime validation hooks.

## Reading and writing values

### Use the generic macros in normal application code

```c
PAR_SET(ePAR_TARGET_TEMP, (float32_t)42.5f);

float32_t target_temp = 0.0f;
PAR_GET(ePAR_TARGET_TEMP, target_temp);
```

### Use typed APIs when explicitness matters

```c
(void)par_set_u16(ePAR_PWM_LIMIT, 1024U);
uint16_t pwm_limit = par_get_u16(ePAR_PWM_LIMIT);
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

### On-change callback

```c
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
```

### Validation callback

```c
static bool validate_target_temp(const par_num_t par_num, const par_type_t val)
{
    (void)par_num;

    return (val.f32 >= -20.0f) && (val.f32 <= 100.0f);
}

static void app_register_validation(void)
{
    par_register_validation(ePAR_TARGET_TEMP, validate_target_temp);
}
```

## Normal vs fast setters

Use the normal setters unless you have a measured reason not to.

### Normal setters

Normal setters go through the full parameter path, including checks and side effects such as validation and change tracking.

```c
(void)par_set_f32(ePAR_TARGET_TEMP, 25.0f);
```

### Fast setters

Fast setters are meant for controlled hot paths where you accept reduced safety or observability in exchange for lower overhead.

```c
(void)par_set_u16_fast(ePAR_PWM_LIMIT, 1200U);
```

Do not use fast setters as the default API for ordinary application code.

## Persistence to NVM

When NVM support is enabled, use the NVM APIs for storing current values.

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
- Enabling NVM without the external NVM module in the build
- Writing `par_table.def` entries with duplicate IDs
- Assuming the repository already ships a ready-to-build `par_table.def` for your project
