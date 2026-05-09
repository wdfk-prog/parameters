#!/usr/bin/env bash
set -euo pipefail

script_dir="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=.github/ci/autogen-pm-ci-profile.sh
. "$script_dir/autogen-pm-ci-profile.sh"
# shellcheck source=.github/ci/host-test-common.sh
. "$script_dir/host-test-common.sh"
# shellcheck source=.github/ci/host-test-nvm-matrix.sh
. "$script_dir/host-test-nvm-matrix.sh"
# shellcheck source=.github/ci/host-test-targets.sh
. "$script_dir/host-test-targets.sh"

run_host_targets "$@"
