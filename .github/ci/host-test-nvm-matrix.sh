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

    compile_and_run_nvm "par_nvm_flash_ee_${profile}" \
        -DPAR_HOST_TEST_NVM \
        -DPAR_HOST_TEST_PROFILE_NAME="\"${profile}\"" \
        -DPAR_CFG_NVM_RECORD_LAYOUT="${record_layout}" \
        -DPAR_CFG_NVM_OBJECT_STORE_MODE="${object_store}" \
        -DPAR_CFG_NVM_OBJECT_ADDR_MODE="${object_addr}" \
        -DPAR_CFG_NVM_OBJECT_FIXED_ADDR="${object_fixed_addr}" \
        -DPAR_CFG_NVM_OBJECT_REGION_SIZE="${object_region_size}" \
        parameters/tests/host/test_par_nvm_flash_ee.c \
        "${base_sources[@]}" \
        "$layout_source" \
        "${nvm_sources[@]}"
}

run_nvm_flash_ee_matrix() {
    autogen_pm_ci_for_each_nvm_flash_ee_profile run_nvm_flash_ee_profile
}
