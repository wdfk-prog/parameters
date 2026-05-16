#!/usr/bin/env bash
set -euo pipefail

workspace="${GITHUB_WORKSPACE:-$(pwd)}"
package_name="${PACKAGE_NAME:-autogen_parameter_manager}"
rtthread_ref="${RTTHREAD_REF:-default}"
bsp_path="${BSP_PATH:-bsp/qemu-vexpress-a9}"
ci_root="${AUTOGEN_PM_CI_ROOT:-$workspace/_ci}"
rtt_root="$ci_root/rt-thread"
bsp_dir="$rtt_root/$bsp_path"
package_dst="$bsp_dir/packages/$package_name"
log_dir="${AUTOGEN_PM_CI_LOG_DIR:-$ci_root/logs}"
rtthread_source_tar="${RTTHREAD_SOURCE_TAR:-}"
rtthread_source_sha_file="${RTTHREAD_SOURCE_SHA_FILE:-}"
profile="${AUTOGEN_PM_CI_PROFILE:-scalar-fixed-slot-with-size}"
profile_log_name="${AUTOGEN_PM_CI_PROFILE_LOG_NAME:-$profile}"
profile_log_name="${profile_log_name//[^A-Za-z0-9_.-]/_}"
at24cxx_repo="${AUTOGEN_PM_CI_AT24CXX_REPO:-https://github.com/XiaojieFan/at24cxx.git}"
at24cxx_ref="${AUTOGEN_PM_CI_AT24CXX_REF:-default}"
at24cxx_package_dir="$bsp_dir/packages/at24cxx"
log_file="$log_dir/rtthread-package-compile-${profile_log_name}.log"
ci_script_dir="$workspace/.github/ci"
profile_script="$ci_script_dir/autogen-pm-ci-profile.sh"

# shellcheck source=.github/ci/autogen-pm-ci-profile.sh
. "$profile_script"

mkdir -p "$log_dir"
: > "$log_file"

run_logged() {
    echo "+ $*" | tee -a "$log_file"
    "$@" 2>&1 | tee -a "$log_file"
}

run_shell_logged() {
    echo "+ $*" | tee -a "$log_file"
    "$@" 2>&1 | tee -a "$log_file"
}

# shellcheck source=.github/ci/rtthread-package-common.sh
. "$ci_script_dir/rtthread-package-common.sh"
# shellcheck source=.github/ci/rtthread-package-stage.sh
. "$ci_script_dir/rtthread-package-stage.sh"
# shellcheck source=.github/ci/rtthread-at24cxx-strict-import.sh
. "$ci_script_dir/rtthread-at24cxx-strict-import.sh"
# shellcheck source=.github/ci/rtthread-ci-stubs.sh
. "$ci_script_dir/rtthread-ci-stubs.sh"

validate_bsp_path
prepare_rtthread
configure_toolchain
stage_package
inject_ci_flash_ee_native_stub
inject_ci_backend_adapter_stubs
inject_package_kconfig
cd "$bsp_dir"
run_rtthread_pyconfig_logged scons --pyconfig-silent
inject_package_sconstruct
verify_backend_adapter_stub_placement
apply_ci_config
verify_at24cxx_package_import
run_shell_logged grep -E '^(#define[[:space:]]+PKG_USING_AUTOGEN_PARAMETER_MANAGER|#define[[:space:]]+AUTOGEN_PM_|#define[[:space:]]+RT_USING_(MUTEX|FINSH|HEAP))' rtconfig.h
run_shell_logged scons -j"${AUTOGEN_PM_CI_SCONS_JOBS:-$(nproc)}"
verify_build_outputs
