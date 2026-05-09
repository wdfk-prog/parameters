#!/usr/bin/env bash
# Shared CI compile profile for autogen_parameter_manager.
#
# Keep this file as the single source of truth for RT-Thread package compile
# configuration and static-analysis preprocessor symbols.

AUTOGEN_PM_CI_BOOL_SYMBOLS=(
    PKG_USING_AUTOGEN_PARAMETER_MANAGER
    RT_USING_MUTEX
    # Cover Kconfig default shell paths when RT-Thread FINSH is available.
    RT_USING_FINSH
    RT_USING_HEAP
    AUTOGEN_PM_USING_MUTEX
    AUTOGEN_PM_ENABLE_RANGE
    AUTOGEN_PM_ENABLE_NAME
    AUTOGEN_PM_ENABLE_UNIT
    AUTOGEN_PM_ENABLE_DESC
    AUTOGEN_PM_ENABLE_DESC_CHECK
    AUTOGEN_PM_ENABLE_ID
    # Mirror Kconfig defaults selected by the CI NVM profile.
    AUTOGEN_PM_USING_TABLE_ID_CHECK
    AUTOGEN_PM_ENABLE_RUNTIME_TABLE_CHECK
    AUTOGEN_PM_ENABLE_ACCESS
    AUTOGEN_PM_ENABLE_ROLE_POLICY
    AUTOGEN_PM_ENABLE_TYPE_F32
    AUTOGEN_PM_ENABLE_TYPE_OBJECT
    AUTOGEN_PM_ENABLE_TYPE_STR
    AUTOGEN_PM_ENABLE_TYPE_BYTES
    AUTOGEN_PM_ENABLE_TYPE_ARR_U8
    AUTOGEN_PM_ENABLE_TYPE_ARR_U16
    AUTOGEN_PM_ENABLE_TYPE_ARR_U32
    AUTOGEN_PM_ENABLE_RUNTIME_VALIDATION
    AUTOGEN_PM_ENABLE_CHANGE_CALLBACK
    AUTOGEN_PM_ENABLE_RESET_ALL_RAW
    AUTOGEN_PM_USING_NVM
    AUTOGEN_PM_NVM_SCALAR
    AUTOGEN_PM_USING_FLASH_EE_BACKEND
    AUTOGEN_PM_FLASH_EE_PORT_NATIVE
    AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE
    AUTOGEN_PM_USING_MSH_TOOL
    AUTOGEN_PM_MSH_CMD_INFO
    AUTOGEN_PM_MSH_CMD_GET
    AUTOGEN_PM_MSH_CMD_GET_OBJECT
    AUTOGEN_PM_MSH_CMD_SET
    AUTOGEN_PM_MSH_CMD_DEF
    AUTOGEN_PM_MSH_CMD_DEF_ALL
    AUTOGEN_PM_MSH_CMD_SAVE
    AUTOGEN_PM_MSH_CMD_JSON
)

AUTOGEN_PM_CI_UNSET_SYMBOLS=(
    AUTOGEN_PM_USING_RTT_AT24CXX_BACKEND
    AUTOGEN_PM_FLASH_EE_PORT_FAL
    AUTOGEN_PM_NVM_OBJECT
    AUTOGEN_PM_NVM_OBJECT_STORE_SHARED
    AUTOGEN_PM_NVM_OBJECT_STORE_DEDICATED
    AUTOGEN_PM_NVM_OBJECT_ADDR_AFTER_SCALAR
    AUTOGEN_PM_NVM_OBJECT_ADDR_FIXED
    AUTOGEN_PM_LAYOUT_SOURCE_SCRIPT
    AUTOGEN_PM_ENABLE_GENERATED_INFO
)

AUTOGEN_PM_CI_MANAGED_EXTRA_SYMBOLS=(
    AUTOGEN_PM_NVM_OBJECT_FIXED_ADDR
    AUTOGEN_PM_NVM_OBJECT_REGION_SIZE
    AUTOGEN_PM_NVM_OBJECT_DEDICATED_BASE_ADDR
    AUTOGEN_PM_RTT_AT24_I2C_BUS_NAME
    AUTOGEN_PM_RTT_AT24_ADDR_INPUT
    AUTOGEN_PM_RTT_AT24_BASE_ADDR
    AUTOGEN_PM_RTT_AT24_ERASE_CHUNK
    AUTOGEN_PM_FLASH_EE_FAL_PARTITION_NAME
    AUTOGEN_PM_NVM_WRITE_VERIFY
)

AUTOGEN_PM_CI_NVM_LAYOUT_SYMBOLS=(
    AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE
    AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE
    AUTOGEN_PM_NVM_RECORD_LAYOUT_COMPACT_PAYLOAD
    AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_PAYLOAD_ONLY
    AUTOGEN_PM_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY
)

AUTOGEN_PM_CI_NVM_OBJECT_STORE_SYMBOLS=(
    AUTOGEN_PM_NVM_OBJECT_STORE_SHARED
    AUTOGEN_PM_NVM_OBJECT_STORE_DEDICATED
)

AUTOGEN_PM_CI_NVM_OBJECT_ADDR_SYMBOLS=(
    AUTOGEN_PM_NVM_OBJECT_ADDR_AFTER_SCALAR
    AUTOGEN_PM_NVM_OBJECT_ADDR_FIXED
)

AUTOGEN_PM_CI_BACKEND_SYMBOLS=(
    AUTOGEN_PM_USING_RTT_AT24CXX_BACKEND
    AUTOGEN_PM_USING_FLASH_EE_BACKEND
)

AUTOGEN_PM_CI_FLASH_EE_PORT_SYMBOLS=(
    AUTOGEN_PM_FLASH_EE_PORT_FAL
    AUTOGEN_PM_FLASH_EE_PORT_NATIVE
)

AUTOGEN_PM_CI_VALUE_SYMBOLS=(
    AUTOGEN_PM_MUTEX_TIMEOUT_MS=10
    AUTOGEN_PM_TABLE_ID_SCHEMA_VER=1
    AUTOGEN_PM_FLASH_EE_LOGICAL_SIZE=0x1000
    AUTOGEN_PM_FLASH_EE_CACHE_SIZE=4096
    AUTOGEN_PM_FLASH_EE_LINE_SIZE=32
    AUTOGEN_PM_FLASH_EE_PROGRAM_SIZE=8
)


autogen_pm_ci_remove_bool() {
    local remove_name="$1"
    local name
    local next_symbols=()

    for name in "${AUTOGEN_PM_CI_BOOL_SYMBOLS[@]}"; do
        if [ "$name" != "$remove_name" ]; then
            next_symbols+=("$name")
        fi
    done

    AUTOGEN_PM_CI_BOOL_SYMBOLS=("${next_symbols[@]}")
}

autogen_pm_ci_remove_unset() {
    local remove_name="$1"
    local name
    local next_symbols=()

    for name in "${AUTOGEN_PM_CI_UNSET_SYMBOLS[@]}"; do
        if [ "$name" != "$remove_name" ]; then
            next_symbols+=("$name")
        fi
    done

    AUTOGEN_PM_CI_UNSET_SYMBOLS=("${next_symbols[@]}")
}

autogen_pm_ci_add_unset() {
    local add_name="$1"
    local name

    autogen_pm_ci_remove_bool "$add_name"

    for name in "${AUTOGEN_PM_CI_UNSET_SYMBOLS[@]}"; do
        if [ "$name" = "$add_name" ]; then
            return 0
        fi
    done

    AUTOGEN_PM_CI_UNSET_SYMBOLS+=("$add_name")
}

autogen_pm_ci_add_bool() {
    local add_name="$1"
    local name

    autogen_pm_ci_remove_unset "$add_name"

    for name in "${AUTOGEN_PM_CI_BOOL_SYMBOLS[@]}"; do
        if [ "$name" = "$add_name" ]; then
            return 0
        fi
    done

    AUTOGEN_PM_CI_BOOL_SYMBOLS+=("$add_name")
}

autogen_pm_ci_set_value() {
    local set_entry="$1"
    local set_name="${set_entry%%=*}"
    local entry
    local next_values=()

    for entry in "${AUTOGEN_PM_CI_VALUE_SYMBOLS[@]}"; do
        if [ "${entry%%=*}" != "$set_name" ]; then
            next_values+=("$entry")
        fi
    done

    next_values+=("$set_entry")
    AUTOGEN_PM_CI_VALUE_SYMBOLS=("${next_values[@]}")
}

autogen_pm_ci_remove_value() {
    local remove_name="$1"
    local entry
    local next_values=()

    for entry in "${AUTOGEN_PM_CI_VALUE_SYMBOLS[@]}"; do
        if [ "${entry%%=*}" != "$remove_name" ]; then
            next_values+=("$entry")
        fi
    done

    AUTOGEN_PM_CI_VALUE_SYMBOLS=("${next_values[@]}")
}

autogen_pm_ci_remove_nvm_values() {
    autogen_pm_ci_remove_value AUTOGEN_PM_TABLE_ID_SCHEMA_VER
    autogen_pm_ci_remove_value AUTOGEN_PM_FLASH_EE_LOGICAL_SIZE
    autogen_pm_ci_remove_value AUTOGEN_PM_FLASH_EE_CACHE_SIZE
    autogen_pm_ci_remove_value AUTOGEN_PM_FLASH_EE_LINE_SIZE
    autogen_pm_ci_remove_value AUTOGEN_PM_FLASH_EE_PROGRAM_SIZE
    autogen_pm_ci_remove_value AUTOGEN_PM_NVM_OBJECT_FIXED_ADDR
    autogen_pm_ci_remove_value AUTOGEN_PM_NVM_OBJECT_REGION_SIZE
    autogen_pm_ci_remove_value AUTOGEN_PM_NVM_OBJECT_DEDICATED_BASE_ADDR
    autogen_pm_ci_remove_value AUTOGEN_PM_RTT_AT24_I2C_BUS_NAME
    autogen_pm_ci_remove_value AUTOGEN_PM_RTT_AT24_ADDR_INPUT
    autogen_pm_ci_remove_value AUTOGEN_PM_RTT_AT24_BASE_ADDR
    autogen_pm_ci_remove_value AUTOGEN_PM_RTT_AT24_ERASE_CHUNK
    autogen_pm_ci_remove_value AUTOGEN_PM_FLASH_EE_FAL_PARTITION_NAME
}

autogen_pm_ci_select_choice() {
    local selected_symbol="$1"
    shift
    local name

    for name in "$@"; do
        autogen_pm_ci_add_unset "$name"
    done

    autogen_pm_ci_add_bool "$selected_symbol"
}

autogen_pm_ci_select_layout() {
    local layout_symbol="$1"

    autogen_pm_ci_select_choice "$layout_symbol" "${AUTOGEN_PM_CI_NVM_LAYOUT_SYMBOLS[@]}"
}

autogen_pm_ci_select_backend() {
    local backend_symbol="$1"

    autogen_pm_ci_select_choice "$backend_symbol" "${AUTOGEN_PM_CI_BACKEND_SYMBOLS[@]}"
}

autogen_pm_ci_select_flash_ee_port() {
    local port_symbol="$1"

    autogen_pm_ci_select_choice "$port_symbol" "${AUTOGEN_PM_CI_FLASH_EE_PORT_SYMBOLS[@]}"
    if [ "$port_symbol" = "AUTOGEN_PM_FLASH_EE_PORT_FAL" ]; then
        autogen_pm_ci_set_value 'AUTOGEN_PM_FLASH_EE_FAL_PARTITION_NAME="autogen_pm"'
    else
        autogen_pm_ci_remove_value AUTOGEN_PM_FLASH_EE_FAL_PARTITION_NAME
    fi
}

autogen_pm_ci_disable_object_types() {
    autogen_pm_ci_add_unset AUTOGEN_PM_ENABLE_TYPE_OBJECT
    autogen_pm_ci_add_unset AUTOGEN_PM_ENABLE_TYPE_STR
    autogen_pm_ci_add_unset AUTOGEN_PM_ENABLE_TYPE_BYTES
    autogen_pm_ci_add_unset AUTOGEN_PM_ENABLE_TYPE_ARR_U8
    autogen_pm_ci_add_unset AUTOGEN_PM_ENABLE_TYPE_ARR_U16
    autogen_pm_ci_add_unset AUTOGEN_PM_ENABLE_TYPE_ARR_U32
    autogen_pm_ci_add_unset AUTOGEN_PM_NVM_OBJECT
    autogen_pm_ci_add_unset AUTOGEN_PM_NVM_OBJECT_STORE_SHARED
    autogen_pm_ci_add_unset AUTOGEN_PM_NVM_OBJECT_STORE_DEDICATED
    autogen_pm_ci_add_unset AUTOGEN_PM_NVM_OBJECT_ADDR_AFTER_SCALAR
    autogen_pm_ci_add_unset AUTOGEN_PM_NVM_OBJECT_ADDR_FIXED
    autogen_pm_ci_add_unset AUTOGEN_PM_MSH_CMD_GET_OBJECT
    autogen_pm_ci_remove_value AUTOGEN_PM_NVM_OBJECT_FIXED_ADDR
    autogen_pm_ci_remove_value AUTOGEN_PM_NVM_OBJECT_REGION_SIZE
    autogen_pm_ci_remove_value AUTOGEN_PM_NVM_OBJECT_DEDICATED_BASE_ADDR
}

autogen_pm_ci_disable_nvm() {
    autogen_pm_ci_add_unset AUTOGEN_PM_USING_NVM
    autogen_pm_ci_add_unset AUTOGEN_PM_NVM_SCALAR
    autogen_pm_ci_add_unset AUTOGEN_PM_NVM_OBJECT
    autogen_pm_ci_add_unset AUTOGEN_PM_USING_TABLE_ID_CHECK
    autogen_pm_ci_add_unset AUTOGEN_PM_USING_RTT_AT24CXX_BACKEND
    autogen_pm_ci_add_unset AUTOGEN_PM_USING_FLASH_EE_BACKEND
    autogen_pm_ci_add_unset AUTOGEN_PM_FLASH_EE_PORT_FAL
    autogen_pm_ci_add_unset AUTOGEN_PM_FLASH_EE_PORT_NATIVE
    autogen_pm_ci_add_unset AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE
    autogen_pm_ci_add_unset AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE
    autogen_pm_ci_add_unset AUTOGEN_PM_NVM_RECORD_LAYOUT_COMPACT_PAYLOAD
    autogen_pm_ci_add_unset AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_PAYLOAD_ONLY
    autogen_pm_ci_add_unset AUTOGEN_PM_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY
    autogen_pm_ci_add_unset AUTOGEN_PM_MSH_CMD_SAVE
    autogen_pm_ci_remove_nvm_values
}

autogen_pm_ci_enable_flash_ee_native() {
    autogen_pm_ci_select_backend AUTOGEN_PM_USING_FLASH_EE_BACKEND
    autogen_pm_ci_select_flash_ee_port AUTOGEN_PM_FLASH_EE_PORT_NATIVE
}

autogen_pm_ci_enable_flash_ee_fal() {
    autogen_pm_ci_select_backend AUTOGEN_PM_USING_FLASH_EE_BACKEND
    autogen_pm_ci_select_flash_ee_port AUTOGEN_PM_FLASH_EE_PORT_FAL
}

autogen_pm_ci_enable_rtt_at24cxx() {
    autogen_pm_ci_select_backend AUTOGEN_PM_USING_RTT_AT24CXX_BACKEND
    autogen_pm_ci_add_bool RT_USING_DEVICE
    autogen_pm_ci_add_bool RT_USING_I2C
    autogen_pm_ci_add_unset AUTOGEN_PM_FLASH_EE_PORT_FAL
    autogen_pm_ci_add_unset AUTOGEN_PM_FLASH_EE_PORT_NATIVE
    autogen_pm_ci_remove_value AUTOGEN_PM_FLASH_EE_LOGICAL_SIZE
    autogen_pm_ci_remove_value AUTOGEN_PM_FLASH_EE_CACHE_SIZE
    autogen_pm_ci_remove_value AUTOGEN_PM_FLASH_EE_LINE_SIZE
    autogen_pm_ci_remove_value AUTOGEN_PM_FLASH_EE_PROGRAM_SIZE
    autogen_pm_ci_remove_value AUTOGEN_PM_FLASH_EE_FAL_PARTITION_NAME
    autogen_pm_ci_set_value 'AUTOGEN_PM_RTT_AT24_I2C_BUS_NAME="i2c1"'
    autogen_pm_ci_set_value AUTOGEN_PM_RTT_AT24_ADDR_INPUT=0
    autogen_pm_ci_set_value AUTOGEN_PM_RTT_AT24_BASE_ADDR=0x0
    autogen_pm_ci_set_value AUTOGEN_PM_RTT_AT24_ERASE_CHUNK=32
}

autogen_pm_ci_enable_object_shared() {
    autogen_pm_ci_add_bool AUTOGEN_PM_NVM_OBJECT
    autogen_pm_ci_select_choice AUTOGEN_PM_NVM_OBJECT_STORE_SHARED \
        "${AUTOGEN_PM_CI_NVM_OBJECT_STORE_SYMBOLS[@]}"
}

autogen_pm_ci_apply_profile() {
    local profile="${AUTOGEN_PM_CI_PROFILE:-scalar-fixed-slot-with-size}"

    case "$profile" in
        scalar-fixed-slot-with-size|default)
            autogen_pm_ci_select_layout AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE
            autogen_pm_ci_enable_flash_ee_native
            ;;
        scalar-fixed-slot-no-size)
            autogen_pm_ci_select_layout AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE
            autogen_pm_ci_enable_flash_ee_native
            ;;
        scalar-compact-payload)
            autogen_pm_ci_select_layout AUTOGEN_PM_NVM_RECORD_LAYOUT_COMPACT_PAYLOAD
            autogen_pm_ci_enable_flash_ee_native
            ;;
        scalar-fixed-payload-only)
            autogen_pm_ci_select_layout AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_PAYLOAD_ONLY
            autogen_pm_ci_enable_flash_ee_native
            ;;
        scalar-grouped-payload-only)
            autogen_pm_ci_select_layout AUTOGEN_PM_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY
            autogen_pm_ci_enable_flash_ee_native
            ;;
        object-shared-after-scalar)
            autogen_pm_ci_select_layout AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE
            autogen_pm_ci_enable_flash_ee_native
            autogen_pm_ci_enable_object_shared
            autogen_pm_ci_select_choice AUTOGEN_PM_NVM_OBJECT_ADDR_AFTER_SCALAR \
                "${AUTOGEN_PM_CI_NVM_OBJECT_ADDR_SYMBOLS[@]}"
            ;;
        object-shared-fixed)
            autogen_pm_ci_select_layout AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE
            autogen_pm_ci_enable_flash_ee_native
            autogen_pm_ci_enable_object_shared
            autogen_pm_ci_select_choice AUTOGEN_PM_NVM_OBJECT_ADDR_FIXED \
                "${AUTOGEN_PM_CI_NVM_OBJECT_ADDR_SYMBOLS[@]}"
            autogen_pm_ci_set_value AUTOGEN_PM_NVM_OBJECT_FIXED_ADDR=0x600
            autogen_pm_ci_set_value AUTOGEN_PM_NVM_OBJECT_REGION_SIZE=512
            ;;
        generated-layout-info)
            autogen_pm_ci_select_layout AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE
            autogen_pm_ci_enable_flash_ee_native
            autogen_pm_ci_add_bool AUTOGEN_PM_LAYOUT_SOURCE_SCRIPT
            autogen_pm_ci_add_bool AUTOGEN_PM_ENABLE_GENERATED_INFO
            ;;
        minimal-scalar-no-nvm)
            autogen_pm_ci_disable_nvm
            autogen_pm_ci_disable_object_types
            autogen_pm_ci_add_unset AUTOGEN_PM_ENABLE_TYPE_F32
            autogen_pm_ci_add_unset AUTOGEN_PM_USING_MSH_TOOL
            autogen_pm_ci_add_unset AUTOGEN_PM_MSH_CMD_INFO
            autogen_pm_ci_add_unset AUTOGEN_PM_MSH_CMD_GET
            autogen_pm_ci_add_unset AUTOGEN_PM_MSH_CMD_GET_OBJECT
            autogen_pm_ci_add_unset AUTOGEN_PM_MSH_CMD_SET
            autogen_pm_ci_add_unset AUTOGEN_PM_MSH_CMD_DEF
            autogen_pm_ci_add_unset AUTOGEN_PM_MSH_CMD_DEF_ALL
            autogen_pm_ci_add_unset AUTOGEN_PM_MSH_CMD_SAVE
            autogen_pm_ci_add_unset AUTOGEN_PM_MSH_CMD_JSON
            ;;
        scalar-no-metadata)
            autogen_pm_ci_disable_nvm
            autogen_pm_ci_disable_object_types
            autogen_pm_ci_add_unset AUTOGEN_PM_ENABLE_NAME
            autogen_pm_ci_add_unset AUTOGEN_PM_ENABLE_UNIT
            autogen_pm_ci_add_unset AUTOGEN_PM_ENABLE_DESC
            autogen_pm_ci_add_unset AUTOGEN_PM_ENABLE_DESC_CHECK
            autogen_pm_ci_add_unset AUTOGEN_PM_USING_MSH_TOOL
            autogen_pm_ci_add_unset AUTOGEN_PM_MSH_CMD_INFO
            autogen_pm_ci_add_unset AUTOGEN_PM_MSH_CMD_GET
            autogen_pm_ci_add_unset AUTOGEN_PM_MSH_CMD_GET_OBJECT
            autogen_pm_ci_add_unset AUTOGEN_PM_MSH_CMD_SET
            autogen_pm_ci_add_unset AUTOGEN_PM_MSH_CMD_DEF
            autogen_pm_ci_add_unset AUTOGEN_PM_MSH_CMD_DEF_ALL
            autogen_pm_ci_add_unset AUTOGEN_PM_MSH_CMD_SAVE
            autogen_pm_ci_add_unset AUTOGEN_PM_MSH_CMD_JSON
            ;;
        scalar-no-access-role)
            autogen_pm_ci_disable_nvm
            autogen_pm_ci_disable_object_types
            autogen_pm_ci_add_unset AUTOGEN_PM_ENABLE_ACCESS
            autogen_pm_ci_add_unset AUTOGEN_PM_ENABLE_ROLE_POLICY
            ;;
        scalar-no-validation-callback)
            autogen_pm_ci_disable_nvm
            autogen_pm_ci_disable_object_types
            autogen_pm_ci_add_unset AUTOGEN_PM_ENABLE_RUNTIME_VALIDATION
            autogen_pm_ci_add_unset AUTOGEN_PM_ENABLE_CHANGE_CALLBACK
            autogen_pm_ci_add_unset AUTOGEN_PM_ENABLE_RESET_ALL_RAW
            ;;
        flash-ee-fal-backend)
            autogen_pm_ci_select_layout AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE
            autogen_pm_ci_enable_flash_ee_fal
            ;;
        rtt-at24cxx-backend)
            autogen_pm_ci_select_layout AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE
            autogen_pm_ci_enable_rtt_at24cxx
            ;;
        *)
            echo "Unknown AUTOGEN_PM_CI_PROFILE: $profile" >&2
            return 1
            ;;
    esac
}

autogen_pm_ci_apply_profile

autogen_pm_ci_for_each_symbol() {
    local entry
    local name

    for name in "${AUTOGEN_PM_CI_BOOL_SYMBOLS[@]}"; do
        printf '%s\n' "$name"
    done

    for name in "${AUTOGEN_PM_CI_UNSET_SYMBOLS[@]}"; do
        printf '%s\n' "$name"
    done

    for entry in "${AUTOGEN_PM_CI_VALUE_SYMBOLS[@]}"; do
        printf '%s\n' "${entry%%=*}"
    done

    for name in "${AUTOGEN_PM_CI_MANAGED_EXTRA_SYMBOLS[@]}"; do
        printf '%s\n' "$name"
    done
}

autogen_pm_ci_emit_config() {
    local entry
    local name

    for name in "${AUTOGEN_PM_CI_BOOL_SYMBOLS[@]}"; do
        printf 'CONFIG_%s=y\n' "$name"
    done

    for name in "${AUTOGEN_PM_CI_UNSET_SYMBOLS[@]}"; do
        printf '# CONFIG_%s is not set\n' "$name"
    done

    for entry in "${AUTOGEN_PM_CI_VALUE_SYMBOLS[@]}"; do
        printf 'CONFIG_%s=%s\n' "${entry%%=*}" "${entry#*=}"
    done
}

autogen_pm_ci_emit_c_defines() {
    local entry
    local name

    for name in "${AUTOGEN_PM_CI_BOOL_SYMBOLS[@]}"; do
        printf -- '-D%s\n' "$name"
    done

    for entry in "${AUTOGEN_PM_CI_VALUE_SYMBOLS[@]}"; do
        printf -- '-D%s=%s\n' "${entry%%=*}" "${entry#*=}"
    done
}

# Keep the legacy cppcheck helper as an alias for existing CI call sites.
autogen_pm_ci_emit_cppcheck_defines() {
    autogen_pm_ci_emit_c_defines
}

autogen_pm_ci_update_config_file() {
    local config_file="$1"
    local tmp_file
    local line
    local symbol
    local keep
    local managed_symbols=()

    mapfile -t managed_symbols < <(autogen_pm_ci_for_each_symbol | sort -u)

    touch "$config_file"
    tmp_file="$(mktemp "${config_file}.tmp.XXXXXX")"

    while IFS= read -r line; do
        keep=1
        for symbol in "${managed_symbols[@]}"; do
            case "$line" in
                "CONFIG_${symbol}="*|"# CONFIG_${symbol} is not set")
                    keep=0
                    break
                    ;;
            esac
        done

        if [ "$keep" -eq 1 ]; then
            printf '%s\n' "$line" >> "$tmp_file"
        fi
    done < "$config_file"

    {
        printf '\n# CI-only autogen_parameter_manager compile profile.\n'
        autogen_pm_ci_emit_config
    } >> "$tmp_file"

    mv "$tmp_file" "$config_file"
}

autogen_pm_ci_verify_rtconfig_defines() {
    local rtconfig="$1"
    local entry
    local name
    local value

    for name in "${AUTOGEN_PM_CI_BOOL_SYMBOLS[@]}"; do
        grep -Eq "^#define[[:space:]]+${name}([[:space:]]|$)" "$rtconfig" || return 1
    done

    for name in "${AUTOGEN_PM_CI_UNSET_SYMBOLS[@]}"; do
        if grep -Eq "^#define[[:space:]]+${name}([[:space:]]|$)" "$rtconfig"; then
            return 1
        fi
    done

    for entry in "${AUTOGEN_PM_CI_VALUE_SYMBOLS[@]}"; do
        name="${entry%%=*}"
        value="${entry#*=}"
        grep -Eq "^#define[[:space:]]+${name}[[:space:]]+${value}([[:space:]]|$)" "$rtconfig" || return 1
    done
}

# Check that static-analysis defines and RT-Thread compile-profile symbols stay
# backed by one symbol list. This catches hand-edited cppcheck coverage drift.
autogen_pm_ci_verify_cppcheck_profile_defines() {
    local define
    local entry
    local name
    local value
    local cppcheck_defines

    cppcheck_defines="$(autogen_pm_ci_emit_cppcheck_defines)"

    for name in "${AUTOGEN_PM_CI_BOOL_SYMBOLS[@]}"; do
        define="-D${name}"
        grep -Fx -- "$define" <<< "$cppcheck_defines" >/dev/null || return 1
    done

    for entry in "${AUTOGEN_PM_CI_VALUE_SYMBOLS[@]}"; do
        name="${entry%%=*}"
        value="${entry#*=}"
        define="-D${name}=${value}"
        grep -Fx -- "$define" <<< "$cppcheck_defines" >/dev/null || return 1
    done
}
