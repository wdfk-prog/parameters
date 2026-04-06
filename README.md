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

1. Add `src/par.c` and the sources from `src/def`, `src/layout`, `src/persist`, and `src/port` to your build. Add `src/persist/backend` only when you use the packaged backend adapter.
2. Provide a project-specific `par_table.def` at the package root.
3. Provide `port/par_cfg_port.h` in your include path.
4. Optionally provide `port/par_if_port.c` and `port/par_atomic_port.h` when your platform needs them.
5. Call `par_init()` before using runtime APIs.
6. Use the typed `par_set_*` / `par_get_*` APIs in application code. Getter APIs now use an explicit output pointer and return `par_status_t`.

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

    (void)par_set_f32(ePAR_CH1_REF_VAL, (float32_t)25.0f);

    float32_t ref_val = 0.0f;
    if (par_get_f32(ePAR_CH1_REF_VAL, &ref_val) != ePAR_OK)
    {
        /* Handle read error */
    }
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
├── docs/
│   ├── DeviceParameter_VerificationReport.xlsx
│   ├── api-reference.md
│   ├── architecture.md
│   └── getting-started.md
├── src/
│   ├── par.c
│   ├── par.h
│   ├── par_cfg.h
│   ├── def/
│   │   ├── par_def.c
│   │   ├── par_def.h
│   │   ├── par_id_map_static.c
│   │   └── par_id_map_static.h
│   ├── detail/
│   │   ├── par_bitwise_impl.inc
│   │   ├── par_storage_init.inc
│   │   └── par_typed_impl.inc
│   ├── layout/
│   │   ├── par_layout.c
│   │   └── par_layout.h
│   ├── persist/
│   │   ├── backend/
│   │   │   ├── par_store_backend.h
│   │   │   └── par_store_backend_gel_nvm.c
│   │   ├── par_nvm.c
│   │   └── par_nvm.h
│   └── port/
│       ├── par_atomic.h
│       ├── par_if.c
│       └── par_if.h
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

- `port/par_if_port.c` when `PAR_CFG_IF_PORT_EN = 1` and the target needs stronger platform hooks than the weak defaults in `src/port/par_if.c`
- `port/par_atomic_port.h` when `PAR_ATOMIC_BACKEND = PAR_ATOMIC_BACKEND_PORT`
- generated static layout header when `PAR_CFG_LAYOUT_SOURCE = PAR_CFG_LAYOUT_SCRIPT`
- a concrete storage backend implementation when `PAR_CFG_NVM_EN = 1`

## When to read which document

- Read [Getting started](docs/getting-started.md) when you are integrating the module into a project.
- Read [Architecture](docs/architecture.md) when you need to understand how `par_table.def`, storage groups, layout, validation, or ID lookup work internally.
- Read [API reference](docs/api-reference.md) when you already understand the model and only need function-level guidance.

## Key integration notes

- `src/par.h` is the main public entry header. Keep `parameters/src` on the compiler include path so application code can use `#include "par.h"`.
- `par_cfg.h` includes `par_cfg_port.h` unconditionally, so your build must provide that header and make its directory visible to the compiler.
- `PAR_CFG_ENABLE_TYPE_F32` controls whether floating-point parameter support and the related typed APIs are compiled in.
- `PAR_CFG_ENABLE_RUNTIME_VALIDATION` and `PAR_CFG_ENABLE_CHANGE_CALLBACK` control whether normal setters include runtime validation callbacks and on-change callbacks.
- The RT-Thread port exposes leveled package logging hooks (`INFO` / `DEBUG` / `WARN` / `ERROR`). Enable them with `AUTOGEN_PM_USING_DEBUG` in package Kconfig.
- The module separates **internal parameter enumeration** (`par_num_t`) from **external parameter IDs** (`id`).
- The current ID lookup implementation uses a one-entry-per-bucket hash map generated at compile time from `par_table.def`. External IDs must therefore be not only unique, but also collision-free under the configured hash geometry. Optional runtime diagnostic scans can be enabled with `PAR_CFG_ENABLE_RUNTIME_ID_DUP_CHECK` and `PAR_CFG_ENABLE_RUNTIME_ID_HASH_COLLISION_CHECK` when additional startup logs are useful. See `docs/architecture.md` for the collision rule and avoidance guidance.
- Unchecked setter APIs skip runtime validation callbacks and on-change callbacks, so they should be reserved for tightly controlled hot paths. Bitwise fast setters are further restricted to `U8` / `U16` / `U32` flags or bitmask parameters. Legacy `*_fast()` names remain as deprecated aliases.
- NVM support is optional. When enabled, `src/persist/par_nvm.c` depends on a mounted storage backend interface and on ID support being enabled. Persistence metadata is compiled in automatically under `PAR_CFG_NVM_EN`. The package can build the `GeneralEmbeddedCLibraries/nvm` adapter from `src/persist/backend/`, or the application can provide its own `par_store_backend_get_api()` implementation.
- Live RAM layout and persisted NVM layout are intentionally different. RAM storage is grouped by value width, while the persistence area is a linear list of fixed 8-byte objects: `id(2) + size(1) + crc8(1) + data(4)`.
- Compile-time persistent order is the primary slot layout contract of the managed NVM image. The stored `id` remains in each record as an integrity and diagnostics field, and the current `size` field is still always written as `4`, so it is mainly a descriptor/integrity helper rather than a space-saving variable-width encoding.
- The serialized NVM header is written explicitly as a fixed 12-byte storage image (`sign(4) + obj_nb(2) + table_id(4) + crc16(2)`), so on-storage layout does not depend on compiler struct padding. Header CRC-16 covers the serialized `obj_nb + table_id` bytes, while each data record carries its own CRC-8 over `id + size + data`.
- CRC calculation is routed through port hooks with bundled software defaults. In this single-target profile the persisted image and the table-ID digest both use the native byte order of the running platform, so no additional byte-order conversion hook is required by the persistence path.
- When `PAR_CFG_TABLE_ID_CHECK_EN = 1`, startup compares the stored table-ID against the live table-ID to validate persisted-image compatibility. The table-ID covers `PAR_CFG_TABLE_ID_SCHEMA_VER`, persistent-parameter count, persistent-parameter order, parameter type, and parameter ID, hashed as platform-native scalar bytes under the single-target profile. It intentionally does not cover defaults, ranges, names, units, descriptions, or access flags.
- A table-ID mismatch is treated as an incompatible persisted-layout change, not as a warning-only condition. Startup restores defaults and rebuilds the managed NVM image. Typical triggers are add/remove/reorder/type/ID changes of persistent parameters and transitions between persistent and non-persistent state. A stored header count smaller than the compile-time persistent count is repaired by appending the missing tail slots from current defaults and rewriting the header count; a stored count larger than the compile-time count is treated as incompatible and rebuilt.
- `PAR_CFG_TABLE_ID_SCHEMA_VER` defaults in `src/par_cfg.h` and may be overridden in `port/par_cfg_port.h`; the integrator should bump it when intentionally changing the serialized table-ID schema.
- `par_init()` applies startup default values directly to live storage. Integer default values from `par_table.def` are compiled into a grouped width-based storage object, while `F32` default values are applied to the 32-bit storage group after layout offsets are available only when `PAR_CFG_ENABLE_TYPE_F32 = 1`. Because this startup initialization does not go through the public setter path, it does not invoke runtime validation or on-change callbacks.
- `PAR_CFG_ENABLE_RESET_ALL_RAW` controls whether raw reset-all support and grouped default mirror snapshot support are enabled.

## Related projects

- [CLI module](https://github.com/GeneralEmbeddedCLibraries/cli)
- [NVM module](https://github.com/GeneralEmbeddedCLibraries/nvm)
- [General Embedded C Library Manual](https://github.com/GeneralEmbeddedCLibraries/documentation/blob/develop/General_Embedded_C_Library_Manual.pdf)
