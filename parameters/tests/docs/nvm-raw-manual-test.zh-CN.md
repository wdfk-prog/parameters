[English](./nvm-raw-manual-test.md)

# Raw NVM 手动测试

本文档索引 `par_test_nvm_raw.c` 提供的布局无关 `par_nvm_raw` helper。

## 范围

`par_nvm_raw` 用于一次性持久化数据上的 destructive 板级验证。它可以通过 scalar NVM 后端相对偏移执行 dump、poke、flip 和字节损坏注入，不依赖具体 scalar record layout。

## 构建选项

| 选项 | 要求 |
| --- | --- |
| `AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE` | 必须启用。 |
| `AUTOGEN_PM_USING_NVM` | 必须启用。 |
| `AUTOGEN_PM_NVM_SCALAR` | 必须启用。 |
| `AUTOGEN_PM_TEST_NVM_RAW_HELPER` | 编译 `par_nvm_raw` 命令。 |
| `RT_USING_FINSH` | MSH 命令需要该选项。 |

## 复用接口

helper 动作层可不经过 MSH 直接复用：

```c
int par_test_nvm_raw_exec(int argc, char **argv);
```

RT-Thread `par_nvm_raw` 命令只是该函数的 shell wrapper。后续 host simulator 可在通过 `par_test_set_vprint()` 绑定输出并提供软件 NVM backend 后调用同一接口。

## 待补 checklist

在此记录板级 dump 范围、header 损坏步骤、预期 reboot 行为和清理命令。
