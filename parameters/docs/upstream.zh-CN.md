[English](./upstream.md)

# 上游关系

## 基线

本仓库基于上游仓库 [`GeneralEmbeddedCLibraries/parameters`](https://github.com/GeneralEmbeddedCLibraries/parameters) 的以下 commit：

```text
a4ad57ffa43b17d88333c2e63ce4e45a5651f7d9
```

导入基线提供了可移植嵌入式 C 参数管理器基础。

## 本地维护方向

本仓库独立维护，原因是 RT-Thread 软件包集成和本地功能集难以持续同步到上游。相关本地工作应保持显式、可追溯，而不是伪装成普通上游变更。

## 本地扩展范围

| 范围 | 本地职责 |
| --- | --- |
| RT-Thread 软件包集成 | Kconfig 选项、SConscript 构建选择、RT-Thread 移植文件和 MSH 工具。 |
| NVM 后端集成 | RT-Thread AT24CXX 后端，以及 flash-ee FAL/native port 绑定。 |
| 元数据 | access、role policy、persistent、description、ID 和生成摘要数据等扩展表字段。 |
| 对象参数 | 通过专用运行时 API 支持定长容量字符串、字节和无符号整数数组。 |
| 持久化布局 | 标量/对象持久化的 compile-scan 与生成静态布局模式。 |
| 校验 | 编译期检查、运行期表检查和可选用户校验回调。 |

## 同步策略

后续导入上游变更时：

1. 在本文档中记录上游仓库和 commit hash。
2. 重点审查 RT-Thread 软件包文件、移植接口、生成产物、对象参数行为和 NVM 后端契约是否冲突。
3. schema 或生成器行为变化时重新生成产物。
4. 尽量将上游同步 commit 与本地扩展 commit 拆分。
5. 行为或集成步骤变化时同步更新中英文文档。

## 归属说明建议

发布或软件包元数据空间允许时，可使用以下表述：

> Based on `GeneralEmbeddedCLibraries/parameters` commit `a4ad57ffa43b17d88333c2e63ce4e45a5651f7d9`, with independent RT-Thread package integration and local parameter-manager extensions.
