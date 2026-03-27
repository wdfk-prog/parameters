/**
 * @file par_cfg.h
 * @brief Provide compile-time configuration for the parameter module.
 * @author Ziga Miklosic
 * @version 1.0
 * @date 2026-03-27
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-01-29  V3.0.1 Ziga Miklosic
 */

/**
 * @addtogroup PAR_CFG
 * @{ <!-- BEGIN GROUP -->
 *
 * @brief Configuration for device parameters.
 */

#ifndef _PAR_CFG_H_
#define _PAR_CFG_H_
/**
 * @brief Include dependencies.
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "par_def.h"

/**
 * @brief USER CODE BEGIN...
 */

/**
 * @brief Platform adaptation bridge.
 *
 * @note Port layer may override default PAR_CFG_* settings.
 *
 * @note This header is included unconditionally.
 * Integrator shall provide "par_cfg_port.h" in include path.
 * If no platform override is needed, create an empty stub header.
 * with include guard (for example in port/par_cfg_port.h).
 */
#include "par_cfg_port.h"

/**
 * @brief USER CODE END...
 */
/**
 * @brief Compile-time definitions.
 */
/**
 * @brief Enable/Disable storing persistent parameters to NVM.
 */
#ifndef PAR_CFG_NVM_EN
#define PAR_CFG_NVM_EN (1)
#endif

/**
 * @brief NVM parameter region option.
 *
 * @note User shall select region based on nvm_cfg.h region.
 * definitions "nvm_region_name_t".
 *
 * Don't care if "PAR_CFG_NVM_EN" set to 0.
 */
#ifndef PAR_CFG_NVM_REGION
#define PAR_CFG_NVM_REGION (eNVM_REGION_INT_FLASH_DEV_PAR)
#endif

/**
 * @brief Enable/Disable parameter table unique ID checking.
 *
 * @note Base on hash unique ID is being calculated with purpose to detect.
 * device and stored parameter table difference.
 *
 * Must be disabled once the device is released in order to prevent.
 * loss of calibrated data stored in NVM.
 *
 * @pre    "PAR_CFG_NVM_EN" must be enabled otherwise it does not make sense
 * to calculate ID at all.
 */
#ifndef PAR_CFG_TABLE_ID_CHECK_EN
#define PAR_CFG_TABLE_ID_CHECK_EN (0)
#if (1 == PAR_CFG_NVM_EN)
#ifndef DEBUG
#undef PAR_CFG_TABLE_ID_CHECK_EN
#define PAR_CFG_TABLE_ID_CHECK_EN (0)
#endif
#endif
#endif

/**
 * @brief Enable/Disable debug mode.
 */
#ifndef PAR_CFG_DEBUG_EN
#define PAR_CFG_DEBUG_EN (1)
#ifndef DEBUG
#undef PAR_CFG_DEBUG_EN
#define PAR_CFG_DEBUG_EN (0)
#endif
#endif

/**
 * @brief Enable/Disable assertions.
 */
#ifndef PAR_CFG_ASSERT_EN
#define PAR_CFG_ASSERT_EN (1)
#ifndef DEBUG
#undef PAR_CFG_ASSERT_EN
#define PAR_CFG_ASSERT_EN (0)
#endif
#endif

/**
 * @brief Platform hook fallbacks.
 */
#ifndef PAR_PORT_ASSERT
#define PAR_PORT_ASSERT(x) \
    do                     \
    {                      \
        (void)(x);         \
    } while (0)
#endif

#ifndef PAR_PORT_LOG
#define PAR_PORT_LOG(tag, ...) \
    do                         \
    {                          \
        (void)(tag);           \
    } while (0)
#endif

#ifndef PAR_PORT_STATIC_ASSERT
#define PAR_PORT_STATIC_ASSERT(name, expn) typedef char _static_assert_##name[(expn) ? 1 : -1]
#endif

/**
 * @brief Platform weak symbol macro.
 *
 * @note Integrator may override this macro (for example: RT_WEAK).
 */
#ifndef PAR_PORT_WEAK
#define PAR_PORT_WEAK __attribute__((weak))
#endif

/**
 * @brief Type alignment abstraction.
 *
 * @note Default uses C11 _Alignof. If unavailable, falls back to.
 * offsetof-based alignment calculation.
 * @note This abstraction is intended for ordinary object types and platform.
 * atomic-wrapper types, but the platform must guarantee the expression.
 * is valid for its custom atomic wrapper definitions.
 */
#ifndef PAR_ALIGNOF
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define PAR_ALIGNOF(type) _Alignof(type)
#else
#define PAR_ALIGNOF(type) offsetof(struct { char _par_align_c; type _par_align_t; }, _par_align_t)
#endif
#endif

/**
 * @brief Package compile-time assert.
 */
#define PAR_STATIC_ASSERT(name, expn) PAR_PORT_STATIC_ASSERT(name, expn);
/**
 * @brief Resolve log/assert routing mode before default macro emission.
 */
#if !defined(PAR_CFG_PORT_HOOK_EN)
#define PAR_CFG_USE_PORT_HOOKS (0)
#elif (1 == PAR_CFG_PORT_HOOK_EN)
#define PAR_CFG_USE_PORT_HOOKS (1)
#else
#define PAR_CFG_USE_PORT_HOOKS (0)
#endif

/**
 * @brief Debug communication port macros.
 */
#if (0 == PAR_CFG_USE_PORT_HOOKS)
#if (1 == PAR_CFG_DEBUG_EN)
#ifndef PAR_CFG_DIRECT_LOG
#define PAR_CFG_DIRECT_LOG(...) (cli_printf((char *)__VA_ARGS__))
#endif
#define PAR_DBG_PRINT(...) PAR_CFG_DIRECT_LOG(__VA_ARGS__)
#else
#define PAR_DBG_PRINT(...) \
    {                      \
        ;                  \
    }
#endif
#else
#if (1 == PAR_CFG_DEBUG_EN)
#define PAR_DBG_PRINT(...) PAR_PORT_LOG(__VA_ARGS__)
#else
#define PAR_DBG_PRINT(...) \
    {                      \
        ;                  \
    }
#endif
#endif

/**
 * @brief Assertion macros.
 */
#if (0 == PAR_CFG_USE_PORT_HOOKS)
#if (1 == PAR_CFG_ASSERT_EN)
#ifndef PAR_CFG_DIRECT_ASSERT
#ifdef PROJ_CFG_ASSERT
#define PAR_CFG_DIRECT_ASSERT(x) PROJ_CFG_ASSERT(x)
#else
#define PAR_CFG_DIRECT_ASSERT(x) \
    do                           \
    {                            \
        (void)(x);               \
    } while (0)
#endif
#endif
#define PAR_ASSERT(x) PAR_CFG_DIRECT_ASSERT(x)
#else
#define PAR_ASSERT(x) \
    {                 \
        ;             \
    }
#endif
#else
#if (1 == PAR_CFG_ASSERT_EN)
#define PAR_ASSERT(x) PAR_PORT_ASSERT(x)
#else
#define PAR_ASSERT(x) \
    {                 \
        ;             \
    }
#endif
#endif

#undef PAR_CFG_USE_PORT_HOOKS

/**
 * @brief Invalid configuration catcher.
 *
 * @note Shall be intact by end user!
 */
#if (0 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_TABLE_ID_CHECK_EN)
#error "Parameter settings invalid: Disable table ID checking (PAR_CFG_TABLE_ID_CHECK_EN)!"
#endif

/**
 * @brief Extended package configurations (non-template additions).
 */
#ifndef PAR_CFG_MUTEX_EN
#define PAR_CFG_MUTEX_EN (1)
#endif

/**
 * @brief Parameter mutex timeout.
 * @details Unit: ms.
 */
#ifndef PAR_CFG_MUTEX_TIMEOUT_MS
#define PAR_CFG_MUTEX_TIMEOUT_MS (10)
#endif

/**
 * @brief Enable/Disable port-specific par_if backend.
 */
#ifndef PAR_CFG_IF_PORT_EN
#define PAR_CFG_IF_PORT_EN (0)
#endif

/**
 * @brief Enable/Disable port hooks for log/assert.
 */
#ifndef PAR_CFG_PORT_HOOK_EN
#define PAR_CFG_PORT_HOOK_EN (0)
#endif

/**
 * @brief Parameter storage layout source.
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
#endif

/**
 * @brief Static layout include path.
 *
 * @note Can be overridden by integrator to include generated layout header.
 */
#ifndef PAR_CFG_LAYOUT_STATIC_INCLUDE
#define PAR_CFG_LAYOUT_STATIC_INCLUDE "par_layout_static.h"
#endif

#ifndef PAR_CFG_ENABLE_TYPE_F32
#define PAR_CFG_ENABLE_TYPE_F32 (1)
#endif

/**
 * @brief Enable/Disable runtime validation callbacks in normal setters.
 */
#ifndef PAR_CFG_ENABLE_RUNTIME_VALIDATION
#define PAR_CFG_ENABLE_RUNTIME_VALIDATION (1)
#endif

/**
 * @brief Enable/Disable on-change callbacks in normal setters.
 */
#ifndef PAR_CFG_ENABLE_CHANGE_CALLBACK
#define PAR_CFG_ENABLE_CHANGE_CALLBACK (1)
#endif

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
#endif

/**
 * @brief Enable/Disable parameter range metadata (min/max).
 */
#ifndef PAR_CFG_ENABLE_RANGE
#define PAR_CFG_ENABLE_RANGE (1)
#endif

/**
 * @brief Enable/Disable parameter name metadata.
 */
#ifndef PAR_CFG_ENABLE_NAME
#define PAR_CFG_ENABLE_NAME (1)
#endif

/**
 * @brief Enable/Disable parameter unit metadata.
 */
#ifndef PAR_CFG_ENABLE_UNIT
#define PAR_CFG_ENABLE_UNIT (1)
#endif

/**
 * @brief Enable/Disable parameter description metadata.
 */
#ifndef PAR_CFG_ENABLE_DESC
#define PAR_CFG_ENABLE_DESC (1)
#endif

/**
 * @brief Enable/Disable parameter ID metadata.
 */
#ifndef PAR_CFG_ENABLE_ID
#define PAR_CFG_ENABLE_ID (1)
#endif

#if (1 == PAR_CFG_ENABLE_ID)
/**
 * @brief Enable/Disable optional runtime duplicate-ID diagnostic scan.
 *
 * @note Static ID-map generation and compile-time duplicate-ID checking remain.
 * enabled by default whenever PAR_CFG_ENABLE_ID = 1.
 */
#ifndef PAR_CFG_ENABLE_RUNTIME_ID_DUP_CHECK
#define PAR_CFG_ENABLE_RUNTIME_ID_DUP_CHECK (0)
#endif

/**
 * @brief Enable/Disable optional runtime ID hash-collision diagnostic scan.
 *
 * @note Static ID-map generation and compile-time hash collision checking.
 * remain enabled by default whenever PAR_CFG_ENABLE_ID = 1.
 */
#ifndef PAR_CFG_ENABLE_RUNTIME_ID_HASH_COLLISION_CHECK
#define PAR_CFG_ENABLE_RUNTIME_ID_HASH_COLLISION_CHECK (0)
#endif

/**
 * @brief Internal-only ID hash geometry helpers.
 *
 * @note Shared by the compile-time table checks, compile-time static ID-map.
 * generation, and optional runtime diagnostic scans.
 *
 * @note These helpers define the internal geometry of the core ID lookup map.
 * Integrators should not treat them as a stable public extension API.
 */
#ifndef PAR_ID_HASH_GOLDEN_RATIO_32
#define PAR_ID_HASH_GOLDEN_RATIO_32 (0x61C88647u)
#endif

#ifndef PAR_ID_HASH_MIN_BUCKETS
#define PAR_ID_HASH_MIN_BUCKETS ((uint32_t)(2u * (uint32_t)ePAR_NUM_OF))
#endif

#ifndef PAR_ID_HASH_BITS_FROM_MIN_BUCKETS
#define PAR_ID_HASH_BITS_FROM_MIN_BUCKETS(min_buckets_)                       \
    (((min_buckets_) <= (1u << 1)) ? 1u : ((min_buckets_) <= (1u << 2)) ? 2u  \
                                      : ((min_buckets_) <= (1u << 3))   ? 3u  \
                                      : ((min_buckets_) <= (1u << 4))   ? 4u  \
                                      : ((min_buckets_) <= (1u << 5))   ? 5u  \
                                      : ((min_buckets_) <= (1u << 6))   ? 6u  \
                                      : ((min_buckets_) <= (1u << 7))   ? 7u  \
                                      : ((min_buckets_) <= (1u << 8))   ? 8u  \
                                      : ((min_buckets_) <= (1u << 9))   ? 9u  \
                                      : ((min_buckets_) <= (1u << 10))  ? 10u \
                                      : ((min_buckets_) <= (1u << 11))  ? 11u \
                                      : ((min_buckets_) <= (1u << 12))  ? 12u \
                                      : ((min_buckets_) <= (1u << 13))  ? 13u \
                                      : ((min_buckets_) <= (1u << 14))  ? 14u \
                                      : ((min_buckets_) <= (1u << 15))  ? 15u \
                                      : ((min_buckets_) <= (1u << 16))  ? 16u \
                                      : ((min_buckets_) <= (1u << 17))  ? 17u \
                                                                        : 18u)
#endif

#ifndef PAR_ID_HASH_BITS
#define PAR_ID_HASH_BITS PAR_ID_HASH_BITS_FROM_MIN_BUCKETS(PAR_ID_HASH_MIN_BUCKETS)
#endif

#ifndef PAR_ID_HASH_SIZE
#define PAR_ID_HASH_SIZE (1u << PAR_ID_HASH_BITS)
#endif

#ifndef PAR_HASH_ID_CONST
#define PAR_HASH_ID_CONST(id_) ((((uint32_t)(id_)) * PAR_ID_HASH_GOLDEN_RATIO_32) >> (32u - PAR_ID_HASH_BITS))
#endif

PAR_STATIC_ASSERT(par_id_hash_size_valid, (PAR_ID_HASH_SIZE >= PAR_ID_HASH_MIN_BUCKETS));
PAR_STATIC_ASSERT(par_id_hash_bits_valid, ((PAR_ID_HASH_BITS > 0u) && (PAR_ID_HASH_BITS < 32u)));
#endif

/**
 * @brief Enable/Disable parameter access metadata.
 */
#ifndef PAR_CFG_ENABLE_ACCESS
#define PAR_CFG_ENABLE_ACCESS (1)
#endif

/**
 * @brief Enable/Disable parameter persistence metadata.
 */
#ifndef PAR_CFG_ENABLE_PERSIST
#define PAR_CFG_ENABLE_PERSIST (1)
#endif

/**
 * @brief Enable/Disable description check.
 *
 * @note Default follows PAR_CFG_ENABLE_DESC.
 */
#ifndef PAR_CFG_ENABLE_DESC_CHECK
#define PAR_CFG_ENABLE_DESC_CHECK (PAR_CFG_ENABLE_DESC)
#endif

/**
 * @brief Configuration dependency checks for optional fields/features.
 */
#if (1 == PAR_CFG_NVM_EN) && (0 == PAR_CFG_ENABLE_ID)
#error "Parameter settings invalid: NVM requires PAR_CFG_ENABLE_ID = 1!"
#endif

#if (1 == PAR_CFG_NVM_EN) && (0 == PAR_CFG_ENABLE_PERSIST)
#error "Parameter settings invalid: NVM requires PAR_CFG_ENABLE_PERSIST = 1!"
#endif

#if (0 == PAR_CFG_ENABLE_ID) && (1 == PAR_CFG_ENABLE_RUNTIME_ID_DUP_CHECK)
#error "Parameter settings invalid: runtime duplicate-ID diagnostics require PAR_CFG_ENABLE_ID = 1!"
#endif

#if (0 == PAR_CFG_ENABLE_ID) && (1 == PAR_CFG_ENABLE_RUNTIME_ID_HASH_COLLISION_CHECK)
#error "Parameter settings invalid: runtime ID hash-collision diagnostics require PAR_CFG_ENABLE_ID = 1!"
#endif

#if (PAR_CFG_LAYOUT_SOURCE != PAR_CFG_LAYOUT_COMPILE_SCAN) && (PAR_CFG_LAYOUT_SOURCE != PAR_CFG_LAYOUT_SCRIPT)
#error "Parameter settings invalid: PAR_CFG_LAYOUT_SOURCE must be PAR_CFG_LAYOUT_COMPILE_SCAN or PAR_CFG_LAYOUT_SCRIPT!"
#endif

#define PAR_UINT16_MAX (65535u)
/**
 * @brief Function declarations.
 */
const par_cfg_t *par_cfg_get_table(void);
const par_cfg_t *par_cfg_get(const par_num_t par_num);
/**
 * @brief Return the number of configuration entries.
 * @return Configuration table size.
 */
uint32_t par_cfg_get_table_size(void); /**< @} <!-- END GROUP --> */

#endif /* _PAR_CFG_H_ */
