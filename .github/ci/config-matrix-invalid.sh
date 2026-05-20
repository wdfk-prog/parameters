#!/usr/bin/env bash
# Invalid configuration matrix helpers for config-matrix.sh.

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

