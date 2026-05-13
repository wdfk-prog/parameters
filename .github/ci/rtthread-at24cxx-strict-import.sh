#!/usr/bin/env bash
# Strict real AT24CXX package import helpers for RT-Thread package CI.

at24cxx_strict_import_enabled() {
    [ "$profile" = "rtt-at24cxx-backend" ] && \
        [ "${AUTOGEN_PM_CI_IMPORT_EXTERNAL_PACKAGES:-0}" = "1" ]
}

resolve_at24cxx_package_ref() {
    local resolved_ref

    case "$at24cxx_ref" in
        ""|default|HEAD)
            resolved_ref="$(git ls-remote --symref "$at24cxx_repo" HEAD | \
                awk '$1 == "ref:" { sub("refs/heads/", "", $2); print $2; exit }')"
            if [ -z "$resolved_ref" ]; then
                echo "Unable to resolve AT24CXX default branch from $at24cxx_repo" | tee -a "$log_file" >&2
                exit 1
            fi
            printf '%s\n' "$resolved_ref"
            ;;
        *)
            printf '%s\n' "$at24cxx_ref"
            ;;
    esac
}

download_at24cxx_package() {
    local resolved_ref
    local resolved_sha

    if ! at24cxx_strict_import_enabled; then
        return 0
    fi

    resolved_ref="$(resolve_at24cxx_package_ref)"
    rm -rf "$at24cxx_package_dir"
    run_logged git clone --depth 1 --branch "$resolved_ref" \
        "$at24cxx_repo" "$at24cxx_package_dir"
    resolved_sha="$(git -C "$at24cxx_package_dir" rev-parse HEAD)"

    test -f "$at24cxx_package_dir/at24cxx.h" || {
        echo "Downloaded AT24CXX package did not provide at24cxx.h: $at24cxx_package_dir" | tee -a "$log_file" >&2
        exit 1
    }
    test -f "$at24cxx_package_dir/at24cxx.c" || {
        echo "Downloaded AT24CXX package did not provide at24cxx.c: $at24cxx_package_dir" | tee -a "$log_file" >&2
        exit 1
    }
    test -f "$at24cxx_package_dir/SConscript" || {
        echo "Downloaded AT24CXX package did not provide SConscript: $at24cxx_package_dir" | tee -a "$log_file" >&2
        exit 1
    }

    echo "AT24CXX_PACKAGE_DOWNLOAD_OK repo=$at24cxx_repo ref=$at24cxx_ref resolved_ref=$resolved_ref sha=$resolved_sha dir=$at24cxx_package_dir" | tee -a "$log_file"
}

inject_at24cxx_package_sconstruct() {
    if ! at24cxx_strict_import_enabled; then
        return 0
    fi

    python3 - "$bsp_dir/SConstruct" <<'PY'
from pathlib import Path
import sys

sconstruct = Path(sys.argv[1])
text = sconstruct.read_text(encoding="utf-8")
line = "objs += SConscript('packages/at24cxx/SConscript')"
if line not in text:
    marker = "DoBuilding(TARGET, objs)"
    if marker not in text:
        raise SystemExit(f"DoBuilding marker not found in {sconstruct}")
    text = text.replace(marker, f"# CI-only real AT24CXX package injection.\n{line}\n\n{marker}", 1)
    sconstruct.write_text(text, encoding="utf-8")
PY
}

add_at24cxx_package_cpppath() {
    local at24_header="$1"
    local at24_include_dir

    if ! at24cxx_strict_import_enabled; then
        return 0
    fi

    at24_include_dir="$(CDPATH= cd -- "$(dirname -- "$at24_header")" && pwd -P)"

    python3 - "$package_dst/SConscript" "$at24_include_dir" <<'PY'
from pathlib import Path
import sys

sconscript = Path(sys.argv[1])
include_dir = sys.argv[2]
text = sconscript.read_text(encoding="utf-8")
external_line = f"    {include_dir!r},"
backend_line = "    cwd + '/backend',"
if external_line not in text:
    if backend_line not in text:
        raise SystemExit(f"Backend include marker not found in {sconscript}")
    text = text.replace(backend_line, f"{external_line}\n{backend_line}", 1)
    sconscript.write_text(text, encoding="utf-8")
PY

    echo "AT24CXX_PACKAGE_CPPPATH_OK dir=$at24_include_dir" | tee -a "$log_file"
}

# Emit -I arguments for include directories that exist in this RT-Thread tree.
autogen_pm_ci_collect_existing_include_dirs() {
    local dir

    for dir in "$@"; do
        if [ -d "$dir" ]; then
            printf -- '-I%s\n' "$dir"
        fi
    done
}

# Emit -I arguments for directories that contain a required compatibility header.
autogen_pm_ci_collect_header_include_dirs() {
    local root_dir="$1"
    local header_name="$2"
    local dir

    find "$root_dir" -type f -name "$header_name" -exec dirname {} \; | sort -u | \
        while IFS= read -r dir; do
            printf -- '-I%s\n' "$dir"
        done
}

verify_at24cxx_include_resolution() {
    local at24_header="$1"
    local at24_include_dir
    local backend_dir
    local expected_header
    local include_trace
    local source_file
    local resolved_header
    local toolchain_cc="$RTT_EXEC_PATH/arm-none-eabi-gcc"
    local include_args=()

    at24_include_dir="$(CDPATH= cd -- "$(dirname -- "$at24_header")" && pwd -P)"
    backend_dir="$(CDPATH= cd -- "$package_dst/backend" && pwd -P)"
    expected_header="$at24_include_dir/$(basename -- "$at24_header")"
    include_trace="$log_dir/at24cxx-include-resolution-${profile}.log"
    source_file="$package_dst/backend/par_store_backend_rtt_at24cxx.c"

    test -f "$source_file" || {
        echo "AT24CXX backend source does not exist: $source_file" | tee -a "$log_file" >&2
        exit 1
    }

    mapfile -t include_args < <(
        autogen_pm_ci_collect_existing_include_dirs \
            "$at24_include_dir" \
            "$bsp_dir" \
            "$bsp_dir/board" \
            "$bsp_dir/drivers" \
            "$rtt_root/include" \
            "$rtt_root/libcpu/arm/cortex-a" \
            "$rtt_root/components/mm" \
            "$rtt_root/components/drivers/include" \
            "$rtt_root/components/finsh" \
            "$rtt_root/components/libc/compilers/common/include" \
            "$package_dst" \
            "$package_dst/backend" \
            "$package_dst/port" \
            "$package_dst/parameters/include" \
            "$package_dst/parameters/src" \
            "$package_dst/parameters/src/def" \
            "$package_dst/parameters/src/detail" \
            "$package_dst/parameters/src/layout" \
            "$package_dst/parameters/src/nvm" \
            "$package_dst/parameters/src/nvm/backend" \
            "$package_dst/parameters/src/nvm/object" \
            "$package_dst/parameters/src/nvm/object/addr" \
            "$package_dst/parameters/src/nvm/object/store" \
            "$package_dst/parameters/src/nvm/scalar" \
            "$package_dst/parameters/src/nvm/scalar/layout" \
            "$package_dst/parameters/src/nvm/scalar/store" \
            "$package_dst/parameters/src/object" \
            "$package_dst/parameters/src/port" \
            "$package_dst/parameters/src/scalar" \
            "$package_dst/parameters/generated"
        autogen_pm_ci_collect_header_include_dirs "$rtt_root" rtlegacy.h
        autogen_pm_ci_collect_header_include_dirs "$rtt_root" finsh.h
        autogen_pm_ci_collect_header_include_dirs "$rtt_root" avl.h
    )

    "$toolchain_cc" -E -H \
        "${include_args[@]}" \
        "$source_file" >/dev/null 2> "$include_trace" || {
        echo "AT24CXX backend include-resolution probe failed. See $include_trace" | tee -a "$log_file" >&2
        cat "$include_trace" | tee -a "$log_file" >&2
        exit 1
    }

    if ! resolved_header="$(python3 - "$include_trace" "$expected_header" "$backend_dir" <<'PY'
from pathlib import Path
import re
import sys

trace = Path(sys.argv[1])
expected = str(Path(sys.argv[2]).resolve())
stub_dir = Path(sys.argv[3]).resolve()
headers = []

for line in trace.read_text(encoding="utf-8", errors="replace").splitlines():
    match = re.match(r"^\.+\s+(.*/at24cxx\.h)$", line)
    if match:
        headers.append(str(Path(match.group(1)).resolve()))

if not headers:
    raise SystemExit("AT24CXX backend source did not include at24cxx.h")

for header in headers:
    try:
        Path(header).relative_to(stub_dir)
    except ValueError:
        continue
    raise SystemExit(f"AT24CXX backend source resolved package-local stub: {header}")

if headers[0] != expected:
    raise SystemExit(
        "AT24CXX backend source resolved unexpected first header: "
        f"{headers[0]} (expected {expected})")

print(headers[0])
PY
)"; then
        echo "AT24CXX backend include-resolution validation failed. See $include_trace" | tee -a "$log_file" >&2
        cat "$include_trace" | tee -a "$log_file" >&2
        exit 1
    fi

    echo "AT24CXX_INCLUDE_RESOLUTION_OK source=$source_file header=$resolved_header" | tee -a "$log_file"
}

verify_at24cxx_package_import() {
    local at24_header

    if [ "$profile" != "rtt-at24cxx-backend" ]; then
        return 0
    fi

    grep -Eq '^#define[[:space:]]+PKG_USING_AT24CXX([[:space:]]|$)' "$bsp_dir/rtconfig.h" || {
        echo "rtt-at24cxx-backend did not select PKG_USING_AT24CXX in rtconfig.h" | tee -a "$log_file" >&2
        exit 1
    }

    if ! at24cxx_strict_import_enabled; then
        echo "AT24CXX_PACKAGE_IMPORT_SKIPPED strict import disabled; CI uses backend header stubs for compile smoke" | tee -a "$log_file"
        return 0
    fi

    download_at24cxx_package
    inject_at24cxx_package_sconstruct
    at24_header="$at24cxx_package_dir/at24cxx.h"
    add_at24cxx_package_cpppath "$at24_header"
    verify_at24cxx_include_resolution "$at24_header"

    echo "AT24CXX_PACKAGE_IMPORT_OK header=$at24_header" | tee -a "$log_file"
}

verify_at24cxx_strict_build_outputs() {
    local at24cxx_object

    if ! at24cxx_strict_import_enabled; then
        return 0
    fi

    if find "$bsp_dir" -type f -name 'par_backend_adapter_ci_stubs.o' -print -quit | grep -q .; then
        echo "Strict AT24CXX import linked CI backend weak stubs." | tee -a "$log_file" >&2
        find "$bsp_dir" -type f -name 'par_backend_adapter_ci_stubs.o' | sort | tee -a "$log_file" >&2
        exit 1
    fi

    at24cxx_object="$(find "$bsp_dir" -type f -path '*/packages/at24cxx/at24cxx.o' -print -quit)"
    if [ -z "$at24cxx_object" ]; then
        echo "Strict AT24CXX import did not build the real AT24CXX package object." | tee -a "$log_file" >&2
        find "$bsp_dir" -type f -name '*.o' | sort | tee -a "$log_file" >&2 || true
        exit 1
    fi

    echo "AT24CXX_STRICT_BUILD_OUTPUTS_OK object=$at24cxx_object" | tee -a "$log_file"
}
