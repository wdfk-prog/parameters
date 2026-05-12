#!/usr/bin/env bash
# Common host-test compile settings and helpers.

build_dir="${BUILD_DIR:-build/host-tests}"
mkdir -p "$build_dir"

mapfile -t autogen_pm_ci_defines < <(autogen_pm_ci_emit_c_defines)

common_includes=(
    -Iparameters/tests/host/fixtures
    -I.
    -Ibackend
    -Iport
    -Iparameters/include
    -Iparameters/src
    -Iparameters/src/def
    -Iparameters/src/detail
    -Iparameters/src/layout
    -Iparameters/src/nvm
    -Iparameters/src/nvm/backend
    -Iparameters/src/nvm/object
    -Iparameters/src/nvm/object/addr
    -Iparameters/src/nvm/object/store
    -Iparameters/src/nvm/scalar
    -Iparameters/src/nvm/scalar/layout
    -Iparameters/src/nvm/scalar/store
    -Iparameters/src/object
    -Iparameters/src/port
    -Iparameters/src/scalar
)

common_cflags=(
    -std=c11
    -Wall
    -Wextra
    -Wno-unused-parameter
    -fsanitize=address,undefined
    -fno-omit-frame-pointer
    "${common_includes[@]}"
)

base_sources=(
    parameters/src/par.c
    parameters/src/object/par_object.c
    parameters/src/object/par_object_api.c
    parameters/src/scalar/par_scalar_api.c
    parameters/src/def/par_def.c
    parameters/src/def/par_id_map_static.c
    parameters/src/layout/par_layout.c
    parameters/src/port/par_if.c
)

nvm_sources=(
    parameters/src/nvm/par_nvm.c
    parameters/src/nvm/par_nvm_table_id.c
    parameters/src/nvm/hash_32a.c
    parameters/src/nvm/scalar/store/par_nvm_scalar_store.c
    parameters/src/nvm/scalar/par_nvm_scalar.c
    parameters/src/nvm/backend/par_store_backend_flash_ee.c
    parameters/src/nvm/object/par_nvm_object.c
    parameters/src/nvm/object/store/par_nvm_object_store_shared.c
    parameters/src/nvm/object/store/par_nvm_object_store_dedicated.c
    parameters/src/nvm/object/addr/par_nvm_object_addr_after_scalar.c
    parameters/src/nvm/object/addr/par_nvm_object_addr_fixed.c
    parameters/src/nvm/object/addr/par_nvm_object_addr_dedicated.c
    backend/par_store_backend_flash_ee_native.c
)

compile_and_run() {
    local name="$1"
    shift
    local output="$build_dir/$name"

    echo "[host-tests] build $name"
    rm -f "$output"
    gcc "${common_cflags[@]}" "${autogen_pm_ci_defines[@]}" "$@" -o "$output" || return $?
    echo "[host-tests] run $name"
    "$output"
}

compile_and_run_nvm() {
    local name="$1"
    shift
    local output="$build_dir/$name"

    echo "[host-tests] build $name"
    rm -f "$output"
    gcc -Iparameters/tests/host/fixtures_nvm "${common_cflags[@]}" "${autogen_pm_ci_defines[@]}" "$@" -o "$output" || return $?
    echo "[host-tests] run $name"
    "$output"
}

compile_only() {
    local name="$1"
    shift
    local output="$build_dir/$name.o"

    echo "[host-tests] compile $name"
    gcc "${common_cflags[@]}" "${autogen_pm_ci_defines[@]}" "$@" -c -o "$output"
}
