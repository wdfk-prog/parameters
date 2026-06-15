[English](./nvm-schema-evolution-acceptance.md)

# NVM schema 演进执行与验收

本文档描述如何使用 `tests/fixtures/schema_evolution/` fixture 和 `par_nvm_schema` MSH helper 验收 V1/V2 参数表变化后的持久化数据行为。

## 适用范围

该验收流程覆盖：

- scalar-only 追加、删除、插入、类型变化；
- object-only 追加、删除、容量变化；
- scalar + object 混合追加或混合兼容/不兼容变化；
- `AUTOGEN_PM_TABLE_ID_SCHEMA_VER` 主动升级。

该测试需要真实或仿真的可擦写 NVM。执行前必须确认测试固件使用的是可丢弃的 EEPROM/flash 分区。

## 构建开关

V1 与 V2 测试固件均建议启用：

```text
PKG_USING_AUTOGEN_PARAMETER_MANAGER=y
AUTOGEN_PM_USING_DEBUG=y
AUTOGEN_PM_USING_NVM=y
AUTOGEN_PM_USING_TABLE_ID_CHECK=y
AUTOGEN_PM_NVM_SCALAR=y
AUTOGEN_PM_NVM_OBJECT=y
AUTOGEN_PM_NVM_OBJECT_STORE_SHARED=y 或 AUTOGEN_PM_NVM_OBJECT_STORE_DEDICATED=y
AUTOGEN_PM_ENABLE_ID=y
AUTOGEN_PM_ENABLE_TYPE_OBJECT=y
AUTOGEN_PM_ENABLE_TYPE_STR=y
AUTOGEN_PM_ENABLE_TYPE_BYTES=y
AUTOGEN_PM_USING_TESTS=y
AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE=y
AUTOGEN_PM_TEST_NVM_SCHEMA_EVOLUTION=y
RT_USING_FINSH=y
```

根据目标后端选择 AT24CXX 或 flash-ee。object placement 需要按本次验收目标选择：

| placement | 用途 |
| --- | --- |
| `AUTOGEN_PM_NVM_OBJECT_ADDR_AFTER_SCALAR` | 覆盖 scalar 区增长导致 object block 地址移动的风险。 |
| `AUTOGEN_PM_NVM_OBJECT_ADDR_FIXED` | 覆盖 object 固定地址与 scalar layout 增长解耦。 |
| `AUTOGEN_PM_NVM_OBJECT_STORE_DEDICATED` | 覆盖 object 独立后端/分区。 |

如果当前 scalar layout 是 `grouped-payload-only`，参数数量变化属于保守重建路径，scalar append fixture 应按 `verify scalar_rebuild` 验收；只有用例明确要求 object 结果时才使用严格 retain/default 命令。

`v2_schema_version_bump` 场景需要在 V2 固件中额外设置：

```text
AUTOGEN_PM_TABLE_ID_SCHEMA_VER=2
```

V1 固件保持默认：

```text
AUTOGEN_PM_TABLE_ID_SCHEMA_VER=1
```

## fixture 使用方式

测试 fixture 位于：

```text
parameters/tests/fixtures/schema_evolution/
```

每个目录都包含一份 `par_table.csv`。构建某个测试镜像前，将目标 fixture 作为生成器输入，生成 `par_table.def` 和 `parameters/generated/*`，不要直接修改生产 schema。

示例：

```sh
python3 parameters/tools/pargen.py \
  --csv parameters/tests/fixtures/schema_evolution/v1_base/par_table.csv \
  --id-lock parameters/schema/par_id_lock.json \
  --out-def par_table.def \
  --out-dir parameters/generated \
  --manifest parameters/generated/par_manifest.json
```

如果需要保持 fixture 独立 ID lock，可把 `--id-lock` 指向临时目录内的 lock 文件。验收固件只要求 fixture 中的 ID 固定。

## 可复用验收接口

schema evolution 验收已经拆成公共 core 和 RT-Thread MSH 包装层：

| 层级 | 文件 | 用途 |
| --- | --- | --- |
| 公共验收接口 | `../schema_evolution/par_schema_evolution_core.h` | 导出 `par_schema_evolution_prepare()`、`par_schema_evolution_dump()` 和 `par_schema_evolution_verify()`，供板端和后续 host runner 复用。 |
| 公共实现 | `../schema_evolution/par_schema_evolution_core.c` | 保存 fixture ID、V1 写入值和 verify 判据，不包含 MSH 命令解析。 |
| 板端包装 | `../par_test_schema_evolution.c` | 注册 `par_nvm_schema`，只负责命令解析并转调公共接口。 |

后续 GitHub Actions 的 host simulator 可以链接同一套 core 接口，再接一个文件或内存模拟 NVM port。本验收文档暂不要求实现 host simulator，只保证当前板端 helper 的结构可以复用。

## 板端命令

启用 `AUTOGEN_PM_TEST_NVM_SCHEMA_EVOLUTION` 后，会导出：

```text
par_nvm_schema prepare
par_nvm_schema dump
par_nvm_schema verify base
par_nvm_schema verify scalar_append
par_nvm_schema verify object_append
par_nvm_schema verify mixed_append
par_nvm_schema verify scalar_append_object_rebuild
par_nvm_schema verify object_rebuild
par_nvm_schema verify scalar_rebuild
par_nvm_schema verify scalar_rebuild_object_retain
par_nvm_schema verify scalar_rebuild_object_default
par_nvm_schema verify full_rebuild
```

命令含义：

| 命令 | 使用阶段 | 说明 |
| --- | --- | --- |
| `prepare` | V1 固件 | 初始化参数模块，写入 V1 非默认值并 `par_save_clean()`。 |
| `dump` | V1/V2 固件 | 打印固定测试 ID 是否存在、内部参数号、类型和 persistent 标记。 |
| `verify base` | V1 固件重启后 | 验证 V1 写入值可从 NVM 重新加载。 |
| `verify scalar_append` | append 类 V2 固件 | 推荐命令。根据当前编译出的 object placement 自动选择“object 保留”或“object 默认重建”判据。 |
| `verify object_append` | object append V2 固件 | 强制验证 V1 scalar/object 旧值都保留，V2 新增参数使用默认值。 |
| `verify scalar_append_object_rebuild` | scalar 增长 + after-scalar object placement | 强制验证 scalar 旧值保留，但 object block 因地址移动按默认值重建。 |
| `verify object_rebuild` | object 删除、object capacity/type 不兼容 V2 固件 | 验证 scalar 旧值保留，同时 object 行恢复默认值或已删除。 |
| `verify scalar_rebuild` | scalar 删除、插入、类型变化等 V2 固件 | 验证 scalar 行恢复默认值；object 行允许保留、默认重建或不存在。 |
| `verify scalar_rebuild_object_retain` | scalar 重建且要求 object 保留的 V2 固件 | 严格验证 scalar 行恢复默认值，同时兼容 object 行保留 V1 写入值。 |
| `verify scalar_rebuild_object_default` | scalar 重建且要求 object 重建的 V2 固件 | 严格验证 scalar 行恢复默认值，同时 object 行恢复默认或不存在。 |
| `verify full_rebuild` | schema version bump 或明确要求全局重建的 V2 固件 | 验证已知存在参数恢复默认值，说明触发了全局受控重建。 |

固定测试 ID：

| ID | 角色 | 默认值 | `prepare` 写入值 |
| --- | --- | --- | --- |
| `60006` | base U8 scalar | `11` | `42` |
| `60007` | tail U16 scalar | `1000` | `4242` |
| `60008` | base STR object | `v1` | `hv1` |
| `60009` | base BYTES object | `10 20 30 40` | `A5 5A C3 3C` |
| `60010` | appended U8 scalar | `55` | V1 不存在 |
| `60011` | appended STR object | `new` | V1 不存在 |

## 通用验收步骤

1. 使用 `v1_base/par_table.csv` 生成并构建 V1 固件。
2. 烧录 V1 固件。
3. 执行：

```text
par_nvm_schema prepare
```

期望输出包含：

```text
PAR_SCHEMA_PREPARED base_u8=42 tail_u16=4242 name=hv1 blob=A5-5A-C3-3C
```

4. 复位或重新上电，仍使用 V1 固件执行：

```text
par_nvm_schema verify base
```

期望输出：

```text
PAR_SCHEMA_VERIFY PASS mode=base(0)
```

5. 不擦除 NVM，只替换为目标 V2 固件。
6. 根据 V2 fixture 类型执行对应 verify 命令。

## 详细执行步骤：`v2_scalar_append` object placement aware

本节把矩阵里的这一行展开成完整操作：

```text
v2_scalar_append    any object placement    par_nvm_schema verify scalar_append
```

该场景验证：V2 只在 scalar 区尾部追加 `60010`。推荐统一使用 `par_nvm_schema verify scalar_append`，helper 会根据当前编译配置自动判断 object 旧值应保留还是应默认重建。

### A. 选择 object placement 配置

三种配置需要分别验收。它们覆盖的风险不同，不能互相替代：

| 目标 | 关键配置 | append 期望 |
| --- | --- | --- |
| after-scalar shared object block | `AUTOGEN_PM_NVM_OBJECT_STORE_SHARED=y` + `AUTOGEN_PM_NVM_OBJECT_ADDR_AFTER_SCALAR=y` | scalar 保留，object 因地址移动恢复默认 |
| fixed shared object block | `AUTOGEN_PM_NVM_OBJECT_STORE_SHARED=y` + `AUTOGEN_PM_NVM_OBJECT_ADDR_FIXED=y` | scalar/object 均保留 |
| dedicated object store | `AUTOGEN_PM_NVM_OBJECT_STORE_DEDICATED=y` | scalar/object 均保留 |

V1 和 V2 必须使用同一组 object placement/backend 配置。只允许替换 schema fixture，不要在 V1/V2 之间切换 object 存储模式。

若从 V2 固件切回 V1 固件且没有擦除 NVM，V1 启动阶段可能先看到旧 V2 NVM header，例如 `stored_obj_count=3` 或 table-ID mismatch，并触发一次默认重建。这不是 V1 基线验收失败；本流程的 V1 基准以随后执行 `par_nvm_schema prepare` 并重启后 `verify base` 通过为准。

### B. 构建并烧录 V1 固件

使用 V1 fixture 生成参数表：

```sh
python3 parameters/tools/pargen.py \
  --csv parameters/tests/fixtures/schema_evolution/v1_base/par_table.csv \
  --id-lock parameters/schema/par_id_lock.json \
  --out-def par_table.def \
  --out-dir parameters/generated \
  --manifest parameters/generated/par_manifest.json
```

然后使用上面选择的 after-scalar、fixed 或 dedicated object placement 配置构建、烧录 V1 固件。

### C. V1 写入基准 NVM 数据

V1 固件启动完成后，先确认 fixture ID 是否符合预期：

```text
par_nvm_schema dump
```

期望首先包含 `SCHEMA_CFG`，用于判断当前验收分支。三种合法输出如下。

如果是 after-scalar placement：

```text
SCHEMA_CFG object_placement=after_scalar append_object_expect=default
```

如果是 fixed placement：

```text
SCHEMA_CFG object_placement=fixed append_object_expect=retain
```

如果是 dedicated placement：

```text
SCHEMA_CFG object_placement=dedicated append_object_expect=retain
```

如果你的输出是 `after_scalar/default`，说明当前正在验收 after-scalar 分支，不应该套用 fixed/dedicated 的 object 保留判据。

然后包含：

```text
SCHEMA_ID id=60006 name=base_u8 ... persistent=1
SCHEMA_ID id=60007 name=tail_u16 ... persistent=1
SCHEMA_ID id=60008 name=name_str ... persistent=1
SCHEMA_ID id=60009 name=blob_bytes ... persistent=1
SCHEMA_ID id=60010 name=new_u8 absent ...
```

执行写入：

```text
par_nvm_schema prepare
```

期望输出：

```text
PAR_SCHEMA_PREPARED base_u8=42 tail_u16=4242 name=hv1 blob=A5-5A-C3-3C
```

复位或重新上电，仍然使用 V1 固件验证 NVM 确实保存成功：

```text
par_nvm_schema verify base
```

期望至少包含：

```text
OK base_u8 id=60006 value=42
OK tail_u16 id=60007 value=4242
OK name_str id=60008 value=hv1
OK blob_bytes id=60009 len=4
PAR_SCHEMA_VERIFY PASS mode=base(0)
```

如果这里失败，不要继续 V2 验收；先排查 NVM 后端、fixture、persistent 配置或 object 类型开关。

### D. 不擦除 NVM，构建并烧录 V2 固件

不要执行全片擦除、不要擦 EEPROM/flash-ee/FAL 分区。V2 验收的核心就是复用 C 步骤写入的 V1 NVM 数据。

使用 `v2_scalar_append` fixture 生成参数表：

```sh
python3 parameters/tools/pargen.py \
  --csv parameters/tests/fixtures/schema_evolution/v2_scalar_append/par_table.csv \
  --id-lock parameters/schema/par_id_lock.json \
  --out-def par_table.def \
  --out-dir parameters/generated \
  --manifest parameters/generated/par_manifest.json
```

保持与 V1 完全相同的 backend、scalar layout、object placement 配置，重新构建并烧录 V2 固件。

### E. V2 验证 `v2_scalar_append`

V2 启动完成后，先确认当前 object placement 与 `60010` 是否符合预期：

```text
par_nvm_schema dump
```

期望首先包含当前配置判据。三种合法输出如下。

如果是 after-scalar placement：

```text
SCHEMA_CFG object_placement=after_scalar append_object_expect=default
```

如果是 fixed placement：

```text
SCHEMA_CFG object_placement=fixed append_object_expect=retain
```

如果是 dedicated placement：

```text
SCHEMA_CFG object_placement=dedicated append_object_expect=retain
```

`scalar_append` 会根据这行自动选择 object 验收判据。

然后包含：

```text
SCHEMA_ID id=60006 name=base_u8 ... persistent=1
SCHEMA_ID id=60007 name=tail_u16 ... persistent=1
SCHEMA_ID id=60008 name=name_str ... persistent=1
SCHEMA_ID id=60009 name=blob_bytes ... persistent=1
SCHEMA_ID id=60010 name=new_u8 ... persistent=1
```

执行 placement-aware append 验证：

```text
par_nvm_schema verify scalar_append
```

如果 `SCHEMA_CFG` 是 `fixed/retain` 或 `dedicated/retain`，期望至少包含：

```text
SCHEMA_VERIFY scalar_append placement=fixed object_expect=retain
OK base_u8 id=60006 value=42
OK tail_u16 id=60007 value=4242
OK name_str id=60008 value=hv1
OK blob_bytes id=60009 len=4
OK new_u8_default id=60010 default type=
PAR_SCHEMA_VERIFY PASS mode=scalar_append(1)
```

如果是 dedicated placement，第一行中的 `placement=fixed` 会变成 `placement=dedicated`。

如果 `SCHEMA_CFG` 是 `after_scalar/default`，不要使用本段 retain 判据，继续看下一节 after-scalar 判据。

判定标准：

| ID | 验证点 | 通过判据 |
| --- | --- | --- |
| `60006` | V1 scalar 保留 | 输出 `value=42` |
| `60007` | V1 scalar 保留 | 输出 `value=4242` |
| `60008` | V1 STR object 保留 | 输出 `value=hv1` |
| `60009` | V1 BYTES object 保留 | 输出 `len=4`，且 helper 内部已比较 `A5 5A C3 3C` |
| `60010` | V2 新增 scalar 默认值 | 输出 `OK new_u8_default id=60010 default type=...`，默认值由 helper 从 V2 编译期默认值读取，应为 `55` |

`verify scalar_append` 已经在代码里完成逐项读取和比较，不需要手工再读每个 ID。人工只需要确认最后一行是：

```text
PAR_SCHEMA_VERIFY PASS mode=scalar_append(1)
```

如果需要手工复核 `60010` 的实际值，可使用普通参数读命令或临时调试接口读取 ID `60010`，应为 `55`。

## 详细执行步骤：`v2_scalar_append` after-scalar object placement

你的日志如果类似下面这样，说明当前就是 after-scalar 分支：

```text
SCHEMA_CFG object_placement=after_scalar append_object_expect=default
```

此时文档中 fixed/dedicated 的 `append_object_expect=retain` 不适用。

如果配置为：

```text
AUTOGEN_PM_NVM_OBJECT_STORE_SHARED=y
AUTOGEN_PM_NVM_OBJECT_ADDR_AFTER_SCALAR=y
```

V1 准备步骤命令仍然使用上节 B/C，但 `par_nvm_schema dump` 的配置行应为：

```text
SCHEMA_CFG object_placement=after_scalar append_object_expect=default
```

V2 仍然推荐使用自动判据命令：

```text
par_nvm_schema verify scalar_append
```

期望至少包含：

```text
SCHEMA_VERIFY scalar_append placement=after_scalar object_expect=default
OK base_u8 id=60006 value=42
OK tail_u16 id=60007 value=4242
OK new_u8_default id=60010 default type=
OK name_str_default id=60008 default=v1
OK blob_bytes_default id=60009 default_len=4
PAR_SCHEMA_VERIFY PASS mode=scalar_append(1)
```

判定标准：

| ID | 验证点 | 通过判据 |
| --- | --- | --- |
| `60006` | V1 scalar 保留 | 输出 `value=42` |
| `60007` | V1 scalar 保留 | 输出 `value=4242` |
| `60008` | object block 地址随 scalar 区增长移动 | 恢复为 V2 编译期默认值，不再是 `hv1` |
| `60009` | object block 地址随 scalar 区增长移动 | 恢复为 V2 编译期默认 payload，不再是 `A5 5A C3 3C` |
| `60010` | V2 新增 scalar 默认值 | 默认值为 `55` |

注意：after-scalar placement 下，`v2_scalar_append` 不应该用 `verify object_append` 强制验收。scalar 区增长会改变 object block 起始地址，旧 object 数据按默认值重建才是该配置的预期行为。若启动日志出现 `object initialization finished with status=WARN SET TO DEF`，同时 `par_nvm_schema dump` 输出 `SCHEMA_CFG object_placement=after_scalar append_object_expect=default`，应使用 `verify scalar_append` 或 `verify scalar_append_object_rebuild`。

## 其他场景的最小操作模板

每个 V2 fixture 都按同一流程执行：

1. 使用 `v1_base` 构建并烧录 V1。
2. V1 执行 `par_nvm_schema prepare`。
3. V1 复位后执行 `par_nvm_schema verify base`，确认 `PASS mode=base(0)`。
4. 不擦除 NVM。
5. 使用目标 V2 fixture 重新生成参数表、构建、烧录 V2。
6. 执行矩阵中对应的 `par_nvm_schema verify ...` 命令。
7. 只要最后不是 `PAR_SCHEMA_VERIFY PASS ...`，该场景失败。

## 场景矩阵

| V2 fixture | V2 额外配置 | 验收命令 | 期望结果 |
| --- | --- | --- | --- |
| `v2_scalar_append` | fixed/dedicated object placement | `par_nvm_schema verify scalar_append` | `60006/60007/60008/60009` 保留 V1 写入值；`60010` 为默认值 `55`。 |
| `v2_scalar_append` | after-scalar object placement | `par_nvm_schema verify scalar_append` | scalar 旧值保留；object 因地址移动恢复默认；`60010` 为默认值。 |
| `v2_object_append` | 无 | `par_nvm_schema verify object_append` | V1 写入值保留；`60011` 为默认值 `new`。 |
| `v2_mixed_append` | fixed/dedicated object placement | `par_nvm_schema verify mixed_append` | V1 scalar/object 写入值保留；`60010/60011` 为默认值。 |
| `v2_mixed_append` | after-scalar object placement | `par_nvm_schema verify mixed_append` | scalar 旧值保留；object 旧值恢复默认；`60010/60011` 为默认值。 |
| `v2_scalar_delete_tail` | 无 | `par_nvm_schema verify scalar_rebuild` | scalar 行恢复默认或被删除；object 行允许保留、默认重建或不存在。需要固定 object 结果时使用严格命令。 |
| `v2_scalar_delete_middle` | 无 | `par_nvm_schema verify object_rebuild` | 当前 fixture 删除的是中间 object；scalar 旧值保留，被删除 object 可不存在，其他 object 恢复默认。 |
| `v2_scalar_insert_middle` | 无 | `par_nvm_schema verify scalar_rebuild` | scalar 行恢复默认；object 行允许保留、默认重建或不存在。需要固定 object 结果时使用严格命令。 |
| `v2_scalar_type_change` | 无 | `par_nvm_schema verify scalar_rebuild` | 同 ID 不同 scalar 类型不得按旧 payload 解释；object 行允许保留、默认重建或不存在。常见 object 保留路径可加跑 `scalar_rebuild_object_retain`。 |
| `v2_object_delete` | 无 | `par_nvm_schema verify object_rebuild` | scalar 旧值保留；被删除 object 可不存在，保留 object 恢复默认。 |
| `v2_object_capacity_change` | 无 | `par_nvm_schema verify object_rebuild` | scalar 旧值保留；同 object ID 不同 capacity 不得复用旧 payload，应恢复默认。 |
| `v2_mixed_scalar_compatible_object_incompatible` | 无 | `par_nvm_schema verify scalar_append_object_rebuild` | scalar 旧值保留，新增 scalar 默认；object 不兼容行恢复默认。 |
| `v2_mixed_scalar_incompatible_object_compatible` | 无 | `par_nvm_schema verify scalar_rebuild` | scalar 行恢复默认；object 行允许保留、默认重建或不存在。需要固定 object 结果时使用严格命令。 |
| `v2_schema_version_bump` | `AUTOGEN_PM_TABLE_ID_SCHEMA_VER=2` | `par_nvm_schema verify full_rebuild` | 表内容不变但 schema version 改变，应恢复默认值。 |

## 通过标准

- V1 `prepare` 成功，并且 V1 重启后的 `verify base` 通过。
- append 类场景优先使用 `verify scalar_append`，并确认最终 `PASS`。
- object-only 不兼容场景的 `verify object_rebuild` 通过。
- scalar 不兼容场景的 `verify scalar_rebuild` 通过；若已知 object 预期结果，可额外执行严格 retain/default 命令。
- schema version bump 场景的 `verify full_rebuild` 通过。
- 日志能区分正常恢复、table-ID mismatch 或 rebuild 路径；若启用 `AUTOGEN_PM_LOG_LEVEL_PREFIX`，WARN/ERROR 日志应带等级前缀。
- 每轮 V2 验收前必须复用同一份 V1 NVM 内容，不能先擦除 NVM。

## 失败定位建议

- `prepare` 失败：先执行 `par_nvm_schema dump`，确认 V1 fixture ID 都存在且 persistent=1。
- `verify scalar_append` 失败：先看 `SCHEMA_VERIFY scalar_append placement=... object_expect=...`。若 `object_expect=default`，object 恢复默认是预期；若仍用 `verify object_append`，会因为 `hv1` 变回默认 `v1` 而失败。
- `verify object_rebuild` 中 scalar 仍保持 V1 写入值是预期行为；该命令只要求 object 默认重建。
- `verify scalar_rebuild` 允许 object 保留、默认重建或不存在，因为 scalar layout 不兼容并不总是强制 object storage 重建。
- `verify full_rebuild` 中仍读到 V1 写入值：检查 `AUTOGEN_PM_USING_TABLE_ID_CHECK` 和 `AUTOGEN_PM_TABLE_ID_SCHEMA_VER` 是否生效。
- object 场景失败：分别验证 object store mode 和 object address mode，尤其是 after-scalar placement 下 scalar 区增长会移动 object block。
