[English](./test-scope.md)

# 测试范围

本仓库的 host tests 应聚焦参数管理器自身行为，或聚焦会生成参数管理器产物的源码工具行为。

## 范围内

- `parameters/src` 和公共头文件暴露的运行时 API 行为。
- 通过参数存储契约暴露的后端行为。
- 生成源码和生成元数据的一致性。
- `tools/pargen.py` 的解析、校验、ID 分配和生成输出语义。
- 产品用户可选择的编译期 feature 与配置约束。

## 范围外

CI harness 自身测试不属于产品 host-test 扩展范围。不要新增主要用于验证 GitHub Actions wrapper、host-test 调度 helper、日志检查器、shell 语法检查或 stale CI binary 行为是否正确的测试。

CI 脚本仍然可以运行产品测试和源码工具测试，但新增产品约束不要求同时补 CI harness 自测，除非该 PR 明确修改的是 CI harness 契约本身。

## 审查规则

新增测试前，先确认它保护的是产品行为还是源码工具行为。如果测试只证明 CI plumbing 能否正确上报失败，应把它归为 CI hygiene，不放入产品测试集。
