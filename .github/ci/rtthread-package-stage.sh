#!/usr/bin/env bash
# Helpers for staging the local package into an RT-Thread BSP build graph.

stage_package() {
    validate_bsp_dir
    : "${workspace:?workspace is required}"
    : "${package_name:?package_name is required}"
    : "${package_dst:?package_dst is required}"
    if [ "$package_dst" = "/" ] || [ "$package_dst" = "." ]; then
        echo "Refusing to remove unsafe package_dst: $package_dst" | tee -a "$log_file" >&2
        exit 1
    fi

    rm -rf "$package_dst"
    mkdir -p "$package_dst"
    tar \
        --exclude='./.git' \
        --exclude='./_ci' \
        --exclude='./build' \
        --exclude='./.sconsign.dblite' \
        -C "$workspace" -cf - . | tar -C "$package_dst" -xf -

    test -f "$package_dst/SConscript"
    test -f "$package_dst/Kconfig"
}

inject_package_kconfig() {
    test -f "$bsp_dir/Kconfig" || {
        echo "BSP Kconfig does not exist: $bsp_dir/Kconfig" | tee -a "$log_file" >&2
        exit 1
    }

    python3 - "$bsp_dir/Kconfig" "$package_name" <<'PY'
from pathlib import Path
import sys

kconfig = Path(sys.argv[1])
package_name = sys.argv[2]
text = kconfig.read_text(encoding="utf-8")
entry = f'source "packages/{package_name}/Kconfig"'
if entry not in text:
    if not text.endswith("\n"):
        text += "\n"
    text += f"\n# CI-only local package configuration entry.\n{entry}\n"
    kconfig.write_text(text, encoding="utf-8")
PY
}

inject_package_sconstruct() {
    python3 - "$bsp_dir/SConstruct" "$package_name" <<'PY'
from pathlib import Path
import sys

sconstruct = Path(sys.argv[1])
package_name = sys.argv[2]
text = sconstruct.read_text(encoding="utf-8")
line = f"objs += SConscript('packages/{package_name}/SConscript')"
if line not in text:
    marker = "DoBuilding(TARGET, objs)"
    if marker not in text:
        raise SystemExit(f"DoBuilding marker not found in {sconstruct}")
    text = text.replace(marker, f"# CI-only local package compile injection.\n{line}\n\n{marker}", 1)
    sconstruct.write_text(text, encoding="utf-8")
PY
}
