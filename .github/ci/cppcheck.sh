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

    if [ "$mode" != "auto" ]; then
        printf '%s\n' "$mode"
        return 0
    fi

    if [ "$ref_name" = "main" ] || [ "$ref_name" = "master" ]; then
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

autogen_pm_ci_select_cppcheck_paths() {
    local mode="$1"
    local shard="$2"

    case "$shard" in
        source)
            cppcheck_paths=(
                backend
                parameters/src
                port
            )
            ;;
        host-tests)
            if [ "$mode" != "deep" ]; then
                echo "Skipping host-test cppcheck shard in $mode mode." >&2
                return 2
            fi
            cppcheck_paths=(
                parameters/tests/host
            )
            ;;
        *)
            echo "Unknown CPPCHECK_SHARD: $shard" >&2
            return 1
            ;;
    esac
}

cppcheck_mode="$(autogen_pm_ci_resolve_cppcheck_mode "$cppcheck_mode")"
cppcheck_options=()
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

echo "cppcheck shard=$cppcheck_shard mode=$cppcheck_mode" >&2

cppcheck_status=0
if cppcheck \
    "${cppcheck_options[@]}" \
    --std=c11 \
    --quiet \
    --template=gcc \
    --suppressions-list="$suppressions" \
    "${cppcheck_defines[@]}" \
    -I. \
    -Ibackend \
    -Iport \
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
    -Iparameters/generated \
    -Iparameters/tests/host \
    -Iparameters/tests/host/fixtures \
    -Iparameters/tests/host/fixtures_backend \
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
