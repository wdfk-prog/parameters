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

## Host 测试分组

Host 测试按 contract 强度拆分：

- `mandatory`：默认分组，用于阻塞式 CI，覆盖 public contract、安全不变量、存储完整性以及已文档化的配置行为。
- `current-policy`：兼容性分组，用于 replay fallback 选择、callback 期间持久化策略、重复 role token 处理、adapter partial-progress 行为等依赖当前策略的用例。该分组会在 CI 中可见运行，但 job 设置为非阻塞。
- `all`：本地诊断分组，同时运行 mandatory 与 current-policy 用例。

可以显式运行指定分组：

```sh
PAR_HOST_TEST_GROUP=current-policy bash .github/ci/host-tests.sh
PAR_HOST_TEST_GROUP=all bash .github/ci/host-tests.sh
```

当一个用例记录的是当前策略，而不是永久 API、ABI 或安全 contract 时，用例名应包含 `current_policy`。

默认阻塞 CI 不编译 current-policy 测试体。需要运行这类用例时，测试入口会设置 `PAR_HOST_TEST_GROUP=current-policy` 并通过 `PAR_HOST_ENABLE_CURRENT_POLICY_TESTS=1` 启用编译；该分组应作为非阻塞检查运行。
