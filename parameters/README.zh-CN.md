[English](./README.md)

# 面向 RT-Thread 的 Device Parameters

`Device Parameters` 是一个可移植的嵌入式 C 参数管理器，并带有可选的 RT-Thread 软件包集成能力。它把参数定义、元数据、校验规则、生成 ID、运行时存储以及可选的非易失化持久化统一到一张参数表中维护。

本仓库面向 RT-Thread 集成进行独立维护。仓库基于上游 [`GeneralEmbeddedCLibraries/parameters`](https://github.com/GeneralEmbeddedCLibraries/parameters) 的 `a4ad57ffa43b17d88333c2e63ce4e45a5651f7d9` commit 建立。

## 为什么独立维护

上游项目提供了可移植参数管理器基础。本仓库保留该基础，同时维护难以持续同步到上游的 RT-Thread 相关能力：

- 通过 Kconfig、SCons/SConscript、移植层和可选 MSH 命令接入 RT-Thread 软件包体系。
- 提供面向 RT-Thread 的 NVM 后端选择，包括 AT24CXX 与 flash-ee 的 FAL/native flash 适配。
- 扩展参数元数据、对象参数、持久化布局选项和校验钩子。
- 维护生成布局和 ID 锁定产物，降低参数表演进风险。

导入基线与维护策略见 [上游关系](./docs/upstream.zh-CN.md)。

## 主要能力

- 以 `schema/par_table.csv` 和生成产物作为参数定义的单一事实源。
- 支持 `U8`、`I8`、`U16`、`I16`、`U32`、`I32` 以及可选 `F32` 的标量类型 API。
- 支持字符串、原始字节缓冲区、固定长度无符号数组等定长容量对象参数。
- 支持 name、unit、description、access、persistent、外部 ID、role policy 等可选元数据。
- 支持编译期与运行期两级校验路径。
- 支持选定标量参数和对象参数的可选 NVM 持久化。
- 支持生成 ID 查找表和可选静态布局表。
- 通过端口边界隔离 RTOS、互斥锁、日志、断言、原子操作、存储和 shell 集成。

## 文档索引

| 主题 | English | 中文 |
| --- | --- | --- |
| 文档总览 | [docs/overview.md](./docs/overview.md) | [docs/overview.zh-CN.md](./docs/overview.zh-CN.md) |
| 快速开始 | [docs/getting-started.md](./docs/getting-started.md) | [docs/getting-started.zh-CN.md](./docs/getting-started.zh-CN.md) |
| RT-Thread 软件包 | [docs/rt-thread-package.md](./docs/rt-thread-package.md) | [docs/rt-thread-package.zh-CN.md](./docs/rt-thread-package.zh-CN.md) |
| 架构 | [docs/architecture.md](./docs/architecture.md) | [docs/architecture.zh-CN.md](./docs/architecture.zh-CN.md) |
| API 参考 | [docs/api-reference.md](./docs/api-reference.md) | [docs/api-reference.zh-CN.md](./docs/api-reference.zh-CN.md) |
| CSV 生成器 | [docs/csv-generator.md](./docs/csv-generator.md) | [docs/csv-generator.zh-CN.md](./docs/csv-generator.zh-CN.md) |
| 对象参数 | [docs/object-parameters.md](./docs/object-parameters.md) | [docs/object-parameters.zh-CN.md](./docs/object-parameters.zh-CN.md) |
| Flash-ee 后端 | [docs/flash-ee-backend-design.md](./docs/flash-ee-backend-design.md) | [docs/flash-ee-backend-design.zh-CN.md](./docs/flash-ee-backend-design.zh-CN.md) |
| 上游关系 | [docs/upstream.md](./docs/upstream.md) | [docs/upstream.zh-CN.md](./docs/upstream.zh-CN.md) |
| 变更记录 | [CHANGE_LOG.md](./CHANGE_LOG.md) | [CHANGE_LOG.zh-CN.md](./CHANGE_LOG.zh-CN.md) |

## 快速集成

非 RT-Thread 固件项目可按以下步骤接入：

1. 将 `src/par.c` 和需要的 `src/` 子目录加入构建。
2. 将 `include/` 和 `src/` 都加入头文件搜索路径。公共配置链仍会包含 `src/` 下的生成和包内配置头，例如 `def/par_def.h` 与 `nvm/par_nvm_cfg.h`。
3. 提供项目自有的 `par_cfg_port.h`，并确保其目录对编译器可见。该头文件是必需项，因为 `par_cfg_base.h` 会无条件包含它；无需覆盖配置时可提供一个带 include guard 的空 stub。
4. 以 `schema/par_table.csv` 作为可编辑参数表。
5. 修改参数表后运行生成器：

   ```sh
   python3 tools/pargen.py
   ```

6. 仅当 weak/default 平台钩子不足或选择 port atomic backend 时，才额外提供 `port/par_if_port.c` 或 `port/par_atomic_port.h`。
7. 调用任何运行时 getter、setter、元数据或 NVM API 前，先调用 `par_init()`。

RT-Thread 软件包接入见 [RT-Thread 软件包集成](./docs/rt-thread-package.zh-CN.md)。完整软件包包装层应在该可移植核心外提供 Kconfig/SCons、RT-Thread 端口、后端适配和 MSH 命令注册。

## 最小运行时示例

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

`F32` API 仅在启用 `PAR_CFG_ENABLE_TYPE_F32` 时可用。

## 仓库结构

```text
.
├── CHANGE_LOG.md
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

完整 RT-Thread 软件包可在该核心外层补充 `Kconfig`、`SConscript`、后端适配、端口文件和 MSH 命令注册等软件包根目录文件。

## 生成文件

`generated/` 下的生成产物和 `schema/` 下的 ID 锁定数据属于可复现参数表工作流。修改 `schema/par_table.csv`、`schema/pargen.json` 或生成器逻辑后，应重新生成并提交相应产物。

## 维护说明

- 通过 [docs/upstream.zh-CN.md](./docs/upstream.zh-CN.md) 保持上游来源可追溯。
- 新增或移动文档时保持中英文成对维护。
- 除非是有意提交生成器输出，不要手工编辑生成文件。
