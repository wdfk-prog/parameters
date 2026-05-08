# Doxygen 注释考核标准 {#doxygen_assessment}

本文档用于在 GitHub Pages 生成的 Doxygen 站点中统一 API 注释评审口径。考核对象包括公开头文件、模块级源文件、公共宏、类型、枚举、结构体、函数声明，以及会进入 Doxygen 输出的 Markdown 文档。

## 考核结论分级

| 等级 | 分数 | 结论 | 处理要求 |
| --- | ---: | --- | --- |
| A | 90-100 | 可发布 | 可直接进入 GitHub Pages 文档发布。 |
| B | 75-89 | 可合入但需跟踪 | 不阻塞合入，但必须记录后续改进项。 |
| C | 60-74 | 有条件合入 | 仅允许在明确排期修复时合入。 |
| D | 0-59 | 不通过 | 阻塞合入，必须先修复必改项。 |

## 评分项

| 项目 | 分值 | 检查要点 |
| --- | ---: | --- |
| API 覆盖完整性 | 25 | 文件、宏、typedef、enum、struct、public function 等 API 实体有对应 Doxygen 注释。 |
| 标签正确性 | 20 | 函数包含 `@brief`、每个参数对应 `@param`，非 `void` 函数包含 `@return`。 |
| 注释位置与归属 | 15 | Doxygen 块紧贴被说明声明；不把 Doxygen 块挂到赋值、调用、分支、循环或 `return` 语句上。 |
| 成员注释一致性 | 15 | 枚举值、结构体字段等成员优先使用 `/**< ... */` 尾随注释，并在局部声明块内对齐。 |
| 内容有效性 | 15 | 注释说明设计意图、约束、错误恢复、生命周期或安全影响，不重复显而易见的代码行为。 |
| 页面生成质量 | 10 | Doxygen 构建无文档语法错误；Markdown 页面链接可访问；GitHub Pages 输出入口清晰。 |

## 必须修复项

出现以下任一问题时，考核结论不得高于 D 级：

- 可执行语句上方存在 `/** ... */`、`@brief`、`@note`、`@warning` 等 Doxygen 注释。
- 函数参数数量与 `@param` 标签不一致，或存在空标签、占位标签、错误参数名。
- 非 `void` 函数缺少 `@return`，且返回值语义不是显而易见的状态转发。
- Doxygen 注释与实际声明不相邻，导致生成文档归属错误。
- 条件编译块的 `#endif` 注释与起始条件不对应。
- Markdown 主页面或本考核页面没有被 Doxygen `INPUT` 纳入，导致 GitHub Pages 不显示。

## 推荐注释形式

### 文件或公共头文件

```c
/**
 * @file par_core_api.h
 * @brief Core runtime API for the parameter manager.
 */
```

### 函数声明

```c
/**
 * @brief Set a 32-bit unsigned parameter value.
 * @param id Parameter identifier.
 * @param value New parameter value.
 * @return Operation status.
 */
par_status_t par_set_u32(par_id_t id, uint32_t value);
```

### 枚举值或结构体成员

```c
typedef enum par_status_t
{
    ePAR_OK      = 0, /**< Operation completed successfully. */
    ePAR_ERROR   = 1, /**< Generic operation failure. */
    ePAR_TIMEOUT = 2  /**< Operation timed out. */
} par_status_t;
```

### 函数体内实现说明

函数体内部不得使用 Doxygen 注释。确有必要解释非显然逻辑时，使用普通 C 注释：

```c
/* Reset all parameters to defaults because partial NVM recovery can
 * leave the runtime state inconsistent.
 */
par_set_all_to_default();
```

## 评审流程

1. 先检查新增或修改的公开声明，确认 Doxygen 注释与声明一一对应。
2. 再检查函数体，删除只复述代码行为的低价值注释，并把有价值的函数体内 Doxygen 注释改为普通 C 注释。
3. 检查枚举、结构体、联合体成员的尾随注释是否对齐且语义准确。
4. 检查条件编译块，确认 `#endif /* ... */` 注释严格对应当前闭合层级。
5. 运行 Doxygen 构建，确认 GitHub Pages 产物包含主页面、本考核页面和 API 入口。

## GitHub Pages 发布验收

- `.github/workflows/pages-doxygen.yml` 使用 GitHub 官方 Pages actions 链路。
- `Doxyfile` 中 `USE_MDFILE_AS_MAINPAGE` 指向 `docs/doxygen-mainpage.md`。
- `Doxyfile` 中 `INPUT` 包含 `docs`、`parameters/docs`、`parameters/include`、`parameters/src`、`backend`、`port`。
- 生成站点中可从首页进入 `Doxygen 注释考核标准`、`File List`、`Globals` 和 `Data Structures`。
- 工作流在 `main` 分支 push 和手动 `workflow_dispatch` 时均可触发。
