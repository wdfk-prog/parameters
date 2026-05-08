/**
 * @file par_cfg_features.h
 * @brief Provide parameter feature and type-switch configuration defaults.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#ifndef _PAR_CFG_FEATURES_H_
#define _PAR_CFG_FEATURES_H_

#include "par_cfg_base.h"
/**
 * @brief Invalid configuration catcher.
 *
 * @note Shall be intact by end user!
 */
#if (0 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_TABLE_ID_CHECK_EN)
#error "Parameter settings invalid: Disable table ID checking (PAR_CFG_TABLE_ID_CHECK_EN)!"
#endif /* (0 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_TABLE_ID_CHECK_EN) */

/**
 * @brief Enable parameter access serialization through mutex hooks.
 *
 * @details Set to 1 when parameters may be accessed from multiple tasks,
 * shell commands, callbacks, or interrupt-deferred contexts. Set to 0 only
 * for single-threaded integrations.
 */
#ifndef PAR_CFG_MUTEX_EN
#define PAR_CFG_MUTEX_EN (1)
#endif /* !defined(PAR_CFG_MUTEX_EN) */

/**
 * @brief Configure parameter mutex wait timeout.
 *
 * @details Unit: ms. Increase only when normal parameter operations are
 * expected to be blocked by longer application-side critical sections.
 */
#ifndef PAR_CFG_MUTEX_TIMEOUT_MS
#define PAR_CFG_MUTEX_TIMEOUT_MS (10)
#endif /* !defined(PAR_CFG_MUTEX_TIMEOUT_MS) */

/**
 * @brief Select the platform-specific par_if backend.
 *
 * @details Set to 1 when the BSP provides mutex, CRC, logging, or other
 * platform hooks through the port layer. Keep 0 for standalone fallback use.
 */
#ifndef PAR_CFG_IF_PORT_EN
#define PAR_CFG_IF_PORT_EN (0)
#endif /* !defined(PAR_CFG_IF_PORT_EN) */

/**
 * @brief Route logs and assertions through platform hooks.
 *
 * @details Set to 1 when PAR_PORT_LOG_* and PAR_PORT_ASSERT are provided by
 * the integration layer, such as RT-Thread logging and RT_ASSERT.
 */
#ifndef PAR_CFG_PORT_HOOK_EN
#define PAR_CFG_PORT_HOOK_EN (0)
#endif /* !defined(PAR_CFG_PORT_HOOK_EN) */

/**
 * @brief Parameter storage layout-source constants.
 *
 * @details COMPILE_SCAN derives offsets in code. SCRIPT uses a generated
 * static layout header when startup scanning must be avoided.
 */
#define PAR_CFG_LAYOUT_COMPILE_SCAN (0u)
#define PAR_CFG_LAYOUT_SCRIPT       (1u)

/**
 * @brief Select parameter storage layout source.
 *
 * @note
 * - COMPILE_SCAN: counts are compile-time constants, offsets are scanned in init.
 * - SCRIPT : counts/offsets are provided by generated static layout header.
 */
#ifndef PAR_CFG_LAYOUT_SOURCE
#define PAR_CFG_LAYOUT_SOURCE PAR_CFG_LAYOUT_COMPILE_SCAN
#endif /* !defined(PAR_CFG_LAYOUT_SOURCE) */

/**
 * @brief Static layout include path.
 *
 * @note Can be overridden by integrator to include generated layout header.
 */
#ifndef PAR_CFG_LAYOUT_STATIC_INCLUDE
#define PAR_CFG_LAYOUT_STATIC_INCLUDE "par_layout_static.h"
#endif /* !defined(PAR_CFG_LAYOUT_STATIC_INCLUDE) */

/**
 * @brief Enable floating-point scalar parameter support.
 *
 * @details Set to 1 when par_table.def uses F32 parameters. Set to 0 to
 * remove float storage and APIs from integer-only builds.
 */
#ifndef PAR_CFG_ENABLE_TYPE_F32
#define PAR_CFG_ENABLE_TYPE_F32 (1)
#endif /* !defined(PAR_CFG_ENABLE_TYPE_F32) */

/**
 * @brief Enable fixed-capacity string object parameter support.
 *
 * @details Set to 1 when par_table.def uses STR rows. When disabled, string
 * setters, getters, storage slots, and object persistence records are not compiled.
 */
#ifndef PAR_CFG_ENABLE_TYPE_STR
#ifdef PAR_CFG_OBJECT_TYPES_ENABLED
#define PAR_CFG_ENABLE_TYPE_STR (PAR_CFG_OBJECT_TYPES_ENABLED)
#else
#define PAR_CFG_ENABLE_TYPE_STR (1)
#endif /* defined(PAR_CFG_OBJECT_TYPES_ENABLED) */
#endif /* !defined(PAR_CFG_ENABLE_TYPE_STR) */
/**
 * @brief Enable raw byte object parameter support.
 *
 * @details Set to 1 when par_table.def uses BYTES rows for opaque binary
 * configuration payloads.
 */
#ifndef PAR_CFG_ENABLE_TYPE_BYTES
#ifdef PAR_CFG_OBJECT_TYPES_ENABLED
#define PAR_CFG_ENABLE_TYPE_BYTES (PAR_CFG_OBJECT_TYPES_ENABLED)
#else
#define PAR_CFG_ENABLE_TYPE_BYTES (1)
#endif /* defined(PAR_CFG_OBJECT_TYPES_ENABLED) */
#endif /* !defined(PAR_CFG_ENABLE_TYPE_BYTES) */
/**
 * @brief Enable uint8_t array object parameter support.
 *
 * @details Set to 1 when par_table.def uses ARR_U8 rows. Lengths are stored
 * and validated in bytes.
 */
#ifndef PAR_CFG_ENABLE_TYPE_ARR_U8
#ifdef PAR_CFG_OBJECT_TYPES_ENABLED
#define PAR_CFG_ENABLE_TYPE_ARR_U8 (PAR_CFG_OBJECT_TYPES_ENABLED)
#else
#define PAR_CFG_ENABLE_TYPE_ARR_U8 (1)
#endif /* defined(PAR_CFG_OBJECT_TYPES_ENABLED) */
#endif /* !defined(PAR_CFG_ENABLE_TYPE_ARR_U8) */
/**
 * @brief Enable uint16_t array object parameter support.
 *
 * @details Set to 1 when par_table.def uses ARR_U16 rows. Object lengths
 * must remain aligned to the uint16_t element size.
 */
#ifndef PAR_CFG_ENABLE_TYPE_ARR_U16
#ifdef PAR_CFG_OBJECT_TYPES_ENABLED
#define PAR_CFG_ENABLE_TYPE_ARR_U16 (PAR_CFG_OBJECT_TYPES_ENABLED)
#else
#define PAR_CFG_ENABLE_TYPE_ARR_U16 (1)
#endif /* defined(PAR_CFG_OBJECT_TYPES_ENABLED) */
#endif /* !defined(PAR_CFG_ENABLE_TYPE_ARR_U16) */
/**
 * @brief Enable uint32_t array object parameter support.
 *
 * @details Set to 1 when par_table.def uses ARR_U32 rows. Object lengths
 * must remain aligned to the uint32_t element size.
 */
#ifndef PAR_CFG_ENABLE_TYPE_ARR_U32
#ifdef PAR_CFG_OBJECT_TYPES_ENABLED
#define PAR_CFG_ENABLE_TYPE_ARR_U32 (PAR_CFG_OBJECT_TYPES_ENABLED)
#else
#define PAR_CFG_ENABLE_TYPE_ARR_U32 (1)
#endif /* defined(PAR_CFG_OBJECT_TYPES_ENABLED) */
#endif /* !defined(PAR_CFG_ENABLE_TYPE_ARR_U32) */
/**
 * @brief Enable the shared object-parameter framework.
 *
 * @details Usually derived from the individual object-type switches. Override
 * only when an integration deliberately wants all object types on or off.
 */
#ifndef PAR_CFG_OBJECT_TYPES_ENABLED
#if ((1 == PAR_CFG_ENABLE_TYPE_STR) || (1 == PAR_CFG_ENABLE_TYPE_BYTES) ||      \
     (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) || (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) || \
     (1 == PAR_CFG_ENABLE_TYPE_ARR_U32))
#define PAR_CFG_OBJECT_TYPES_ENABLED (1)
#else
#define PAR_CFG_OBJECT_TYPES_ENABLED (0)
#endif /* ((1 == PAR_CFG_ENABLE_TYPE_STR) || (1 == PAR_CFG_ENABLE_TYPE_BYTES) || (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) || (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) || (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)) */
#endif /* !defined(PAR_CFG_OBJECT_TYPES_ENABLED) */

/**
 * @brief True when at least one object parameter type is enabled.
 */
#define PAR_CFG_ANY_OBJECT_TYPE_ENABLED                                         \
    ((1 == PAR_CFG_ENABLE_TYPE_STR) || (1 == PAR_CFG_ENABLE_TYPE_BYTES) ||      \
     (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) || (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) || \
     (1 == PAR_CFG_ENABLE_TYPE_ARR_U32))

PAR_STATIC_ASSERT(par_cfg_object_types_enabled_matches_type_switches,
                  (((1 == PAR_CFG_OBJECT_TYPES_ENABLED) && PAR_CFG_ANY_OBJECT_TYPE_ENABLED) ||
                   ((0 == PAR_CFG_OBJECT_TYPES_ENABLED) && (!PAR_CFG_ANY_OBJECT_TYPE_ENABLED))));

#undef PAR_CFG_ANY_OBJECT_TYPE_ENABLED

/**
 * @brief Enable/Disable runtime validation callbacks in normal setters.
 */
#ifndef PAR_CFG_ENABLE_RUNTIME_VALIDATION
#define PAR_CFG_ENABLE_RUNTIME_VALIDATION (1)
#endif /* !defined(PAR_CFG_ENABLE_RUNTIME_VALIDATION) */

/**
 * @brief Enable/Disable optional runtime parameter-table diagnostics.
 *
 * @details Keeps startup checks that protect metadata not covered by
 * portable compile-time checks, such as F32 range/default validation.
 */
#ifndef PAR_CFG_ENABLE_RUNTIME_TABLE_CHECK
#define PAR_CFG_ENABLE_RUNTIME_TABLE_CHECK (1)
#endif /* !defined(PAR_CFG_ENABLE_RUNTIME_TABLE_CHECK) */

/**
 * @brief Enable/Disable on-change callbacks in normal setters.
 */
#ifndef PAR_CFG_ENABLE_CHANGE_CALLBACK
#define PAR_CFG_ENABLE_CHANGE_CALLBACK (1)
#endif /* !defined(PAR_CFG_ENABLE_CHANGE_CALLBACK) */

/**
 * @brief Enable/Disable raw reset-all API and default mirror storage.
 *
 * @note When enabled, module compiles par_reset_all_to_default_raw() and.
 * keeps a grouped default mirror snapshot for raw restore.
 *
 * @note The speedup comes from bypassing per-parameter setter-side logic such.
 * as runtime validation, change callback, and range handling.
 *
 * @note The raw reset path restores parameter storage directly from that.
 * grouped default mirror snapshot, so it is typically faster than.
 * par_set_all_to_default(), which resets parameters one by one through.
 * the normal runtime setter path.
 */
#ifndef PAR_CFG_ENABLE_RESET_ALL_RAW
#define PAR_CFG_ENABLE_RESET_ALL_RAW (1)
#endif /* !defined(PAR_CFG_ENABLE_RESET_ALL_RAW) */

/**
 * @brief Enable/Disable parameter range metadata (min/max).
 */
#ifndef PAR_CFG_ENABLE_RANGE
#define PAR_CFG_ENABLE_RANGE (1)
#endif /* !defined(PAR_CFG_ENABLE_RANGE) */

/**
 * @brief Enable/Disable parameter name metadata.
 */
#ifndef PAR_CFG_ENABLE_NAME
#define PAR_CFG_ENABLE_NAME (1)
#endif /* !defined(PAR_CFG_ENABLE_NAME) */

/**
 * @brief Enable/Disable parameter unit metadata.
 */
#ifndef PAR_CFG_ENABLE_UNIT
#define PAR_CFG_ENABLE_UNIT (1)
#endif /* !defined(PAR_CFG_ENABLE_UNIT) */

/**
 * @brief Enable/Disable parameter description metadata.
 */
#ifndef PAR_CFG_ENABLE_DESC
#define PAR_CFG_ENABLE_DESC (1)
#endif /* !defined(PAR_CFG_ENABLE_DESC) */

/**
 * @brief Enable/Disable parameter ID metadata.
 */
#ifndef PAR_CFG_ENABLE_ID
#define PAR_CFG_ENABLE_ID (1)
#endif /* !defined(PAR_CFG_ENABLE_ID) */

#endif /* !defined(_PAR_CFG_FEATURES_H_) */
