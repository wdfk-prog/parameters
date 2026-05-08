[中文](./getting-started.zh-CN.md)

# Getting started

This page covers the shortest safe path from source checkout to first parameter access.

## Integration checklist

1. Add the public headers in `include/` to your include path.
2. Add `src/` to the include path as well, because `par_cfg.h` still includes package-generated headers such as `def/par_def.h` and `nvm/par_nvm_cfg.h`.
3. Add the required sources from `src/` to your build.
4. Provide a project-owned `par_cfg_port.h` in a visible include directory. This file is mandatory; an empty guarded stub is valid when no overrides are needed.
5. Keep `schema/par_table.csv` as the editable parameter definition table.
6. Run `python3 tools/pargen.py` after changing the table or generator configuration.
7. Provide `par_if_port.c` or `par_atomic_port.h` only when the default hooks are not enough or the selected atomic backend requires them.
8. Call `par_init()` once before normal runtime access.
9. Use typed APIs for normal firmware code.
10. Enable NVM only after selecting and testing a storage backend.

## Required files

| Path | Purpose |
| --- | --- |
| `include/` | Public headers and configuration defaults; add this directory to the include path. |
| `src/` | Private/generated configuration headers reached by the current public configuration chain; add this directory to the include path. |
| project `port/` directory | Must provide `par_cfg_port.h`; may also provide `par_if_port.c` and `par_atomic_port.h` depending on configuration. |
| `src/par.c` | Main runtime implementation. |
| `src/def/` | Parameter table expansion and generated enum/data access. |
| `src/layout/` | Runtime layout calculation and generated layout support. |
| `src/scalar/` | Scalar typed access. |
| `src/object/` | Object parameter support when object types are enabled. |
| `src/nvm/` | Optional persistence layer. |
| `src/port/` | Port abstraction for platform-dependent pieces. |
| `schema/par_table.csv` | Editable parameter table. |
| `tools/pargen.py` | CSV generator. |

`par_cfg_port.h` is not optional: `par_cfg_base.h` includes it unconditionally. Keep the header small and project-owned so product builds can override package defaults without editing upstream-managed headers.

## Generate table artifacts

Run the generator from the repository root:

```sh
python3 tools/pargen.py
```

The generator consumes `schema/par_table.csv`, `schema/pargen.json`, and `schema/par_id_lock.json`, then updates generated table/layout artifacts. Treat generated artifacts as build inputs, not hand-written source.

## First configuration choices

| Option | Typical decision |
| --- | --- |
| `PAR_CFG_ENABLE_TYPE_F32` | Enable only when floating-point parameters are needed. |
| `PAR_CFG_ENABLE_ID` | Keep enabled when CLI, RPC, protocol, NVM, or object persistence needs stable external IDs. |
| `PAR_CFG_LAYOUT_SOURCE` | Use compile-scan for simple builds; use script layout for reproducible generated offsets. |
| `PAR_CFG_NVM_EN` | Enable only after selecting a backend and testing recovery behavior. |
| `PAR_CFG_ENABLE_RUNTIME_VALIDATION` | Enable when parameter constraints depend on runtime state. |
| `PAR_CFG_ENABLE_CHANGE_CALLBACK` | Enable when dependent modules must react to parameter changes. |

## Minimal example

```c
#include "par.h"

void app_parameters_init(void)
{
    if (par_init() != ePAR_OK)
    {
        return;
    }

    (void)par_set_u8(ePAR_CH1_CTRL, 2u);

    uint8_t ctrl = 0u;
    if (par_get_u8(ePAR_CH1_CTRL, &ctrl) != ePAR_OK)
    {
        return;
    }
}
```

## Runtime access rules

- Prefer `par_set_u8()`, `par_get_u8()`, and other typed APIs in firmware code.
- Use `par_set_scalar()` and `par_get_scalar()` only when the caller already knows the runtime type.
- Use object APIs such as `par_set_str()`, `par_get_bytes()`, and `par_get_arr_u16()` for object rows.
- Do not pass object rows to scalar getter/setter APIs.
- Check every `par_status_t` return value.

## Persistence workflow

1. Mark rows as persistent in `schema/par_table.csv`.
2. Enable `PAR_CFG_NVM_EN`.
3. Select one backend implementation.
4. Initialize the storage backend before or during parameter initialization according to the port design.
5. Use `par_save()`, `par_save_by_id()`, `par_save_all()`, or NVM-aware setters where appropriate.
6. Test power-loss recovery for the selected backend and storage medium.

## RT-Thread users

When this core is wrapped as an RT-Thread package, the package layer should provide build selection, port glue, storage adapters, and optional MSH commands. See [RT-Thread package integration](./rt-thread-package.md).

## Common mistakes

- Editing generated files without updating the CSV source.
- Reusing external IDs without updating the ID lock file intentionally.
- Enabling object persistence without stable external IDs.
- Calling fast setters from untrusted external interfaces.
- Forgetting that role metadata is policy input; enforcement must be done by the integration layer.
