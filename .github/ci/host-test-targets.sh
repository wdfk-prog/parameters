#!/usr/bin/env bash
# Host-test target dispatcher.
#
# Keep this list focused on product source, generated-source, and source-tool
# behavior. CI harness self-tests are intentionally out of scope for these
# host-test targets.

host_targets=(
    par_core_runtime
    par_object_runtime
    par_mutex_runtime
    par_shell_tool
    par_shell_feature_gate
    par_shell_feature_gate_no_get
    par_shell_feature_gate_no_id
    par_shell_feature_gate_no_save_json
    par_nvm_flash_ee
    par_nvm_flash_ee_write_verify
    par_nvm_flash_ee_schema_evolution
    par_nvm_flash_ee_object_write_verify
    par_nvm_flash_ee_object_only
    par_nvm_flash_ee_object_array_nvm
    par_nvm_flash_ee_fixed_object_addr_overflow
    par_nvm_flash_ee_fixed_object_region_too_small
    par_nvm_flash_ee_fixed_object_overlap
    par_nvm_flash_ee_matrix
    par_nvm_feature_matrix
    par_config_feature_matrix
    par_backend_adapter_smoke
    par_generated_runtime_consistency
    par_generated_runtime_condition_disabled
    par_generated_runtime_scalar_only
)

current_policy_host_targets=(
    par_mutex_runtime
    par_shell_tool
    par_nvm_flash_ee
    par_nvm_flash_ee_schema_current_policy
    par_backend_adapter_smoke
)

run_host_targets_parallel() {
    local target
    local pid
    local max_parallel="${AUTOGEN_PM_HOST_TEST_PARALLEL_JOBS:-4}"
    local failed=0
    local pids=()
    local targets=()

    case "$max_parallel" in
        ''|*[!0-9]*|0)
            echo "AUTOGEN_PM_HOST_TEST_PARALLEL_JOBS must be a positive integer: $max_parallel" >&2
            return 1
            ;;
    esac

    for target in "$@"; do
        targets+=("$target")
        (
            run_host_target "$target"
        ) > "$build_dir/$target.log" 2>&1 &
        pids+=("$!")

        if [ "${#pids[@]}" -ge "$max_parallel" ]; then
            run_host_targets_wait_batch
        fi
    done

    if [ "${#pids[@]}" -gt 0 ]; then
        run_host_targets_wait_batch
    fi

    return "$failed"
}

run_host_targets_wait_batch() {
    local pid
    local target

    for pid in "${pids[@]}"; do
        if ! wait "$pid"; then
            failed=1
        fi
    done

    for target in "${targets[@]}"; do
        cat "$build_dir/$target.log"
    done

    pids=()
    targets=()
}

run_all_host_targets() {
    local targets=("${host_targets[@]}")

    if [ "${PAR_HOST_TEST_GROUP:-mandatory}" = "current-policy" ] || \
       [ "${PAR_HOST_TEST_GROUP:-mandatory}" = "current_policy" ]; then
        targets=("${current_policy_host_targets[@]}")
    elif [ "${PAR_HOST_TEST_GROUP:-mandatory}" = "all" ]; then
        targets+=(par_nvm_flash_ee_schema_current_policy)
    fi

    if [ "${AUTOGEN_PM_HOST_TEST_PARALLEL:-1}" = "0" ]; then
        for target in "${targets[@]}"; do
            run_host_target "$target"
        done
    else
        run_host_targets_parallel "${targets[@]}"
    fi
}

run_host_basic_target() {
    local target="$1"

    case "$target" in
        par_core_runtime)
            compile_and_run par_core_runtime \
                parameters/tests/host/test_par_core_runtime.c \
                "${base_sources[@]}"
            ;;
        par_object_runtime)
            compile_and_run par_object_runtime \
                parameters/tests/host/test_par_object_runtime.c \
                "${base_sources[@]}"
            ;;
        par_mutex_runtime)
            compile_and_run par_mutex_runtime \
                -DPAR_CFG_MUTEX_EN=1 \
                parameters/tests/host/test_par_mutex_runtime.c \
                "${base_sources[@]}"
            ;;
        par_shell_tool)
            compile_and_run par_shell_tool \
                parameters/tests/host/test_par_shell_tool.c \
                "${base_sources[@]}"
            ;;
        *)
            return 2
            ;;
    esac
}

run_host_shell_gate_target() {
    local target="$1"

    case "$target" in
        par_shell_feature_gate)
            compile_and_run par_shell_feature_gate \
                -UAUTOGEN_PM_MSH_CMD_INFO \
                -UAUTOGEN_PM_MSH_CMD_GET_OBJECT \
                -UAUTOGEN_PM_MSH_CMD_SET \
                -UAUTOGEN_PM_MSH_CMD_DEF \
                -UAUTOGEN_PM_MSH_CMD_DEF_ALL \
                -UAUTOGEN_PM_MSH_CMD_SAVE \
                -UAUTOGEN_PM_MSH_CMD_SAVE_CLEAN \
                -UAUTOGEN_PM_MSH_CMD_JSON \
                parameters/tests/host/test_par_shell_feature_gate.c \
                "${base_sources[@]}"
            ;;
        par_shell_feature_gate_no_get)
            compile_and_run par_shell_feature_gate_no_get \
                -DPAR_HOST_TEST_SHELL_NO_GET \
                -Wno-unused-function \
                -UAUTOGEN_PM_MSH_CMD_INFO \
                -UAUTOGEN_PM_MSH_CMD_GET \
                -UAUTOGEN_PM_MSH_CMD_GET_OBJECT \
                -UAUTOGEN_PM_MSH_CMD_SET \
                -UAUTOGEN_PM_MSH_CMD_DEF \
                -UAUTOGEN_PM_MSH_CMD_DEF_ALL \
                -UAUTOGEN_PM_MSH_CMD_SAVE \
                -UAUTOGEN_PM_MSH_CMD_SAVE_CLEAN \
                -UAUTOGEN_PM_MSH_CMD_JSON \
                parameters/tests/host/test_par_shell_feature_gate.c \
                "${base_sources[@]}"
            ;;
        par_shell_feature_gate_no_id)
            compile_and_run par_shell_feature_gate_no_id \
                -DPAR_CFG_ENABLE_ID=0 \
                -DPAR_CFG_TABLE_ID_CHECK_EN=0 \
                parameters/tests/host/test_par_shell_feature_gate.c \
                "${base_sources[@]}"
            ;;
        par_shell_feature_gate_no_save_json)
            compile_and_run par_shell_feature_gate_no_save_json \
                -DPAR_HOST_TEST_SHELL_NO_SAVE_JSON \
                -UAUTOGEN_PM_MSH_CMD_SAVE \
                -UAUTOGEN_PM_MSH_CMD_SAVE_CLEAN \
                -UAUTOGEN_PM_MSH_CMD_JSON \
                parameters/tests/host/test_par_shell_feature_gate.c \
                "${base_sources[@]}"
            ;;
        *)
            return 2
            ;;
    esac
}

run_host_nvm_target() {
    local target="$1"

    case "$target" in
        par_nvm_flash_ee)
            run_nvm_flash_ee_profile default \
                PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE \
                PAR_CFG_NVM_OBJECT_STORE_SHARED \
                PAR_CFG_NVM_OBJECT_ADDR_FIXED \
                0xC0U 0x40U \
                parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_with_size.c
            ;;
        par_nvm_flash_ee_write_verify)
            run_nvm_flash_ee_write_verify_profile
            ;;
        par_nvm_flash_ee_schema_evolution)
            run_nvm_flash_ee_schema_evolution_profile
            ;;
        par_nvm_flash_ee_schema_current_policy)
            run_nvm_flash_ee_schema_current_policy_profile
            ;;
        par_nvm_flash_ee_object_write_verify)
            run_nvm_flash_ee_object_write_verify_profile
            ;;
        par_nvm_flash_ee_object_only)
            run_nvm_flash_ee_object_only_profile
            ;;
        par_nvm_flash_ee_object_array_nvm)
            run_nvm_flash_ee_object_array_nvm_profile
            ;;
        par_nvm_flash_ee_fixed_object_overlap)
            run_nvm_flash_ee_fixed_object_invalid_profile fixed_object_overlap 0x20U 0x40U
            ;;
        par_nvm_flash_ee_fixed_object_region_too_small)
            run_nvm_flash_ee_fixed_object_invalid_profile fixed_object_region_too_small 0xC0U 1U
            ;;
        par_nvm_flash_ee_fixed_object_addr_overflow)
            run_nvm_flash_ee_fixed_object_invalid_profile fixed_object_addr_overflow 0xFFFFFFFEUL 0U
            ;;
        par_nvm_flash_ee_matrix)
            run_nvm_flash_ee_matrix
            ;;
        *)
            return 2
            ;;
    esac
}

run_host_ci_target() {
    local target="$1"

    case "$target" in
        par_nvm_feature_matrix)
            bash "$script_dir/nvm-feature-matrix.sh"
            ;;
        par_config_feature_matrix)
            bash "$script_dir/config-feature-matrix.sh"
            ;;
        par_backend_adapter_smoke)
            bash "$script_dir/backend-adapter-smoke.sh"
            ;;
        *)
            return 2
            ;;
    esac
}

run_host_target() {
    local target="$1"

    case "$target" in
        par_core_runtime|par_object_runtime|par_mutex_runtime|par_shell_tool)
            run_host_basic_target "$target"
            ;;
        par_shell_feature_gate|par_shell_feature_gate_no_get)
            run_host_shell_gate_target "$target"
            ;;
        par_shell_feature_gate_no_id|par_shell_feature_gate_no_save_json)
            run_host_shell_gate_target "$target"
            ;;
        par_nvm_flash_ee|par_nvm_flash_ee_write_verify)
            run_host_nvm_target "$target"
            ;;
        par_nvm_flash_ee_schema_evolution|par_nvm_flash_ee_schema_current_policy)
            run_host_nvm_target "$target"
            ;;
        par_nvm_flash_ee_object_write_verify|par_nvm_flash_ee_object_only)
            run_host_nvm_target "$target"
            ;;
        par_nvm_flash_ee_object_array_nvm|par_nvm_flash_ee_fixed_object_overlap)
            run_host_nvm_target "$target"
            ;;
        par_nvm_flash_ee_fixed_object_region_too_small)
            run_host_nvm_target "$target"
            ;;
        par_nvm_flash_ee_fixed_object_addr_overflow|par_nvm_flash_ee_matrix)
            run_host_nvm_target "$target"
            ;;
        par_nvm_feature_matrix|par_config_feature_matrix|par_backend_adapter_smoke)
            run_host_ci_target "$target"
            ;;
        par_generated_runtime_consistency|par_generated_runtime_condition_disabled)
            run_host_generated_target "$target"
            ;;
        par_generated_runtime_scalar_only)
            run_host_generated_target "$target"
            ;;
        *)
            echo "Unknown host test target: $target" >&2
            echo "Known targets: ${host_targets[*]}" >&2
            exit 1
            ;;
    esac
}

run_host_targets() {
    local target

    if [ "$#" -eq 0 ]; then
        set -- all
    fi

    for target in "$@"; do
        if [ "$target" = "all" ]; then
            run_all_host_targets
        else
            run_host_target "$target"
        fi
    done
}
