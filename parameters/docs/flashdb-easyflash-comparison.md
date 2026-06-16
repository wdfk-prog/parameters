[中文](./flashdb-easyflash-comparison.zh-CN.md)

# Difference from FlashDB and EasyFlash

This document clarifies how `autogen_parameter_manager` / `Device Parameters` differs from FlashDB and EasyFlash, so the package is not mistaken for a general Flash database, IAP library, or log-storage library.

Reference projects:

- [FlashDB](https://github.com/armink/FlashDB): an embedded Flash database that provides KVDB and TSDB database modes.
- [EasyFlash](https://github.com/armink/EasyFlash): a lightweight embedded Flash storage library that mainly provides ENV, IAP, and Log features.

## Summary

`autogen_parameter_manager` mainly answers the question: how firmware product parameters are defined, validated, accessed, generated, and optionally persisted. Its center is the parameter table, typed APIs, metadata, validation rules, generated IDs/layouts, and optional NVM management.

FlashDB and EasyFlash mainly answer the question: how data is stored on Flash. FlashDB is closer to a general embedded database for KV and time-series data. EasyFlash is closer to a Flash application utility library for environment variables, IAP, and logs.

Therefore, this package is not a peer replacement for FlashDB or EasyFlash. It lives closer to the product parameter model. FlashDB and EasyFlash live closer to the Flash storage-service layer.

## One-line boundary

| Package | More precise positioning | Main problem solved |
| --- | --- | --- |
| `autogen_parameter_manager` | Firmware parameter-table manager | Parameter definition, typed access, metadata, range/access/callback validation, generated IDs/layouts, optional parameter persistence |
| FlashDB | Embedded Flash database | KV data, blob data, time-series records, historical log-like data, database instances and partition management |
| EasyFlash | Flash application storage utility library | ENV variables, IAP, Flash logs, and basic Flash storage abstraction |

## Detailed comparison

| Dimension | `autogen_parameter_manager` | FlashDB | EasyFlash |
| --- | --- | --- | --- |
| Primary goal | Manage a known set of product parameters | Provide Flash-oriented KVDB/TSDB databases | Simplify ENV/IAP/Log use on Flash |
| Data model | Compile-time parameter table with scalar and fixed-capacity object parameters | Runtime key-value, blob, and time-series records | Runtime environment-variable key-value data, IAP data, and log data |
| Schema source | `schema/par_table.csv` and generated artifacts are the single source of truth | Application code defines key, value, and TSDB record semantics | Application code plus `ef_cfg.h`/default environment variables define stored data |
| Access model | Generated enum, optional external ID, and typed getter/setter APIs | Database instance, key, or time-series APIs | ENV key, IAP, and Log APIs |
| Type constraints | Built-in `U8/I8/U16/I16/U32/I32`, optional `F32`, strings, bytes, and fixed-size arrays | Database stores strings/blobs/records; business types are interpreted by the application | ENV values may store different-length data; business types are interpreted by the application |
| Parameter metadata | Supports name, unit, description, access, persistent flag, external ID, role policy, and related metadata | Parameter metadata tables are not its core model | Parameter metadata tables are not its core model |
| Validation and callbacks | Supports range checks, runtime validation hooks, change callbacks, and table consistency checks | Mainly provides database storage reliability; business validation is application-owned | Mainly provides Flash storage utilities; business validation is application-owned |
| Persistence boundary | Persists only table rows marked as persistent; backends can be AT24CXX, flash-ee, GEL/NVM, or product-owned | KVDB/TSDB are persistent database features | ENV/IAP/Log features directly target Flash persistence |
| Table/version evolution | Generated IDs, ID locks, table-ID/schema version, and layout policies control parameter-table evolution risk | KV incremental upgrade and database evolution are handled by FlashDB mechanisms | ENV version and automatic update are handled by EasyFlash configuration |
| Good fit | Calibration values, configuration items, control parameters, debug parameters, and product parameters that need fixed IDs/access/unit/description | User configuration, dynamic KV data, small files, sample history, alarm/runtime logs, time-series data | Simple environment variables, IAP upgrade data, Flash logs, and existing EasyFlash projects |
| Poor fit | Large dynamic key sets, long history records, general log streams, file systems, IAP images | Projects that need generated C parameter APIs, strongly typed parameter tables, or access/unit/range metadata | Projects that need generated parameter tables, typed APIs, rich metadata, and object-parameter modeling |

## Relationship to the flash-ee backend in this repository

The `flash-ee` code in this repository is an optional NVM backend for `Device Parameters`. It writes the parameter layer's logical address space into a Flash-emulated EEPROM region. It uses bank metadata, append records, cache/window handling, and checkpoint-based recovery. Its consumer is this package's parameter-persistence path.

`flash-ee` is not FlashDB or EasyFlash:

- It does not provide TSDB.
- It does not provide a general KV database API.
- It does not provide IAP or a general log module.
- It does not make dynamic keys the primary access model.
- It does not replace the parameter table, metadata, validation, or generation workflow.

Layer-wise, `flash-ee` is an internal optional storage backend. The external API of `autogen_parameter_manager` remains the parameter-management API.

## Selection guidance

### Prefer `autogen_parameter_manager` when

- The parameter set is mostly known at compile time and must evolve under firmware-version control.
- The firmware needs typed C APIs instead of scattered handwritten key strings.
- Parameters need unified IDs, names, units, descriptions, access attributes, persistent flags, and related metadata.
- Parameters need range validation, runtime validation, change callbacks, or startup table consistency checks.
- The project needs CSV-generated parameter tables, ID maps, static layouts, and metadata for documentation/debug tooling.
- Persistence should be limited to explicitly marked parameters instead of accepting arbitrary keys.

### Prefer FlashDB when

- A general KV database or time-series database is required.
- Keys or record types are dynamic and do not fit a fixed parameter table.
- The product must store sample history, alarm history, runtime logs, or other dynamic user data.
- The application wants to use FlashDB database instances, partitions, KV/TSDB APIs, and database recovery mechanisms directly.

### Prefer EasyFlash when

- The goal is only to store a small number of environment variables quickly.
- The project already uses EasyFlash ENV/IAP/Log and should keep those interfaces.
- The product needs EasyFlash IAP or Flash log features instead of parameter-table management.
- Generated parameter IDs, typed getter/setter APIs, metadata tables, and complex validation chains are not needed.

### Combine them when appropriate

A product may use the packages together by responsibility:

- Use `autogen_parameter_manager` for firmware parameters, calibration items, debug items, and configuration items that need access/unit/range policy.
- Use FlashDB for dynamic user data, sampled history, or event logs.
- Use EasyFlash to preserve existing ENV/IAP/Log functionality.

When combining them, isolate Flash partitions, erase granularity, power-loss tests, and schema/data migration policies so multiple modules do not write the same physical region.

## Suggested external description

When describing this package externally, use wording like this:

> `autogen_parameter_manager` is a parameter-table manager for embedded firmware. It uses CSV/generation to maintain the type, ID, metadata, validation rules, and optional NVM persistence of fixed product parameters. It is not a general KV/TSDB database and not an IAP/log storage library; dynamic KV, time-series data, or Flash logs should be handled by FlashDB or EasyFlash when those responsibilities are required.
