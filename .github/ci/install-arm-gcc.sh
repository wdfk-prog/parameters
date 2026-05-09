#!/usr/bin/env bash
set -euo pipefail

arm_gcc_dir="${ARM_GCC_DIR:-/opt/gcc-arm-none-eabi}"
arm_gcc_version="${ARM_GCC_VERSION:-13.3.rel1}"
arm_gcc_sha256="${ARM_GCC_SHA256:-95c011cee430e64dd6087c75c800f04b9c49832cc1000127a92a97f9c8d83af4}"
archive_name="arm-gnu-toolchain-${arm_gcc_version}-x86_64-arm-none-eabi.tar.xz"
archive_url="${ARM_GCC_URL:-https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/${archive_name}}"
tmp_dir=""

autogen_pm_ci_validate_arm_gcc_dir() {
    while [ "$arm_gcc_dir" != "/" ] && [ "${arm_gcc_dir%/}" != "$arm_gcc_dir" ]; do
        arm_gcc_dir="${arm_gcc_dir%/}"
    done

    case "$arm_gcc_dir" in
        ""|"/"|"/opt"|"/usr"|"/usr/local"|"/home"|"/tmp"|"/var"|"/etc"|"/bin"|"/sbin"|"/lib"|"/lib64")
            echo "Refusing unsafe ARM_GCC_DIR: $arm_gcc_dir" >&2
            exit 1
            ;;
        */../*|*/..|*/./*|*/.)
            echo "ARM_GCC_DIR must not contain dot path components: $arm_gcc_dir" >&2
            exit 1
            ;;
        /*)
            ;;
        *)
            echo "ARM_GCC_DIR must be an absolute path: $arm_gcc_dir" >&2
            exit 1
            ;;
    esac
}

autogen_pm_ci_validate_arm_gcc_dir

test -x "$arm_gcc_dir/bin/arm-none-eabi-gcc" && {
    "$arm_gcc_dir/bin/arm-none-eabi-gcc" --version
    exit 0
}

cleanup() {
    if [ -n "$tmp_dir" ]; then
        rm -rf "$tmp_dir"
    fi
}
trap cleanup EXIT

tmp_dir="$(mktemp -d "${TMPDIR:-/tmp}/autogen-pm-arm-gcc.XXXXXX")"
archive_path="$tmp_dir/$archive_name"

mkdir -p "$(dirname "$arm_gcc_dir")"

curl \
    --fail \
    --location \
    --retry 5 \
    --retry-all-errors \
    --connect-timeout 30 \
    --max-time 1800 \
    --output "$archive_path" \
    "$archive_url"

printf '%s  %s\n' "$arm_gcc_sha256" "$archive_path" | sha256sum -c -

tar -xJf "$archive_path" -C "$tmp_dir"
rm -rf "$arm_gcc_dir"
mv "$tmp_dir/arm-gnu-toolchain-${arm_gcc_version}-x86_64-arm-none-eabi" "$arm_gcc_dir"

test -x "$arm_gcc_dir/bin/arm-none-eabi-gcc"
"$arm_gcc_dir/bin/arm-none-eabi-gcc" --version
