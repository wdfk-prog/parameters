[English](./overview.md)

# 文档总览

本文档是面向 RT-Thread 的 `Device Parameters` 仓库文档入口。

## 仓库范围

本仓库包含可移植参数管理器核心，以及 RT-Thread 软件包集成说明。核心基于上游 [`GeneralEmbeddedCLibraries/parameters`](https://github.com/GeneralEmbeddedCLibraries/parameters) 的 `a4ad57ffa43b17d88333c2e63ce4e45a5651f7d9` commit，并在本仓库中扩展 RT-Thread 集成、NVM 后端、元数据、对象参数、生成布局选项和校验钩子。

## 阅读路径

| 目标 | 起点 | 后续阅读 |
| --- | --- | --- |
| 将模块加入固件构建 | [快速开始](./getting-started.zh-CN.md) | [API 参考](./api-reference.zh-CN.md) |
| 对比 FlashDB 或 EasyFlash | [与 FlashDB 和 EasyFlash 的区别](./flashdb-easyflash-comparison.zh-CN.md) | [Flash-ee 后端设计](./flash-ee-backend-design.zh-CN.md) |
| 作为 RT-Thread 软件包集成 | [RT-Thread 软件包](./rt-thread-package.zh-CN.md) | [Flash-ee 后端设计](./flash-ee-backend-design.zh-CN.md) |
| 理解内部所有权和数据流 | [架构](./architecture.zh-CN.md) | [对象参数](./object-parameters.zh-CN.md) |
| 维护参数表 | [CSV 生成器](./csv-generator.zh-CN.md) | [架构](./architecture.zh-CN.md) |
| 审查上游来源 | [上游关系](./upstream.zh-CN.md) | [架构](./architecture.zh-CN.md) |
| 验收 AT24CXX fixed-slot NVM | [Fixed-slot-with-size NVM 硬件手动测试](../tests/docs/nvm-fixed-slot-with-size-manual-test.zh-CN.md) | [CSV 生成器](./csv-generator.zh-CN.md) |
| 验收 object NVM | [Object NVM 硬件手动测试](../tests/docs/nvm-object-manual-test.zh-CN.md) | [对象参数](./object-parameters.zh-CN.md) |

## 文档集合

- [快速开始](./getting-started.zh-CN.md)：集成检查表、配置选择、生成流程和首次运行时调用。
- [与 FlashDB 和 EasyFlash 的区别](./flashdb-easyflash-comparison.zh-CN.md)：说明本软件包与 FlashDB、EasyFlash 和包内 flash-ee 后端的产品定位边界。
- [RT-Thread 软件包](./rt-thread-package.zh-CN.md)：Kconfig/SCons 预期、移植层、MSH 工具和 RT-Thread NVM 后端选择。
- [架构](./architecture.zh-CN.md)：数据所有权、生成产物、校验、ID 查找、布局策略、持久化边界和移植边界。
- [API 参考](./api-reference.zh-CN.md)：按生命周期、标量访问、对象访问、元数据、注册和 NVM 分组的公共 API。
- [CSV 生成器](./csv-generator.zh-CN.md)：CSV 字段、ID 范围、锁文件、生成布局文件和重新生成流程。
- [对象参数](./object-parameters.zh-CN.md)：定长容量对象模型、存储池、专用 API 和对象持久化约束。
- [Flash-ee 后端设计](./flash-ee-backend-design.zh-CN.md)：flash 模拟 EEPROM 模型、bank 切换、记录可见性和适配器契约。
- [测试总览](../tests/README.zh-CN.md)：索引运行时测试、NVM 手动测试、生成器测试和待补测试项。
- [Fixed-slot-with-size NVM 硬件手动测试](../tests/docs/nvm-fixed-slot-with-size-manual-test.zh-CN.md)：说明 AT24CXX 后端上的 scalar persistent CSV 配置、Kconfig、MSH 命令、reboot 和掉电验收流程。
- [Object NVM 硬件手动测试](../tests/docs/nvm-object-manual-test.zh-CN.md)：说明 object persistent CSV 配置、Kconfig、payload 持久化命令、object block 损坏注入、reboot 和验收流程。
- [上游关系](./upstream.zh-CN.md)：导入基线、本地扩展策略和同步规则。

## 文档结构

```mermaid
flowchart TD
    Root[README.md] --> Overview[docs/overview.zh-CN.md]
    Root --> Upstream[docs/upstream.zh-CN.md]
    Overview --> Start[docs/getting-started.zh-CN.md]
    Overview --> Compare[docs/flashdb-easyflash-comparison.zh-CN.md]
    Overview --> RTT[docs/rt-thread-package.zh-CN.md]
    Overview --> Arch[docs/architecture.zh-CN.md]
    Overview --> API[docs/api-reference.zh-CN.md]
    Overview --> CSV[docs/csv-generator.zh-CN.md]
    Overview --> Obj[docs/object-parameters.zh-CN.md]
    Overview --> Flash[docs/flash-ee-backend-design.zh-CN.md]
    Overview --> TestOverview[tests/README.zh-CN.md]
    TestOverview --> NVMTest[tests/docs/nvm-fixed-slot-with-size-manual-test.zh-CN.md]
    TestOverview --> ObjNVMTest[tests/docs/nvm-object-manual-test.zh-CN.md]
```

## 维护规则

保持 `README.md` 简洁，把详细设计、软件包集成、API、后端和维护主题放在 `docs/` 下。每个维护中的英文文档都有同名中文对应页，中文文件名使用 `.zh-CN.md` 后缀。
