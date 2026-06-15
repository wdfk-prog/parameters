[中文](./overview.zh-CN.md)

# Documentation overview

This document is the entry point for the maintained documentation set of the RT-Thread-oriented `Device Parameters` repository.

## Repository scope

The repository contains a portable parameter manager core and documentation for RT-Thread package integration. The core is based on upstream [`GeneralEmbeddedCLibraries/parameters`](https://github.com/GeneralEmbeddedCLibraries/parameters) commit `a4ad57ffa43b17d88333c2e63ce4e45a5651f7d9`, with local extensions for RT-Thread integration, NVM backends, metadata, object parameters, generated layout options, and validation hooks.

## Reader paths

| Reader goal | Start here | Then read |
| --- | --- | --- |
| Add the module to a firmware build | [Getting started](./getting-started.md) | [API reference](./api-reference.md) |
| Integrate as an RT-Thread package | [RT-Thread package](./rt-thread-package.md) | [Flash-ee backend design](./flash-ee-backend-design.md) |
| Accept fixed-slot scalar NVM on hardware | [Fixed-slot-with-size NVM manual hardware test](../tests/docs/nvm-fixed-slot-with-size-manual-test.md) | [RT-Thread package](./rt-thread-package.md) |
| Accept object NVM on hardware | [Object NVM manual hardware test](../tests/docs/nvm-object-manual-test.md) | [Object parameters](./object-parameters.md) |
| Understand internal ownership and data flow | [Architecture](./architecture.md) | [Object parameters](./object-parameters.md) |
| Maintain the parameter table | [CSV generator](./csv-generator.md) | [Architecture](./architecture.md) |
| Review upstream provenance | [Upstream relationship](./upstream.md) | [Architecture](./architecture.md) |

## Document set

- [Getting started](./getting-started.md): integration checklist, configuration choices, generation workflow, and first runtime calls.
- [RT-Thread package](./rt-thread-package.md): Kconfig/SCons expectations, port layer, MSH tooling, and RT-Thread NVM backend choices.
- [Architecture](./architecture.md): data ownership, generated artifacts, validation, ID lookup, layout policy, persistence boundaries, and port boundaries.
- [API reference](./api-reference.md): public APIs grouped by lifecycle, scalar access, object access, metadata, registration, and NVM.
- [CSV generator](./csv-generator.md): CSV fields, ID ranges, lock file, generated layout files, and regeneration workflow.
- [Object parameters](./object-parameters.md): fixed-capacity object model, storage pool, dedicated APIs, and object persistence constraints.
- [Flash-ee backend design](./flash-ee-backend-design.md): flash-emulated EEPROM model, bank switching, record visibility, and adapter contracts.
- [Test overview](../tests/README.md): runtime tests, manual NVM tests, generator tests, and missing test backlog items.
- [Fixed-slot-with-size NVM manual hardware test](../tests/docs/nvm-fixed-slot-with-size-manual-test.md): Kconfig/CSV profiles, destructive AT24CXX MSH commands, reboot checks, and acceptance criteria for scalar fixed-slot NVM.
- [Object NVM manual hardware test](../tests/docs/nvm-object-manual-test.md): Kconfig/CSV profiles, object payload persistence commands, object block corruption injection, reboot checks, and acceptance criteria.
- [Upstream relationship](./upstream.md): import baseline, local extension policy, and synchronization rules.

## Documentation structure

```mermaid
flowchart TD
    Root[README.md] --> Overview[docs/overview.md]
    Root --> Upstream[docs/upstream.md]
    Overview --> Start[docs/getting-started.md]
    Overview --> RTT[docs/rt-thread-package.md]
    Overview --> Arch[docs/architecture.md]
    Overview --> API[docs/api-reference.md]
    Overview --> CSV[docs/csv-generator.md]
    Overview --> Obj[docs/object-parameters.md]
    Overview --> Flash[docs/flash-ee-backend-design.md]
    Overview --> TestOverview[tests/README.md]
    TestOverview --> NVMTest[tests/docs/nvm-fixed-slot-with-size-manual-test.md]
    TestOverview --> ObjNVMTest[tests/docs/nvm-object-manual-test.md]
```

## Maintenance rule

Keep `README.md` concise and use `docs/` for detailed design, package integration, API, backend, and maintenance topics. Each maintained English document has a Chinese counterpart with the same stem and `.zh-CN.md` suffix.
