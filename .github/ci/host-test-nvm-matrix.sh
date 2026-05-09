#!/usr/bin/env bash
# Host-test helpers for flash-ee NVM matrix cases.

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
    run_nvm_flash_ee_profile fixed_slot_with_size \
        PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE \
        PAR_CFG_NVM_OBJECT_STORE_SHARED \
        PAR_CFG_NVM_OBJECT_ADDR_FIXED \
        0xC0U 0x40U \
        parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_with_size.c
    run_nvm_flash_ee_profile fixed_slot_no_size \
        PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE \
        PAR_CFG_NVM_OBJECT_STORE_SHARED \
        PAR_CFG_NVM_OBJECT_ADDR_FIXED \
        0xC0U 0x40U \
        parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_no_size.c
    run_nvm_flash_ee_profile compact_payload \
        PAR_CFG_NVM_RECORD_LAYOUT_COMPACT_PAYLOAD \
        PAR_CFG_NVM_OBJECT_STORE_SHARED \
        PAR_CFG_NVM_OBJECT_ADDR_FIXED \
        0xC0U 0x40U \
        parameters/src/nvm/scalar/layout/par_nvm_layout_compact_payload.c
    run_nvm_flash_ee_profile fixed_payload_only \
        PAR_CFG_NVM_RECORD_LAYOUT_FIXED_PAYLOAD_ONLY \
        PAR_CFG_NVM_OBJECT_STORE_SHARED \
        PAR_CFG_NVM_OBJECT_ADDR_FIXED \
        0xC0U 0x40U \
        parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_payload_only.c
    run_nvm_flash_ee_profile grouped_payload_only \
        PAR_CFG_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY \
        PAR_CFG_NVM_OBJECT_STORE_SHARED \
        PAR_CFG_NVM_OBJECT_ADDR_FIXED \
        0xC0U 0x40U \
        parameters/src/nvm/scalar/layout/par_nvm_layout_grouped_payload_only.c
    run_nvm_flash_ee_profile object_shared_after_scalar \
        PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE \
        PAR_CFG_NVM_OBJECT_STORE_SHARED \
        PAR_CFG_NVM_OBJECT_ADDR_AFTER_SCALAR \
        0xC0U 0x40U \
        parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_with_size.c
    run_nvm_flash_ee_profile object_dedicated \
        PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE \
        PAR_CFG_NVM_OBJECT_STORE_DEDICATED \
        PAR_CFG_NVM_OBJECT_ADDR_FIXED \
        0x00U 0x40U \
        parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_with_size.c
}
