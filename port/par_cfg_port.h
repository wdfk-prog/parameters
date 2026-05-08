/**
 * @file par_cfg_port.h
 * @brief Provide platform-specific configuration overrides.
 * @author wdfk-prog
 * @version 1.1
 * @date 2026-03-29
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-03-12 1.0     wdfk-prog    first version
 * 2026-03-29 1.1     wdfk-prog    add RT-Thread AT24CXX backend bridge
 */
#ifndef _PAR_CFG_PORT_H_
#define _PAR_CFG_PORT_H_

#include <rtthread.h>

/**
 * @brief Include AT24CXX declarations when the backend is enabled.
 *
 * @details The storage-backend bridge needs AT24CXX_MAX_MEM_ADDRESS and
 * AT24CXX types only when AUTOGEN_PM_USING_RTT_AT24CXX_BACKEND is selected.
 */
#ifdef AUTOGEN_PM_USING_RTT_AT24CXX_BACKEND
#include <at24cxx.h>
#endif /* defined(AUTOGEN_PM_USING_RTT_AT24CXX_BACKEND) */

#ifdef __cplusplus
extern "C"
{
#endif /* defined(__cplusplus) */
/**
 * @brief Kconfig bridge: AUTOGEN_PM_* overrides PAR_CFG_* settings.
 */
/**
 * @brief Map RT-Thread Kconfig symbols to PAR_CFG_* macros.
 *
 * @details Each bridge block below documents the matching Kconfig option and
 * the generated PAR_CFG_* macro immediately before the macro is defined.
 */
/**
 * @brief Enable parameter mutex protection from AUTOGEN_PM_USING_MUTEX.
 *
 * @details Set by menuconfig when parameters may be accessed from multiple
 * RT-Thread threads, shell commands, or deferred callbacks.
 */
#ifdef AUTOGEN_PM_USING_MUTEX
#define PAR_CFG_MUTEX_EN (1)
#else
#define PAR_CFG_MUTEX_EN (0)
#endif /* defined(AUTOGEN_PM_USING_MUTEX) */

/**
 * @brief Forward AUTOGEN_PM_MUTEX_TIMEOUT_MS to PAR_CFG_MUTEX_TIMEOUT_MS.
 *
 * @details Unit is milliseconds. Increase only for known long application
 * critical sections around parameter access.
 */
#ifdef AUTOGEN_PM_MUTEX_TIMEOUT_MS
#define PAR_CFG_MUTEX_TIMEOUT_MS AUTOGEN_PM_MUTEX_TIMEOUT_MS
#endif /* defined(AUTOGEN_PM_MUTEX_TIMEOUT_MS) */

/**
 * @brief Enable diagnostic logs from AUTOGEN_PM_USING_DEBUG.
 *
 * @details Enable during bring-up or persistence debugging. Disable to reduce
 * log code and runtime formatting in production images.
 */
#ifdef AUTOGEN_PM_USING_DEBUG
#define PAR_CFG_DEBUG_EN (1)
#else
#define PAR_CFG_DEBUG_EN (0)
#endif /* defined(AUTOGEN_PM_USING_DEBUG) */

/**
 * @brief Enable RT_ASSERT-backed checks from AUTOGEN_PM_USING_ASSERT.
 *
 * @details Enable during development to catch invalid generated tables or
 * integration misuse early.
 */
#ifdef AUTOGEN_PM_USING_ASSERT
#define PAR_CFG_ASSERT_EN (1)
#else
#define PAR_CFG_ASSERT_EN (0)
#endif /* defined(AUTOGEN_PM_USING_ASSERT) */

/**
 * @brief Select generated static layout mode from AUTOGEN_PM_LAYOUT_SOURCE_SCRIPT.
 *
 * @details When enabled, par_layout_init() consumes the generated static offset
 * tables instead of scanning the compiled parameter table at startup.
 */
#ifdef AUTOGEN_PM_LAYOUT_SOURCE_SCRIPT
#define PAR_CFG_LAYOUT_SOURCE         PAR_CFG_LAYOUT_SCRIPT
#define PAR_CFG_LAYOUT_STATIC_INCLUDE "par_layout_static.h"
#endif /* defined(AUTOGEN_PM_LAYOUT_SOURCE_SCRIPT) */

/**
 * @brief Enable managed NVM storage from AUTOGEN_PM_USING_NVM.
 *
 * @details Required before any par_table.def row with pers_ = 1 can be stored
 * by the managed persistence layer.
 */
#ifdef AUTOGEN_PM_USING_NVM
#define PAR_CFG_NVM_EN (1)
#else
#define PAR_CFG_NVM_EN (0)
#endif /* defined(AUTOGEN_PM_USING_NVM) */

/**
 * @brief Select backend storage region from AUTOGEN_PM_NVM_REGION.
 *
 * @details Passed through unchanged for backends that support a named or
 * indexed storage region.
 */
#ifdef AUTOGEN_PM_NVM_REGION
#define PAR_CFG_NVM_REGION AUTOGEN_PM_NVM_REGION
#endif /* defined(AUTOGEN_PM_NVM_REGION) */

/**
 * @brief Enable the legacy GEL NVM backend.
 *
 * @details Use only when the project still integrates the original
 * GeneralEmbeddedCLibraries persistence backend.
 */
#ifdef AUTOGEN_PM_NVM_BACKEND_GEL
#define PAR_CFG_NVM_BACKEND_GEL_EN (1)
#else
#define PAR_CFG_NVM_BACKEND_GEL_EN (0)
#endif /* defined(AUTOGEN_PM_NVM_BACKEND_GEL) */

/**
 * @brief Enable the RT-Thread AT24CXX NVM backend.
 *
 * @details Select when persistent parameters should be stored in an AT24CXX
 * EEPROM device managed by the RT-Thread package.
 */
#ifdef AUTOGEN_PM_USING_RTT_AT24CXX_BACKEND
#define PAR_CFG_NVM_BACKEND_RTT_AT24CXX_EN (1)
#else
#define PAR_CFG_NVM_BACKEND_RTT_AT24CXX_EN (0)
#endif /* defined(AUTOGEN_PM_USING_RTT_AT24CXX_BACKEND) */

/**
 * @brief Enable persisted image compatibility checks.
 *
 * @details Reject scalar/object persistence blocks whose table digest does not match
 * the current serialized layout contract.
 */
#ifdef AUTOGEN_PM_USING_TABLE_ID_CHECK
#define PAR_CFG_TABLE_ID_CHECK_EN (1)
#else
#define PAR_CFG_TABLE_ID_CHECK_EN (0)
#endif /* defined(AUTOGEN_PM_USING_TABLE_ID_CHECK) */

/**
 * @brief Forward the manual table-ID schema version.
 *
 * @details Bump this Kconfig value when semantic compatibility changes without
 * a serialized byte-layout change.
 */
#ifdef AUTOGEN_PM_TABLE_ID_SCHEMA_VER
#define PAR_CFG_TABLE_ID_SCHEMA_VER ((uint32_t)AUTOGEN_PM_TABLE_ID_SCHEMA_VER)
#endif /* defined(AUTOGEN_PM_TABLE_ID_SCHEMA_VER) */

/**
 * @brief Enable the Flash EEPROM-emulation backend.
 *
 * @details Select when persistent parameters should be stored by the bundled
 * Flash EE backend instead of an external backend.
 */
#ifdef AUTOGEN_PM_USING_FLASH_EE_BACKEND
#define PAR_CFG_NVM_BACKEND_FLASH_EE_EN (1)
#else
#define PAR_CFG_NVM_BACKEND_FLASH_EE_EN (0)
#endif /* defined(AUTOGEN_PM_USING_FLASH_EE_BACKEND) */

/**
 * @brief Bind the Flash EE backend to FAL.
 *
 * @details Use when the logical EEPROM window is backed by a named FAL
 * partition.
 */
#ifdef AUTOGEN_PM_FLASH_EE_PORT_FAL
#define PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_FAL_EN (1)
#else
#define PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_FAL_EN (0)
#endif /* defined(AUTOGEN_PM_FLASH_EE_PORT_FAL) */

/**
 * @brief Bind the Flash EE backend to native flash hooks.
 *
 * @details Use when the BSP provides direct erase/program/read primitives
 * instead of FAL.
 */
#ifdef AUTOGEN_PM_FLASH_EE_PORT_NATIVE
#define PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_NATIVE_EN (1)
#else
#define PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_NATIVE_EN (0)
#endif /* defined(AUTOGEN_PM_FLASH_EE_PORT_NATIVE) */

/**
 * @brief Enable scalar NVM write-readback verification.
 *
 * @details Adds backend sync and readback comparison after writes. This
 * improves diagnostics at the cost of write latency.
 */
#ifdef AUTOGEN_PM_NVM_WRITE_VERIFY
#define PAR_CFG_NVM_WRITE_VERIFY_EN (1)
#else
#define PAR_CFG_NVM_WRITE_VERIFY_EN (0)
#endif /* defined(AUTOGEN_PM_NVM_WRITE_VERIFY) */
/**
 * @brief Enable scalar parameter persistence.
 *
 * @details Stores scalar rows whose pers_ column is set to 1 in the managed
 * scalar NVM block. Disable this for object-only products that do not need
 * scalar records.
 */
#ifdef AUTOGEN_PM_NVM_SCALAR
#define PAR_CFG_NVM_SCALAR_EN (1)
#else
#define PAR_CFG_NVM_SCALAR_EN (0)
#endif /* defined(AUTOGEN_PM_NVM_SCALAR) */

/**
 * @brief Enable object parameter persistence.
 *
 * @details Stores STR, BYTES, ARR_U8, ARR_U16, and ARR_U32 payloads in a
 * dedicated object block. Storage target and address placement are selected
 * separately by the object persistence store/address options.
 */
#ifdef AUTOGEN_PM_NVM_OBJECT
#define PAR_CFG_NVM_OBJECT_EN (1)
#else
#define PAR_CFG_NVM_OBJECT_EN (0)
#endif /* defined(AUTOGEN_PM_NVM_OBJECT) */

/**
 * @brief Reject removed object persistence migration Kconfig symbols.
 *
 * @details Generic object block migration is intentionally unsupported because
 * flash-safe relocation requires product-specific transaction storage,
 * power-loss recovery, and erase-block planning.
 */
#if defined(AUTOGEN_PM_NVM_OBJECT_MIGRATION) || \
    defined(AUTOGEN_PM_NVM_OBJECT_MIGRATION_SRC_ADDR)
#error "Object persistence migration Kconfig is not supported; use fixed or dedicated placement!"
#endif /* defined(AUTOGEN_PM_NVM_OBJECT_MIGRATION) || defined(AUTOGEN_PM_NVM_OBJECT_MIGRATION_SRC_ADDR) */

/**
 * @brief Select shared scalar/object persistence backend mode from Kconfig.
 *
 * @details Object records use the same backend address space as scalar records.
 * The object address mode below chooses after-scalar or fixed placement.
 */
#ifdef AUTOGEN_PM_NVM_OBJECT_STORE_SHARED
#define PAR_CFG_NVM_OBJECT_STORE_MODE PAR_CFG_NVM_OBJECT_STORE_SHARED
#endif /* defined(AUTOGEN_PM_NVM_OBJECT_STORE_SHARED) */

/**
 * @brief Select dedicated object backend or partition mode from Kconfig.
 *
 * @details Object records use a port-provided object backend through
 * par_object_store_backend_bind() and par_object_store_backend_get_api(). This
 * keeps scalar and object persistence independent even when they use different
 * physical media or partitions.
 */
#ifdef AUTOGEN_PM_NVM_OBJECT_STORE_DEDICATED
#define PAR_CFG_NVM_OBJECT_STORE_MODE PAR_CFG_NVM_OBJECT_STORE_DEDICATED
#endif /* defined(AUTOGEN_PM_NVM_OBJECT_STORE_DEDICATED) */

/**
 * @brief Select after-scalar object persistence address mode from Kconfig.
 *
 * @details The object block is appended after the current scalar block. This
 * compact mode does not provide generic migration because safe relocation on
 * flash-class media depends on product-specific transaction storage,
 * power-loss recovery, and erase-block planning. Use fixed or dedicated
 * placement before release when object values must survive scalar-layout growth.
 */
#ifdef AUTOGEN_PM_NVM_OBJECT_ADDR_AFTER_SCALAR
#define PAR_CFG_NVM_OBJECT_ADDR_MODE PAR_CFG_NVM_OBJECT_ADDR_AFTER_SCALAR
#endif /* defined(AUTOGEN_PM_NVM_OBJECT_ADDR_AFTER_SCALAR) */

/**
 * @brief Select fixed-address object persistence mode from Kconfig.
 *
 * @details The object block starts at PAR_CFG_NVM_OBJECT_FIXED_ADDR so scalar
 * persistent-layout growth does not move the object block.
 */
#ifdef AUTOGEN_PM_NVM_OBJECT_ADDR_FIXED
#define PAR_CFG_NVM_OBJECT_ADDR_MODE PAR_CFG_NVM_OBJECT_ADDR_FIXED
#endif /* defined(AUTOGEN_PM_NVM_OBJECT_ADDR_FIXED) */

/**
 * @brief Forward fixed object persistence base address from Kconfig.
 *
 * @details Used only in fixed-address mode. The address is in the active
 * backend address space and must not overlap the scalar NVM block.
 */
#ifdef AUTOGEN_PM_NVM_OBJECT_FIXED_ADDR
#define PAR_CFG_NVM_OBJECT_FIXED_ADDR (AUTOGEN_PM_NVM_OBJECT_FIXED_ADDR)
#endif /* defined(AUTOGEN_PM_NVM_OBJECT_FIXED_ADDR) */

/**
 * @brief Forward fixed object persistence reserved-region size from Kconfig.
 *
 * @details Used only in fixed-address mode when non-zero. Initialization
 * checks that the object block fits inside the configured region.
 */
#ifdef AUTOGEN_PM_NVM_OBJECT_REGION_SIZE
#define PAR_CFG_NVM_OBJECT_REGION_SIZE (AUTOGEN_PM_NVM_OBJECT_REGION_SIZE)
#endif /* defined(AUTOGEN_PM_NVM_OBJECT_REGION_SIZE) */

/**
 * @brief Forward dedicated object persistence base address from Kconfig.
 *
 * @details Used only when the dedicated object backend or partition mode is
 * selected. The address is relative to the object backend address space.
 */
#ifdef AUTOGEN_PM_NVM_OBJECT_DEDICATED_BASE_ADDR
#define PAR_CFG_NVM_OBJECT_DEDICATED_BASE_ADDR (AUTOGEN_PM_NVM_OBJECT_DEDICATED_BASE_ADDR)
#endif /* defined(AUTOGEN_PM_NVM_OBJECT_DEDICATED_BASE_ADDR) */

/**
 * @brief Configure Flash EE logical EEPROM size.
 *
 * @details Defines the parameter-owned logical window size exposed by the
 * Flash EE backend.
 */
#ifdef AUTOGEN_PM_FLASH_EE_LOGICAL_SIZE
#define PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE (AUTOGEN_PM_FLASH_EE_LOGICAL_SIZE)
#endif /* defined(AUTOGEN_PM_FLASH_EE_LOGICAL_SIZE) */

/**
 * @brief Configure Flash EE RAM cache size.
 *
 * @details Increase when the backend needs a larger cache window; decrease
 * when RAM is more constrained.
 */
#ifdef AUTOGEN_PM_FLASH_EE_CACHE_SIZE
#define PAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE (AUTOGEN_PM_FLASH_EE_CACHE_SIZE)
#endif /* defined(AUTOGEN_PM_FLASH_EE_CACHE_SIZE) */

/**
 * @brief Configure Flash EE logical line size.
 *
 * @details Must match the selected Flash EE backend geometry and write
 * granularity expectations.
 */
#ifdef AUTOGEN_PM_FLASH_EE_LINE_SIZE
#define PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE (AUTOGEN_PM_FLASH_EE_LINE_SIZE)
#endif /* defined(AUTOGEN_PM_FLASH_EE_LINE_SIZE) */

/**
 * @brief Configure Flash EE minimum program size.
 *
 * @details Set to the smallest programmable flash unit supported by the
 * selected flash port.
 */
#ifdef AUTOGEN_PM_FLASH_EE_PROGRAM_SIZE
#define PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE (AUTOGEN_PM_FLASH_EE_PROGRAM_SIZE)
#endif /* defined(AUTOGEN_PM_FLASH_EE_PROGRAM_SIZE) */

/**
 * @brief Configure the FAL partition used by Flash EE.
 *
 * @details Must match an existing FAL partition name when FAL binding is
 * selected.
 */
#ifdef AUTOGEN_PM_FLASH_EE_FAL_PARTITION_NAME
#define PAR_CFG_NVM_BACKEND_FLASH_EE_FAL_PARTITION_NAME AUTOGEN_PM_FLASH_EE_FAL_PARTITION_NAME
#endif /* defined(AUTOGEN_PM_FLASH_EE_FAL_PARTITION_NAME) */

/**
 * @brief Select the scalar NVM record layout from menuconfig.
 *
 * @details Exactly one AUTOGEN_PM_NVM_RECORD_LAYOUT_* symbol should be set;
 * this maps it to PAR_CFG_NVM_RECORD_LAYOUT.
 */
#if defined(AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE)
#define PAR_CFG_NVM_RECORD_LAYOUT (PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE)
#elif defined(AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE)
#define PAR_CFG_NVM_RECORD_LAYOUT (PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE)
#elif defined(AUTOGEN_PM_NVM_RECORD_LAYOUT_COMPACT_PAYLOAD)
#define PAR_CFG_NVM_RECORD_LAYOUT (PAR_CFG_NVM_RECORD_LAYOUT_COMPACT_PAYLOAD)
#elif defined(AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_PAYLOAD_ONLY)
#define PAR_CFG_NVM_RECORD_LAYOUT (PAR_CFG_NVM_RECORD_LAYOUT_FIXED_PAYLOAD_ONLY)
#elif defined(AUTOGEN_PM_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY)
#define PAR_CFG_NVM_RECORD_LAYOUT (PAR_CFG_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY)
#endif /* defined(AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE) */

/**
 * @brief Enable scalar min/max range metadata.
 *
 * @details Required when setters should reject values outside configured
 * scalar ranges.
 */
#ifdef AUTOGEN_PM_ENABLE_RANGE
#define PAR_CFG_ENABLE_RANGE (1)
#else
#define PAR_CFG_ENABLE_RANGE (0)
#endif /* defined(AUTOGEN_PM_ENABLE_RANGE) */

/**
 * @brief Enable F32 scalar parameter support.
 *
 * @details Compile this when par_table.def contains floating-point scalar
 * entries.
 */
#ifdef AUTOGEN_PM_ENABLE_TYPE_F32
#define PAR_CFG_ENABLE_TYPE_F32 (1)
#else
#define PAR_CFG_ENABLE_TYPE_F32 (0)
#endif /* defined(AUTOGEN_PM_ENABLE_TYPE_F32) */

/**
 * @brief Enable STR object parameter support.
 *
 * @details Compile this when par_table.def contains fixed-capacity string
 * object entries.
 */
#ifdef AUTOGEN_PM_ENABLE_TYPE_STR
#define PAR_CFG_ENABLE_TYPE_STR (1)
#else
#define PAR_CFG_ENABLE_TYPE_STR (0)
#endif /* defined(AUTOGEN_PM_ENABLE_TYPE_STR) */

/**
 * @brief Enable BYTES object parameter support.
 *
 * @details Compile this when par_table.def contains opaque binary object
 * entries.
 */
#ifdef AUTOGEN_PM_ENABLE_TYPE_BYTES
#define PAR_CFG_ENABLE_TYPE_BYTES (1)
#else
#define PAR_CFG_ENABLE_TYPE_BYTES (0)
#endif /* defined(AUTOGEN_PM_ENABLE_TYPE_BYTES) */

/**
 * @brief Enable ARR_U8 object parameter support.
 *
 * @details Compile this when par_table.def contains uint8_t array object
 * entries.
 */
#ifdef AUTOGEN_PM_ENABLE_TYPE_ARR_U8
#define PAR_CFG_ENABLE_TYPE_ARR_U8 (1)
#else
#define PAR_CFG_ENABLE_TYPE_ARR_U8 (0)
#endif /* defined(AUTOGEN_PM_ENABLE_TYPE_ARR_U8) */

/**
 * @brief Enable ARR_U16 object parameter support.
 *
 * @details Compile this when par_table.def contains uint16_t array object
 * entries.
 */
#ifdef AUTOGEN_PM_ENABLE_TYPE_ARR_U16
#define PAR_CFG_ENABLE_TYPE_ARR_U16 (1)
#else
#define PAR_CFG_ENABLE_TYPE_ARR_U16 (0)
#endif /* defined(AUTOGEN_PM_ENABLE_TYPE_ARR_U16) */

/**
 * @brief Enable ARR_U32 object parameter support.
 *
 * @details Compile this when par_table.def contains uint32_t array object
 * entries.
 */
#ifdef AUTOGEN_PM_ENABLE_TYPE_ARR_U32
#define PAR_CFG_ENABLE_TYPE_ARR_U32 (1)
#else
#define PAR_CFG_ENABLE_TYPE_ARR_U32 (0)
#endif /* defined(AUTOGEN_PM_ENABLE_TYPE_ARR_U32) */

/**
 * @brief Derive the aggregate object framework switch.
 *
 * @details PAR_CFG_OBJECT_TYPES_ENABLED is enabled when at least one object
 * type is enabled by menuconfig.
 */
#if ((1 == PAR_CFG_ENABLE_TYPE_STR) || (1 == PAR_CFG_ENABLE_TYPE_BYTES) ||      \
     (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) || (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) || \
     (1 == PAR_CFG_ENABLE_TYPE_ARR_U32))
#define PAR_CFG_OBJECT_TYPES_ENABLED (1)
#else
#define PAR_CFG_OBJECT_TYPES_ENABLED (0)
#endif /* ((1 == PAR_CFG_ENABLE_TYPE_STR) || (1 == PAR_CFG_ENABLE_TYPE_BYTES) || (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) || (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) || (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)) */

/**
 * @brief Enable runtime validation callbacks.
 *
 * @details Normal setters call user validation hooks before committing values
 * when this option is enabled.
 */
#ifdef AUTOGEN_PM_ENABLE_RUNTIME_VALIDATION
#define PAR_CFG_ENABLE_RUNTIME_VALIDATION (1)
#else
#define PAR_CFG_ENABLE_RUNTIME_VALIDATION (0)
#endif /* defined(AUTOGEN_PM_ENABLE_RUNTIME_VALIDATION) */

/**
 * @brief Enable scalar change callbacks.
 *
 * @details Normal scalar setters notify registered callbacks after successful
 * value changes.
 */
#ifdef AUTOGEN_PM_ENABLE_CHANGE_CALLBACK
#define PAR_CFG_ENABLE_CHANGE_CALLBACK (1)
#else
#define PAR_CFG_ENABLE_CHANGE_CALLBACK (0)
#endif /* defined(AUTOGEN_PM_ENABLE_CHANGE_CALLBACK) */

/**
 * @brief Enable raw reset-all mirror storage.
 *
 * @details Speeds up reset-all by restoring mirrored defaults directly, at the
 * cost of extra RAM.
 */
#ifdef AUTOGEN_PM_ENABLE_RESET_ALL_RAW
#define PAR_CFG_ENABLE_RESET_ALL_RAW (1)
#else
#define PAR_CFG_ENABLE_RESET_ALL_RAW (0)
#endif /* defined(AUTOGEN_PM_ENABLE_RESET_ALL_RAW) */

/**
 * @brief Enable parameter name metadata.
 *
 * @details Compile names when shell, diagnostics, or UI code needs readable
 * parameter names.
 */
#ifdef AUTOGEN_PM_ENABLE_NAME
#define PAR_CFG_ENABLE_NAME (1)
#else
#define PAR_CFG_ENABLE_NAME (0)
#endif /* defined(AUTOGEN_PM_ENABLE_NAME) */

/**
 * @brief Enable parameter unit metadata.
 *
 * @details Compile engineering unit strings for diagnostics, shell, or UI
 * display.
 */
#ifdef AUTOGEN_PM_ENABLE_UNIT
#define PAR_CFG_ENABLE_UNIT (1)
#else
#define PAR_CFG_ENABLE_UNIT (0)
#endif /* defined(AUTOGEN_PM_ENABLE_UNIT) */

/**
 * @brief Enable parameter description metadata.
 *
 * @details Compile longer descriptions for shell help, diagnostics, or
 * generated documentation.
 */
#ifdef AUTOGEN_PM_ENABLE_DESC
#define PAR_CFG_ENABLE_DESC (1)
#else
#define PAR_CFG_ENABLE_DESC (0)
#endif /* defined(AUTOGEN_PM_ENABLE_DESC) */

/**
 * @brief Enable description validation during init.
 *
 * @details Use when generated table descriptions should be checked at startup
 * for missing or invalid strings.
 */
#ifdef AUTOGEN_PM_ENABLE_DESC_CHECK
#define PAR_CFG_ENABLE_DESC_CHECK (1)
#else
#define PAR_CFG_ENABLE_DESC_CHECK (0)
#endif /* defined(AUTOGEN_PM_ENABLE_DESC_CHECK) */

/**
 * @brief Enable external parameter IDs.
 *
 * @details Required by ID lookup APIs and by scalar NVM layouts that store
 * parameter IDs.
 */
#ifdef AUTOGEN_PM_ENABLE_ID
#define PAR_CFG_ENABLE_ID (1)
#else
#define PAR_CFG_ENABLE_ID (0)
#endif /* defined(AUTOGEN_PM_ENABLE_ID) */

/**
 * @brief Enable runtime parameter-table diagnostics.
 *
 * @details Keeps startup checks that protect metadata not covered by
 * portable compile-time checks, such as F32 range/default validation.
 */
#ifdef AUTOGEN_PM_ENABLE_RUNTIME_TABLE_CHECK
#define PAR_CFG_ENABLE_RUNTIME_TABLE_CHECK (1)
#else
#define PAR_CFG_ENABLE_RUNTIME_TABLE_CHECK (0)
#endif /* defined(AUTOGEN_PM_ENABLE_RUNTIME_TABLE_CHECK) */

/**
 * @brief Enable runtime duplicate-ID diagnostics.
 *
 * @details Adds startup checking beyond compile-time generated ID checks.
 */
#ifdef AUTOGEN_PM_ENABLE_RUNTIME_ID_DUP_CHECK
#define PAR_CFG_ENABLE_RUNTIME_ID_DUP_CHECK (1)
#else
#define PAR_CFG_ENABLE_RUNTIME_ID_DUP_CHECK (0)
#endif /* defined(AUTOGEN_PM_ENABLE_RUNTIME_ID_DUP_CHECK) */

/**
 * @brief Enable runtime ID-map hash collision diagnostics.
 *
 * @details Adds startup diagnostics for the static ID lookup hash table.
 */
#ifdef AUTOGEN_PM_ENABLE_RUNTIME_ID_HASH_COLLISION_CHECK
#define PAR_CFG_ENABLE_RUNTIME_ID_HASH_COLLISION_CHECK (1)
#else
#define PAR_CFG_ENABLE_RUNTIME_ID_HASH_COLLISION_CHECK (0)
#endif /* defined(AUTOGEN_PM_ENABLE_RUNTIME_ID_HASH_COLLISION_CHECK) */

/**
 * @brief Enable read/write access metadata.
 *
 * @details Compile per-parameter access masks for external authorization and
 * shell/API filtering.
 */
#ifdef AUTOGEN_PM_ENABLE_ACCESS
#define PAR_CFG_ENABLE_ACCESS (1)
#else
#define PAR_CFG_ENABLE_ACCESS (0)
#endif /* defined(AUTOGEN_PM_ENABLE_ACCESS) */

/**
 * @brief Enable optional role-policy metadata.
 *
 * @details Compile role policy masks for integrations that enforce access by
 * caller role.
 */
#ifdef AUTOGEN_PM_ENABLE_ROLE_POLICY
#define PAR_CFG_ENABLE_ROLE_POLICY (1)
#else
#define PAR_CFG_ENABLE_ROLE_POLICY (0)
#endif /* defined(AUTOGEN_PM_ENABLE_ROLE_POLICY) */

#ifdef AUTOGEN_PM_USING_RTT_AT24CXX_BACKEND
/**
 * @brief RT-Thread AT24CXX backend I2C bus name.
 */
#ifndef PAR_CFG_RTT_AT24_I2C_BUS_NAME
#define PAR_CFG_RTT_AT24_I2C_BUS_NAME AUTOGEN_PM_RTT_AT24_I2C_BUS_NAME
#endif /* !defined(PAR_CFG_RTT_AT24_I2C_BUS_NAME) */

/**
 * @brief RT-Thread AT24CXX address-input bits (A2:A0).
 */
#ifndef PAR_CFG_RTT_AT24_ADDR_INPUT
#define PAR_CFG_RTT_AT24_ADDR_INPUT ((uint8_t)AUTOGEN_PM_RTT_AT24_ADDR_INPUT)
#endif /* !defined(PAR_CFG_RTT_AT24_ADDR_INPUT) */

/**
 * @brief Start offset of the parameter-owned EEPROM window.
 */
#ifndef PAR_CFG_RTT_AT24_BASE_ADDR
#define PAR_CFG_RTT_AT24_BASE_ADDR ((uint32_t)AUTOGEN_PM_RTT_AT24_BASE_ADDR)
#endif /* !defined(PAR_CFG_RTT_AT24_BASE_ADDR) */

/**
 * @brief Size of the parameter-owned EEPROM window.
 *
 * @details The RTT AT24CXX backend uses the memory geometry exposed by the
 * AT24CXX package directly instead of duplicating size configuration here.
 */
#ifndef PAR_CFG_RTT_AT24_SIZE
#define PAR_CFG_RTT_AT24_SIZE ((uint32_t)AT24CXX_MAX_MEM_ADDRESS)
#endif /* !defined(PAR_CFG_RTT_AT24_SIZE) */

/**
 * @brief Chunk size used when erase is emulated by writing 0xFF.
 */
#ifndef PAR_STORE_RTT_AT24_ERASE_CHUNK
#define PAR_STORE_RTT_AT24_ERASE_CHUNK ((uint32_t)AUTOGEN_PM_RTT_AT24_ERASE_CHUNK)
#endif /* !defined(PAR_STORE_RTT_AT24_ERASE_CHUNK) */
#endif /* defined(AUTOGEN_PM_USING_RTT_AT24CXX_BACKEND) */

/**
 * @brief Always use the RT-Thread par_if port in package builds.
 */
#define PAR_CFG_IF_PORT_EN (1)

/**
 * @brief Route module logs and assertions through RT-Thread hooks.
 */
#define PAR_CFG_PORT_HOOK_EN (1)

/**
 * @brief Platform assert hook.
 */
#define PAR_PORT_ASSERT(x) RT_ASSERT(x)

/**
 * @brief Platform compile-time assert hook.
 */
#define PAR_PORT_STATIC_ASSERT(name, expn) RT_STATIC_ASSERT(name, expn)

/**
 * @brief Map weak-symbol declarations to RT-Thread rt_weak.
 */
#define PAR_PORT_WEAK rt_weak

/**
 * @brief RT-Thread log tag used by this package.
 */
#define DBG_TAG "parameter"
/**
 * @brief Select RT-Thread debug log level from AUTOGEN_PM_USING_DEBUG.
 */
#ifdef AUTOGEN_PM_USING_DEBUG
#define DBG_LVL DBG_LOG
#else
#define DBG_LVL DBG_INFO
#endif /* defined(AUTOGEN_PM_USING_DEBUG) */
#include <rtdbg.h>
/**
 * @brief Map module log macros to RT-Thread rtdbg.
 */
#define PAR_PORT_LOG_INFO(...) LOG_I(__VA_ARGS__)
#define PAR_PORT_LOG_DEBUG(...) LOG_D(__VA_ARGS__)
#define PAR_PORT_LOG_WARN(...) LOG_W(__VA_ARGS__)
#define PAR_PORT_LOG_ERROR(...) LOG_E(__VA_ARGS__)

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* !defined(_PAR_CFG_PORT_H_) */
