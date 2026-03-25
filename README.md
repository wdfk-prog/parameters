# Device Parameters

`Device Parameters` is a portable embedded C module for managing runtime parameters, validation, metadata, and optional NVM persistence from a single parameter definition table.

It is designed for projects that need a clean way to:

- define parameters once
- read and write them through a consistent API
- expose them to CLI, PC tools, or protocol layers by stable external IDs
- validate values before they are accepted
- persist selected values to NVM
- keep memory usage predictable on embedded targets

## What this module provides

- **Single source of truth** through `par_table.def`
- **Typed APIs** for `U8`, `I8`, `U16`, `I16`, `U32`, `I32`, and, when enabled, `F32`
- **Optional metadata** such as name, unit, description, access, ID, and persistence flags
- **Validation pipeline** with compile-time checks for integer ranges and optional runtime hooks for dynamic rules
- **Static live-value storage** grouped by width instead of heap allocation
- **Fast external lookup by ID** through a compile-time generated static hash map
- **Optional NVM integration** for persistent parameters
- **Portable core + platform hooks** for RTOS, mutex, logging, assertions, and atomic backends

## Start here

### Quick start

1. Add the core sources from `src/` to your build.
2. Provide a project-specific `par_table.def` at the package root.
3. Provide `port/par_cfg_port.h` in your include path.
4. Optionally provide `port/par_if_port.c` and `port/par_atomic_port.h` when your platform needs them.
5. Call `par_init()` before using runtime APIs.
6. Use the typed macro wrappers such as `PAR_SET_U16` and `PAR_GET_U16`, or the typed `par_set_*` / `par_get_*` APIs in application code.

A minimal example:

This quick-start example assumes `PAR_CFG_ENABLE_TYPE_F32 = 1`.

```c
#include "par.h"

static void app_init(void)
{
    if (par_init() != ePAR_OK)
    {
        /* Handle initialization error */
    }

    PAR_SET_F32(ePAR_CH1_REF_VAL, (float32_t)25.0f);

    float32_t ref_val = 0.0f;
    PAR_GET_F32(ePAR_CH1_REF_VAL, ref_val);
}
```

## Documentation map

- [Getting started](docs/getting-started.md) for integration steps, required files, and first-use examples
- [Architecture](docs/architecture.md) for storage model, validation flow, ID lookup, and layout design
- [API reference](docs/api-reference.md) for the public API grouped by responsibility

## Package layout

```text
parameters/
├── README.md
├── CHANGE_LOG.md
├── doc/
│   └── DeviceParameter_VerificationReport.xlsx
├── docs/
│   ├── api-reference.md
│   ├── architecture.md
│   └── getting-started.md
├── src/
│   ├── par.c
│   ├── par.h
│   ├── par_atomic.h
│   ├── par_bitwise_impl.inc
│   ├── par_cfg.h
│   ├── par_def.c
│   ├── par_def.h
│   ├── par_if.c
│   ├── par_if.h
│   ├── par_layout.c
│   ├── par_layout.h
│   ├── par_nvm.c
│   ├── par_nvm.h
│   ├── par_storage_init.inc
│   └── par_typed_impl.inc
└── template/
    ├── par_cfg_port.htmp
    ├── par_layout_static.htmp
    └── par_table.deftmp
```

## Required integration files

This repository contains the reusable module core and templates. A real integration still needs project-owned files.

### Required

- `par_table.def` at the package root
- `port/par_cfg_port.h`

### Optional, depending on configuration

- `port/par_if_port.c` when `PAR_CFG_IF_PORT_EN = 1`
- `port/par_atomic_port.h` when `PAR_ATOMIC_BACKEND = PAR_ATOMIC_BACKEND_PORT`
- generated static layout header when `PAR_CFG_LAYOUT_SOURCE = PAR_CFG_LAYOUT_SCRIPT`
- the external NVM module when `PAR_CFG_NVM_EN = 1`

## When to read which document

- Read [Getting started](docs/getting-started.md) when you are integrating the module into a project.
- Read [Architecture](docs/architecture.md) when you need to understand how `par_table.def`, storage groups, layout, validation, or ID lookup work internally.
- Read [API reference](docs/api-reference.md) when you already understand the model and only need function-level guidance.

## Key integration notes

- `par_cfg.h` includes `par_cfg_port.h` unconditionally, so your build must provide that header.
- `PAR_CFG_ENABLE_TYPE_F32` controls whether floating-point parameter support, related typed APIs, and the `PAR_SET_F32` / `PAR_GET_F32` macro wrappers are compiled in.
- `PAR_CFG_ENABLE_RUNTIME_VALIDATION` and `PAR_CFG_ENABLE_CHANGE_CALLBACK` control whether normal setters include runtime validation callbacks and on-change callbacks.
- The module separates **internal parameter enumeration** (`par_num_t`) from **external parameter IDs** (`id`).
- The current ID lookup implementation uses a one-entry-per-bucket hash map generated at compile time from `par_table.def`. External IDs must therefore be not only unique, but also collision-free under the configured hash geometry. Optional runtime diagnostic scans can be enabled with `PAR_CFG_ENABLE_RUNTIME_ID_DUP_CHECK` and `PAR_CFG_ENABLE_RUNTIME_ID_HASH_COLLISION_CHECK` when additional startup logs are useful. See `docs/architecture.md` for the collision rule and avoidance guidance.
- Fast setter APIs skip part of the safety and observability path, including runtime validation callbacks and on-change callbacks, so they should be reserved for tightly controlled hot paths.
- NVM support is optional, but when enabled it depends on the external NVM module and on ID and persistence metadata being enabled.
- `par_init()` applies startup default values directly to live storage. Integer default values from `par_table.def` are compiled into a grouped width-based storage object, while `F32` default values are applied to the 32-bit storage group after layout offsets are available only when `PAR_CFG_ENABLE_TYPE_F32 = 1`. Because this startup initialization does not go through the public setter path, it does not invoke runtime validation or on-change callbacks.
- `PAR_CFG_ENABLE_RESET_ALL_RAW` controls whether raw reset-all support and grouped default mirror snapshot support are enabled.

## Related projects

- [CLI module](https://github.com/GeneralEmbeddedCLibraries/cli)
- [NVM module](https://github.com/GeneralEmbeddedCLibraries/nvm)
- [General Embedded C Library Manual](https://github.com/GeneralEmbeddedCLibraries/documentation/blob/develop/General_Embedded_C_Library_Manual.pdf)
