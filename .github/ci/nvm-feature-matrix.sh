#!/usr/bin/env bash
# Validate legal and intentionally invalid NVM feature combinations.
#
# This matrix validates product configuration contracts in the parameter
# sources. It should not grow CI harness self-tests.
set -euo pipefail

build_dir="${BUILD_DIR:-build/host-tests}/nvm-feature-matrix"
mkdir -p "$build_dir"

common_includes=(
    -Iparameters/tests/host/fixtures_scalar_min
    -Iparameters/tests/host/fixtures
    -I.
    -Ibackend
    -Iport
    -Iparameters/include
    -Iparameters/src
    -Iparameters/src/def
    -Iparameters/src/detail
    -Iparameters/src/layout
    -Iparameters/src/nvm
    -Iparameters/src/nvm/backend
    -Iparameters/src/nvm/object
    -Iparameters/src/nvm/object/addr
    -Iparameters/src/nvm/object/store
    -Iparameters/src/nvm/scalar
    -Iparameters/src/nvm/scalar/layout
    -Iparameters/src/nvm/scalar/store
    -Iparameters/src/object
    -Iparameters/src/port
    -Iparameters/src/scalar
)

common_cflags=(
    -std=c11
    -Wall
    -Wextra
    -Wno-unused-parameter
    -fsanitize=address,undefined
    -fno-omit-frame-pointer
    "${common_includes[@]}"
)

sources=(
    parameters/tests/host/test_par_config_smoke.c
    parameters/src/par.c
    parameters/src/object/par_object.c
    parameters/src/object/par_object_api.c
    parameters/src/scalar/par_scalar_api.c
    parameters/src/def/par_def.c
    parameters/src/def/par_id_map_static.c
    parameters/src/layout/par_layout.c
    parameters/src/port/par_if.c
)

object_type_off_defines=(
    -DPAR_CFG_OBJECT_TYPES_ENABLED=0
    -DPAR_CFG_ENABLE_TYPE_STR=0
    -DPAR_CFG_ENABLE_TYPE_BYTES=0
    -DPAR_CFG_ENABLE_TYPE_ARR_U8=0
    -DPAR_CFG_ENABLE_TYPE_ARR_U16=0
    -DPAR_CFG_ENABLE_TYPE_ARR_U32=0
)

run_compile_pass() {
    local name="$1"
    shift
    echo "[nvm-feature-matrix] compile-pass $name"
    gcc "${common_cflags[@]}" "$@" "${sources[@]}" -fsyntax-only
    echo "NVM_FEATURE_CASE_PASS $name"
}

run_compile_fail() {
    local name="$1"
    local expected="$2"
    shift 2
    local log="$build_dir/$name.log"

    echo "[nvm-feature-matrix] compile-fail $name"
    if gcc "${common_cflags[@]}" "$@" "${sources[@]}" -fsyntax-only >"$log" 2>&1; then
        echo "expected compile failure for $name" >&2
        return 1
    fi
    if ! grep -F -- "$expected" "$log" >/dev/null; then
        echo "compile failure for $name did not contain expected diagnostic: $expected" >&2
        cat "$log" >&2
        return 1
    fi
    echo "NVM_FEATURE_CASE_PASS $name"
}

# Scalar-only write-verify profiles set object storage count to zero.
# Keep their syntax-only logs focused on configuration diagnostics.
run_compile_pass scalar_write_verify_off \
    -DPAR_HOST_TEST_NVM \
    -DPAR_CFG_NVM_WRITE_VERIFY_EN=0 \
    -Wno-type-limits

run_compile_pass scalar_write_verify_on \
    -DPAR_HOST_TEST_NVM \
    -DPAR_CFG_NVM_WRITE_VERIFY_EN=1 \
    -Wno-type-limits

run_compile_pass scalar_nvm_object_disabled \
    -DPAR_HOST_TEST_NVM \
    -DPAR_CFG_NVM_OBJECT_EN=0 \
    "${object_type_off_defines[@]}"

run_compile_fail table_id_without_nvm \
    "Disable table ID checking" \
    -DPAR_CFG_NVM_EN=0 \
    -DPAR_CFG_NVM_SCALAR_EN=0 \
    -DPAR_CFG_NVM_OBJECT_EN=0 \
    -DPAR_CFG_TABLE_ID_CHECK_EN=1 \
    "${object_type_off_defines[@]}"

run_compile_fail scalar_persistence_without_nvm \
    "scalar persistence requires PAR_CFG_NVM_EN = 1" \
    -DPAR_CFG_NVM_EN=0 \
    -DPAR_CFG_NVM_SCALAR_EN=1 \
    -DPAR_CFG_NVM_OBJECT_EN=0 \
    -DPAR_CFG_TABLE_ID_CHECK_EN=0 \
    "${object_type_off_defines[@]}"

run_compile_fail object_persistence_without_object_types \
    "object persistence requires at least one object type enabled" \
    -DPAR_HOST_TEST_NVM \
    -DPAR_CFG_NVM_OBJECT_EN=1 \
    "${object_type_off_defines[@]}"

run_compile_fail object_persistence_without_id \
    "object persistence requires PAR_CFG_ENABLE_ID = 1" \
    -DPAR_HOST_TEST_NVM \
    -DPAR_CFG_NVM_OBJECT_EN=1 \
    -DPAR_CFG_ENABLE_ID=0

printf 'NVM_FEATURE_SUMMARY Passed %u/%u\n' 7 7
