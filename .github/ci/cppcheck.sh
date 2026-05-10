#!/usr/bin/env bash
set -euo pipefail

script_dir="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=.github/ci/autogen-pm-ci-profile.sh
. "$script_dir/autogen-pm-ci-profile.sh"
# shellcheck source=.github/ci/host-test-nvm-profiles.sh
. "$script_dir/host-test-nvm-profiles.sh"

report="${CPPCHECK_REPORT:-cppcheck.txt}"
suppressions="$script_dir/cppcheck-suppressions.txt"
cppcheck_mode="${CPPCHECK_MODE:-auto}"
cppcheck_shard="${CPPCHECK_SHARD:-source}"
cppcheck_nvm_profile="${CPPCHECK_NVM_PROFILE:-default}"
: > "$report"

test -f "$suppressions" || {
    echo "cppcheck suppressions file is missing: $suppressions" >&2
    exit 1
}

if ! autogen_pm_ci_verify_cppcheck_profile_defines; then
    echo "cppcheck define profile is inconsistent with CI compile symbols." >&2
    exit 1
fi

_defines_output="$(autogen_pm_ci_emit_cppcheck_defines)" || {
    echo "autogen_pm_ci_emit_cppcheck_defines failed; cannot build cppcheck define list." >&2
    exit 1
}
mapfile -t cppcheck_defines <<< "$_defines_output"
unset _defines_output

autogen_pm_ci_report_has_cppcheck_errors() {
    local report_file="$1"

    awk '
        /^[^:]+:[0-9]+(:[0-9]+)?: error: / { found = 1 }
        /^cppcheck: error: / { found = 1 }
        END { exit(found ? 0 : 1) }
    ' "$report_file"
}

autogen_pm_ci_resolve_cppcheck_mode() {
    local mode="$1"
    local ref_name="${GITHUB_REF_NAME:-}"
    local base_ref="${GITHUB_BASE_REF:-$ref_name}"

    if [ "$mode" != "auto" ]; then
        printf '%s\n' "$mode"
        return 0
    fi

    if [ "$base_ref" = "main" ] || [ "$base_ref" = "master" ]; then
        printf '%s\n' deep
    else
        printf '%s\n' fast
    fi
}

autogen_pm_ci_select_cppcheck_options() {
    local mode="$1"

    case "$mode" in
        deep)
            cppcheck_options=(
                --enable=warning,style,performance,portability
                --inconclusive
                --force
            )
            ;;
        fast)
            cppcheck_options=(
                --enable=warning,performance,portability
            )
            ;;
        *)
            echo "Unknown CPPCHECK_MODE: $mode" >&2
            return 1
            ;;
    esac
}

autogen_pm_ci_add_source_cppcheck_includes() {
    cppcheck_includes+=(
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
        -Iparameters/generated
    )
}

autogen_pm_ci_add_host_cppcheck_includes() {
    cppcheck_includes+=(
        -Iparameters/tests/host/fixtures
    )
    autogen_pm_ci_add_source_cppcheck_includes
    cppcheck_includes+=(
        -Iparameters/tests/host
        -Iparameters/tests/host/fixtures_backend
    )
}

autogen_pm_ci_add_host_nvm_cppcheck_includes() {
    cppcheck_includes+=(
        -Iparameters/tests/host/fixtures_nvm
        -Iparameters/tests/host/fixtures
    )
    autogen_pm_ci_add_source_cppcheck_includes
    cppcheck_includes+=(
        -Iparameters/tests/host
        -Iparameters/tests/host/fixtures_backend
    )
}

autogen_pm_ci_add_host_runtime_cppcheck_paths() {
    cppcheck_paths+=(
        parameters/tests/host/test_par_backend_flash_ee_fal_smoke.c
        parameters/tests/host/test_par_backend_rtt_at24cxx_smoke.c
        parameters/tests/host/test_par_config_smoke.c
        parameters/tests/host/test_par_core_runtime.c
        parameters/tests/host/test_par_generated_runtime_consistency.c
        parameters/tests/host/test_par_mutex_runtime.c
        parameters/tests/host/test_par_object_runtime.c
        parameters/tests/host/test_par_shell_tool.c
    )
}

autogen_pm_ci_add_host_nvm_cppcheck_paths() {
    local layout_source="$1"

    cppcheck_paths+=(
        parameters/tests/host/test_par_nvm_flash_ee.c
        parameters/src/par.c
        parameters/src/object/par_object.c
        parameters/src/object/par_object_api.c
        parameters/src/scalar/par_scalar_api.c
        parameters/src/def/par_def.c
        parameters/src/def/par_id_map_static.c
        parameters/src/layout/par_layout.c
        parameters/src/port/par_if.c
        parameters/src/nvm/par_nvm.c
        parameters/src/nvm/par_nvm_table_id.c
        parameters/src/nvm/hash_32a.c
        parameters/src/nvm/scalar/store/par_nvm_scalar_store.c
        parameters/src/nvm/scalar/par_nvm_scalar.c
        parameters/src/nvm/backend/par_store_backend_flash_ee.c
        parameters/src/nvm/object/par_nvm_object.c
        parameters/src/nvm/object/store/par_nvm_object_store_shared.c
        parameters/src/nvm/object/store/par_nvm_object_store_dedicated.c
        parameters/src/nvm/object/addr/par_nvm_object_addr_after_scalar.c
        parameters/src/nvm/object/addr/par_nvm_object_addr_fixed.c
        parameters/src/nvm/object/addr/par_nvm_object_addr_dedicated.c
        backend/par_store_backend_flash_ee_native.c
        "$layout_source"
    )
}

autogen_pm_ci_select_host_nvm_cppcheck_profile() {
    local profile="$1"

    autogen_pm_ci_select_nvm_flash_ee_profile "$profile" || return 1

    cppcheck_defines+=(
        -DPAR_HOST_TEST_NVM
        -DPAR_HOST_TEST_PROFILE_NAME="\"${AUTOGEN_PM_CI_NVM_PROFILE_NAME}\""
        -DPAR_CFG_NVM_RECORD_LAYOUT="$AUTOGEN_PM_CI_NVM_PROFILE_RECORD_LAYOUT"
        -DPAR_CFG_NVM_OBJECT_STORE_MODE="$AUTOGEN_PM_CI_NVM_PROFILE_OBJECT_STORE"
        -DPAR_CFG_NVM_OBJECT_ADDR_MODE="$AUTOGEN_PM_CI_NVM_PROFILE_OBJECT_ADDR"
        -DPAR_CFG_NVM_OBJECT_FIXED_ADDR="$AUTOGEN_PM_CI_NVM_PROFILE_OBJECT_FIXED_ADDR"
        -DPAR_CFG_NVM_OBJECT_REGION_SIZE="$AUTOGEN_PM_CI_NVM_PROFILE_OBJECT_REGION_SIZE"
    )
    autogen_pm_ci_add_host_nvm_cppcheck_paths "$AUTOGEN_PM_CI_NVM_PROFILE_LAYOUT_SOURCE"
}

autogen_pm_ci_select_cppcheck_paths() {
    local mode="$1"
    local shard="$2"

    case "$shard" in
        source)
            autogen_pm_ci_add_source_cppcheck_includes
            cppcheck_paths+=(
                backend
                parameters/src
                port
            )
            ;;
        host-tests|host-runtime-tests)
            if [ "$mode" != "deep" ]; then
                echo "Skipping host-test cppcheck shard in $mode mode." >&2
                return 2
            fi
            autogen_pm_ci_add_host_cppcheck_includes
            autogen_pm_ci_add_host_runtime_cppcheck_paths
            ;;
        host-nvm-tests)
            if [ "$mode" != "deep" ]; then
                echo "Skipping host NVM cppcheck shard in $mode mode." >&2
                return 2
            fi
            autogen_pm_ci_add_host_nvm_cppcheck_includes
            autogen_pm_ci_select_host_nvm_cppcheck_profile "$cppcheck_nvm_profile"
            ;;
        *)
            echo "Unknown CPPCHECK_SHARD: $shard" >&2
            return 1
            ;;
    esac
}

cppcheck_mode="$(autogen_pm_ci_resolve_cppcheck_mode "$cppcheck_mode")"
cppcheck_options=()
cppcheck_includes=()
cppcheck_paths=()
autogen_pm_ci_select_cppcheck_options "$cppcheck_mode"
select_status=0
autogen_pm_ci_select_cppcheck_paths "$cppcheck_mode" "$cppcheck_shard" || select_status=$?
if [ "$select_status" -ne 0 ]; then
    if [ "$select_status" -eq 2 ]; then
        echo "cppcheck skipped; shard=$cppcheck_shard mode=$cppcheck_mode" > "$report"
        cat "$report" >&2
        exit 0
    fi
    exit "$select_status"
fi

if [ "$cppcheck_shard" = "host-nvm-tests" ]; then
    echo "cppcheck shard=$cppcheck_shard mode=$cppcheck_mode profile=$cppcheck_nvm_profile" >&2
else
    echo "cppcheck shard=$cppcheck_shard mode=$cppcheck_mode" >&2
fi

cppcheck_status=0
if cppcheck \
    "${cppcheck_options[@]}" \
    --std=c11 \
    --quiet \
    --template=gcc \
    --suppressions-list="$suppressions" \
    "${cppcheck_defines[@]}" \
    "${cppcheck_includes[@]}" \
    "${cppcheck_paths[@]}" \
    2> "$report"; then
    cppcheck_status=0
else
    cppcheck_status=$?
fi

cat "$report" >&2

if [ "$cppcheck_status" -ne 0 ]; then
    echo "cppcheck execution failed with exit code $cppcheck_status." >&2
    exit "$cppcheck_status"
fi

if autogen_pm_ci_report_has_cppcheck_errors "$report"; then
    echo "cppcheck reported error-severity diagnostics. See $report." >&2
    exit 1
fi

if [ "$cppcheck_mode" = "deep" ]; then
    echo "cppcheck deep scan completed; warning/style/performance/portability diagnostics are report-only."
else
    echo "cppcheck fast scan completed; warning/performance/portability diagnostics are report-only."
fi
