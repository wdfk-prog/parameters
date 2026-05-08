[English](./getting-started.md)

# 快速开始

本文说明从源码到首次参数访问的最短安全路径。

## 集成检查表

1. 将 `include/` 下公共头文件加入 include path。
2. 同时将 `src/` 加入 include path，因为 `par_cfg.h` 仍会包含 `def/par_def.h`、`nvm/par_nvm_cfg.h` 等包内生成头/配置头。
3. 将需要的 `src/` 源文件加入构建。
4. 在可见 include 目录中提供项目自有的 `par_cfg_port.h`。该文件是必需项；无需覆盖配置时可提供带 include guard 的空 stub。
5. 以 `schema/par_table.csv` 作为可编辑参数定义表。
6. 修改参数表或生成器配置后运行 `python3 tools/pargen.py`。
7. 仅在默认钩子不足或选定 atomic 后端要求时，提供 `par_if_port.c` 或 `par_atomic_port.h`。
8. 正常运行时访问前调用一次 `par_init()`。
9. 固件业务代码优先使用类型化 API。
10. 只有在选定并测试存储后端后再启用 NVM。

## 必要文件

| 路径 | 用途 |
| --- | --- |
| `include/` | 公共头文件和默认配置；需要加入 include path。 |
| `src/` | 当前公共配置链会间接包含的私有/生成配置头；需要加入 include path。 |
| 项目 `port/` 目录 | 必须提供 `par_cfg_port.h`；按配置可提供 `par_if_port.c` 和 `par_atomic_port.h`。 |
| `src/par.c` | 主运行时实现。 |
| `src/def/` | 参数表展开和生成 enum/data 访问。 |
| `src/layout/` | 运行时布局计算和生成布局支持。 |
| `src/scalar/` | 标量类型化访问。 |
| `src/object/` | 启用对象类型时的对象参数支持。 |
| `src/nvm/` | 可选持久化层。 |
| `src/port/` | 平台相关内容的端口抽象。 |
| `schema/par_table.csv` | 可编辑参数表。 |
| `tools/pargen.py` | CSV 生成器。 |

`par_cfg_port.h` 不是可选项：`par_cfg_base.h` 会无条件包含它。建议保持该头文件短小并由项目维护，以便产品构建覆盖包默认配置，而不直接编辑上游管理的头文件。

## 生成参数表产物

在仓库根目录执行：

```sh
python3 tools/pargen.py
```

生成器读取 `schema/par_table.csv`、`schema/pargen.json` 和 `schema/par_id_lock.json`，并更新生成的参数表/布局产物。生成产物是构建输入，不应当作手写源码维护。

## 首要配置选择

| 选项 | 常见决策 |
| --- | --- |
| `PAR_CFG_ENABLE_TYPE_F32` | 仅在需要浮点参数时启用。 |
| `PAR_CFG_ENABLE_ID` | CLI、RPC、协议、NVM 或对象持久化需要稳定外部 ID 时保持启用。 |
| `PAR_CFG_LAYOUT_SOURCE` | 简单构建使用 compile-scan；需要可复现偏移时使用 script layout。 |
| `PAR_CFG_NVM_EN` | 选定后端并测试恢复行为后再启用。 |
| `PAR_CFG_ENABLE_RUNTIME_VALIDATION` | 参数约束依赖运行时状态时启用。 |
| `PAR_CFG_ENABLE_CHANGE_CALLBACK` | 其他模块需要响应参数变化时启用。 |

## 最小示例

```c
#include "par.h"

void app_parameters_init(void)
{
    if (par_init() != ePAR_OK)
    {
        return;
    }

    (void)par_set_u8(ePAR_CH1_CTRL, 2u);

    uint8_t ctrl = 0u;
    if (par_get_u8(ePAR_CH1_CTRL, &ctrl) != ePAR_OK)
    {
        return;
    }
}
```

## 运行时访问规则

- 固件代码优先使用 `par_set_u8()`、`par_get_u8()` 等类型化 API。
- 仅当调用者已知运行时类型时使用 `par_set_scalar()` 和 `par_get_scalar()`。
- 对象行使用 `par_set_str()`、`par_get_bytes()`、`par_get_arr_u16()` 等对象 API。
- 不要把对象参数传给标量 getter/setter API。
- 检查每个 `par_status_t` 返回值。

## 持久化流程

1. 在 `schema/par_table.csv` 中将参数行标记为 persistent。
2. 启用 `PAR_CFG_NVM_EN`。
3. 选择一个后端实现。
4. 按端口设计在参数初始化前或初始化过程中初始化存储后端。
5. 根据需要使用 `par_save()`、`par_save_by_id()`、`par_save_all()` 或带 NVM 行为的 setter。
6. 针对选定后端和存储介质测试掉电恢复。

## RT-Thread 用户

当该核心被封装为 RT-Thread 软件包时，软件包层应提供构建选择、端口胶水、存储适配和可选 MSH 命令。见 [RT-Thread 软件包集成](./rt-thread-package.zh-CN.md)。

## 常见错误

- 修改生成文件但没有更新 CSV 源。
- 未有意更新 ID 锁文件就复用外部 ID。
- 未启用稳定外部 ID 就启用对象持久化。
- 从不可信外部接口调用 fast setter。
- 误以为 role metadata 会自动完成权限控制；实际 enforcement 必须由集成层执行。
