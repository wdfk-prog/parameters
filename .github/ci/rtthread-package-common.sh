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

apply_ci_config() {
    local config_file="$bsp_dir/.config"
    local rtconfig="$bsp_dir/rtconfig.h"

    test -f "$config_file" || {
        echo ".config was not generated: $config_file" | tee -a "$log_file" >&2
        exit 1
    }

    autogen_pm_ci_update_config_file "$config_file"
    run_shell_logged scons -C "$bsp_dir" --pyconfig-silent

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
