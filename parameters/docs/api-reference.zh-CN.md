[English](./api-reference.md)

# API 参考

本文按职责对公共 API 分组。除非狭义内部集成确实需要低层头文件，否则统一包含 `par.h`。

## 生命周期

| API | 用途 |
| --- | --- |
| `par_init()` | 初始化表检查、存储、默认值和可选恢复路径。 |
| `par_deinit()` | 反初始化模块，并在支持时释放平台资源。 |
| `par_is_init()` | 查询模块是否已初始化。 |

## 互斥锁辅助 API

| API | 用途 |
| --- | --- |
| `par_acquire_mutex(par_num)` | 启用互斥锁支持时获取参数互斥锁。 |
| `par_release_mutex(par_num)` | 释放参数互斥锁。 |

仅在集成代码需要在同一锁下组合多个操作时使用这些 API。

## 标量 setter

| API 族 | 用途 |
| --- | --- |
| `par_set_scalar()` | 调用者已知类型时使用的指针式标量 setter。 |
| `par_set_scalar_by_id()` | 启用 ID 支持时按 ID 写入标量。 |
| `par_set_u8()` / `par_set_i8()` | 8 位类型化 setter。 |
| `par_set_u16()` / `par_set_i16()` | 16 位类型化 setter。 |
| `par_set_u32()` / `par_set_i32()` | 32 位类型化 setter。 |
| `par_set_f32()` | 启用 `PAR_CFG_ENABLE_TYPE_F32` 时的浮点 setter。 |

## Fast 标量 setter

`par_set_u8_fast()`、`par_set_u32_fast()` 等 fast setter 面向可信内部代码路径。不应作为外部命令或远程接口的默认 API。

无符号标量支持以下按位辅助 API：

- `par_bitand_set_u8_fast()` / `par_bitor_set_u8_fast()`
- `par_bitand_set_u16_fast()` / `par_bitor_set_u16_fast()`
- `par_bitand_set_u32_fast()` / `par_bitor_set_u32_fast()`

## 标量 getter

| API 族 | 用途 |
| --- | --- |
| `par_get_scalar()` | 指针式标量 getter。 |
| `par_get_scalar_by_id()` | 启用 ID 支持时按 ID 读取标量。 |
| `par_get_u8()` / `par_get_i8()` | 8 位类型化 getter。 |
| `par_get_u16()` / `par_get_i16()` | 16 位类型化 getter。 |
| `par_get_u32()` / `par_get_i32()` | 32 位类型化 getter。 |
| `par_get_f32()` | 启用 `PAR_CFG_ENABLE_TYPE_F32` 时的浮点 getter。 |
| `par_get_scalar_default()` | 从生成表读取标量默认值。 |

所有公共 getter 使用输出指针并返回 `par_status_t`。

## 对象 API

对象 API 仅在启用对象类型支持时编译。

| 类型 | Set API | Get API | 默认值 API |
| --- | --- | --- | --- |
| Bytes | `par_set_bytes()` | `par_get_bytes()` | `par_get_default_bytes()` |
| String | `par_set_str()` | `par_get_str()` | `par_get_default_str()` |
| `U8` array | `par_set_arr_u8()` | `par_get_arr_u8()` | `par_get_default_arr_u8()` |
| `U16` array | `par_set_arr_u16()` | `par_get_arr_u16()` | `par_get_default_arr_u16()` |
| `U32` array | `par_set_arr_u32()` | `par_get_arr_u32()` | `par_get_default_arr_u32()` |

其他对象元数据 API：

- `par_get_obj_len()`
- `par_get_obj_capacity()`
- 启用 `PAR_CFG_ENABLE_ID` 时的 ID 版本。

## 元数据 API

| API | 可用条件 |
| --- | --- |
| `par_get_config()` | 始终可用。 |
| `par_get_name()` | `PAR_CFG_ENABLE_NAME` |
| `par_get_range()` | `PAR_CFG_ENABLE_RANGE` |
| `par_get_unit()` | `PAR_CFG_ENABLE_UNIT` |
| `par_get_desc()` | `PAR_CFG_ENABLE_DESC` |
| `par_get_type()` | 始终可用。 |
| `par_get_access()` | `PAR_CFG_ENABLE_ACCESS` |
| `par_is_persistent()` | `PAR_CFG_NVM_EN` |
| `par_get_num_by_id()` / `par_get_id_by_num()` | `PAR_CFG_ENABLE_ID` |

## Role policy API

启用 `PAR_CFG_ENABLE_ROLE_POLICY` 时可用：

- `par_get_read_roles()`
- `par_get_write_roles()`
- `par_can_read()`
- `par_can_write()`

这些 API 只评估元数据。当前调用者身份和 enforcement 点仍由集成层负责。

## 注册 API

| API | 可用条件 | 用途 |
| --- | --- | --- |
| `par_register_on_change_cb()` | `PAR_CFG_ENABLE_CHANGE_CALLBACK` | 注册参数值变更回调。 |
| `par_register_validation()` | `PAR_CFG_ENABLE_RUNTIME_VALIDATION` | 注册标量运行期校验。 |
| `par_register_obj_validation()` | `PAR_CFG_ENABLE_RUNTIME_VALIDATION` 且 `PAR_CFG_OBJECT_TYPES_ENABLED` | 注册对象运行期校验。 |

核心不会将注册变更与并发 callback 派发串行化。应在单线程初始化阶段注册钩子，或由应用层同步保护。

## 默认值恢复和变更跟踪

默认值恢复 API 是维护/恢复路径，不是外部写入路径。它们有意绕过外部写权限检查和 role-policy 检查。`par_set_to_default()` 以及非 raw 的 `par_set_all_to_default()` 路径还会通过 fast default-restore 语义绕过运行期 validation callback 和 on-change callback。

| API | 用途 |
| --- | --- |
| `par_set_to_default()` | 通过 fast default-restore 路径将单个参数恢复到配置默认值。 |
| `par_set_all_to_default()` | 恢复全部参数；启用 raw reset 时调用 `par_reset_all_to_default_raw()`，否则逐个调用 `par_set_to_default()`。 |
| `par_reset_all_to_default_raw()` | 启用时通过 raw grouped-storage 维护路径恢复全部默认值；同时绕过 setter 侧 range 行为。 |
| `par_has_changed()` | 查询参数是否发生变化。 |

## NVM API

仅在启用 `PAR_CFG_NVM_EN` 时编译。

| API | 用途 |
| --- | --- |
| `par_set_scalar_n_save()` | 写入标量并保存。 |
| `par_save()` | 按内部编号保存单个参数。 |
| `par_save_by_id()` | 按外部 ID 保存单个参数。 |
| `par_save_all()` | 保存全部 persistent 参数。 |
| `par_save_clean()` | 后端支持时清理或压缩后端数据。 |

后端 API 与参数 API 有意分离。见 [Flash-ee 后端设计](./flash-ee-backend-design.zh-CN.md) 和 [RT-Thread 软件包集成](./rt-thread-package.zh-CN.md)。
