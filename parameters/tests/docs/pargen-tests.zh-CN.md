[English](./pargen-tests.md)

# pargen tests

本文档索引 `parameters/tools/pargen.py` 的 host 侧测试。

## 入口

```sh
python3 -m unittest parameters/tests/test_pargen.py
```

## 测试代码映射

| 文件 | 覆盖范围 |
| --- | --- |
| `test_pargen.py` | 当前 host 测试 case 覆盖的生成器解析、校验、ID 处理、static layout 输出和生成产物行为。 |

## 待补文档

当生成器测试矩阵稳定后，在此补充准确的 host Python 版本、fixture 文件、生成输出和验收标准。
