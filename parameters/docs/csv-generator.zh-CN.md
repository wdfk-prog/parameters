[English](./csv-generator.md)

# CSV 参数生成器

`tools/pargen.py` 将可编辑 CSV 参数表转换为 C 构建使用的生成产物。

## 输入文件

| 文件 | 用途 |
| --- | --- |
| `schema/par_table.csv` | 参数行和元数据。 |
| `schema/pargen.json` | ID 范围策略和生成器选项。 |
| `schema/par_id_lock.json` | 参数 enum 名到外部 ID 的稳定映射。 |
| `template/*.htmp` / `template/*.deftmp` | 输出模板。 |

## 常用命令

```sh
python3 tools/pargen.py
```

修改参数行、ID 范围、模板或生成器逻辑后，在仓库根目录执行该命令。

## CSV 字段

| 字段 | 含义 |
| --- | --- |
| `group` | 逻辑分组，也用于 ID 范围策略。 |
| `section` | 文档或 UI 分组。 |
| `condition` | 可选编译期条件。 |
| `enum` | 生成的 `par_num_t` enum 符号。 |
| `id` | 稳定外部 ID。 |
| `type` | 标量或对象类型。 |
| `name` | 可读参数名。 |
| `min` / `max` | 标量范围，或按类型表示对象长度/容量策略。 |
| `default` | 默认标量值或对象默认值编码。 |
| `unit` | 可选显示单位。 |
| `access` | 访问模式，通常是 `RO` 或 `RW`。 |
| `read_roles` / `write_roles` | 可选 role-policy 元数据。 |
| `persistent` | 启用 NVM 时该参数是否持久化。 |
| `desc` | 可读描述。 |
| `comment` | 维护者备注，不一定编译进运行时元数据。 |

## ID 策略

使用 `schema/pargen.json` 按 group 预留 ID 范围。使用 `schema/par_id_lock.json` 让已有 enum 到 ID 的映射在参数表演进过程中保持稳定。

推荐规则：

- 除非有意废弃旧持久化记录和外部工具，否则不要把旧 ID 复用给不同语义的参数。
- 新参数放入所属 group 的分配范围。
- 代码审查时检查 `par_id_lock.json` diff。
- hash 冲突应视为必须修复的构建期错误，可通过调整 ID 或 hash 几何解决。

## 生成输出

典型生成产物包括：

- 生成的参数定义数据
- 生成的 ID map 数据
- script-layout 模式下的生成布局信息
- 用于诊断或审查的生成摘要信息

具体文件集合取决于生成器配置和模板。

## 静态布局模式

使用生成布局数据时，构建配置应保持一致：

```c
#define PAR_CFG_LAYOUT_SOURCE         PAR_CFG_LAYOUT_SCRIPT
#define PAR_CFG_LAYOUT_STATIC_INCLUDE "par_layout_static.h"
```

该模式使持久化布局变化更容易审查，因为偏移和容量是确定性生成的。

## 变更流程

1. 修改 `schema/par_table.csv`。
2. 执行 `python3 tools/pargen.py`。
3. 审查生成源码、头文件和 json 的 diff。
4. 构建项目。
5. Python 测试依赖可用时运行生成器测试。
6. 对 persistent 参数，在发布前审查布局和 ID 兼容性。
