[中文](./upstream.zh-CN.md)

# Upstream relationship

## Baseline

This repository is based on the upstream repository [`GeneralEmbeddedCLibraries/parameters`](https://github.com/GeneralEmbeddedCLibraries/parameters) at commit:

```text
a4ad57ffa43b17d88333c2e63ce4e45a5651f7d9
```

The imported baseline provides the portable embedded C parameter-manager foundation.

## Local maintenance direction

This repository is maintained independently because the RT-Thread package work and local feature set are difficult to synchronize upstream continuously. Local work should stay explicit and traceable rather than being hidden as generic upstream changes.

## Local extension areas

| Area | Local responsibility |
| --- | --- |
| RT-Thread package integration | Kconfig options, SConscript build selection, RT-Thread port files, and MSH tooling. |
| NVM backend integration | RT-Thread AT24CXX backend and flash-ee FAL/native port bindings. |
| Metadata | Extended table fields such as access, role policy, persistent flag, description, ID, and generated summary data. |
| Object parameters | Fixed-capacity strings, bytes, and unsigned integer arrays with dedicated runtime APIs. |
| Persistence layout | Compile-scan and generated static layout modes for scalar/object persistence. |
| Validation | Compile-time checks, runtime table checks, and optional user validation callbacks. |

## Synchronization policy

When importing future upstream work:

1. Record the upstream repository and commit hash in this document.
2. Review conflicts against RT-Thread package files, port interfaces, generator output, object parameter behavior, and NVM backend contracts.
3. Regenerate generated artifacts when schema or generator behavior changes.
4. Keep local extension commits separate from upstream sync commits where practical.
5. Update both English and Chinese documentation if behavior or integration steps change.

## Attribution wording

Use this wording in releases or package metadata when space allows:

> Based on `GeneralEmbeddedCLibraries/parameters` commit `a4ad57ffa43b17d88333c2e63ce4e45a5651f7d9`, with independent RT-Thread package integration and local parameter-manager extensions.
