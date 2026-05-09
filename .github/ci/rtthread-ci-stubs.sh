#!/usr/bin/env bash
# Helpers for CI-only package backend compile stubs.

inject_ci_flash_ee_native_stub() {
    local stub_src="$workspace/.github/ci/test/par_store_backend_flash_ee_native_ci.c"
    local stub_dst="$package_dst/backend/par_store_backend_flash_ee_native_ci.c"

    test -f "$stub_src" || {
        echo "CI native flash-ee stub does not exist: $stub_src" | tee -a "$log_file" >&2
        exit 1
    }

    cp "$stub_src" "$stub_dst"

    python3 - "$package_dst/SConscript" <<'PY'
from pathlib import Path
import sys

sconscript = Path(sys.argv[1])
text = sconscript.read_text(encoding="utf-8")
native_line = "            backend/par_store_backend_flash_ee_native.c"
stub_line = "            backend/par_store_backend_flash_ee_native_ci.c"
if stub_line not in text:
    if native_line not in text:
        raise SystemExit(f"Native flash-ee source marker not found in {sconscript}")
    text = text.replace(native_line, f"{native_line}\n{stub_line}", 1)
    sconscript.write_text(text, encoding="utf-8")
PY
}

inject_ci_backend_adapter_stubs() {
    local stub_src="$workspace/.github/ci/test/par_backend_adapter_ci_stubs.c"
    local fal_header_src="$workspace/.github/ci/test/fal.h"
    local fal_cfg_src="$workspace/.github/ci/test/fal_cfg.h"
    local at24_header_src="$workspace/.github/ci/test/at24cxx.h"
    local stub_dst="$package_dst/backend/par_backend_adapter_ci_stubs.c"

    case "$profile" in
        flash-ee-fal-backend|rtt-at24cxx-backend)
            ;;
        *)
            return 0
            ;;
    esac

    if at24cxx_strict_import_enabled; then
        rm -f "$stub_dst" "$package_dst/backend/at24cxx.h"
        echo "AT24CXX_STRICT_IMPORT_NO_BACKEND_STUB path=$package_dst/backend/at24cxx.h" | tee -a "$log_file"
        return 0
    fi

    test -f "$stub_src" || {
        echo "CI backend adapter stub does not exist: $stub_src" | tee -a "$log_file" >&2
        exit 1
    }
    test -f "$fal_header_src" || {
        echo "CI FAL header stub does not exist: $fal_header_src" | tee -a "$log_file" >&2
        exit 1
    }
    test -f "$at24_header_src" || {
        echo "CI AT24CXX header stub does not exist: $at24_header_src" | tee -a "$log_file" >&2
        exit 1
    }
    test -f "$fal_cfg_src" || {
        echo "CI FAL config stub does not exist: $fal_cfg_src" | tee -a "$log_file" >&2
        exit 1
    }

    cp "$stub_src" "$stub_dst"
    cp "$fal_header_src" "$package_dst/backend/fal.h"
    cp "$fal_cfg_src" "$package_dst/backend/fal_cfg.h"
    mkdir -p "$bsp_dir/board"
    cp "$fal_cfg_src" "$bsp_dir/board/fal_cfg.h"
    cp "$at24_header_src" "$package_dst/backend/at24cxx.h"

    python3 - "$package_dst/SConscript" "$profile" <<'PY'
from pathlib import Path
import sys

sconscript = Path(sys.argv[1])
profile = sys.argv[2]
text = sconscript.read_text(encoding="utf-8")
stub_line = "            backend/par_backend_adapter_ci_stubs.c"

if profile == "flash-ee-fal-backend":
    marker = "            backend/par_store_backend_flash_ee_fal.c"
elif profile == "rtt-at24cxx-backend":
    marker = "        backend/par_store_backend_rtt_at24cxx.c"
else:
    raise SystemExit(f"Unsupported backend adapter profile: {profile}")

if marker not in text:
    raise SystemExit(f"Backend source marker not found for {profile}: {marker}")

if stub_line not in text:
    text = text.replace(marker, f"{marker}\n{stub_line}", 1)
    sconscript.write_text(text, encoding="utf-8")
else:
    marker_index = text.find(marker)
    stub_index = text.find(stub_line)
    if marker_index < 0 or stub_index < marker_index:
        raise SystemExit(f"Backend CI stub is not attached to the {profile} source block")
PY
}

verify_backend_adapter_stub_placement() {
    if at24cxx_strict_import_enabled; then
        test ! -f "$package_dst/backend/at24cxx.h" || {
            echo "Strict AT24CXX import must not stage package backend/at24cxx.h" | tee -a "$log_file" >&2
            exit 1
        }
        python3 - "$package_dst/SConscript" <<'PY'
from pathlib import Path
import sys

sconscript = Path(sys.argv[1])
text = sconscript.read_text(encoding="utf-8")
stub_line = "backend/par_backend_adapter_ci_stubs.c"
if stub_line in text:
    raise SystemExit("Strict AT24CXX import must not inject backend adapter weak stubs")
PY
        return 0
    fi

    python3 - "$package_dst/SConscript" "$profile" <<'PY'
from pathlib import Path
import sys

sconscript = Path(sys.argv[1])
profile = sys.argv[2]
text = sconscript.read_text(encoding="utf-8")
stub_line = "            backend/par_backend_adapter_ci_stubs.c"
markers = {
    "flash-ee-fal-backend": "            backend/par_store_backend_flash_ee_fal.c",
    "rtt-at24cxx-backend": "        backend/par_store_backend_rtt_at24cxx.c",
}
marker = markers.get(profile)
if marker is None:
    raise SystemExit(0)
if marker not in text:
    raise SystemExit(f"Backend source marker not found for {profile}: {marker}")
if stub_line not in text:
    raise SystemExit(f"Backend CI stub was not injected for {profile}")
marker_index = text.find(marker)
stub_index = text.find(stub_line)
if stub_index < marker_index:
    raise SystemExit(f"Backend CI stub appears before the selected {profile} source marker")
between = text[marker_index:stub_index]
for other_profile, other_marker in markers.items():
    if other_profile != profile and other_marker in between:
        raise SystemExit(f"Backend CI stub is attached to the wrong source block for {profile}")
PY
}
