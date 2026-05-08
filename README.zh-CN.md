[English](./README.md)

# autogen_parameter_manager

`autogen_parameter_manager` 是围绕可移植 `Device Parameters` 模块封装的 RT-Thread 软件包。它提供软件包配置、面向 RT-Thread 的移植接口、存储后端适配器，以及随包提供的参数表模板/配置 profile。

## 仓库结构

```text
.
├── Kconfig                         # RT-Thread 软件包配置选项
├── SConscript                      # RT-Thread/SCons 源文件选择
├── backend/                        # 软件包级存储后端桥接层
├── port/                           # 软件包级 RT-Thread 移植文件
├── par_table.def                   # 当前软件包 profile 使用的生成 X-Macro 参数表
└── parameters/                     # 可移植 Device Parameters 模块
```

## 文档

- [模块 README](parameters/README.zh-CN.md) 是参数模块的主要集成入口。
- [文档总览](parameters/docs/overview.zh-CN.md) 按读者目标映射到详细文档。
- [快速开始](parameters/docs/getting-started.zh-CN.md) 说明必要文件、构建输入和首次使用示例。
- [CSV 参数生成器](parameters/docs/csv-generator.zh-CN.md) 说明 CSV schema 维护、Python 要求、校验、ID 分配和生成布局文件。
- [架构](parameters/docs/architecture.zh-CN.md) 描述运行时模型、布局、校验、ID 查找和 NVM 拆分。
- [API 参考](parameters/docs/api-reference.zh-CN.md) 按职责分组说明公共 API。
- [对象参数](parameters/docs/object-parameters.zh-CN.md) 说明定长容量字符串、字节缓冲区和数组参数行。
- [Flash-ee 后端设计](parameters/docs/flash-ee-backend-design.zh-CN.md) 说明可移植 flash 模拟 EEPROM 后端。
- [上游关系](parameters/docs/upstream.zh-CN.md) 说明本仓库与 `GeneralEmbeddedCLibraries/parameters` 的导入基线和独立维护原因。

## 集成入口

- 通过 `Kconfig` 配置软件包选项。
- 作为 RT-Thread 软件包构建时，通过 `SConscript` 纳入源文件。
- 修改 `parameters/schema/par_table.csv` 后，使用 `parameters/tools/pargen.py` 重新生成 `par_table.def` 和生成布局文件。
- 在 `port/par_cfg_port.h` 中提供产品相关策略。
- 启用持久化时选择一条 NVM 后端路径：RT-Thread AT24CXX、通过 FAL 接入的 flash-ee、通过 native hooks 接入的 flash-ee、GEL/NVM 适配器，或产品自有后端。
