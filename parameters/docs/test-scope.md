[中文](./test-scope.zh-CN.md)

# Test Scope

This repository keeps host tests focused on behavior that belongs to the parameter manager itself or to source tools that generate parameter-manager artifacts.

## In scope

- Runtime API behavior in `parameters/src` and public headers.
- Backend behavior exposed through the parameter storage contracts.
- Generated source and generated metadata consistency.
- `tools/pargen.py` parsing, validation, ID allocation, and generated output semantics.
- Compile-time feature and configuration constraints that product users can select.

## Out of scope

CI harness self-tests are intentionally out of scope for product host-test expansion. Do not add tests whose main purpose is to verify that GitHub Actions wrappers, host-test dispatch helpers, log checkers, shell syntax checks, or stale CI binaries behave correctly.

CI scripts may still run product tests and source-tool tests, but a new product constraint does not require a matching CI harness self-test unless the PR explicitly changes the CI harness contract.

## Review rule

When adding tests, first identify the product behavior or source-tool behavior being protected. If the test only proves that CI plumbing reports a failure correctly, treat it as CI hygiene and keep it out of the product test suite.

## Host test groups

Host tests are split by contract strength:

- `mandatory`: default group used by blocking CI. It covers public contracts, safety invariants, storage integrity, and documented configuration behavior.
- `current-policy`: compatibility group for strategy-dependent behavior such as replay fallback choices, callback-time persistence policy, duplicate role token handling, and adapter partial-progress behavior. This group is visible in CI but uses a non-blocking job.
- `all`: local diagnostic group that runs both mandatory and current-policy cases.

Run a group explicitly with:

```sh
PAR_HOST_TEST_GROUP=current-policy bash .github/ci/host-tests.sh
PAR_HOST_TEST_GROUP=all bash .github/ci/host-tests.sh
```

Use `current_policy` in the case name when a test records today's policy rather than a permanent API, ABI, or safety contract.

Blocking CI does not compile current-policy test bodies by default. Current-policy jobs set `PAR_HOST_TEST_GROUP=current-policy` so the host harness builds them with `PAR_HOST_ENABLE_CURRENT_POLICY_TESTS=1`; keep that group non-blocking.
