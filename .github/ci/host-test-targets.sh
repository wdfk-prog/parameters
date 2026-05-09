#!/usr/bin/env bash
# Host-test target dispatcher.

host_targets=(
    par_core_runtime
    par_object_runtime
    par_mutex_runtime
    par_shell_tool
    par_nvm_flash_ee
    par_nvm_flash_ee_matrix
    par_config_feature_matrix
    par_backend_adapter_smoke
    par_generated_runtime_consistency
)

run_host_targets_parallel() {
    local target
    local pid
    local failed=0
    local pids=()
    local targets=()

    for target in "$@"; do
        targets+=("$target")
        (
            run_host_target "$target"
        ) > "$build_dir/$target.log" 2>&1 &
        pids+=("$!")
    done

    for pid in "${pids[@]}"; do
        if ! wait "$pid"; then
            failed=1
        fi
    done

    for target in "${targets[@]}"; do
        cat "$build_dir/$target.log"
    done

    return "$failed"
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
        par_nvm_flash_ee)
            run_nvm_flash_ee_profile default \
                PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE \
                PAR_CFG_NVM_OBJECT_STORE_SHARED \
                PAR_CFG_NVM_OBJECT_ADDR_FIXED \
                0xC0U 0x40U \
                parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_with_size.c
            ;;
        par_nvm_flash_ee_matrix)
            run_nvm_flash_ee_matrix
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
            python3 - "$build_dir/generated-runtime/par_table.no-persist.csv" <<'PY'
from pathlib import Path
import csv
import sys

src = Path("parameters/schema/par_table.csv")
dst = Path(sys.argv[1])
with src.open(newline="", encoding="utf-8") as in_file, \
        dst.open("w", newline="", encoding="utf-8") as out_file:
    reader = csv.DictReader(in_file)
    writer = csv.DictWriter(out_file, fieldnames=reader.fieldnames)
    writer.writeheader()
    written = 0
    for row in reader:
        if row["type"] not in {"U8", "U16", "U32", "I8", "I16", "I32", "F32"}:
            continue
        row["persistent"] = "0"
        writer.writerow(row)
        written += 1
        if written >= 5:
            break
    if written == 0:
        raise SystemExit("generated runtime smoke CSV has no scalar rows")
PY
            cp parameters/schema/par_id_lock.json \
                "$build_dir/generated-runtime/par_id_lock.json"
            python3 parameters/tools/pargen.py \
                --csv "$build_dir/generated-runtime/par_table.no-persist.csv" \
                --id-lock "$build_dir/generated-runtime/par_id_lock.json" \
                --config parameters/schema/pargen.json \
                --out-def "$build_dir/generated-runtime/par_table.def" \
                --out-dir "$build_dir/generated-runtime/out" \
                --manifest "$build_dir/generated-runtime/par_manifest.json"
            echo "[host-tests] build par_generated_runtime_consistency"
            gcc -I"$build_dir/generated-runtime" \
                -I"$build_dir/generated-runtime/out" \
                "${common_cflags[@]}" \
                "${autogen_pm_ci_defines[@]}" \
                -DPAR_CFG_LAYOUT_SOURCE=PAR_CFG_LAYOUT_SCRIPT \
                parameters/tests/host/test_par_generated_runtime_consistency.c \
                "$build_dir/generated-runtime/out/par_layout_static.c" \
                "$build_dir/generated-runtime/out/par_generated_info.c" \
                "${base_sources[@]}" \
                -o "$build_dir/par_generated_runtime_consistency"
            echo "[host-tests] run par_generated_runtime_consistency"
            "$build_dir/par_generated_runtime_consistency"
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
