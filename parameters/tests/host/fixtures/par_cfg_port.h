/**
 * @file par_cfg_port.h
 * @brief Provide host-test configuration overrides for the parameter module.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#ifndef _PAR_HOST_TEST_CFG_PORT_H_
#define _PAR_HOST_TEST_CFG_PORT_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/** @brief Use portable platform hooks for host-test builds. */
#ifndef PAR_CFG_IF_PORT_EN
#define PAR_CFG_IF_PORT_EN (1)
#endif /* !defined(PAR_CFG_IF_PORT_EN) */
/** @brief Host tests are single-threaded unless a matrix case enables mutex hooks. */
#ifndef PAR_CFG_MUTEX_EN
#define PAR_CFG_MUTEX_EN (0)
#endif /* !defined(PAR_CFG_MUTEX_EN) */
/** @brief Keep host-test logs silent unless a test overrides this file. */
#ifndef PAR_CFG_DEBUG_EN
#define PAR_CFG_DEBUG_EN (0)
#endif /* !defined(PAR_CFG_DEBUG_EN) */
/** @brief Keep host invalid-argument tests from aborting on assertions. */
#ifndef PAR_CFG_ASSERT_EN
#define PAR_CFG_ASSERT_EN (0)
#endif /* !defined(PAR_CFG_ASSERT_EN) */
/** @brief Use portable C weak symbols for host builds. */
#ifndef PAR_PORT_WEAK
#define PAR_PORT_WEAK __attribute__((weak))
#endif /* !defined(PAR_PORT_WEAK) */
/** @brief Compile parameter storage offsets by scanning the X-macro table. */
#ifndef PAR_CFG_LAYOUT_SOURCE
#define PAR_CFG_LAYOUT_SOURCE (0U)
#endif /* !defined(PAR_CFG_LAYOUT_SOURCE) */

/** @brief Enable all scalar and object API surfaces in host tests. */
#ifndef PAR_CFG_ENABLE_TYPE_F32
#define PAR_CFG_ENABLE_TYPE_F32    (1)
#endif /* !defined(PAR_CFG_ENABLE_TYPE_F32) */
#ifndef PAR_CFG_ENABLE_TYPE_STR
#define PAR_CFG_ENABLE_TYPE_STR    (1)
#endif /* !defined(PAR_CFG_ENABLE_TYPE_STR) */
#ifndef PAR_CFG_ENABLE_TYPE_BYTES
#define PAR_CFG_ENABLE_TYPE_BYTES  (1)
#endif /* !defined(PAR_CFG_ENABLE_TYPE_BYTES) */
#ifndef PAR_CFG_ENABLE_TYPE_ARR_U8
#define PAR_CFG_ENABLE_TYPE_ARR_U8 (1)
#endif /* !defined(PAR_CFG_ENABLE_TYPE_ARR_U8) */
#ifndef PAR_CFG_ENABLE_TYPE_ARR_U16
#define PAR_CFG_ENABLE_TYPE_ARR_U16 (1)
#endif /* !defined(PAR_CFG_ENABLE_TYPE_ARR_U16) */
#ifndef PAR_CFG_ENABLE_TYPE_ARR_U32
#define PAR_CFG_ENABLE_TYPE_ARR_U32 (1)
#endif /* !defined(PAR_CFG_ENABLE_TYPE_ARR_U32) */
#ifndef PAR_CFG_OBJECT_TYPES_ENABLED
#define PAR_CFG_OBJECT_TYPES_ENABLED (1)
#endif /* !defined(PAR_CFG_OBJECT_TYPES_ENABLED) */

/** @brief Keep host tests close to the full package feature set. */
#ifndef PAR_CFG_ENABLE_RANGE
#define PAR_CFG_ENABLE_RANGE               (1)
#endif /* !defined(PAR_CFG_ENABLE_RANGE) */
#ifndef PAR_CFG_ENABLE_NAME
#define PAR_CFG_ENABLE_NAME                (1)
#endif /* !defined(PAR_CFG_ENABLE_NAME) */
#ifndef PAR_CFG_ENABLE_UNIT
#define PAR_CFG_ENABLE_UNIT                (1)
#endif /* !defined(PAR_CFG_ENABLE_UNIT) */
#ifndef PAR_CFG_ENABLE_DESC
#define PAR_CFG_ENABLE_DESC                (1)
#endif /* !defined(PAR_CFG_ENABLE_DESC) */
#ifndef PAR_CFG_ENABLE_DESC_CHECK
#define PAR_CFG_ENABLE_DESC_CHECK          (1)
#endif /* !defined(PAR_CFG_ENABLE_DESC_CHECK) */
#ifndef PAR_CFG_ENABLE_ID
#define PAR_CFG_ENABLE_ID                  (1)
#endif /* !defined(PAR_CFG_ENABLE_ID) */
#ifndef PAR_CFG_ENABLE_ACCESS
#define PAR_CFG_ENABLE_ACCESS              (1)
#endif /* !defined(PAR_CFG_ENABLE_ACCESS) */
#ifndef PAR_CFG_ENABLE_ROLE_POLICY
#define PAR_CFG_ENABLE_ROLE_POLICY         (1)
#endif /* !defined(PAR_CFG_ENABLE_ROLE_POLICY) */
#ifndef PAR_CFG_ENABLE_RUNTIME_VALIDATION
#define PAR_CFG_ENABLE_RUNTIME_VALIDATION  (1)
#endif /* !defined(PAR_CFG_ENABLE_RUNTIME_VALIDATION) */
#ifndef PAR_CFG_ENABLE_RUNTIME_TABLE_CHECK
#define PAR_CFG_ENABLE_RUNTIME_TABLE_CHECK (1)
#endif /* !defined(PAR_CFG_ENABLE_RUNTIME_TABLE_CHECK) */
#ifndef PAR_CFG_ENABLE_CHANGE_CALLBACK
#define PAR_CFG_ENABLE_CHANGE_CALLBACK     (1)
#endif /* !defined(PAR_CFG_ENABLE_CHANGE_CALLBACK) */
#ifndef PAR_CFG_ENABLE_RESET_ALL_RAW
#define PAR_CFG_ENABLE_RESET_ALL_RAW       (1)
#endif /* !defined(PAR_CFG_ENABLE_RESET_ALL_RAW) */

#if defined(PAR_HOST_TEST_NVM)
/** @brief Enable managed NVM persistence for host persistence tests. */
#ifndef PAR_CFG_NVM_EN
#define PAR_CFG_NVM_EN (1)
#endif /* !defined(PAR_CFG_NVM_EN) */
/** @brief Enable scalar persistence in host persistence tests. */
#ifndef PAR_CFG_NVM_SCALAR_EN
#define PAR_CFG_NVM_SCALAR_EN (1)
#endif /* !defined(PAR_CFG_NVM_SCALAR_EN) */
/** @brief Enable object persistence in host persistence tests. */
#ifndef PAR_CFG_NVM_OBJECT_EN
#define PAR_CFG_NVM_OBJECT_EN (1)
#endif /* !defined(PAR_CFG_NVM_OBJECT_EN) */
/** @brief Exercise stored table-ID compatibility checks. */
#ifndef PAR_CFG_TABLE_ID_CHECK_EN
#define PAR_CFG_TABLE_ID_CHECK_EN (1)
#endif /* !defined(PAR_CFG_TABLE_ID_CHECK_EN) */
/** @brief Enable readback verification after host test writes. */
#ifndef PAR_CFG_NVM_WRITE_VERIFY_EN
#define PAR_CFG_NVM_WRITE_VERIFY_EN (1)
#endif /* !defined(PAR_CFG_NVM_WRITE_VERIFY_EN) */
/** @brief Route scalar and object persistence through the Flash EE backend. */
#ifndef PAR_CFG_NVM_BACKEND_FLASH_EE_EN
#define PAR_CFG_NVM_BACKEND_FLASH_EE_EN (1)
#endif /* !defined(PAR_CFG_NVM_BACKEND_FLASH_EE_EN) */
/** @brief Bind Flash EE to the native host-test flash hooks. */
#ifndef PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_NATIVE_EN
#define PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_NATIVE_EN (1)
#endif /* !defined(PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_NATIVE_EN) */
/** @brief Disable unrelated storage backends. */
#ifndef PAR_CFG_NVM_BACKEND_GEL_EN
#define PAR_CFG_NVM_BACKEND_GEL_EN (0)
#endif /* !defined(PAR_CFG_NVM_BACKEND_GEL_EN) */
#ifndef PAR_CFG_NVM_BACKEND_RTT_AT24CXX_EN
#define PAR_CFG_NVM_BACKEND_RTT_AT24CXX_EN (0)
#endif /* !defined(PAR_CFG_NVM_BACKEND_RTT_AT24CXX_EN) */
/** @brief Use the default self-describing scalar record layout. */
#ifndef PAR_CFG_NVM_RECORD_LAYOUT
#define PAR_CFG_NVM_RECORD_LAYOUT PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE
#endif /* !defined(PAR_CFG_NVM_RECORD_LAYOUT) */
/** @brief Size of the logical Flash EE byte-addressable region. */
#ifndef PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE
#define PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE (0x100U)
#endif /* !defined(PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE) */
/** @brief Size of the Flash EE cache window used by host tests. */
#ifndef PAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE
#define PAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE (128U)
#endif /* !defined(PAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE) */
/** @brief Logical Flash EE line size used by host tests. */
#ifndef PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE
#define PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE (16U)
#endif /* !defined(PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE) */
/** @brief Minimum programmable unit in the host Flash EE fake. */
#ifndef PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE
#define PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE (4U)
#endif /* !defined(PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE) */
/** @brief Store object payloads in the same fake flash address space. */
#ifndef PAR_CFG_NVM_OBJECT_STORE_MODE
#define PAR_CFG_NVM_OBJECT_STORE_MODE PAR_CFG_NVM_OBJECT_STORE_SHARED
#endif /* !defined(PAR_CFG_NVM_OBJECT_STORE_MODE) */
/** @brief Place object payloads at a fixed offset to catch overlap mistakes. */
#ifndef PAR_CFG_NVM_OBJECT_ADDR_MODE
#define PAR_CFG_NVM_OBJECT_ADDR_MODE PAR_CFG_NVM_OBJECT_ADDR_FIXED
#endif /* !defined(PAR_CFG_NVM_OBJECT_ADDR_MODE) */
/** @brief Fixed host-test object block address in the shared fake flash. */
#ifndef PAR_CFG_NVM_OBJECT_FIXED_ADDR
#define PAR_CFG_NVM_OBJECT_FIXED_ADDR (0xC0U)
#endif /* !defined(PAR_CFG_NVM_OBJECT_FIXED_ADDR) */
/** @brief Reserved host-test object block size. */
#ifndef PAR_CFG_NVM_OBJECT_REGION_SIZE
#define PAR_CFG_NVM_OBJECT_REGION_SIZE (0x40U)
#endif /* !defined(PAR_CFG_NVM_OBJECT_REGION_SIZE) */
/** @brief Dedicated object-store base address used by host NVM tests. */
#ifndef PAR_CFG_NVM_OBJECT_DEDICATED_BASE_ADDR
#define PAR_CFG_NVM_OBJECT_DEDICATED_BASE_ADDR (0x00U)
#endif /* !defined(PAR_CFG_NVM_OBJECT_DEDICATED_BASE_ADDR) */
/** @brief Mark fixture items as persistent when NVM tests are enabled. */
#ifndef PAR_HOST_TEST_PERSISTENT
#define PAR_HOST_TEST_PERSISTENT (1)
#endif /* !defined(PAR_HOST_TEST_PERSISTENT) */
#else
/** @brief Disable managed persistence for ordinary host runtime tests. */
#ifndef PAR_CFG_NVM_EN
#define PAR_CFG_NVM_EN (0)
#endif /* !defined(PAR_CFG_NVM_EN) */
/** @brief Table-ID checks require NVM and are disabled in non-NVM tests. */
#ifndef PAR_CFG_TABLE_ID_CHECK_EN
#define PAR_CFG_TABLE_ID_CHECK_EN (0)
#endif /* !defined(PAR_CFG_TABLE_ID_CHECK_EN) */
/** @brief Keep fixture items volatile for non-NVM runtime tests. */
#ifndef PAR_HOST_TEST_PERSISTENT
#define PAR_HOST_TEST_PERSISTENT (0)
#endif /* !defined(PAR_HOST_TEST_PERSISTENT) */
#endif /* defined(PAR_HOST_TEST_NVM) */

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* !defined(_PAR_HOST_TEST_CFG_PORT_H_) */
