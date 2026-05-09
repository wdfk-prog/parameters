#!/usr/bin/env bash
set -euo pipefail

script_dir="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=.github/ci/autogen-pm-ci-profile.sh
. "$script_dir/autogen-pm-ci-profile.sh"

report="${CPPCHECK_REPORT:-cppcheck.txt}"
suppressions="$script_dir/cppcheck-suppressions.txt"
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

cppcheck_status=0
if cppcheck \
    --enable=warning,style,performance,portability \
    --inconclusive \
    --std=c11 \
    --force \
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
    backend \
    parameters/src \
    port \
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

echo "cppcheck completed; warning/style/performance/portability diagnostics are report-only."
