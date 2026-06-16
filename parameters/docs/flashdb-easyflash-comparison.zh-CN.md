[English](./flashdb-easyflash-comparison.md)

# 与 FlashDB 和 EasyFlash 的区别

本文用于说明 `autogen_parameter_manager` / `Device Parameters` 与 FlashDB、EasyFlash 的定位差异，避免把参数管理器误理解为通用 Flash 数据库或 IAP/日志存储库。

参考项目：

- [FlashDB](https://github.com/armink/FlashDB)：面向 Flash 的嵌入式数据库，提供 KVDB 和 TSDB 两类数据库模式。
- [EasyFlash](https://github.com/armink/EasyFlash)：轻量级嵌入式 Flash 存储库，主要提供 ENV、IAP 和 Log 功能。

## 结论

`autogen_parameter_manager` 主要解决“固件中的产品参数如何定义、校验、访问、生成和可选持久化”的问题。它的核心是参数表、类型化 API、元数据、校验规则、生成 ID/布局和可选 NVM 管理。

FlashDB 和 EasyFlash 主要解决“如何在 Flash 上保存数据”的问题。FlashDB 更接近通用嵌入式数据库，覆盖 KV 和时序数据；EasyFlash 更接近 Flash 应用工具库，覆盖环境变量、在线升级和日志。

因此，本软件包不是 FlashDB/EasyFlash 的同类替代品。它处在更靠近业务参数模型的一层；FlashDB/EasyFlash 处在更靠近 Flash 存储服务的一层。

## 一句话边界

| 软件包 | 更准确的定位 | 主要解决的问题 |
| --- | --- | --- |
| `autogen_parameter_manager` | 固件参数表管理器 | 参数定义、类型化访问、元数据、范围/权限/回调校验、生成 ID/布局、可选参数持久化 |
| FlashDB | Flash 嵌入式数据库 | KV 数据、Blob 数据、时序记录、日志类历史数据、数据库实例和分区管理 |
| EasyFlash | Flash 应用存储工具库 | ENV 环境变量、IAP 在线升级、Flash 日志和基础 Flash 存储抽象 |

## 详细差异

| 维度 | `autogen_parameter_manager` | FlashDB | EasyFlash |
| --- | --- | --- | --- |
| 首要目标 | 管理一组已知的产品参数 | 提供面向 Flash 的 KVDB/TSDB 数据库 | 简化 Flash 上的 ENV/IAP/Log 应用 |
| 数据模型 | 编译期参数表，包含标量和定长容量对象参数 | 运行期 key-value、blob、time-series record | 运行期环境变量 key-value、IAP 数据、日志数据 |
| Schema 来源 | `schema/par_table.csv` 和生成产物是单一事实源 | 应用代码定义 key、value、TSDB record 语义 | 应用代码和 `ef_cfg.h`/默认环境变量定义存储内容 |
| 访问方式 | 按生成枚举、可选外部 ID 和类型化 getter/setter 访问 | 按数据库实例、key 或 time-series API 访问 | 按 ENV key、IAP 和 Log API 访问 |
| 类型约束 | 内建 `U8/I8/U16/I16/U32/I32`、可选 `F32`、字符串、字节和定长数组 | 数据库保存 string/blob/record，业务类型由应用解释 | ENV value 可保存不同长度数据，业务类型由应用解释 |
| 参数元数据 | 支持 name、unit、description、access、persistent、外部 ID、role policy 等 | 不以“参数元数据表”为核心模型 | 不以“参数元数据表”为核心模型 |
| 校验与回调 | 支持范围校验、运行期校验钩子、变更回调、表一致性检查 | 主要提供数据库存储可靠性，业务校验由应用实现 | 主要提供 Flash 存储功能，业务校验由应用实现 |
| 持久化边界 | 只持久化表中标记为 persistent 的参数；后端可选 AT24CXX、flash-ee、GEL/NVM 或产品自有后端 | KVDB/TSDB 本身就是持久化数据库 | ENV/IAP/Log 功能直接面向 Flash 持久化 |
| 表/版本演进 | 通过生成 ID、ID 锁定、table-ID/schema version、布局策略控制参数表演进风险 | KV 增量升级和数据库层演进由 FlashDB 机制处理 | ENV 版本和自动更新由 EasyFlash 配置处理 |
| 适合数据 | 校准值、配置项、控制参数、调试参数、需要固定 ID/权限/单位/描述的产品参数 | 用户配置、动态 KV、小文件、采样历史、告警/运行日志、时序数据 | 简单环境变量、IAP 升级数据、Flash 日志、已有 EasyFlash 项目 |
| 不适合数据 | 大量动态 key、长历史记录、通用日志流、文件系统、IAP 镜像 | 需要生成 C 参数 API、强类型参数表、权限/单位/范围元数据的场景 | 需要完整参数表生成、类型化 API、复杂元数据和对象参数模型的场景 |

## 与本仓库 flash-ee 后端的关系

本仓库中的 `flash-ee` 是 `Device Parameters` 的一个可选 NVM 后端，用于把参数层的逻辑地址写入 Flash 模拟 EEPROM 存储区域。它采用 bank metadata、append record、cache/window 和 checkpoint 恢复模型，服务对象是本软件包的参数持久化路径。

`flash-ee` 不是 FlashDB，也不是 EasyFlash：

- 不提供 TSDB。
- 不提供通用 KV 数据库 API。
- 不提供 IAP 或通用日志模块。
- 不把动态 key 作为核心访问模型。
- 不替代参数表、元数据、校验和生成流程。

从分层看，`flash-ee` 是本软件包内部可选的存储后端；`autogen_parameter_manager` 对外暴露的仍是参数管理 API。

## 选型建议

### 优先选择 `autogen_parameter_manager` 的情况

- 参数集合在编译期基本确定，需要随固件版本受控演进。
- 需要类型化 C API，而不是到处手写 key 字符串。
- 需要参数 ID、名称、单位、描述、访问属性、持久化标记等统一元数据。
- 需要范围校验、运行期校验、变更回调或启动时表一致性检查。
- 需要从 CSV 生成参数表、ID 映射、静态布局和文档/调试所需元信息。
- 需要把持久化限定在明确标记的参数上，而不是开放式保存任意 key。

### 优先选择 FlashDB 的情况

- 需要通用 KV 数据库或时序数据库。
- 数据 key 或 record 类型更动态，不适合固定参数表。
- 需要保存大量采样历史、告警历史、运行日志或用户动态数据。
- 应用希望直接使用 FlashDB 的数据库实例、分区、KV/TSDB API 和数据库恢复机制。

### 优先选择 EasyFlash 的情况

- 目标只是快速保存少量环境变量。
- 项目已经使用 EasyFlash ENV/IAP/Log，并希望保持既有接口。
- 需要 EasyFlash 提供的 IAP 或 Flash 日志功能，而不是参数表管理。
- 不需要生成参数 ID、类型化 getter/setter、元数据表和复杂校验链路。

### 可以组合使用的情况

在同一个产品中可以按职责组合使用：

- 用 `autogen_parameter_manager` 管理固件参数、校准项、调试项和需要权限/单位/范围的配置项。
- 用 FlashDB 保存动态用户数据、采样历史或事件日志。
- 用 EasyFlash 保留既有 ENV/IAP/Log 功能。

组合使用时应注意 Flash 分区隔离、擦除粒度、掉电测试和 schema/数据迁移策略，避免多个模块写入同一物理区域。
