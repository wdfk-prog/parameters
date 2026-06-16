/**
 * @file par_cfg_port.h
 * @brief Provide host-simulator configuration overrides for runtime tests.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#ifndef PAR_HOST_CFG_PORT_H
#define PAR_HOST_CFG_PORT_H

#include <assert.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/**
 * @brief Use the portable weak interface port in host tests.
 */
#define PAR_CFG_IF_PORT_EN (1)

/**
 * @brief Route host-test logs through the portable hook layer.
 */
#define PAR_CFG_PORT_HOOK_EN (1)

/**
 * @brief Keep host-test diagnostics quiet unless a test explicitly prints logs.
 */
#define PAR_CFG_DEBUG_EN (0)

/**
 * @brief Keep assertions enabled in host-simulator builds.
 */
#define PAR_CFG_ASSERT_EN (1)

/**
 * @brief Implement package assertions with the host C library assert.
 */
#define PAR_PORT_ASSERT(x) assert(x)

/**
 * @brief Host log hook for informational messages.
 */
#define PAR_PORT_LOG_INFO(...)  (void)printf(__VA_ARGS__)

/**
 * @brief Host log hook for debug messages.
 */
#define PAR_PORT_LOG_DEBUG(...) (void)printf(__VA_ARGS__)

/**
 * @brief Host log hook for warning messages.
 */
#define PAR_PORT_LOG_WARN(...)  (void)printf(__VA_ARGS__)

/**
 * @brief Host log hook for error messages.
 */
#define PAR_PORT_LOG_ERROR(...) (void)printf(__VA_ARGS__)

/**
 * @brief Enable managed scalar and object persistence for the generated test table.
 */
#define PAR_CFG_NVM_EN        (1)
#define PAR_CFG_NVM_SCALAR_EN (1)
#define PAR_CFG_NVM_OBJECT_EN (1)

/**
 * @brief Use the scalar backend as the shared object-persistence backend.
 */
#define PAR_CFG_NVM_OBJECT_STORE_MODE (0U)

/**
 * @brief Place the object-persistence block after scalar records in host tests.
 */
#define PAR_CFG_NVM_OBJECT_ADDR_MODE (0U)


/**
 * @brief Select the scalar NVM record layout requested by the host build.
 */
#if defined(AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE)
#define PAR_CFG_NVM_RECORD_LAYOUT (0U)
#elif defined(AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE)
#define PAR_CFG_NVM_RECORD_LAYOUT (1U)
#elif defined(AUTOGEN_PM_NVM_RECORD_LAYOUT_COMPACT_PAYLOAD)
#define PAR_CFG_NVM_RECORD_LAYOUT (2U)
#elif defined(AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_PAYLOAD_ONLY)
#define PAR_CFG_NVM_RECORD_LAYOUT (3U)
#elif defined(AUTOGEN_PM_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY)
#define PAR_CFG_NVM_RECORD_LAYOUT (4U)
#else
#define PAR_CFG_NVM_RECORD_LAYOUT (0U)
#endif /* defined(AUTOGEN_PM_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE) */

/**
 * @brief Enable table-ID checking so host images exercise compatibility headers.
 */
#define PAR_CFG_TABLE_ID_CHECK_EN (1)

/**
 * @brief Enable the feature set used by the generated runtime-test table.
 */
#define PAR_CFG_ENABLE_RANGE              (1)
#define PAR_CFG_ENABLE_TYPE_F32           (1)
#define PAR_CFG_ENABLE_TYPE_STR           (1)
#define PAR_CFG_ENABLE_TYPE_BYTES         (1)
#define PAR_CFG_ENABLE_TYPE_ARR_U8        (1)
#define PAR_CFG_ENABLE_TYPE_ARR_U16       (1)
#define PAR_CFG_ENABLE_TYPE_ARR_U32       (1)
#define PAR_CFG_ENABLE_RUNTIME_VALIDATION (1)
#define PAR_CFG_ENABLE_CHANGE_CALLBACK    (1)
#define PAR_CFG_ENABLE_RESET_ALL_RAW      (1)
#define PAR_CFG_ENABLE_NAME               (1)
#define PAR_CFG_ENABLE_UNIT               (1)
#define PAR_CFG_ENABLE_DESC               (1)
#define PAR_CFG_ENABLE_DESC_CHECK         (1)
#define PAR_CFG_ENABLE_ID                 (1)
#define PAR_CFG_ENABLE_ACCESS             (1)
#define PAR_CFG_ENABLE_ROLE_POLICY        (1)
#define PAR_CFG_ENABLE_RUNTIME_TABLE_CHECK             (1)
#define PAR_CFG_ENABLE_RUNTIME_ID_DUP_CHECK            (1)
#define PAR_CFG_ENABLE_RUNTIME_ID_HASH_COLLISION_CHECK (1)

#ifdef PAR_HOST_BACKEND_FLASH_EE
/**
 * @brief Select the flash-emulated EEPROM backend with native host hooks.
 */
#define PAR_CFG_NVM_BACKEND_FLASH_EE_EN             (1)
#define PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_NATIVE_EN (1)
#define PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE   (4096u)
#define PAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE     (4096u)
#define PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE      (32u)
#define PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE   (8u)
#else
/**
 * @brief Disable packaged flash backends for byte-addressable host EEPROM tests.
 */
#define PAR_CFG_NVM_BACKEND_FLASH_EE_EN             (0)
#define PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_NATIVE_EN (0)
#endif /* defined(PAR_HOST_BACKEND_FLASH_EE) */

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* !defined(PAR_HOST_CFG_PORT_H) */
