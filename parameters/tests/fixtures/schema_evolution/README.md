[中文](./README.zh-CN.md)

# NVM schema evolution fixtures

These CSV fixtures drive host generation checks and board-level V1/V2 persistence acceptance tests.

The IDs are intentionally fixed so the board helper can seed and verify values through public by-ID APIs:

| ID | Role | V1 default | Prepared value |
| --- | --- | --- | --- |
| `60006` | retained scalar U8 | `11` | `42` |
| `60007` | tail scalar U16 | `1000` | `4242` |
| `60008` | retained STR object | `v1` | `hv1` |
| `60009` | retained BYTES object | `10 20 30 40` | `A5 5A C3 3C` |
| `60010` | appended scalar U8 | `55` | not present in V1 |
| `60011` | appended STR object | `new` | not present in V1 |

Use `v1_base` to build the V1 image, then build one V2 image from each `v2_*` fixture while preserving the same NVM contents.
