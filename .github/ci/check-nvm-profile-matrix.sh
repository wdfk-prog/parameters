#!/usr/bin/env bash
# Verify that the GitHub Actions NVM profile matrix matches the shared profile list.
set -euo pipefail

script_dir="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(CDPATH= cd -- "$script_dir/../.." && pwd)"
workflow_file="${1:-$repo_root/.github/workflows/ci.yml}"
profile_script="$script_dir/host-test-nvm-profiles.sh"

extract_workflow_nvm_profiles() {
    local file="$1"

    # This lightweight parser intentionally validates only the CI layout used by
    # this workflow: the host-cppcheck-host-nvm-tests job key must be unquoted at
    # two-space top-level indentation, and nvm_profile must be a block-style list
    # with dash entries. Quoted keys, flow-style lists, or inline nvm_profile
    # comments can make the parser miss profiles; use yq or Python YAML parsing
    # if the workflow layout needs to be relaxed.
    awk '
        /^  host-cppcheck-host-nvm-tests:/ {
            in_job = 1
            next
        }
        in_job && /^  [A-Za-z0-9_-]+:/ {
            exit
        }
        in_job && /^[[:space:]]+nvm_profile:[[:space:]]*$/ {
            in_profiles = 1
            next
        }
        in_profiles && /^[[:space:]]*-[[:space:]]*/ {
            sub(/^[[:space:]]*-[[:space:]]*/, "")
            sub(/[[:space:]]*#.*/, "")
            gsub(/["'\''[:space:]]/, "")
            if ($0 != "") {
                print $0
            }
            next
        }
        in_profiles && /^[[:space:]]*[A-Za-z0-9_-]+:/ {
            exit
        }
    ' "$file"
}

expected_profiles="$(bash "$profile_script" --list-names)"
actual_profiles="$(extract_workflow_nvm_profiles "$workflow_file")"

if [ -z "$actual_profiles" ]; then
    echo "No host-cppcheck-host-nvm-tests matrix profiles found in $workflow_file" >&2
    exit 1
fi

if [ "$actual_profiles" != "$expected_profiles" ]; then
    echo "host-cppcheck-host-nvm-tests matrix does not match host-test-nvm-profiles.sh" >&2
    echo "Expected profiles:" >&2
    printf '%s\n' "$expected_profiles" >&2
    echo "Actual profiles:" >&2
    printf '%s\n' "$actual_profiles" >&2
    exit 1
fi

printf 'host-cppcheck-host-nvm-tests matrix matches host-test-nvm-profiles.sh\n'
