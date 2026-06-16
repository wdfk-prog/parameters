cmake_minimum_required(VERSION 3.20)

project(par_host_sim C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(PAR_HOST_BACKEND "fake_eeprom" CACHE STRING "Host backend: fake_eeprom or fake_flash_ee")
set(PAR_HOST_NVM_LAYOUT "fixed_slot_with_size" CACHE STRING "NVM layout profile")

set(PAR_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")

set(PAR_COMMON_SOURCES
    ${PAR_ROOT}/parameters/src/par.c
    ${PAR_ROOT}/parameters/src/object/par_object.c
    ${PAR_ROOT}/parameters/src/object/par_object_api.c
    ${PAR_ROOT}/parameters/src/scalar/par_scalar_api.c
    ${PAR_ROOT}/parameters/src/def/par_def.c
    ${PAR_ROOT}/parameters/src/def/par_id_map_static.c
    ${PAR_ROOT}/parameters/src/layout/par_layout.c
    ${PAR_ROOT}/parameters/src/port/par_if.c
    ${PAR_ROOT}/parameters/src/nvm/par_nvm.c
    ${PAR_ROOT}/parameters/src/nvm/par_nvm_table_id.c
    ${PAR_ROOT}/parameters/src/nvm/hash_32a.c
    ${PAR_ROOT}/parameters/src/nvm/scalar/store/par_nvm_scalar_store.c
    ${PAR_ROOT}/parameters/src/nvm/scalar/par_nvm_scalar.c
    ${PAR_ROOT}/parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_with_size.c
    ${PAR_ROOT}/parameters/src/nvm/object/par_nvm_object.c
    ${PAR_ROOT}/parameters/src/nvm/object/store/par_nvm_object_store_shared.c
    ${PAR_ROOT}/parameters/src/nvm/object/addr/par_nvm_object_addr_after_scalar.c
    ${PAR_ROOT}/parameters/tests/par_test_common.c
    ${PAR_ROOT}/parameters/tests/par_test_runner.c
    ${PAR_ROOT}/parameters/tests/par_test_ram_config.c
    ${PAR_ROOT}/parameters/tests/host/par_host_fake_storage.c
    ${PAR_ROOT}/parameters/tests/host/par_test_host_main.c
)

if(PAR_HOST_NVM_LAYOUT STREQUAL "fixed_slot_with_size")
    set(PAR_LAYOUT_DEFINE AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE)
elseif(PAR_HOST_NVM_LAYOUT STREQUAL "fixed_slot_no_size")
    list(REMOVE_ITEM PAR_COMMON_SOURCES ${PAR_ROOT}/parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_with_size.c)
    list(APPEND PAR_COMMON_SOURCES ${PAR_ROOT}/parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_no_size.c)
    set(PAR_LAYOUT_DEFINE AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE)
elseif(PAR_HOST_NVM_LAYOUT STREQUAL "compact_payload")
    list(REMOVE_ITEM PAR_COMMON_SOURCES ${PAR_ROOT}/parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_with_size.c)
    list(APPEND PAR_COMMON_SOURCES ${PAR_ROOT}/parameters/src/nvm/scalar/layout/par_nvm_layout_compact_payload.c)
    set(PAR_LAYOUT_DEFINE AUTOGEN_PM_NVM_RECORD_LAYOUT_COMPACT_PAYLOAD)
elseif(PAR_HOST_NVM_LAYOUT STREQUAL "fixed_payload_only")
    list(REMOVE_ITEM PAR_COMMON_SOURCES ${PAR_ROOT}/parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_with_size.c)
    list(APPEND PAR_COMMON_SOURCES ${PAR_ROOT}/parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_payload_only.c)
    set(PAR_LAYOUT_DEFINE AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_PAYLOAD_ONLY)
elseif(PAR_HOST_NVM_LAYOUT STREQUAL "grouped_payload_only")
    list(REMOVE_ITEM PAR_COMMON_SOURCES ${PAR_ROOT}/parameters/src/nvm/scalar/layout/par_nvm_layout_fixed_slot_with_size.c)
    list(APPEND PAR_COMMON_SOURCES ${PAR_ROOT}/parameters/src/nvm/scalar/layout/par_nvm_layout_grouped_payload_only.c)
    set(PAR_LAYOUT_DEFINE AUTOGEN_PM_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY)
else()
    message(FATAL_ERROR "Unsupported PAR_HOST_NVM_LAYOUT=${PAR_HOST_NVM_LAYOUT}")
endif()

if(PAR_HOST_BACKEND STREQUAL "fake_flash_ee")
    list(APPEND PAR_COMMON_SOURCES
        ${PAR_ROOT}/parameters/src/nvm/backend/par_store_backend_flash_ee.c
        ${PAR_ROOT}/backend/par_store_backend_flash_ee_native.c
        ${PAR_ROOT}/parameters/tests/host/par_host_flash_ee_native_port.c
        ${PAR_ROOT}/parameters/tests/par_test_flash_ee_common.c
        ${PAR_ROOT}/parameters/tests/par_test_flash_ee_host.c
    )
    set(PAR_BACKEND_DEFINES PAR_HOST_BACKEND_FLASH_EE AUTOGEN_PM_TEST_USING_FLASH_EE_HOST)
elseif(PAR_HOST_BACKEND STREQUAL "fake_eeprom")
    list(APPEND PAR_COMMON_SOURCES
        ${PAR_ROOT}/parameters/tests/host/par_host_fake_eeprom_backend.c
        ${PAR_ROOT}/parameters/tests/par_test_eeprom_host.c
    )
    set(PAR_BACKEND_DEFINES AUTOGEN_PM_TEST_USING_EEPROM_HOST)
else()
    message(FATAL_ERROR "Unsupported PAR_HOST_BACKEND=${PAR_HOST_BACKEND}")
endif()

add_executable(par_host_test ${PAR_COMMON_SOURCES})

target_include_directories(par_host_test PRIVATE
    ${PAR_ROOT}/parameters/tests/host
    ${PAR_ROOT}/parameters/tests/host/include
    ${PAR_ROOT}/parameters/tests
    ${PAR_ROOT}/parameters/tests/schema_evolution
    ${PAR_ROOT}
    ${PAR_ROOT}/backend
    ${PAR_ROOT}/parameters/include
    ${PAR_ROOT}/parameters/src
    ${PAR_ROOT}/parameters/src/def
    ${PAR_ROOT}/parameters/src/detail
    ${PAR_ROOT}/parameters/src/layout
    ${PAR_ROOT}/parameters/src/nvm
    ${PAR_ROOT}/parameters/src/nvm/backend
    ${PAR_ROOT}/parameters/src/nvm/object
    ${PAR_ROOT}/parameters/src/nvm/object/addr
    ${PAR_ROOT}/parameters/src/nvm/object/store
    ${PAR_ROOT}/parameters/src/nvm/scalar
    ${PAR_ROOT}/parameters/src/nvm/scalar/layout
    ${PAR_ROOT}/parameters/src/nvm/scalar/store
    ${PAR_ROOT}/parameters/src/object
    ${PAR_ROOT}/parameters/src/port
    ${PAR_ROOT}/parameters/src/scalar
    ${PAR_ROOT}/parameters/generated
)

target_compile_definitions(par_host_test PRIVATE
    AUTOGEN_PM_TEST_USING_RAM_CONFIG
    AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE
    ${PAR_LAYOUT_DEFINE}
    ${PAR_BACKEND_DEFINES}
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(par_host_test PRIVATE -Wall -Wextra)
endif()
