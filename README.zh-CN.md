[English](./README.md)

# autogen_parameter_manager

`autogen_parameter_manager` 是面向 RT-Thread 的生成式参数表管理软件包。它把可移植参数核心、RT-Thread 构建文件、RT-Thread 移植层、可选 MSH 工具和可选持久化存储后端放在同一个软件包中。

适用场景是：固件需要固定参数 schema、类型化访问 API、稳定 ID、生成布局信息、元数据、范围/权限校验，以及受控的 NVM 持久化。它不是通用 Flash KV 数据库、TSDB、IAP 存储库或日志后端。

## 源码包目录结构

```text
.
├── Kconfig                         # RT-Thread menuconfig 使用的软件包选项
├── SConscript                      # RT-Thread/SCons 源文件选择
├── backend/                        # 面向 RT-Thread 的存储后端桥接层
├── port/                           # RT-Thread 移植文件和可选 MSH 命令
├── par_table.def                   # 当前 package profile 编译使用的生成 X-Macro 参数表
└── parameters/                     # 可移植 Device Parameters 模块
```

## 使用步骤：在 RT-Thread BSP 中启用并拉取软件包

### 1. 进入 BSP 工程目录

先进入需要集成本软件包的 BSP 目录，例如：

```sh
cd rt-thread/bsp/stm32/stm32f407-st-discovery
```

### 2. 打开配置界面

使用 RT-Thread Env 或普通 shell 打开配置界面：

```sh
menuconfig
```

如果工程使用 SCons 的 Python 配置界面，也可以使用：

```sh
scons --pyconfig
```

### 3. 选择软件包

在配置界面中进入以下路径并勾选软件包：

```text
RT-Thread online packages
    miscellaneous packages
        autogen_parameter_manager: parameter table management library
```

![Select autogen_parameter_manager in RT-Thread menuconfig](docs/images/rtt-menuconfig-select-autogen-parameter-manager.png)

### 4. 配置软件包功能

进入 `autogen_parameter_manager Configuration` 后，根据项目实际需要选择功能。常用选项如下：

| 配置组 | 常用设置 | 说明 |
| --- | --- | --- |
| 核心元数据 | 按需启用 name/unit/description/range | 未使用的元数据可关闭以减少 ROM。 |
| 对象类型 | 仅当参数表包含 `STR`、`BYTES` 或数组参数时启用 | 纯标量产品可以关闭对象类型。 |
| 生成布局 | 提交生成布局文件后再启用 `AUTOGEN_PM_LAYOUT_SOURCE_SCRIPT` | 不启用时启动阶段扫描参数表。 |
| MSH 工具 | 需要板端查看/维护且启用 `RT_USING_FINSH` 时再启用 | 编译 `par` 命令。 |
| NVM 持久化 | 只有存在 persistent 参数行时启用 | 标量持久化需要选择且只选择一条后端路径。 |

![Configure autogen_parameter_manager package options](docs/images/rtt-pyconfig-autogen-parameter-manager-options.png)

### 5. 保存配置

保存配置后，BSP 的 `.config` 会生成或更新类似配置项：

```text
PKG_USING_AUTOGEN_PARAMETER_MANAGER=y
```

只有保存配置后，软件包管理工具才会根据 `.config` 拉取对应 package。

### 6. 更新并拉取软件包

选择软件包并保存后，需要在 BSP 目录执行以下命令之一来更新并拉取软件包源码。

RT-Thread Env / 命令行常用方式：

```sh
pkgs --update
```

使用 SCons Python 配置流程时，也可以使用：

```sh
scons --pyconfig
```

二者根据工程环境选择一种即可。拉取成功后，BSP 目录下会出现类似目录：

```text
packages/autogen_parameter_manager-latest/
```

如果 `pkgs --update` 克隆成功但 checkout 失败，例如出现 `pathspec 'main' did not match any file(s) known to git`，需要检查 `package.json` 中 `VER` 字段是否和远端实际分支或 tag 一致。

### 7. 构建工程

软件包拉取完成后，正常执行 SCons 构建：

```sh
scons -j8
```

Linux 环境也可以使用：

```sh
scons -j$(nproc)
```

## 参数表维护流程

可编辑参数表是 `parameters/schema/par_table.csv`。修改参数行、ID、生成器配置或模板后，在源码包根目录执行：

```sh
python3 parameters/tools/pargen.py
```

默认输入和输出如下：

| 路径 | 作用 |
| --- | --- |
| `parameters/schema/par_table.csv` | 可编辑参数 schema。 |
| `parameters/schema/par_id_lock.json` | 稳定 enum-to-ID 锁定文件。 |
| `parameters/schema/pargen.json` | 生成器配置。 |
| `par_table.def` | `parameters/src/def/par_def.c` 编译使用的生成 X-Macro 表。 |
| `parameters/generated/` | 生成的静态布局、生成信息和 manifest 文件。 |

需要只校验或用于 CI 时可执行：

```sh
python3 parameters/tools/pargen.py --check
python3 parameters/tools/pargen.py --verify
```

## SCons 集成模型

RT-Thread package manager 下载后的源码包根目录必须保持为当前目录结构。`SConscript` 通过 `DefineGroup()` 纳入源文件，并根据 Kconfig 符号选择性编译。BSP 不应直接手工添加后端适配器或测试文件；需要的功能应通过 `menuconfig` 或 `scons --pyconfig` 选择。

主要源文件选择规则：

- 启用 `PKG_USING_AUTOGEN_PARAMETER_MANAGER` 后，核心 scalar/object/table 源文件会被编译。
- 只有启用 `AUTOGEN_PM_USING_MSH_TOOL` 时才编译 `port/par_shell_tool.c`。
- 只有启用 `AUTOGEN_PM_USING_NVM` 以及对应 layout 选项时才编译 NVM core 和 layout 文件。
- AT24CXX 或 flash-ee 后端桥接文件只在所选后端需要时编译。
- 运行时/手动测试文件只在启用 `AUTOGEN_PM_USING_TESTS` 及具体测试选项时编译。

## BSP 侧最小使用方式

软件包启用、拉取并构建后，应用代码包含公共 API 即可：

```c
#include <par.h>

void parameter_init(void)
{
    (void)par_init();
}
```

业务代码应使用 `par_table.def` 生成的 ID 和类型化 API。产品相关策略放在 `port/par_cfg_port.h`，存储区域大小、地址、分区名等板级配置放在 Kconfig 或 BSP board 配置中。

## NVM 后端选择

启用持久化时，标量持久化需要选择一条后端路径：

| 后端 | Kconfig 路径 | 适用情况 |
| --- | --- | --- |
| RT-Thread AT24CXX | `AUTOGEN_PM_USING_RTT_AT24CXX_BACKEND` | 参数存放在 AT24CXX 兼容外部 EEPROM。 |
| flash-ee through FAL | `AUTOGEN_PM_USING_FLASH_EE_BACKEND` + `AUTOGEN_PM_FLASH_EE_PORT_FAL` | BSP 已启用 FAL，并能预留独立 partition。 |
| flash-ee native | `AUTOGEN_PM_USING_FLASH_EE_BACKEND` + `AUTOGEN_PM_FLASH_EE_PORT_NATIVE` | BSP 不使用 FAL，但能提供原生 flash read/program/erase hook。 |

持久化区域必须由本软件包独占。不要把同一个 EEPROM window 或 Flash partition 同时给 KV 数据库、文件系统、日志区或 bootloader 存储使用。

## 文档入口

- [模块 README](parameters/README.zh-CN.md) 是可移植模块入口。
- [文档总览](parameters/docs/overview.zh-CN.md) 按读者目标映射到详细文档。
- [快速开始](parameters/docs/getting-started.zh-CN.md) 说明必要文件、构建输入和首次使用示例。
- [CSV 参数生成器](parameters/docs/csv-generator.zh-CN.md) 说明 schema 维护、校验、ID 分配和生成布局文件。
- [RT-Thread 软件包集成](parameters/docs/rt-thread-package.zh-CN.md) 说明 wrapper 职责和验证点。
- [与 FlashDB 和 EasyFlash 的区别](parameters/docs/flashdb-easyflash-comparison.zh-CN.md) 说明本软件包与常见 Flash 存储库的职责边界。
- [架构](parameters/docs/architecture.zh-CN.md) 描述运行时模型、布局、校验、ID 查找和 NVM 拆分。
- [API 参考](parameters/docs/api-reference.zh-CN.md) 按职责分组说明公共 API。
- [测试总览](parameters/tests/README.zh-CN.md) 索引运行时测试、NVM 手动测试、生成器测试和待补测试项。
