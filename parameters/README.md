[中文](./README.zh-CN.md)

# Device Parameters for RT-Thread

`Device Parameters` is a portable embedded C parameter manager with optional RT-Thread package integration. It keeps parameter definitions, metadata, validation rules, generated IDs, runtime storage, and optional non-volatile persistence tied to one source table.

This repository is independently maintained for RT-Thread-oriented integration work. It is based on the upstream repository [`GeneralEmbeddedCLibraries/parameters`](https://github.com/GeneralEmbeddedCLibraries/parameters) at commit `a4ad57ffa43b17d88333c2e63ce4e45a5651f7d9`.

## Why this repository exists

The upstream project provides the portable parameter-manager foundation. This repository keeps that base while adding package-level work that is difficult to keep synchronized upstream:

- RT-Thread package integration through Kconfig, SCons/SConscript, port glue, and optional MSH commands.
- RT-Thread-oriented NVM backend options, including AT24CXX and flash-ee adapters for FAL/native flash ports.
- Extended parameter metadata, object parameter support, persistence layout choices, and validation hooks.
- Generated layout and ID-lock artifacts for reproducible table evolution.

See [Upstream relationship](./docs/upstream.md) for the import baseline and maintenance policy.

## Main capabilities

- Single source of truth through `schema/par_table.csv` and generated parameter definition artifacts.
- Typed scalar APIs for `U8`, `I8`, `U16`, `I16`, `U32`, `I32`, and optional `F32`.
- Fixed-capacity object parameters for strings, raw byte buffers, and fixed-size unsigned arrays.
- Optional metadata for name, unit, description, access, persistent flag, external ID, and role policy.
- Compile-time and runtime validation paths.
- Optional NVM persistence for selected scalar and object parameters.
- Generated ID lookup tables and optional static layout tables.
- Portable core with RTOS, mutex, logging, assertion, atomic, storage, and shell integration boundaries.

## Documentation map

| Topic | English | 中文 |
| --- | --- | --- |
| Documentation overview | [docs/overview.md](./docs/overview.md) | [docs/overview.zh-CN.md](./docs/overview.zh-CN.md) |
| Getting started | [docs/getting-started.md](./docs/getting-started.md) | [docs/getting-started.zh-CN.md](./docs/getting-started.zh-CN.md) |
| RT-Thread package | [docs/rt-thread-package.md](./docs/rt-thread-package.md) | [docs/rt-thread-package.zh-CN.md](./docs/rt-thread-package.zh-CN.md) |
| Architecture | [docs/architecture.md](./docs/architecture.md) | [docs/architecture.zh-CN.md](./docs/architecture.zh-CN.md) |
| API reference | [docs/api-reference.md](./docs/api-reference.md) | [docs/api-reference.zh-CN.md](./docs/api-reference.zh-CN.md) |
| CSV generator | [docs/csv-generator.md](./docs/csv-generator.md) | [docs/csv-generator.zh-CN.md](./docs/csv-generator.zh-CN.md) |
| Object parameters | [docs/object-parameters.md](./docs/object-parameters.md) | [docs/object-parameters.zh-CN.md](./docs/object-parameters.zh-CN.md) |
| Flash-ee backend | [docs/flash-ee-backend-design.md](./docs/flash-ee-backend-design.md) | [docs/flash-ee-backend-design.zh-CN.md](./docs/flash-ee-backend-design.zh-CN.md) |
| Upstream relationship | [docs/upstream.md](./docs/upstream.md) | [docs/upstream.zh-CN.md](./docs/upstream.zh-CN.md) |

## Quick integration

For a non-RT-Thread firmware project:

1. Add `src/par.c` and the required source subdirectories to the build.
2. Add both `include/` and `src/` to the include path. The public configuration chain still includes generated and packaged headers under `src/`, such as `def/par_def.h` and `nvm/par_nvm_cfg.h`.
3. Provide a project-owned `par_cfg_port.h` and make its directory visible to the compiler. This header is mandatory because `par_cfg_base.h` includes it unconditionally; use an empty guarded stub when no override is needed.
4. Keep `schema/par_table.csv` as the editable parameter table.
5. Run the generator after table edits:

   ```sh
   python3 tools/pargen.py
   ```

6. Provide `port/par_if_port.c` or `port/par_atomic_port.h` only when the weak/default platform hooks are not sufficient or when a port atomic backend is selected.
7. Call `par_init()` before using any runtime getter, setter, metadata, or NVM API.

For RT-Thread package use, read [RT-Thread package integration](./docs/rt-thread-package.md). The package wrapper is expected to provide Kconfig/SCons glue and RT-Thread-specific ports around this portable core.

## Minimal runtime example

```c
#include "par.h"

static void app_init(void)
{
    if (par_init() != ePAR_OK)
    {
        return;
    }

    (void)par_set_f32(ePAR_CH1_REF_VAL, 25.0f);

    float32_t ref_val = 0.0f;
    if (par_get_f32(ePAR_CH1_REF_VAL, &ref_val) != ePAR_OK)
    {
        return;
    }
}
```

The `F32` APIs are available only when `PAR_CFG_ENABLE_TYPE_F32` is enabled.

## Repository layout

```text
.
├── README.md
├── docs/
├── generated/
├── include/
├── schema/
├── src/
├── template/
├── tests/
└── tools/
```

A complete RT-Thread package may wrap this core with additional package-root files such as `Kconfig`, `SConscript`, backend adapters, port files, and MSH command registration.

## Generated files

The generated files under `generated/` and the ID-lock data under `schema/` are part of the reproducible parameter table workflow. Regenerate them whenever `schema/par_table.csv`, `schema/pargen.json`, or generator logic changes.

## Maintenance notes

- Keep upstream-origin changes traceable through [docs/upstream.md](./docs/upstream.md).
- Keep English and Chinese documents paired when adding or moving documentation.
- Avoid editing generated files manually unless the change is intentionally checked in as generator output.
