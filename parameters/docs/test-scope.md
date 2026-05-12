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
