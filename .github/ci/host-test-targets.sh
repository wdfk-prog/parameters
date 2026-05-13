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
    par_generated_runtime_scalar_only
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
    if [ "${AUTOGEN_PM_HOST_TEST_PARALLEL:-1}" = "0" ]; then
        for target in "${host_targets[@]}"; do
            run_host_target "$target"
        done
    else
        run_host_targets_parallel "${host_targets[@]}"
    fi
}

run_host_target() {
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
        par_nvm_feature_matrix)
            bash "$script_dir/nvm-feature-matrix.sh"
            ;;
        par_config_feature_matrix)
            bash "$script_dir/config-feature-matrix.sh"
            ;;
        par_backend_adapter_smoke)
            bash "$script_dir/backend-adapter-smoke.sh"
            ;;
        par_generated_runtime_consistency)
            rm -rf "$build_dir/generated-runtime"
            mkdir -p "$build_dir/generated-runtime/out"
            python3 - "$build_dir/generated-runtime/par_table.generated-runtime.csv" <<'PYGEN'
from pathlib import Path
import csv
import sys

columns = [
    "group", "section", "condition", "enum", "id", "type", "name", "min", "max",
    "default", "unit", "access", "read_roles", "write_roles", "persistent", "desc", "comment",
]
rows = [
    ["GEN", "Scalar", "", "ePAR_GEN_MODE", "0", "U8", "Gen Mode", "0", "10", "2", "", "RW", "ALL", "ALL", "0", "Generated U8 mode.", ""],
    ["GEN", "Scalar", "", "ePAR_GEN_RATE", "1", "U16", "Gen Rate", "0", "1000", "100", "Hz", "RW", "ALL", "ALL", "0", "Generated U16 rate.", ""],
    ["GEN", "Scalar", "", "ePAR_GEN_FLAGS", "2", "U32", "Gen Flags", "0", "65535", "0x10", "", "RW", "ALL", "ALL", "0", "Generated U32 flags.", ""],
    ["GEN", "Object", "", "ePAR_GEN_NAME", "3", "STR", "Gen Name", "0", "8", "ap", "", "RW", "ALL", "ALL", "0", "Generated string object.", ""],
    ["GEN", "Object", "(1 == PAR_CFG_ENABLE_TYPE_BYTES)", "ePAR_GEN_KEY", "4", "BYTES", "Gen Key", "0", "4", "0x01,0x02,0x03", "", "RW", "ALL", "ALL", "0", "Generated bytes object.", ""],
    ["GEN", "Object", "(1 == PAR_CFG_ENABLE_TYPE_ARR_U16)", "ePAR_GEN_ARR16", "6", "ARR_U16", "Gen Arr16", "2", "2", "10,20", "", "RW", "ALL", "ALL", "0", "Generated U16 array object.", ""],
]

dst = Path(sys.argv[1])
with dst.open("w", newline="", encoding="utf-8") as out_file:
    writer = csv.writer(out_file)
    writer.writerow(columns)
    writer.writerows(rows)
PYGEN
            cp parameters/schema/par_id_lock.json \
                "$build_dir/generated-runtime/par_id_lock.json"
            python3 parameters/tools/pargen.py \
                --csv "$build_dir/generated-runtime/par_table.generated-runtime.csv" \
                --id-lock "$build_dir/generated-runtime/par_id_lock.json" \
                --config parameters/schema/pargen.json \
                --out-def "$build_dir/generated-runtime/par_table.def" \
                --out-dir "$build_dir/generated-runtime/out" \
                --manifest "$build_dir/generated-runtime/par_manifest.json"
            manifest_defines_file="$build_dir/generated-runtime/manifest-defines.txt"
            python3 - "$build_dir/generated-runtime/par_manifest.json" > "$manifest_defines_file" <<'PYMANIFEST'
import json
import sys

manifest = json.load(open(sys.argv[1], encoding="utf-8"))
layout = manifest["layout_max"]
print(f"-DPAR_HOST_TEST_MANIFEST_PARAM_COUNT_MAX={manifest['param_count_max']}U")
print(f"-DPAR_HOST_TEST_MANIFEST_COUNT8={layout['count8']}U")
print(f"-DPAR_HOST_TEST_MANIFEST_COUNT16={layout['count16']}U")
print(f"-DPAR_HOST_TEST_MANIFEST_COUNT32={layout['count32']}U")
print(f"-DPAR_HOST_TEST_MANIFEST_COUNTOBJ={layout['count_obj']}U")
print(f"-DPAR_HOST_TEST_MANIFEST_OBJ_POOL_BYTES={layout['obj_pool_bytes']}UL")
PYMANIFEST
            mapfile -t manifest_defines < "$manifest_defines_file"
            echo "[host-tests] build par_generated_runtime_consistency"
            gcc -I"$build_dir/generated-runtime" \
                -I"$build_dir/generated-runtime/out" \
                "${common_cflags[@]}" \
                "${autogen_pm_ci_defines[@]}" \
                "${manifest_defines[@]}" \
                -DPAR_CFG_LAYOUT_SOURCE=PAR_CFG_LAYOUT_SCRIPT \
                parameters/tests/host/test_par_generated_runtime_consistency.c \
                "$build_dir/generated-runtime/out/par_layout_static.c" \
                "$build_dir/generated-runtime/out/par_generated_info.c" \
                "${base_sources[@]}" \
                -o "$build_dir/par_generated_runtime_consistency"
            echo "[host-tests] run par_generated_runtime_consistency"
            "$build_dir/par_generated_runtime_consistency"
            ;;
        par_generated_runtime_scalar_only)
            rm -rf "$build_dir/generated-runtime-scalar-only"
            mkdir -p "$build_dir/generated-runtime-scalar-only/out"
            python3 - "$build_dir/generated-runtime-scalar-only/par_table.generated-runtime.csv" <<'PYGEN_SCALAR'
from pathlib import Path
import csv
import sys

columns = [
    "group", "section", "condition", "enum", "id", "type", "name", "min", "max",
    "default", "unit", "access", "read_roles", "write_roles", "persistent", "desc", "comment",
]
rows = [
    ["GEN", "Scalar", "", "ePAR_GEN_MODE", "0", "U8", "Gen Mode", "0", "10", "2", "", "RW", "ALL", "ALL", "0", "Generated U8 mode.", ""],
    ["GEN", "Scalar", "", "ePAR_GEN_RATE", "1", "U16", "Gen Rate", "0", "1000", "100", "Hz", "RW", "ALL", "ALL", "0", "Generated U16 rate.", ""],
    ["GEN", "Scalar", "", "ePAR_GEN_FLAGS", "2", "U32", "Gen Flags", "0", "65535", "0x10", "", "RW", "ALL", "ALL", "0", "Generated U32 flags.", ""],
]

dst = Path(sys.argv[1])
with dst.open("w", newline="", encoding="utf-8") as out_file:
    writer = csv.writer(out_file)
    writer.writerow(columns)
    writer.writerows(rows)
PYGEN_SCALAR
            cp parameters/schema/par_id_lock.json \
                "$build_dir/generated-runtime-scalar-only/par_id_lock.json"
            python3 parameters/tools/pargen.py \
                --csv "$build_dir/generated-runtime-scalar-only/par_table.generated-runtime.csv" \
                --id-lock "$build_dir/generated-runtime-scalar-only/par_id_lock.json" \
                --config parameters/schema/pargen.json \
                --out-def "$build_dir/generated-runtime-scalar-only/par_table.def" \
                --out-dir "$build_dir/generated-runtime-scalar-only/out" \
                --manifest "$build_dir/generated-runtime-scalar-only/par_manifest.json"
            manifest_defines_file="$build_dir/generated-runtime-scalar-only/manifest-defines.txt"
            python3 - "$build_dir/generated-runtime-scalar-only/par_manifest.json" > "$manifest_defines_file" <<'PYMANIFEST_SCALAR'
import json
import sys

manifest = json.load(open(sys.argv[1], encoding="utf-8"))
layout = manifest["layout_max"]
print(f"-DPAR_HOST_TEST_MANIFEST_PARAM_COUNT_MAX={manifest['param_count_max']}U")
print(f"-DPAR_HOST_TEST_MANIFEST_COUNT8={layout['count8']}U")
print(f"-DPAR_HOST_TEST_MANIFEST_COUNT16={layout['count16']}U")
print(f"-DPAR_HOST_TEST_MANIFEST_COUNT32={layout['count32']}U")
print(f"-DPAR_HOST_TEST_MANIFEST_COUNTOBJ={layout['count_obj']}U")
print(f"-DPAR_HOST_TEST_MANIFEST_OBJ_POOL_BYTES={layout['obj_pool_bytes']}UL")
PYMANIFEST_SCALAR
            mapfile -t manifest_defines < "$manifest_defines_file"
            echo "[host-tests] build par_generated_runtime_scalar_only"
            gcc -I"$build_dir/generated-runtime-scalar-only" \
                -I"$build_dir/generated-runtime-scalar-only/out" \
                "${common_cflags[@]}" \
                "${autogen_pm_ci_defines[@]}" \
                "${manifest_defines[@]}" \
                -DPAR_HOST_TEST_GENERATED_SCALAR_ONLY \
                -DPAR_CFG_OBJECT_TYPES_ENABLED=0 \
                -DPAR_CFG_ENABLE_TYPE_STR=0 \
                -DPAR_CFG_ENABLE_TYPE_BYTES=0 \
                -DPAR_CFG_ENABLE_TYPE_ARR_U8=0 \
                -DPAR_CFG_ENABLE_TYPE_ARR_U16=0 \
                -DPAR_CFG_ENABLE_TYPE_ARR_U32=0 \
                -DPAR_CFG_LAYOUT_SOURCE=PAR_CFG_LAYOUT_SCRIPT \
                parameters/tests/host/test_par_generated_runtime_consistency.c \
                "$build_dir/generated-runtime-scalar-only/out/par_layout_static.c" \
                "$build_dir/generated-runtime-scalar-only/out/par_generated_info.c" \
                "${base_sources[@]}" \
                -o "$build_dir/par_generated_runtime_scalar_only"
            echo "[host-tests] run par_generated_runtime_scalar_only"
            "$build_dir/par_generated_runtime_scalar_only"
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
