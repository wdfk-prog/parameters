#!/usr/bin/env bash
set -euo pipefail

script_dir="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
profile_file="$script_dir/profile-list.txt"
profiles=()
AUTOGEN_PM_CI_INVALID_CONFIG_EXPECTED_COUNT=25

while IFS= read -r profile; do
    profile="${profile%$'\r'}"
    case "$profile" in
        ''|'#'*)
            continue
            ;;
    esac
    profiles+=("$profile")
done < "$profile_file"

if [ "${#profiles[@]}" -eq 0 ]; then
    echo "No CI profiles listed in $profile_file" >&2
    exit 1
fi

autogen_pm_ci_count_config_prefix() {
    local config_file="$1"
    local prefix="$2"

    awk -v prefix="CONFIG_${prefix}" \
        '$0 ~ "^" prefix ".*=y$" { count++ } END { print count + 0 }' \
        "$config_file"
}

autogen_pm_ci_count_config_symbols() {
    local config_file="$1"
    local symbol
    local count=0

    shift
    for symbol in "$@"; do
        if grep -Fx -- "CONFIG_${symbol}=y" "$config_file" >/dev/null; then
            count=$((count + 1))
        fi
    done

    printf '%u\n' "$count"
}

autogen_pm_ci_assert_config_choice_count() {
    local profile="$1"
    local config_file="$2"
    local prefix="$3"
    local expected_count="$4"
    local actual_count

    actual_count="$(autogen_pm_ci_count_config_prefix "$config_file" "$prefix")"
    if [ "$actual_count" -ne "$expected_count" ]; then
        echo "profile $profile expected $expected_count active ${prefix} choices, got $actual_count" >&2
        cat "$config_file" >&2
        exit 1
    fi
}

autogen_pm_ci_assert_config_symbol_choice_count() {
    local profile="$1"
    local config_file="$2"
    local expected_count="$3"
    local actual_count

    shift 3
    actual_count="$(autogen_pm_ci_count_config_symbols "$config_file" "$@")"
    if [ "$actual_count" -ne "$expected_count" ]; then
        echo "profile $profile expected $expected_count active choices among: $*; got $actual_count" >&2
        cat "$config_file" >&2
        exit 1
    fi
}

autogen_pm_ci_assert_config_bool_state() {
    local profile="$1"
    local config_file="$2"
    local symbol="$3"
    local expected_state="$4"

    case "$expected_state" in
        y)
            grep -Fx -- "CONFIG_${symbol}=y" "$config_file" >/dev/null || {
                echo "profile $profile expected CONFIG_${symbol}=y" >&2
                cat "$config_file" >&2
                exit 1
            }
            ;;
        n)
            if grep -Fx -- "CONFIG_${symbol}=y" "$config_file" >/dev/null; then
                echo "profile $profile left stale CONFIG_${symbol}=y" >&2
                cat "$config_file" >&2
                exit 1
            fi
            ;;
        *)
            echo "invalid expected state: $expected_state" >&2
            exit 1
            ;;
    esac
}

autogen_pm_ci_assert_config_value_absent() {
    local profile="$1"
    local config_file="$2"
    local symbol="$3"

    if grep -Eq "^CONFIG_${symbol}=" "$config_file"; then
        echo "profile $profile left stale CONFIG_${symbol} value" >&2
        cat "$config_file" >&2
        exit 1
    fi
}

autogen_pm_ci_write_stale_config_choices() {
    local config_file="$1"
    local name

    : > "$config_file"
    for name in "${AUTOGEN_PM_CI_NVM_LAYOUT_SYMBOLS[@]}"; do
        printf 'CONFIG_%s=y\n' "$name" >> "$config_file"
    done
    for name in "${AUTOGEN_PM_CI_BACKEND_SYMBOLS[@]}"; do
        printf 'CONFIG_%s=y\n' "$name" >> "$config_file"
    done
    for name in "${AUTOGEN_PM_CI_NVM_OBJECT_STORE_SYMBOLS[@]}"; do
        printf 'CONFIG_%s=y\n' "$name" >> "$config_file"
    done
    for name in "${AUTOGEN_PM_CI_NVM_OBJECT_ADDR_SYMBOLS[@]}"; do
        printf 'CONFIG_%s=y\n' "$name" >> "$config_file"
    done
    printf 'CONFIG_AUTOGEN_PM_NVM_OBJECT=y\n' >> "$config_file"
    printf 'CONFIG_AUTOGEN_PM_LAYOUT_SOURCE_SCRIPT=y\n' >> "$config_file"
    printf 'CONFIG_AUTOGEN_PM_ENABLE_GENERATED_INFO=y\n' >> "$config_file"
    printf 'CONFIG_AUTOGEN_PM_NVM_OBJECT_FIXED_ADDR=0x600\n' >> "$config_file"
    printf 'CONFIG_AUTOGEN_PM_NVM_OBJECT_REGION_SIZE=512\n' >> "$config_file"
}

autogen_pm_ci_verify_config_pruning() {
    local profile="$1"
    local config_file
    local generated_state=n
    local object_choice_count=0
    local backend_choice_count=1
    local flash_port_choice_count=1
    local layout_choice_count=1

    config_file="$(mktemp "${TMPDIR:-/tmp}/autogen-pm-profile.XXXXXX")"
    autogen_pm_ci_write_stale_config_choices "$config_file"
    autogen_pm_ci_update_config_file "$config_file"

    case "$profile" in
        minimal-scalar-no-nvm|scalar-no-metadata|scalar-no-access-role|scalar-no-validation-callback)
            backend_choice_count=0
            flash_port_choice_count=0
            layout_choice_count=0
            ;;
        rtt-at24cxx-backend)
            flash_port_choice_count=0
            ;;
    esac

    autogen_pm_ci_assert_config_choice_count \
        "$profile" "$config_file" AUTOGEN_PM_NVM_RECORD_LAYOUT_ "$layout_choice_count"
    autogen_pm_ci_assert_config_symbol_choice_count \
        "$profile" "$config_file" "$backend_choice_count" \
        "${AUTOGEN_PM_CI_BACKEND_SYMBOLS[@]}"
    autogen_pm_ci_assert_config_choice_count \
        "$profile" "$config_file" AUTOGEN_PM_FLASH_EE_PORT_ "$flash_port_choice_count"

    case "$profile" in
        object-shared-after-scalar|object-shared-fixed)
            object_choice_count=1
            ;;
    esac

    case "$profile" in
        generated-layout-info)
            generated_state=y
            ;;
    esac

    autogen_pm_ci_assert_config_choice_count \
        "$profile" "$config_file" AUTOGEN_PM_NVM_OBJECT_STORE_ "$object_choice_count"
    autogen_pm_ci_assert_config_choice_count \
        "$profile" "$config_file" AUTOGEN_PM_NVM_OBJECT_ADDR_ "$object_choice_count"
    autogen_pm_ci_assert_config_bool_state \
        "$profile" "$config_file" AUTOGEN_PM_LAYOUT_SOURCE_SCRIPT "$generated_state"
    autogen_pm_ci_assert_config_bool_state \
        "$profile" "$config_file" AUTOGEN_PM_ENABLE_GENERATED_INFO "$generated_state"

    if [ "$profile" != "object-shared-fixed" ]; then
        autogen_pm_ci_assert_config_value_absent \
            "$profile" "$config_file" AUTOGEN_PM_NVM_OBJECT_FIXED_ADDR
        autogen_pm_ci_assert_config_value_absent \
            "$profile" "$config_file" AUTOGEN_PM_NVM_OBJECT_REGION_SIZE
    fi

    case "$profile" in
        flash-ee-fal-backend)
            autogen_pm_ci_assert_config_bool_state \
                "$profile" "$config_file" AUTOGEN_PM_FLASH_EE_PORT_FAL y
            ;;
        rtt-at24cxx-backend)
            autogen_pm_ci_assert_config_bool_state \
                "$profile" "$config_file" AUTOGEN_PM_USING_RTT_AT24CXX_BACKEND y
            autogen_pm_ci_assert_config_value_absent \
                "$profile" "$config_file" AUTOGEN_PM_FLASH_EE_LOGICAL_SIZE
            ;;
        minimal-scalar-no-nvm|scalar-no-metadata|scalar-no-access-role|scalar-no-validation-callback)
            autogen_pm_ci_assert_config_bool_state \
                "$profile" "$config_file" AUTOGEN_PM_USING_NVM n
            autogen_pm_ci_assert_config_bool_state \
                "$profile" "$config_file" AUTOGEN_PM_USING_DEBUG n
            autogen_pm_ci_assert_config_bool_state \
                "$profile" "$config_file" AUTOGEN_PM_MSH_CMD_SAVE_CLEAN n
            autogen_pm_ci_assert_config_value_absent \
                "$profile" "$config_file" AUTOGEN_PM_FLASH_EE_LOGICAL_SIZE
            autogen_pm_ci_assert_config_value_absent \
                "$profile" "$config_file" AUTOGEN_PM_RTT_AT24_I2C_BUS_NAME
            ;;
        *)
            autogen_pm_ci_assert_config_bool_state \
                "$profile" "$config_file" AUTOGEN_PM_USING_DEBUG y
            autogen_pm_ci_assert_config_bool_state \
                "$profile" "$config_file" AUTOGEN_PM_MSH_CMD_SAVE_CLEAN y
            ;;
    esac

    rm -f "$config_file"
}


autogen_pm_ci_expect_invalid_config() {
    local name="$1"
    local expected="$2"
    local src_file
    local log_file
    local stub_dir

    shift 2
    stub_dir="$(mktemp -d "${AUTOGEN_PM_CI_INVALID_TMP_ROOT:-${TMPDIR:-/tmp}}/autogen-pm-invalid-stub-${name}.XXXXXX")"
    cat > "$stub_dir/rtthread.h" <<'RTTHREAD_STUB'
#ifndef AUTOGEN_PM_INVALID_RTTHREAD_H
#define AUTOGEN_PM_INVALID_RTTHREAD_H
#include <stddef.h>
typedef size_t rt_size_t;
#define RT_NULL ((void *)0)
#define RT_UNUSED(x_) ((void)(x_))
#define RT_ASSERT(x_) ((void)sizeof(x_))
#define RT_STATIC_ASSERT(name_, expr_) typedef char rt_static_assert_##name_[(expr_) ? 1 : -1]
#define rt_weak __attribute__((weak))
#endif /* !defined(AUTOGEN_PM_INVALID_RTTHREAD_H) */
RTTHREAD_STUB
    cat > "$stub_dir/rtdbg.h" <<'RTDBG_STUB'
#ifndef AUTOGEN_PM_INVALID_RTDBG_H
#define AUTOGEN_PM_INVALID_RTDBG_H
#define DBG_LOG 0
#define DBG_INFO 1
#define LOG_I(...) ((void)0)
#define LOG_D(...) ((void)0)
#define LOG_W(...) ((void)0)
#define LOG_E(...) ((void)0)
#endif /* !defined(AUTOGEN_PM_INVALID_RTDBG_H) */
RTDBG_STUB
    src_file="$(mktemp "${AUTOGEN_PM_CI_INVALID_TMP_ROOT:-${TMPDIR:-/tmp}}/autogen-pm-invalid-${name}.XXXXXX.c")"
    log_file="${src_file%.c}.log"
    printf '#include "par.h"\nint main(void) { return 0; }\n' > "$src_file"

    if gcc -fsyntax-only \
        -I"$stub_dir" \
        -Iport \
        -Iparameters/tests/host/fixtures \
        -Iparameters/tests/host/fixtures_backend \
        -I. \
        -Ibackend \
        -Iparameters/include \
        -Iparameters/src \
        -Iparameters/src/def \
        -Iparameters/src/detail \
        -Iparameters/src/layout \
        -Iparameters/src/nvm \
        -Iparameters/src/nvm/backend \
        -Iparameters/src/nvm/object \
        -Iparameters/src/nvm/object/addr \
        -Iparameters/src/nvm/object/store \
        -Iparameters/src/nvm/scalar \
        -Iparameters/src/nvm/scalar/layout \
        -Iparameters/src/nvm/scalar/store \
        -Iparameters/src/object \
        -Iparameters/src/port \
        -Iparameters/src/scalar \
        "$@" "$src_file" > "$log_file" 2>&1; then
        echo "invalid config $name unexpectedly compiled" >&2
        cat "$log_file" >&2
        rm -f "$src_file" "$log_file"
        rm -rf "$stub_dir"
        return 1
    fi

    if ! grep -F -- "$expected" "$log_file" >/dev/null; then
        echo "invalid config $name failed for the wrong reason; expected: $expected" >&2
        cat "$log_file" >&2
        rm -f "$src_file" "$log_file"
        rm -rf "$stub_dir"
        return 1
    fi

    rm -f "$src_file" "$log_file"
    rm -rf "$stub_dir"
    ((AUTOGEN_PM_CI_INVALID_CONFIG_COUNT += 1))
    echo "CONFIG_INVALID_OK $name"
}

autogen_pm_ci_cleanup_invalid_config_matrix() {
    if [ -n "${AUTOGEN_PM_CI_INVALID_TMP_ROOT:-}" ]; then
        rm -rf "$AUTOGEN_PM_CI_INVALID_TMP_ROOT"
        unset AUTOGEN_PM_CI_INVALID_TMP_ROOT
    fi
}

autogen_pm_ci_verify_invalid_config_matrix() {
    AUTOGEN_PM_CI_INVALID_CONFIG_COUNT=0
    AUTOGEN_PM_CI_INVALID_TMP_ROOT="$(mktemp -d "${TMPDIR:-/tmp}/autogen-pm-invalid-matrix.XXXXXX")"
    export AUTOGEN_PM_CI_INVALID_TMP_ROOT
    trap 'autogen_pm_ci_cleanup_invalid_config_matrix' RETURN

    autogen_pm_ci_expect_invalid_config nvm-requires-storage-class \
        "NVM requires scalar or object persistence to be enabled" \
        -DAUTOGEN_PM_USING_NVM \
        -DAUTOGEN_PM_ENABLE_TYPE_F32

    autogen_pm_ci_expect_invalid_config scalar-requires-nvm \
        "scalar persistence requires PAR_CFG_NVM_EN = 1" \
        -DAUTOGEN_PM_NVM_SCALAR \
        -DAUTOGEN_PM_ENABLE_TYPE_F32

    autogen_pm_ci_expect_invalid_config object-requires-object-types \
        "object persistence requires at least one object type enabled" \
        -DAUTOGEN_PM_USING_NVM \
        -DAUTOGEN_PM_NVM_OBJECT \
        -DAUTOGEN_PM_ENABLE_ID \
        -DAUTOGEN_PM_ENABLE_TYPE_F32

    autogen_pm_ci_expect_invalid_config layout-id-required \
        "selected NVM layout requires PAR_CFG_ENABLE_ID = 1" \
        -DAUTOGEN_PM_USING_NVM \
        -DAUTOGEN_PM_NVM_SCALAR \
        -DAUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE \
        -DAUTOGEN_PM_ENABLE_TYPE_F32

    autogen_pm_ci_expect_invalid_config payload-layout-table-id-required \
        "payload-only NVM layouts require PAR_CFG_TABLE_ID_CHECK_EN = 1" \
        -DAUTOGEN_PM_USING_NVM \
        -DAUTOGEN_PM_NVM_SCALAR \
        -DAUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_PAYLOAD_ONLY \
        -DAUTOGEN_PM_ENABLE_ID \
        -DAUTOGEN_PM_ENABLE_TYPE_F32

    autogen_pm_ci_expect_invalid_config fixed-object-address-required \
        "fixed object persistence mode requires PAR_CFG_NVM_OBJECT_FIXED_ADDR != 0" \
        -DAUTOGEN_PM_USING_NVM \
        -DAUTOGEN_PM_NVM_SCALAR \
        -DAUTOGEN_PM_NVM_OBJECT \
        -DAUTOGEN_PM_NVM_OBJECT_STORE_SHARED \
        -DAUTOGEN_PM_NVM_OBJECT_ADDR_FIXED \
        -DAUTOGEN_PM_ENABLE_ID \
        -DAUTOGEN_PM_ENABLE_TYPE_F32 \
        -DAUTOGEN_PM_ENABLE_TYPE_STR

    autogen_pm_ci_expect_invalid_config object-requires-id \
        "object persistence requires PAR_CFG_ENABLE_ID = 1" \
        -DAUTOGEN_PM_USING_NVM \
        -DAUTOGEN_PM_NVM_OBJECT \
        -DAUTOGEN_PM_ENABLE_TYPE_STR

    autogen_pm_ci_expect_invalid_config invalid-object-store-mode \
        "PAR_CFG_NVM_OBJECT_STORE_MODE must be SHARED or DEDICATED" \
        -DAUTOGEN_PM_USING_NVM \
        -DAUTOGEN_PM_NVM_OBJECT \
        -DAUTOGEN_PM_ENABLE_ID \
        -DAUTOGEN_PM_ENABLE_TYPE_STR \
        -DPAR_CFG_NVM_OBJECT_STORE_MODE=99U

    autogen_pm_ci_expect_invalid_config invalid-object-address-mode \
        "PAR_CFG_NVM_OBJECT_ADDR_MODE must be AFTER_SCALAR or FIXED" \
        -DAUTOGEN_PM_USING_NVM \
        -DAUTOGEN_PM_NVM_OBJECT \
        -DAUTOGEN_PM_NVM_OBJECT_STORE_SHARED \
        -DAUTOGEN_PM_ENABLE_ID \
        -DAUTOGEN_PM_ENABLE_TYPE_STR \
        -DPAR_CFG_NVM_OBJECT_ADDR_MODE=99U

    autogen_pm_ci_expect_invalid_config runtime-dup-id-requires-id \
        "runtime duplicate-ID diagnostics require PAR_CFG_ENABLE_ID = 1" \
        -DAUTOGEN_PM_ENABLE_RUNTIME_ID_DUP_CHECK

    autogen_pm_ci_expect_invalid_config runtime-id-hash-requires-id \
        "runtime ID hash-collision diagnostics require PAR_CFG_ENABLE_ID = 1" \
        -DAUTOGEN_PM_ENABLE_RUNTIME_ID_HASH_COLLISION_CHECK

    autogen_pm_ci_expect_invalid_config invalid-layout-source \
        "PAR_CFG_LAYOUT_SOURCE must be PAR_CFG_LAYOUT_COMPILE_SCAN or PAR_CFG_LAYOUT_SCRIPT" \
        -DPAR_CFG_LAYOUT_SOURCE=99U

    autogen_pm_ci_expect_invalid_config flash-ee-logical-size-not-line-multiple \
        "flash-ee logical size must be an integer multiple of the line size" \
        -DAUTOGEN_PM_USING_NVM \
        -DAUTOGEN_PM_NVM_SCALAR \
        -DAUTOGEN_PM_USING_FLASH_EE_BACKEND \
        -DPAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE=130U \
        -DPAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE=16U

    autogen_pm_ci_expect_invalid_config flash-ee-line-size-zero \
        "flash-ee line size must be greater than 0" \
        -DAUTOGEN_PM_USING_NVM \
        -DAUTOGEN_PM_NVM_SCALAR \
        -DAUTOGEN_PM_USING_FLASH_EE_BACKEND \
        -DPAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE=0U

    autogen_pm_ci_expect_invalid_config flash-ee-logical-size-zero \
        "flash-ee logical size must be greater than 0" \
        -DAUTOGEN_PM_USING_NVM \
        -DAUTOGEN_PM_NVM_SCALAR \
        -DAUTOGEN_PM_USING_FLASH_EE_BACKEND \
        -DPAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE=0U

    autogen_pm_ci_expect_invalid_config flash-ee-cache-size-zero \
        "flash-ee cache size must be greater than 0" \
        -DAUTOGEN_PM_USING_NVM \
        -DAUTOGEN_PM_NVM_SCALAR \
        -DAUTOGEN_PM_USING_FLASH_EE_BACKEND \
        -DPAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE=0U

    autogen_pm_ci_expect_invalid_config flash-ee-cache-size-not-line-multiple \
        "flash-ee cache size must be an integer multiple of the line size" \
        -DAUTOGEN_PM_USING_NVM \
        -DAUTOGEN_PM_NVM_SCALAR \
        -DAUTOGEN_PM_USING_FLASH_EE_BACKEND \
        -DPAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE=130U \
        -DPAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE=16U

    autogen_pm_ci_expect_invalid_config flash-ee-program-size-zero \
        "flash-ee program size must be greater than 0" \
        -DAUTOGEN_PM_USING_NVM \
        -DAUTOGEN_PM_NVM_SCALAR \
        -DAUTOGEN_PM_USING_FLASH_EE_BACKEND \
        -DPAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE=0U

    autogen_pm_ci_expect_invalid_config flash-ee-program-size-not-header-divisor \
        "flash-ee program size must divide the 64-byte bank header exactly" \
        -DAUTOGEN_PM_USING_NVM \
        -DAUTOGEN_PM_NVM_SCALAR \
        -DAUTOGEN_PM_USING_FLASH_EE_BACKEND \
        -DPAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE=6U

    autogen_pm_ci_expect_invalid_config at24-window-size-zero \
        "par_rtt_at24_window_size_nonzero" \
        -DAUTOGEN_PM_USING_NVM \
        -DAUTOGEN_PM_NVM_SCALAR \
        -DAUTOGEN_PM_USING_RTT_AT24CXX_BACKEND \
        -DAUTOGEN_PM_ENABLE_ID \
        -DPAR_CFG_RTT_AT24_BASE_ADDR=0U \
        -DPAR_STORE_RTT_AT24_ERASE_CHUNK=8U \
        -DPAR_CFG_RTT_AT24_SIZE=0U \
        -include backend/par_store_backend_rtt_at24cxx.c

    autogen_pm_ci_expect_invalid_config at24-window-base-out-of-range \
        "par_rtt_at24_window_base_in_range" \
        -DAUTOGEN_PM_USING_NVM \
        -DAUTOGEN_PM_NVM_SCALAR \
        -DAUTOGEN_PM_USING_RTT_AT24CXX_BACKEND \
        -DAUTOGEN_PM_ENABLE_ID \
        -DPAR_STORE_RTT_AT24_ERASE_CHUNK=8U \
        -DPAR_CFG_RTT_AT24_BASE_ADDR=AT24CXX_MAX_MEM_ADDRESS \
        -include backend/par_store_backend_rtt_at24cxx.c

    autogen_pm_ci_expect_invalid_config at24-window-end-out-of-range \
        "par_rtt_at24_window_end_in_range" \
        -DAUTOGEN_PM_USING_NVM \
        -DAUTOGEN_PM_NVM_SCALAR \
        -DAUTOGEN_PM_USING_RTT_AT24CXX_BACKEND \
        -DAUTOGEN_PM_ENABLE_ID \
        -DPAR_STORE_RTT_AT24_ERASE_CHUNK=8U \
        -DPAR_CFG_RTT_AT24_BASE_ADDR=250U \
        -DPAR_CFG_RTT_AT24_SIZE=16U \
        -include backend/par_store_backend_rtt_at24cxx.c

    autogen_pm_ci_expect_invalid_config at24-window-end-overflow \
        "par_rtt_at24_window_no_overflow" \
        -DAUTOGEN_PM_USING_NVM \
        -DAUTOGEN_PM_NVM_SCALAR \
        -DAUTOGEN_PM_USING_RTT_AT24CXX_BACKEND \
        -DAUTOGEN_PM_ENABLE_ID \
        -DPAR_STORE_RTT_AT24_ERASE_CHUNK=8U \
        -DPAR_CFG_RTT_AT24_BASE_ADDR=0xFFFFFFF0UL \
        -DPAR_CFG_RTT_AT24_SIZE=0x20UL \
        -include backend/par_store_backend_rtt_at24cxx.c

    autogen_pm_ci_expect_invalid_config at24-page-size-zero \
        "par_rtt_at24_page_size_nonzero" \
        -DAUTOGEN_PM_USING_NVM \
        -DAUTOGEN_PM_NVM_SCALAR \
        -DAUTOGEN_PM_USING_RTT_AT24CXX_BACKEND \
        -DAUTOGEN_PM_ENABLE_ID \
        -DPAR_CFG_RTT_AT24_BASE_ADDR=0U \
        -DPAR_STORE_RTT_AT24_ERASE_CHUNK=8U \
        -DAT24CXX_PAGE_BYTE=0U \
        -include backend/par_store_backend_rtt_at24cxx.c

    autogen_pm_ci_expect_invalid_config at24-erase-chunk-zero \
        "par_rtt_at24_erase_chunk_nonzero" \
        -DAUTOGEN_PM_USING_NVM \
        -DAUTOGEN_PM_NVM_SCALAR \
        -DAUTOGEN_PM_USING_RTT_AT24CXX_BACKEND \
        -DAUTOGEN_PM_ENABLE_ID \
        -DPAR_CFG_RTT_AT24_BASE_ADDR=0U \
        -DPAR_STORE_RTT_AT24_ERASE_CHUNK=0U \
        -include backend/par_store_backend_rtt_at24cxx.c

    autogen_pm_ci_cleanup_invalid_config_matrix
    trap - RETURN
}

for profile in "${profiles[@]}"; do
    export AUTOGEN_PM_CI_PROFILE="$profile"
    # shellcheck source=.github/ci/autogen-pm-ci-profile.sh
    . "$script_dir/autogen-pm-ci-profile.sh"
    profile_defines="$(autogen_pm_ci_emit_cppcheck_defines)"

    expected_layout_count=1
    case "$profile" in
        minimal-scalar-no-nvm|scalar-no-metadata|scalar-no-access-role|scalar-no-validation-callback)
            expected_layout_count=0
            ;;
    esac
    layout_count="$(grep -Ec '^-DAUTOGEN_PM_NVM_RECORD_LAYOUT_' <<< "$profile_defines" || true)"
    if [ "$layout_count" -ne "$expected_layout_count" ]; then
        echo "profile $profile must select $expected_layout_count scalar NVM layout choices, got $layout_count" >&2
        exit 1
    fi

    autogen_pm_ci_verify_config_pruning "$profile"

    case "$profile" in
        object-shared-after-scalar)
            grep -Fx -- '-DAUTOGEN_PM_NVM_OBJECT' <<< "$profile_defines" >/dev/null
            grep -Fx -- '-DAUTOGEN_PM_NVM_OBJECT_STORE_SHARED' <<< "$profile_defines" >/dev/null
            grep -Fx -- '-DAUTOGEN_PM_NVM_OBJECT_ADDR_AFTER_SCALAR' <<< "$profile_defines" >/dev/null
            ;;
        object-shared-fixed)
            grep -Fx -- '-DAUTOGEN_PM_NVM_OBJECT' <<< "$profile_defines" >/dev/null
            grep -Fx -- '-DAUTOGEN_PM_NVM_OBJECT_STORE_SHARED' <<< "$profile_defines" >/dev/null
            grep -Fx -- '-DAUTOGEN_PM_NVM_OBJECT_ADDR_FIXED' <<< "$profile_defines" >/dev/null
            grep -Fx -- '-DAUTOGEN_PM_NVM_OBJECT_FIXED_ADDR=0x600' <<< "$profile_defines" >/dev/null
            ;;
        generated-layout-info)
            grep -Fx -- '-DAUTOGEN_PM_LAYOUT_SOURCE_SCRIPT' <<< "$profile_defines" >/dev/null
            grep -Fx -- '-DAUTOGEN_PM_ENABLE_GENERATED_INFO' <<< "$profile_defines" >/dev/null
            ;;
        flash-ee-fal-backend)
            grep -Fx -- '-DAUTOGEN_PM_USING_FLASH_EE_BACKEND' <<< "$profile_defines" >/dev/null
            grep -Fx -- '-DAUTOGEN_PM_FLASH_EE_PORT_FAL' <<< "$profile_defines" >/dev/null
            ;;
        rtt-at24cxx-backend)
            grep -Fx -- '-DAUTOGEN_PM_USING_RTT_AT24CXX_BACKEND' <<< "$profile_defines" >/dev/null
            grep -Fx -- '-DAUTOGEN_PM_RTT_AT24_I2C_BUS_NAME="i2c1"' <<< "$profile_defines" >/dev/null
            ;;
    esac

    echo "CONFIG_PROFILE_OK $profile"
done

autogen_pm_ci_verify_invalid_config_matrix

printf 'CONFIG_PROFILE_SUMMARY Passed %u/%u\n' "${#profiles[@]}" "${#profiles[@]}"
if [ "$AUTOGEN_PM_CI_INVALID_CONFIG_COUNT" -ne "$AUTOGEN_PM_CI_INVALID_CONFIG_EXPECTED_COUNT" ]; then
    echo "CONFIG_INVALID_SUMMARY expected $AUTOGEN_PM_CI_INVALID_CONFIG_EXPECTED_COUNT cases, got $AUTOGEN_PM_CI_INVALID_CONFIG_COUNT" >&2
    exit 1
fi

printf 'CONFIG_INVALID_SUMMARY Passed %u/%u\n' \
    "$AUTOGEN_PM_CI_INVALID_CONFIG_COUNT" \
    "$AUTOGEN_PM_CI_INVALID_CONFIG_EXPECTED_COUNT"
