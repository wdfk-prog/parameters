[English](./README.md)

# NVM schema 演进 fixture

这些 CSV fixture 用于 host 侧生成检查和板端 V1/V2 持久化验收测试。

ID 固定，便于板端 helper 通过公开 by-ID API 写入和校验：

| ID | 角色 | V1 默认值 | prepare 写入值 |
| --- | --- | --- | --- |
| `60006` | 保留 scalar U8 | `11` | `42` |
| `60007` | 尾部 scalar U16 | `1000` | `4242` |
| `60008` | 保留 STR object | `v1` | `hv1` |
| `60009` | 保留 BYTES object | `10 20 30 40` | `A5 5A C3 3C` |
| `60010` | 追加 scalar U8 | `55` | V1 不存在 |
| `60011` | 追加 STR object | `new` | V1 不存在 |

使用 `v1_base` 构建 V1 镜像；保留同一份 NVM 内容后，再用各个 `v2_*` fixture 构建 V2 镜像做验收。
