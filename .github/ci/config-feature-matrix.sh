#!/usr/bin/env bash
set -euo pipefail

build_dir="${BUILD_DIR:-build/host-tests}/config-feature-matrix"
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

run_case() {
    local name="$1"
    shift
    local output="$build_dir/$name"

    echo "[config-feature-matrix] build $name"
    gcc "${common_cflags[@]}" "$@" "${sources[@]}" -o "$output"
    echo "[config-feature-matrix] run $name"
    "$output"
    echo "CONFIG_FEATURE_CASE_PASS $name"
}

run_case minimal_scalar_no_nvm \
    -DPAR_CFG_OBJECT_TYPES_ENABLED=0 \
    -DPAR_CFG_ENABLE_TYPE_F32=0 \
    -DPAR_CFG_ENABLE_TYPE_STR=0 \
    -DPAR_CFG_ENABLE_TYPE_BYTES=0 \
    -DPAR_CFG_ENABLE_TYPE_ARR_U8=0 \
    -DPAR_CFG_ENABLE_TYPE_ARR_U16=0 \
    -DPAR_CFG_ENABLE_TYPE_ARR_U32=0

run_case scalar_no_id_no_range \
    -DPAR_CFG_OBJECT_TYPES_ENABLED=0 \
    -DPAR_CFG_ENABLE_TYPE_F32=0 \
    -DPAR_CFG_ENABLE_TYPE_STR=0 \
    -DPAR_CFG_ENABLE_TYPE_BYTES=0 \
    -DPAR_CFG_ENABLE_TYPE_ARR_U8=0 \
    -DPAR_CFG_ENABLE_TYPE_ARR_U16=0 \
    -DPAR_CFG_ENABLE_TYPE_ARR_U32=0 \
    -DPAR_CFG_ENABLE_ID=0 \
    -DPAR_CFG_ENABLE_RANGE=0 \
    -DPAR_CFG_TABLE_ID_CHECK_EN=0

run_case scalar_no_metadata \
    -DPAR_CFG_OBJECT_TYPES_ENABLED=0 \
    -DPAR_CFG_ENABLE_TYPE_F32=0 \
    -DPAR_CFG_ENABLE_TYPE_STR=0 \
    -DPAR_CFG_ENABLE_TYPE_BYTES=0 \
    -DPAR_CFG_ENABLE_TYPE_ARR_U8=0 \
    -DPAR_CFG_ENABLE_TYPE_ARR_U16=0 \
    -DPAR_CFG_ENABLE_TYPE_ARR_U32=0 \
    -DPAR_CFG_ENABLE_NAME=0 \
    -DPAR_CFG_ENABLE_UNIT=0 \
    -DPAR_CFG_ENABLE_DESC=0 \
    -DPAR_CFG_ENABLE_DESC_CHECK=0

run_case scalar_no_access_role \
    -DPAR_CFG_OBJECT_TYPES_ENABLED=0 \
    -DPAR_CFG_ENABLE_TYPE_F32=0 \
    -DPAR_CFG_ENABLE_TYPE_STR=0 \
    -DPAR_CFG_ENABLE_TYPE_BYTES=0 \
    -DPAR_CFG_ENABLE_TYPE_ARR_U8=0 \
    -DPAR_CFG_ENABLE_TYPE_ARR_U16=0 \
    -DPAR_CFG_ENABLE_TYPE_ARR_U32=0 \
    -DPAR_CFG_ENABLE_ACCESS=0 \
    -DPAR_CFG_ENABLE_ROLE_POLICY=0

run_case scalar_no_validation_callback \
    -DPAR_CFG_OBJECT_TYPES_ENABLED=0 \
    -DPAR_CFG_ENABLE_TYPE_F32=0 \
    -DPAR_CFG_ENABLE_TYPE_STR=0 \
    -DPAR_CFG_ENABLE_TYPE_BYTES=0 \
    -DPAR_CFG_ENABLE_TYPE_ARR_U8=0 \
    -DPAR_CFG_ENABLE_TYPE_ARR_U16=0 \
    -DPAR_CFG_ENABLE_TYPE_ARR_U32=0 \
    -DPAR_CFG_ENABLE_RUNTIME_VALIDATION=0 \
    -DPAR_CFG_ENABLE_CHANGE_CALLBACK=0 \
    -DPAR_CFG_ENABLE_RESET_ALL_RAW=0

printf 'CONFIG_FEATURE_SUMMARY Passed %u/%u\n' 5 5
