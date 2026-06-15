[English](./runtime-tests.md)

# Runtime tests

本文档索引由 `parameters/tests/*.c` 编译出的 RT-Thread 运行时测试。

## 构建选项

| 选项 | 用途 |
| --- | --- |
| `AUTOGEN_PM_USING_TESTS` | 启用公共运行时测试框架和 `par_test` MSH 命令。 |
| `AUTOGEN_PM_TEST_USING_RAM_CONFIG` | 启用 `par_test_ram_config.c` 中的 RAM-only 参数 API 覆盖。 |
| `AUTOGEN_PM_TEST_FORCE_RAM_FEATURES` | 选择完整 RAM 测试矩阵需要的可选功能。 |
| `AUTOGEN_PM_TEST_TABLE_ROWS` | 启用 `par_table.def` 中的专用 scalar RAM 测试行。 |
| `AUTOGEN_PM_TEST_OBJECT_ROWS` | 启用 RAM object 测试使用的专用 object 行。 |
| `AUTOGEN_PM_TEST_USING_AT24CXX` | 启用 `par_test_at24cxx.c` 中的 AT24CXX 持久化测试。 |
| `AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE` | 允许 destructive tests 和 helper 修改一次性持久化存储。 |
| `AUTOGEN_PM_TEST_AUTO_RUN` | 应用初始化后自动运行已启用 suite。 |
| `AUTOGEN_PM_TEST_FAIL_FAST` | 当前 suite 首个失败 case 后停止。 |

## 测试代码映射

| 文件 | 覆盖范围 |
| --- | --- |
| `par_test.h` | 公共测试框架声明、suite/case 描述和断言 helper。 |
| `par_test_common.c` | 运行时测试共用的格式化、状态和辅助函数。 |
| `par_test_runner.c` | 可被硬件/host 复用的 suite 注册表、输出回调和 suite 调度 core。 |
| `par_test_msh.c` | `par_test_runner.c` 的 RT-Thread MSH wrapper 和可选 auto-run 入口。 |
| `par_test_ram_config.c` | 非持久化 RAM 参数 API 行为和功能覆盖。 |
| `par_test_at24cxx.c` | 通过公共 API 验证 AT24CXX 后端 scalar persistence 行为。 |

## 复用模型

运行时测试框架已拆分，测试逻辑不再绑定 RT-Thread shell 命令：

| 接口 | 预期使用方 | 说明 |
| --- | --- | --- |
| `par_test_set_vprint()` | 硬件和 host harness | 把稳定测试输出绑定到 RT-Thread、stdout 或捕获缓冲区。 |
| `par_test_run_by_name()` | 硬件和 host harness | 不经过 MSH，直接运行 `all` 或指定 suite。 |
| `par_test_run_suite()` / `par_test_get_suite()` | host harness | 后续 simulator runner 可直接枚举和调度 suite。 |
| `par_test` MSH 命令 | 硬件 HIL | 只作为薄包装层，不保存测试 pass/fail 逻辑。 |

本文件暂不实现 host simulator 代码。后续 runner 应提供软件 parameter/NVM backend，通过 `par_test_set_vprint()` 绑定输出，再调用上述 core 接口。

## 手动 checklist 占位

当 CI 或 HIL 板级 profile 固定后，在此补充逐 case 的期望输出。
