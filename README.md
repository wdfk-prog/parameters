[中文](./README.zh-CN.md)

# autogen_parameter_manager

`autogen_parameter_manager` is an RT-Thread package wrapper around the portable `Device Parameters` module. It provides package configuration, RT-Thread-facing ports, storage backend adapters, and a bundled parameter table template/profile.

## Repository layout

```text
.
├── Kconfig                         # RT-Thread package options
├── SConscript                      # RT-Thread/SCons source selection
├── backend/                        # Package-level storage backend bridges
├── port/                           # Package-level RT-Thread port files
├── par_table.def                   # Generated X-Macro table used by this package profile
└── parameters/                     # Portable Device Parameters module
```

## Documentation

- [Module README](parameters/README.md) is the primary integration entry point.
- [Documentation overview](parameters/docs/overview.md) maps reader goals to the detailed documents.
- [Getting started](parameters/docs/getting-started.md) covers required files, build inputs, and first-use examples.
- [CSV parameter generator](parameters/docs/csv-generator.md) covers CSV schema maintenance, Python requirements, validation, ID allocation, and generated layout files.
- [Architecture](parameters/docs/architecture.md) describes the runtime model, layout, validation, ID lookup, and NVM split.
- [API reference](parameters/docs/api-reference.md) groups public APIs by responsibility.
- [Object parameters](parameters/docs/object-parameters.md) explains fixed-capacity string, bytes, and array rows.
- [Flash-ee backend design](parameters/docs/flash-ee-backend-design.md) documents the portable flash-emulated EEPROM backend.

## Integration entry points

- Configure package options through `Kconfig`.
- Include sources through `SConscript` when building as an RT-Thread package.
- Edit `parameters/schema/par_table.csv`, then regenerate `par_table.def` and generated layout files with `parameters/tools/pargen.py`.
- Provide product-specific policy in `port/par_cfg_port.h`.
- Select one NVM backend path when persistence is enabled: RT-Thread AT24CXX, flash-ee through FAL, flash-ee through native hooks, GEL/NVM adapter, or a product-owned backend.
