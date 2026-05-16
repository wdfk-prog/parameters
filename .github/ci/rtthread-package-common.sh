#!/usr/bin/env bash
# Common RT-Thread package compile helpers.

validate_bsp_path() {
    case "$bsp_path" in
        ""|"."|".."|/*|./*|../*|*/../*|*/..|*/./*|*/.)
            echo "BSP_PATH must be a clean relative path under rt-thread/bsp: $bsp_path" | tee -a "$log_file" >&2
            exit 1
            ;;
        bsp/*)
            ;;
        *)
            echo "BSP_PATH must start with bsp/: $bsp_path" | tee -a "$log_file" >&2
            exit 1
            ;;
    esac
}

validate_bsp_dir() {
    local resolved_bsp_dir
    local resolved_bsp_root

    test -d "$bsp_dir" || {
        echo "BSP path does not exist: $bsp_dir" | tee -a "$log_file" >&2
        find "$rtt_root/bsp" -maxdepth 3 -type f -name SConstruct | sort | tee -a "$log_file" >&2
        exit 1
    }

    resolved_bsp_dir="$(CDPATH= cd -- "$bsp_dir" && pwd -P)"
    resolved_bsp_root="$(CDPATH= cd -- "$rtt_root/bsp" && pwd -P)"

    case "$resolved_bsp_dir" in
        "$resolved_bsp_root"/*)
            ;;
        *)
            echo "Resolved BSP path escapes rt-thread/bsp: $resolved_bsp_dir" | tee -a "$log_file" >&2
            exit 1
            ;;
    esac
}

prepare_rtthread() {
    local source_sha=""

    mkdir -p "$ci_root"
    rm -rf "$rtt_root"

    if [ -n "$rtthread_source_tar" ]; then
        test -f "$rtthread_source_tar" || {
            echo "RT-Thread source tar does not exist: $rtthread_source_tar" | tee -a "$log_file" >&2
            exit 1
        }

        run_logged tar -xf "$rtthread_source_tar" -C "$ci_root"
        test -d "$rtt_root" || {
            echo "RT-Thread source tar did not contain rt-thread/: $rtthread_source_tar" | tee -a "$log_file" >&2
            exit 1
        }

        if [ -n "$rtthread_source_sha_file" ] && [ -f "$rtthread_source_sha_file" ]; then
            source_sha="$(cat "$rtthread_source_sha_file")"
        fi
    else
        local rtthread_remote="${RTTHREAD_REMOTE:-https://github.com/RT-Thread/rt-thread.git}"
        local clone_ref="$rtthread_ref"

        case "$clone_ref" in
            ""|default|HEAD)
                clone_ref="$(git ls-remote --symref "$rtthread_remote" HEAD | \
                    awk '$1 == "ref:" { sub("refs/heads/", "", $2); print $2; exit }')"
                if [ -z "$clone_ref" ]; then
                    echo "Unable to resolve RT-Thread default branch from $rtthread_remote" | tee -a "$log_file" >&2
                    exit 1
                fi
                ;;
        esac
        run_logged git clone --depth 1 --branch "$clone_ref" \
            "$rtthread_remote" "$rtt_root"
        source_sha="$(git -C "$rtt_root" rev-parse HEAD)"
    fi

    echo "RTTHREAD_SOURCE ref=$rtthread_ref resolved_ref=${clone_ref:-tarball} sha=${source_sha:-unknown}" | tee -a "$log_file"
}

configure_toolchain() {
    export RTT_ROOT="$rtt_root"
    export RTT_CC="${RTT_CC:-gcc}"

    if [ -z "${RTT_EXEC_PATH:-}" ]; then
        RTT_EXEC_PATH="$(dirname "$(command -v arm-none-eabi-gcc)")"
        export RTT_EXEC_PATH
    fi

    local toolchain_cc="$RTT_EXEC_PATH/arm-none-eabi-gcc"

    {
        echo "RTT_ROOT=$RTT_ROOT"
        echo "RTT_CC=$RTT_CC"
        echo "RTT_EXEC_PATH=$RTT_EXEC_PATH"
    } | tee -a "$log_file"

    test -x "$toolchain_cc" || {
        echo "arm-none-eabi-gcc not found: $toolchain_cc" | tee -a "$log_file" >&2
        exit 1
    }

    printf '%s\n' '#include <stdio.h>' '#include <sys/types.h>' | \
        "$toolchain_cc" -E -x c - >/dev/null || {
        echo "arm-none-eabi-gcc cannot find newlib C library headers." | tee -a "$log_file" >&2
        echo "Install libnewlib-arm-none-eabi in the CI image." | tee -a "$log_file" >&2
        exit 1
    }
}

rtthread_pyconfig_lock_owner_file() {
    printf '%s/owner\n' "$1"
}

rtthread_pyconfig_normalize_lock_dir() {
    local lock_dir="$1"

    while [ "$lock_dir" != "/" ] && [ "${lock_dir%/}" != "$lock_dir" ]; do
        lock_dir="${lock_dir%/}"
    done
    printf '%s\n' "$lock_dir"
}

rtthread_pyconfig_lock_parent_dir() {
    local lock_dir="$1"
    local lock_parent

    case "$lock_dir" in
        */*)
            lock_parent="${lock_dir%/*}"
            if [ -z "$lock_parent" ]; then
                lock_parent="/"
            fi
            ;;
        *)
            lock_parent="."
            ;;
    esac
    printf '%s\n' "$lock_parent"
}

rtthread_pyconfig_prepare_lock_parent() {
    local lock_dir="$1"
    local lock_parent

    lock_parent="$(rtthread_pyconfig_lock_parent_dir "$lock_dir")"
    if ! mkdir -p "$lock_parent" 2>/dev/null; then
        echo "Unable to create RT-Thread env lock parent: $lock_parent" | \
            tee -a "$log_file" >&2
        return 1
    fi
    if [ ! -d "$lock_parent" ]; then
        echo "RT-Thread env lock parent is not a directory: $lock_parent" | \
            tee -a "$log_file" >&2
        return 1
    fi
    return 0
}

rtthread_pyconfig_lock_owner_value() {
    local lock_dir="$1"
    local key="$2"

    awk -F= -v key="$key" '$1 == key { print substr($0, length($1) + 2); exit }' \
        "$(rtthread_pyconfig_lock_owner_file "$lock_dir")" 2>/dev/null || true
}

rtthread_pyconfig_lock_owner_pid() {
    rtthread_pyconfig_lock_owner_value "$1" "pid"
}

rtthread_pyconfig_current_host() {
    hostname 2>/dev/null || uname -n 2>/dev/null || printf 'unknown'
}

rtthread_pyconfig_process_start_tick() {
    local pid="$1"

    if [ -r "/proc/$pid/stat" ]; then
        awk '{ print $22; exit }' "/proc/$pid/stat" 2>/dev/null || true
    fi
}

rtthread_pyconfig_owner_age_exceeded() {
    local lock_dir="$1"
    local max_age_sec="$2"
    local owner_epoch
    local now_epoch

    owner_epoch="$(rtthread_pyconfig_lock_owner_value "$lock_dir" "started_epoch")"
    case "$owner_epoch" in
        ""|*[!0-9]*)
            return 1
            ;;
    esac
    now_epoch="$(date -u '+%s' 2>/dev/null || true)"
    case "$now_epoch" in
        ""|*[!0-9]*)
            return 1
            ;;
    esac
    [ $((now_epoch - owner_epoch)) -gt "$max_age_sec" ]
}

rtthread_pyconfig_log_lock_owner() {
    local lock_dir="$1"
    local owner_file

    owner_file="$(rtthread_pyconfig_lock_owner_file "$lock_dir")"
    if [ -f "$owner_file" ]; then
        echo "RT-Thread env lock owner for $lock_dir:" | tee -a "$log_file" >&2
        sed 's/^/  /' "$owner_file" | tee -a "$log_file" >&2 || true
    else
        echo "RT-Thread env lock owner for $lock_dir: unknown" | tee -a "$log_file" >&2
    fi
}

rtthread_pyconfig_lock_is_stale() {
    local lock_dir="$1"
    local wait_count="${2:-0}"
    local owner_file
    local owner_pid
    local owner_host
    local current_host
    local owner_start_tick
    local current_start_tick
    local owner_epoch

    owner_file="$(rtthread_pyconfig_lock_owner_file "$lock_dir")"
    test -d "$lock_dir" || return 1
    if [ ! -f "$owner_file" ]; then
        # The owner file is written only after mkdir acquires the lock.
        # Keep a grace period before treating owner-less locks as stale.
        [ "$wait_count" -gt 30 ] || return 1
        return 0
    fi

    owner_pid="$(rtthread_pyconfig_lock_owner_pid "$lock_dir")"
    case "$owner_pid" in
        ""|*[!0-9]*)
            return 0
            ;;
    esac

    owner_host="$(rtthread_pyconfig_lock_owner_value "$lock_dir" "host")"
    current_host="$(rtthread_pyconfig_current_host)"
    if [ -n "$owner_host" ] && [ -n "$current_host" ] && \
        [ "$owner_host" != "$current_host" ]; then
        [ "$wait_count" -gt 300 ] || \
            rtthread_pyconfig_owner_age_exceeded "$lock_dir" 300 || return 1
        return 0
    fi

    if ! kill -0 "$owner_pid" 2>/dev/null; then
        return 0
    fi

    owner_start_tick="$(rtthread_pyconfig_lock_owner_value "$lock_dir" "pid_start_tick")"
    if [ -z "$owner_start_tick" ]; then
        owner_epoch="$(rtthread_pyconfig_lock_owner_value "$lock_dir" "started_epoch")"
        # Legacy owner files cannot disambiguate a reused live PID.
        [ -n "$owner_epoch" ] && return 1
        [ "$wait_count" -gt 300 ] || return 1
        return 0
    fi

    current_start_tick="$(rtthread_pyconfig_process_start_tick "$owner_pid")"
    if [ -n "$current_start_tick" ] && \
        [ "$owner_start_tick" != "$current_start_tick" ]; then
        return 0
    fi

    return 1
}

rtthread_pyconfig_remove_stale_lock() {
    local lock_dir="$1"
    local wait_count="${2:-0}"

    rtthread_pyconfig_lock_is_stale "$lock_dir" "$wait_count" || return 1
    echo "Removing stale RT-Thread env lock: $lock_dir" | tee -a "$log_file" >&2
    rtthread_pyconfig_log_lock_owner "$lock_dir"
    rm -f "$(rtthread_pyconfig_lock_owner_file "$lock_dir")"
    rmdir "$lock_dir" 2>/dev/null || return 1
    return 0
}

rtthread_pyconfig_write_lock_owner() {
    local lock_dir="$1"
    local owner_pid="$2"
    local owner_file
    local started_at
    local started_epoch
    local host_name
    local pid_start_tick

    shift 2
    owner_file="$(rtthread_pyconfig_lock_owner_file "$lock_dir")"
    started_at="$(date -u '+%Y-%m-%dT%H:%M:%SZ' 2>/dev/null || date)"
    started_epoch="$(date -u '+%s' 2>/dev/null || true)"
    host_name="$(rtthread_pyconfig_current_host)"
    pid_start_tick="$(rtthread_pyconfig_process_start_tick "$owner_pid")"
    {
        printf 'pid=%s\n' "$owner_pid"
        printf 'started_at=%s\n' "$started_at"
        [ -z "$started_epoch" ] || printf 'started_epoch=%s\n' "$started_epoch"
        printf 'host=%s\n' "$host_name"
        [ -z "$pid_start_tick" ] || printf 'pid_start_tick=%s\n' "$pid_start_tick"
        printf 'command='
        printf '%s ' "$@"
        printf '\n'
    } > "$owner_file"
}

run_rtthread_pyconfig_logged() {
    local lock_dir="${AUTOGEN_PM_CI_RTT_ENV_LOCK_DIR:-}"
    local wait_count=0
    local status

    if [ -z "$lock_dir" ]; then
        run_shell_logged "$@"
        return $?
    fi

    lock_dir="$(rtthread_pyconfig_normalize_lock_dir "$lock_dir")"
    if [ "$lock_dir" = "/" ] || [ "$lock_dir" = "." ]; then
        echo "RT-Thread env lock must not use filesystem root or current directory" | \
            tee -a "$log_file" >&2
        return 1
    fi
    rtthread_pyconfig_prepare_lock_parent "$lock_dir" || return 1

    while ! mkdir "$lock_dir" 2>/dev/null; do
        if [ ! -d "$lock_dir" ]; then
            if mkdir "$lock_dir" 2>/dev/null; then
                break
            fi
            if [ ! -d "$lock_dir" ]; then
                echo "Unable to create RT-Thread env lock: $lock_dir" | \
                    tee -a "$log_file" >&2
                return 1
            fi
        fi
        wait_count=$((wait_count + 1))
        if [ "$wait_count" -gt 5 ] && rtthread_pyconfig_remove_stale_lock "$lock_dir" "$wait_count"; then
            continue
        fi
        if [ "$wait_count" -gt 300 ]; then
            echo "Timed out waiting for RT-Thread env lock: $lock_dir" | \
                tee -a "$log_file" >&2
            rtthread_pyconfig_log_lock_owner "$lock_dir"
            return 1
        fi
        sleep 1
    done

    (
        cleanup_lock() {
            rm -f "$(rtthread_pyconfig_lock_owner_file "$lock_dir")"
            rmdir "$lock_dir" 2>/dev/null || true
        }

        trap 'status=$?; cleanup_lock; trap - EXIT HUP INT TERM; exit "$status"' EXIT
        trap 'cleanup_lock; trap - EXIT HUP INT TERM; exit 129' HUP
        trap 'cleanup_lock; trap - EXIT HUP INT TERM; exit 130' INT
        trap 'cleanup_lock; trap - EXIT HUP INT TERM; exit 143' TERM

        if ! rtthread_pyconfig_write_lock_owner "$lock_dir" "${BASHPID:-$$}" "$@"; then
            exit 1
        fi

        set +e
        run_shell_logged "$@"
        status=$?
        set -e
        exit "$status"
    )
}

apply_ci_config() {
    local config_file="$bsp_dir/.config"
    local rtconfig="$bsp_dir/rtconfig.h"

    test -f "$config_file" || {
        echo ".config was not generated: $config_file" | tee -a "$log_file" >&2
        exit 1
    }

    autogen_pm_ci_update_config_file "$config_file"
    run_rtthread_pyconfig_logged scons -C "$bsp_dir" --pyconfig-silent

    test -f "$rtconfig" || {
        echo "rtconfig.h was not generated: $rtconfig" | tee -a "$log_file" >&2
        exit 1
    }

    if ! autogen_pm_ci_verify_rtconfig_defines "$rtconfig"; then
        echo "rtconfig.h missed CI profile symbols after .config regeneration." | tee -a "$log_file" >&2
        echo "This usually means the package Kconfig entry was not loaded" | tee -a "$log_file" >&2
        echo "or dependency checks rejected the CI profile." | tee -a "$log_file" >&2
        echo "Generated .config values for CI profile symbols:" | tee -a "$log_file" >&2
        autogen_pm_ci_for_each_symbol | while IFS= read -r symbol; do
            grep -E "^(CONFIG_${symbol}=|# CONFIG_${symbol} is not set)" "$config_file" || true
        done | tee -a "$log_file" >&2
        exit 1
    fi
}

verify_build_outputs() {
    local elf_file

    elf_file="$(find "$bsp_dir" -maxdepth 3 -type f \( -name '*.elf' -o -name '*.axf' \) -print -quit)"
    test -n "$elf_file" || {
        echo "No ELF/AXF output was produced by the selected BSP." | tee -a "$log_file" >&2
        find "$bsp_dir" -maxdepth 3 -type f | sort | tee -a "$log_file" >&2
        exit 1
    }

    if command -v arm-none-eabi-size >/dev/null 2>&1; then
        run_logged arm-none-eabi-size "$elf_file"
    else
        run_logged size "$elf_file"
    fi

    if ! find "$package_dst" -type f -name '*.o' -print -quit | grep -q .; then
        echo "Package object files were not found under the staged package directory." | tee -a "$log_file" >&2
        find "$package_dst" -type f | sort | tee -a "$log_file" >&2 || true
        exit 1
    fi

    verify_at24cxx_strict_build_outputs
}
