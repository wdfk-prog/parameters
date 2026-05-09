#!/usr/bin/env bash
set -euo pipefail

build_dir="${BUILD_DIR:-build/host-tests}/backend-adapter-smoke"
mkdir -p "$build_dir"

common_includes=(
    -I.github/ci/test
    -Iparameters/tests/host/fixtures_backend
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
    -Iparameters/src/nvm/scalar/layout
    -Iparameters/src/port
)

common_cflags=(
    -std=c11
    -Wall
    -Wextra
    -Wno-unused-parameter
    -fsanitize=address,undefined
    -fno-omit-frame-pointer
    "${common_includes[@]}"
    -DPAR_HOST_TEST_NVM
    -DPAR_CFG_NVM_OBJECT_EN=0
    -DPAR_CFG_OBJECT_TYPES_ENABLED=0
    -DPAR_CFG_ENABLE_TYPE_F32=0
    -DPAR_CFG_ENABLE_TYPE_STR=0
    -DPAR_CFG_ENABLE_TYPE_BYTES=0
    -DPAR_CFG_ENABLE_TYPE_ARR_U8=0
    -DPAR_CFG_ENABLE_TYPE_ARR_U16=0
    -DPAR_CFG_ENABLE_TYPE_ARR_U32=0
)

build_and_run() {
    local name="$1"
    shift
    local output="$build_dir/$name"

    echo "[backend-adapter-smoke] build $name"
    gcc "${common_cflags[@]}" "$@" -o "$output"
    echo "[backend-adapter-smoke] run $name"
    "$output"
}

build_and_run flash_ee_fal \
    -DPAR_CFG_NVM_BACKEND_FLASH_EE_EN=1 \
    -DPAR_CFG_NVM_BACKEND_FLASH_EE_PORT_NATIVE_EN=0 \
    -DPAR_CFG_NVM_BACKEND_FLASH_EE_PORT_FAL_EN=1 \
    parameters/tests/host/test_par_backend_flash_ee_fal_smoke.c \
    .github/ci/test/par_backend_adapter_ci_stubs.c \
    backend/par_store_backend_flash_ee_fal.c \
    parameters/src/nvm/backend/par_store_backend_flash_ee.c \
    parameters/src/port/par_if.c

build_and_run rtt_at24cxx \
    -DPAR_CFG_NVM_BACKEND_FLASH_EE_EN=0 \
    -DPAR_CFG_NVM_BACKEND_RTT_AT24CXX_EN=1 \
    -DPAR_CFG_RTT_AT24_I2C_BUS_NAME='"i2c1"' \
    -DPAR_CFG_RTT_AT24_ADDR_INPUT=0 \
    -DPAR_CFG_RTT_AT24_BASE_ADDR=0 \
    -DPAR_CFG_RTT_AT24_SIZE=64 \
    -DPAR_STORE_RTT_AT24_ERASE_CHUNK=8 \
    parameters/tests/host/test_par_backend_rtt_at24cxx_smoke.c \
    .github/ci/test/par_backend_adapter_ci_stubs.c \
    backend/par_store_backend_rtt_at24cxx.c

printf 'BACKEND_ADAPTER_SUMMARY Passed %u/%u\n' 2 2
