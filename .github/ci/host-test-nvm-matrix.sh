#!/usr/bin/env bash
# Host-test helpers for flash-ee NVM matrix cases.

nvm_profile_script_dir="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=.github/ci/host-test-nvm-profiles.sh
. "$nvm_profile_script_dir/host-test-nvm-profiles.sh"

run_nvm_flash_ee_profile() {
    local profile="$1"
    local record_layout="$2"
    local object_store="$3"
    local object_addr="$4"
    local object_fixed_addr="$5"
    local object_region_size="$6"
    local layout_source="$7"
    local extra_defines=()

    if [ "$profile" = "object_dedicated_nonzero_base" ]; then
        extra_defines=(-DPAR_CFG_NVM_OBJECT_DEDICATED_BASE_ADDR=0x20U)
    fi

    compile_and_run_nvm "par_nvm_flash_ee_${profile}" \
        -DPAR_HOST_TEST_NVM \
        -DPAR_HOST_TEST_PROFILE_NAME="\"${profile}\"" \
        -DPAR_CFG_NVM_RECORD_LAYOUT="${record_layout}" \
        -DPAR_CFG_NVM_OBJECT_STORE_MODE="${object_store}" \
        -DPAR_CFG_NVM_OBJECT_ADDR_MODE="${object_addr}" \
        -DPAR_CFG_NVM_OBJECT_FIXED_ADDR="${object_fixed_addr}" \
        -DPAR_CFG_NVM_OBJECT_REGION_SIZE="${object_region_size}" \
        "${extra_defines[@]}" \
        parameters/tests/host/test_par_nvm_flash_ee.c \
        "${base_sources[@]}" \
        "$layout_source" \
        "${nvm_sources[@]}"
}

run_nvm_flash_ee_matrix() {
    autogen_pm_ci_for_each_nvm_flash_ee_profile run_nvm_flash_ee_profile
}


run_nvm_flash_ee_write_verify_profile() {
    compile_and_run_nvm "par_nvm_flash_ee_write_verify" \
        -DPAR_HOST_TEST_NVM \
        -DPAR_HOST_TEST_PROFILE_NAME='"write_verify"' \
        -DPAR_CFG_NVM_WRITE_VERIFY_EN=1 \
        -DPAR_CFG_NVM_RECORD_LAYOUT=PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE \
        -DPAR_CFG_NVM_OBJECT_STORE_MODE=PAR_CFG_NVM_OBJECT_STORE_SHARED \
        -DPAR_CFG_NVM_OBJECT_ADDR_MODE=PAR_CFG_NVM_OBJECT_ADDR_FIXED \
        -DPAR_CFG_NVM_OBJECT_FIXED_ADDR=0xC0U \
        -DPAR_CFG_NVM_OBJECT_REGION_SIZE=0x40U \
        -Wno-unused-function \
        parameters/tests/host/test_par_nvm_flash_ee.c \
        "${base_sources[@]}" \
        parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_with_size.c \
        "${nvm_sources[@]}"
}

run_nvm_flash_ee_object_write_verify_profile() {
    compile_and_run_nvm "par_nvm_flash_ee_object_write_verify" \
        -DPAR_HOST_TEST_NVM \
        -DPAR_HOST_TEST_PROFILE_NAME='"object_write_verify"' \
        -DPAR_CFG_NVM_WRITE_VERIFY_EN=1 \
        -DPAR_CFG_NVM_OBJECT_WRITE_VERIFY_EN=1 \
        -DPAR_CFG_NVM_RECORD_LAYOUT=PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE \
        -DPAR_CFG_NVM_OBJECT_STORE_MODE=PAR_CFG_NVM_OBJECT_STORE_DEDICATED \
        -DPAR_CFG_NVM_OBJECT_ADDR_MODE=PAR_CFG_NVM_OBJECT_ADDR_FIXED \
        -DPAR_CFG_NVM_OBJECT_FIXED_ADDR=0x00U \
        -DPAR_CFG_NVM_OBJECT_REGION_SIZE=0x40U \
        -Wno-unused-function \
        parameters/tests/host/test_par_nvm_flash_ee.c \
        "${base_sources[@]}" \
        parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_with_size.c \
        "${nvm_sources[@]}"
}

run_nvm_flash_ee_object_only_profile() {
    compile_and_run_nvm_with_fixture "par_nvm_flash_ee_object_only" \
        parameters/tests/host/fixtures_object_nvm \
        -DPAR_HOST_TEST_NVM \
        -DPAR_HOST_TEST_OBJECT_ONLY \
        -DPAR_HOST_TEST_PROFILE_NAME='"object_only"' \
        -DPAR_CFG_NVM_SCALAR_EN=0 \
        -DPAR_CFG_NVM_RECORD_LAYOUT=PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE \
        -DPAR_CFG_NVM_OBJECT_STORE_MODE=PAR_CFG_NVM_OBJECT_STORE_SHARED \
        -DPAR_CFG_NVM_OBJECT_ADDR_MODE=PAR_CFG_NVM_OBJECT_ADDR_FIXED \
        -DPAR_CFG_NVM_OBJECT_FIXED_ADDR=0x40U \
        -DPAR_CFG_NVM_OBJECT_REGION_SIZE=0x40U \
        -Wno-type-limits \
        -Wno-unused-function \
        parameters/tests/host/test_par_nvm_flash_ee.c \
        "${base_sources[@]}" \
        parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_with_size.c \
        "${nvm_sources[@]}"
}


run_nvm_flash_ee_object_array_nvm_profile() {
    compile_and_run_nvm_with_fixture "par_nvm_flash_ee_object_array_nvm" \
        parameters/tests/host/fixtures_object_array_nvm \
        -DPAR_HOST_TEST_NVM \
        -DPAR_HOST_TEST_OBJECT_ARRAY_NVM \
        -DPAR_HOST_TEST_PROFILE_NAME='"object_array_nvm"' \
        -DPAR_CFG_NVM_RECORD_LAYOUT=PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE \
        -DPAR_CFG_NVM_OBJECT_STORE_MODE=PAR_CFG_NVM_OBJECT_STORE_SHARED \
        -DPAR_CFG_NVM_OBJECT_ADDR_MODE=PAR_CFG_NVM_OBJECT_ADDR_FIXED \
        -DPAR_CFG_NVM_OBJECT_FIXED_ADDR=0x80U \
        -DPAR_CFG_NVM_OBJECT_REGION_SIZE=0x80U \
        -Wno-type-limits \
        -Wno-unused-function \
        parameters/tests/host/test_par_nvm_flash_ee.c \
        "${base_sources[@]}" \
        parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_with_size.c \
        "${nvm_sources[@]}"
}

run_nvm_flash_ee_fixed_object_invalid_profile() {
    local profile="$1"
    local object_fixed_addr="$2"
    local object_region_size="$3"

    compile_and_run_nvm "par_nvm_flash_ee_${profile}" \
        -DPAR_HOST_TEST_NVM \
        -DPAR_HOST_TEST_FIXED_OBJECT_INVALID \
        -DPAR_HOST_TEST_PROFILE_NAME="\"${profile}\"" \
        -DPAR_CFG_NVM_RECORD_LAYOUT=PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE \
        -DPAR_CFG_NVM_OBJECT_STORE_MODE=PAR_CFG_NVM_OBJECT_STORE_SHARED \
        -DPAR_CFG_NVM_OBJECT_ADDR_MODE=PAR_CFG_NVM_OBJECT_ADDR_FIXED \
        -DPAR_CFG_NVM_OBJECT_FIXED_ADDR="${object_fixed_addr}" \
        -DPAR_CFG_NVM_OBJECT_REGION_SIZE="${object_region_size}" \
        -Wno-unused-function \
        parameters/tests/host/test_par_nvm_flash_ee.c \
        "${base_sources[@]}" \
        parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_with_size.c \
        "${nvm_sources[@]}"
}
