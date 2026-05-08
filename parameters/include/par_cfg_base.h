/**
 * @file par_cfg_base.h
 * @brief Provide base platform, logging, assert, and storage configuration defaults.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#ifndef _PAR_CFG_BASE_H_
#define _PAR_CFG_BASE_H_

/**
 * @brief Include dependencies.
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief USER CODE BEGIN...
 */

/**
 * @brief Platform adaptation bridge.
 *
 * @note Object-type feature switches may be overridden from
 * `par_cfg_port.h`; otherwise par_cfg.h keeps them enabled by default.
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
 * @brief Include persisted-storage configuration defaults.
 *
 * @details Override NVM-related PAR_CFG_* macros from par_cfg_port.h before
 * this include, or keep the defaults provided by par_nvm_cfg.h.
 */
#include "par_nvm_cfg.h"

/**
 * @brief Enable diagnostic logging in the parameter module.
 *
 * @details Set to 1 while debugging or validating integration. Set to 0
 * for production builds that must avoid log formatting and output overhead.
 */
#ifndef PAR_CFG_DEBUG_EN
#define PAR_CFG_DEBUG_EN (1)
#ifndef DEBUG
#undef PAR_CFG_DEBUG_EN
#define PAR_CFG_DEBUG_EN (0)
#endif /* !defined(DEBUG) */
#endif /* !defined(PAR_CFG_DEBUG_EN) */

/**
 * @brief Enable PAR_ASSERT checks.
 *
 * @details Set to 1 during bring-up and validation. Set to 0 only when
 * the platform has a separate production fault-handling policy.
 */
#ifndef PAR_CFG_ASSERT_EN
#define PAR_CFG_ASSERT_EN (1)
#ifndef DEBUG
#undef PAR_CFG_ASSERT_EN
#define PAR_CFG_ASSERT_EN (0)
#endif /* !defined(DEBUG) */
#endif /* !defined(PAR_CFG_ASSERT_EN) */

/**
 * @brief Platform hook fallbacks.
 */
#ifndef PAR_PORT_ASSERT
#define PAR_PORT_ASSERT(x) \
    do                     \
    {                      \
        (void)(x);         \
    } while (0)
#endif /* !defined(PAR_PORT_ASSERT) */

#ifndef PAR_PORT_LOG_INFO
#define PAR_PORT_LOG_INFO(...) \
    do                         \
    {                          \
    } while (0)
#endif /* !defined(PAR_PORT_LOG_INFO) */

#ifndef PAR_PORT_LOG_DEBUG
#define PAR_PORT_LOG_DEBUG(...) \
    do                          \
    {                           \
    } while (0)
#endif /* !defined(PAR_PORT_LOG_DEBUG) */

#ifndef PAR_PORT_LOG_WARN
#define PAR_PORT_LOG_WARN(...) \
    do                         \
    {                          \
    } while (0)
#endif /* !defined(PAR_PORT_LOG_WARN) */

#ifndef PAR_PORT_LOG_ERROR
#define PAR_PORT_LOG_ERROR(...) \
    do                          \
    {                           \
    } while (0)
#endif /* !defined(PAR_PORT_LOG_ERROR) */

#ifndef PAR_PORT_STATIC_ASSERT
#define PAR_PORT_STATIC_ASSERT(name, expn) typedef char _static_assert_##name[(expn) ? 1 : -1]
#endif /* !defined(PAR_PORT_STATIC_ASSERT) */

/**
 * @brief Platform weak symbol macro.
 *
 * @note Integrator may override this macro (for example: RT_WEAK).
 */
#ifndef PAR_PORT_WEAK
#define PAR_PORT_WEAK __attribute__((weak))
#endif /* !defined(PAR_PORT_WEAK) */

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
#endif /* defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) */
#endif /* !defined(PAR_ALIGNOF) */

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
#endif /* !defined(PAR_CFG_PORT_HOOK_EN) */

/**
 * @brief Debug communication port macros.
 */
#if (0 == PAR_CFG_USE_PORT_HOOKS)
#if (1 == PAR_CFG_DEBUG_EN)
#ifndef PAR_CFG_DIRECT_LOG
#define PAR_CFG_DIRECT_LOG(...) (cli_printf((char *)__VA_ARGS__))
#endif /* !defined(PAR_CFG_DIRECT_LOG) */
#ifndef PAR_CFG_DIRECT_INFO_LOG
#define PAR_CFG_DIRECT_INFO_LOG(...) PAR_CFG_DIRECT_LOG(__VA_ARGS__)
#endif /* !defined(PAR_CFG_DIRECT_INFO_LOG) */
#ifndef PAR_CFG_DIRECT_DEBUG_LOG
#define PAR_CFG_DIRECT_DEBUG_LOG(...) PAR_CFG_DIRECT_LOG(__VA_ARGS__)
#endif /* !defined(PAR_CFG_DIRECT_DEBUG_LOG) */
#ifndef PAR_CFG_DIRECT_WARN_LOG
#define PAR_CFG_DIRECT_WARN_LOG(...) PAR_CFG_DIRECT_LOG(__VA_ARGS__)
#endif /* !defined(PAR_CFG_DIRECT_WARN_LOG) */
#ifndef PAR_CFG_DIRECT_ERROR_LOG
#define PAR_CFG_DIRECT_ERROR_LOG(...) PAR_CFG_DIRECT_LOG(__VA_ARGS__)
#endif /* !defined(PAR_CFG_DIRECT_ERROR_LOG) */
#define PAR_INFO_PRINT(...) PAR_CFG_DIRECT_INFO_LOG(__VA_ARGS__)
#define PAR_DBG_PRINT(...)  PAR_CFG_DIRECT_DEBUG_LOG(__VA_ARGS__)
#define PAR_WARN_PRINT(...) PAR_CFG_DIRECT_WARN_LOG(__VA_ARGS__)
#define PAR_ERR_PRINT(...)  PAR_CFG_DIRECT_ERROR_LOG(__VA_ARGS__)
#else
#define PAR_INFO_PRINT(...) \
    {                       \
        ;                   \
    }
#define PAR_DBG_PRINT(...) \
    {                      \
        ;                  \
    }
#define PAR_WARN_PRINT(...) \
    {                       \
        ;                   \
    }
#define PAR_ERR_PRINT(...) \
    {                      \
        ;                  \
    }
#endif /* (1 == PAR_CFG_DEBUG_EN) */
#else
#if (1 == PAR_CFG_DEBUG_EN)
#define PAR_INFO_PRINT(...) PAR_PORT_LOG_INFO(__VA_ARGS__)
#define PAR_DBG_PRINT(...)  PAR_PORT_LOG_DEBUG(__VA_ARGS__)
#define PAR_WARN_PRINT(...) PAR_PORT_LOG_WARN(__VA_ARGS__)
#define PAR_ERR_PRINT(...)  PAR_PORT_LOG_ERROR(__VA_ARGS__)
#else
#define PAR_INFO_PRINT(...) \
    {                       \
        ;                   \
    }
#define PAR_DBG_PRINT(...) \
    {                      \
        ;                  \
    }
#define PAR_WARN_PRINT(...) \
    {                       \
        ;                   \
    }
#define PAR_ERR_PRINT(...) \
    {                      \
        ;                  \
    }
#endif /* (1 == PAR_CFG_DEBUG_EN) */
#endif /* (0 == PAR_CFG_USE_PORT_HOOKS) */

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
#endif /* defined(PROJ_CFG_ASSERT) */
#endif /* !defined(PAR_CFG_DIRECT_ASSERT) */
#define PAR_ASSERT(x) PAR_CFG_DIRECT_ASSERT(x)
#else
#define PAR_ASSERT(x) \
    {                 \
        ;             \
    }
#endif /* (1 == PAR_CFG_ASSERT_EN) */
#else
#if (1 == PAR_CFG_ASSERT_EN)
#define PAR_ASSERT(x) PAR_PORT_ASSERT(x)
#else
#define PAR_ASSERT(x) \
    {                 \
        ;             \
    }
#endif /* (1 == PAR_CFG_ASSERT_EN) */
#endif /* (0 == PAR_CFG_USE_PORT_HOOKS) */

#undef PAR_CFG_USE_PORT_HOOKS

#endif /* !defined(_PAR_CFG_BASE_H_) */
