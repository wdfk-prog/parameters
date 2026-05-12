#!/usr/bin/env bash
# Shared host flash-ee NVM profile definitions for host tests and cppcheck.

AUTOGEN_PM_CI_NVM_FLASH_EE_PROFILES=(
    'fixed_slot_with_size|PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE|PAR_CFG_NVM_OBJECT_STORE_SHARED|PAR_CFG_NVM_OBJECT_ADDR_FIXED|0xC0U|0x40U|parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_with_size.c'
    'fixed_slot_no_size|PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE|PAR_CFG_NVM_OBJECT_STORE_SHARED|PAR_CFG_NVM_OBJECT_ADDR_FIXED|0xC0U|0x40U|parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_no_size.c'
    'compact_payload|PAR_CFG_NVM_RECORD_LAYOUT_COMPACT_PAYLOAD|PAR_CFG_NVM_OBJECT_STORE_SHARED|PAR_CFG_NVM_OBJECT_ADDR_FIXED|0xC0U|0x40U|parameters/src/nvm/scalar/layout/par_nvm_layout_compact_payload.c'
    'fixed_payload_only|PAR_CFG_NVM_RECORD_LAYOUT_FIXED_PAYLOAD_ONLY|PAR_CFG_NVM_OBJECT_STORE_SHARED|PAR_CFG_NVM_OBJECT_ADDR_FIXED|0xC0U|0x40U|parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_payload_only.c'
    'grouped_payload_only|PAR_CFG_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY|PAR_CFG_NVM_OBJECT_STORE_SHARED|PAR_CFG_NVM_OBJECT_ADDR_FIXED|0xC0U|0x40U|parameters/src/nvm/scalar/layout/par_nvm_layout_grouped_payload_only.c'
    'object_shared_after_scalar|PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE|PAR_CFG_NVM_OBJECT_STORE_SHARED|PAR_CFG_NVM_OBJECT_ADDR_AFTER_SCALAR|0xC0U|0x40U|parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_with_size.c'
    'object_shared_fixed_exact_fit|PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE|PAR_CFG_NVM_OBJECT_STORE_SHARED|PAR_CFG_NVM_OBJECT_ADDR_FIXED|0xC0U|54U|parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_with_size.c'
    'object_dedicated|PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE|PAR_CFG_NVM_OBJECT_STORE_DEDICATED|PAR_CFG_NVM_OBJECT_ADDR_FIXED|0x00U|0x40U|parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_with_size.c'
)

autogen_pm_ci_unpack_nvm_flash_ee_profile() {
    local entry="$1"

    IFS='|' read -r \
        AUTOGEN_PM_CI_NVM_PROFILE_NAME \
        AUTOGEN_PM_CI_NVM_PROFILE_RECORD_LAYOUT \
        AUTOGEN_PM_CI_NVM_PROFILE_OBJECT_STORE \
        AUTOGEN_PM_CI_NVM_PROFILE_OBJECT_ADDR \
        AUTOGEN_PM_CI_NVM_PROFILE_OBJECT_FIXED_ADDR \
        AUTOGEN_PM_CI_NVM_PROFILE_OBJECT_REGION_SIZE \
        AUTOGEN_PM_CI_NVM_PROFILE_LAYOUT_SOURCE <<< "$entry"
}

autogen_pm_ci_select_nvm_flash_ee_profile() {
    local requested_profile="$1"
    local display_profile="$requested_profile"
    local entry

    if [ "$requested_profile" = "default" ]; then
        requested_profile="fixed_slot_with_size"
    fi

    for entry in "${AUTOGEN_PM_CI_NVM_FLASH_EE_PROFILES[@]}"; do
        autogen_pm_ci_unpack_nvm_flash_ee_profile "$entry"
        if [ "$AUTOGEN_PM_CI_NVM_PROFILE_NAME" = "$requested_profile" ]; then
            AUTOGEN_PM_CI_NVM_PROFILE_NAME="$display_profile"
            return 0
        fi
    done

    echo "Unknown host flash-ee NVM profile: $display_profile" >&2
    return 1
}

autogen_pm_ci_for_each_nvm_flash_ee_profile() {
    local callback="$1"
    local entry

    for entry in "${AUTOGEN_PM_CI_NVM_FLASH_EE_PROFILES[@]}"; do
        autogen_pm_ci_unpack_nvm_flash_ee_profile "$entry"
        "$callback" \
            "$AUTOGEN_PM_CI_NVM_PROFILE_NAME" \
            "$AUTOGEN_PM_CI_NVM_PROFILE_RECORD_LAYOUT" \
            "$AUTOGEN_PM_CI_NVM_PROFILE_OBJECT_STORE" \
            "$AUTOGEN_PM_CI_NVM_PROFILE_OBJECT_ADDR" \
            "$AUTOGEN_PM_CI_NVM_PROFILE_OBJECT_FIXED_ADDR" \
            "$AUTOGEN_PM_CI_NVM_PROFILE_OBJECT_REGION_SIZE" \
            "$AUTOGEN_PM_CI_NVM_PROFILE_LAYOUT_SOURCE" || return $?
    done
}

if [ "${BASH_SOURCE[0]}" = "$0" ] && [ "${1:-}" = "--list-names" ]; then
    for entry in "${AUTOGEN_PM_CI_NVM_FLASH_EE_PROFILES[@]}"; do
        autogen_pm_ci_unpack_nvm_flash_ee_profile "$entry"
        printf '%s\n' "$AUTOGEN_PM_CI_NVM_PROFILE_NAME"
    done
fi
