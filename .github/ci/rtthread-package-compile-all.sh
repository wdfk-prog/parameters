#!/usr/bin/env bash
set -euo pipefail

workspace="${GITHUB_WORKSPACE:-$(pwd)}"
script_dir="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
profile_list_file="${AUTOGEN_PM_CI_PROFILE_LIST_FILE:-$workspace/.github/ci/rtthread-profile-list.txt}"
work_root="$workspace/_ci/rtthread-package-profiles"
log_dir="$workspace/_ci/logs"
max_parallel="${AUTOGEN_PM_CI_PROFILE_PARALLEL:-2}"
failed=0
profiles=()
active_pids=()
active_profiles=()

load_profiles() {
    local line

    test -f "$profile_list_file" || {
        echo "RT-Thread profile list not found: $profile_list_file" >&2
        exit 1
    }

    while IFS= read -r line || [ -n "$line" ]; do
        line="${line%$'\r'}"
        case "$line" in
            ""|\#*)
                continue
                ;;
            *)
                profiles+=("$line")
                ;;
        esac
    done < "$profile_list_file"

    if [ "${#profiles[@]}" -eq 0 ]; then
        echo "RT-Thread profile list is empty: $profile_list_file" >&2
        exit 1
    fi
}

sanitize_profile_path() {
    local value="$1"

    value="${value//[^A-Za-z0-9_.-]/_}"
    printf '%s\n' "$value"
}

verify_unique_profile_paths() {
    local profile
    local profile_dir
    declare -A seen_profile_dirs=()

    for profile in "${profiles[@]}"; do
        profile_dir="$(sanitize_profile_path "$profile")"
        if [ -n "${seen_profile_dirs[$profile_dir]:-}" ]; then
            printf 'RT-Thread profile path collision: %s and %s -> %s\n' \
                "${seen_profile_dirs[$profile_dir]}" "$profile" "$profile_dir" >&2
            exit 1
        fi
        seen_profile_dirs["$profile_dir"]="$profile"
    done
}

run_profile() {
    local profile="$1"
    local profile_dir
    local import_external_packages=0

    profile_dir="$(sanitize_profile_path "$profile")"
    if [ "$profile" = "rtt-at24cxx-backend" ]; then
        import_external_packages=1
    fi

    AUTOGEN_PM_CI_PROFILE="$profile" \
    AUTOGEN_PM_CI_PROFILE_LOG_NAME="$profile_dir" \
    AUTOGEN_PM_CI_IMPORT_EXTERNAL_PACKAGES="$import_external_packages" \
    AUTOGEN_PM_CI_ROOT="$work_root/$profile_dir" \
    AUTOGEN_PM_CI_LOG_DIR="$log_dir" \
        bash "$script_dir/rtthread-package-compile.sh"
}

start_profile() {
    local profile="$1"
    local profile_dir
    local console_log

    profile_dir="$(sanitize_profile_path "$profile")"
    console_log="$log_dir/rtthread-package-compile-${profile_dir}.console.log"

    echo "[rtthread-package] start $profile"
    (
        run_profile "$profile"
    ) > "$console_log" 2>&1 &
    active_pids+=("$!")
    active_profiles+=("$profile")
}

wait_active_profiles() {
    local idx
    local pid
    local profile

    for idx in "${!active_pids[@]}"; do
        pid="${active_pids[$idx]}"
        profile="${active_profiles[$idx]}"
        if ! wait "$pid"; then
            echo "[rtthread-package] failed $profile" >&2
            failed=1
        else
            echo "[rtthread-package] done $profile"
        fi
    done
    active_pids=()
    active_profiles=()
}

print_profile_logs() {
    local profile
    local profile_dir
    local log_file
    local console_log

    for profile in "${profiles[@]}"; do
        profile_dir="$(sanitize_profile_path "$profile")"
        log_file="$log_dir/rtthread-package-compile-${profile_dir}.log"
        console_log="$log_dir/rtthread-package-compile-${profile_dir}.console.log"
        echo "===== RT-Thread package profile: $profile ====="
        if [ -f "$log_file" ]; then
            cat "$log_file"
        elif [ -f "$console_log" ]; then
            cat "$console_log"
        else
            echo "No log was produced for profile: $profile" >&2
        fi
    done
}

case "$max_parallel" in
    ''|*[!0-9]*)
        echo "AUTOGEN_PM_CI_PROFILE_PARALLEL must be a positive integer: $max_parallel" >&2
        exit 1
        ;;
    0)
        echo "AUTOGEN_PM_CI_PROFILE_PARALLEL must be greater than 0" >&2
        exit 1
        ;;
esac

load_profiles
verify_unique_profile_paths
rm -rf "$work_root"
mkdir -p "$work_root" "$log_dir"

for profile in "${profiles[@]}"; do
    start_profile "$profile"
    if [ "${#active_pids[@]}" -ge "$max_parallel" ]; then
        wait_active_profiles
    fi
done

if [ "${#active_pids[@]}" -gt 0 ]; then
    wait_active_profiles
fi

print_profile_logs
exit "$failed"
