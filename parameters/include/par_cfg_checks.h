/**
 * @file par_cfg_checks.h
 * @brief Provide compile-time configuration dependency checks.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#ifndef _PAR_CFG_CHECKS_H_
#define _PAR_CFG_CHECKS_H_

#include "par_cfg_access.h"
#include "par_def.h"
/**
 * @brief Configuration dependency checks for optional fields/features.
 */
#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_SCALAR_EN) &&                       \
    (0 == PAR_CFG_ENABLE_ID) &&                                                    \
    (PAR_CFG_NVM_RECORD_LAYOUT != PAR_CFG_NVM_RECORD_LAYOUT_FIXED_PAYLOAD_ONLY) && \
    (PAR_CFG_NVM_RECORD_LAYOUT != PAR_CFG_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY)
#error "Parameter settings invalid: selected NVM layout requires PAR_CFG_ENABLE_ID = 1!"
#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_SCALAR_EN) && (0 == PAR_CFG_ENABLE_ID) && (PAR_CFG_NVM_RECORD_LAYOUT != PAR_CFG_NVM_RECORD_LAYOUT_FIXED_PAYLOAD_ONLY) && (PAR_CFG_NVM_RECORD_LAYOUT != PAR_CFG_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY) */

#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_SCALAR_EN) &&                           \
    ((PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_PAYLOAD_ONLY) ||    \
     (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY)) && \
    (0 == PAR_CFG_TABLE_ID_CHECK_EN)
#error "Parameter settings invalid: payload-only NVM layouts require PAR_CFG_TABLE_ID_CHECK_EN = 1!"
#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_SCALAR_EN) && ((PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_PAYLOAD_ONLY) || (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY)) && (0 == PAR_CFG_TABLE_ID_CHECK_EN) */

#if (1 == PAR_CFG_NVM_SCALAR_EN) && (0 == PAR_CFG_NVM_EN)
#error "Parameter settings invalid: scalar persistence requires PAR_CFG_NVM_EN = 1!"
#endif /* (1 == PAR_CFG_NVM_SCALAR_EN) && (0 == PAR_CFG_NVM_EN) */

#if (1 == PAR_CFG_NVM_EN) && (0 == PAR_CFG_NVM_SCALAR_EN) && \
    (0 == PAR_CFG_NVM_OBJECT_EN)
#error "Parameter settings invalid: NVM requires scalar or object persistence to be enabled!"
#endif /* (1 == PAR_CFG_NVM_EN) && (0 == PAR_CFG_NVM_SCALAR_EN) && (0 == PAR_CFG_NVM_OBJECT_EN) */

#if (1 == PAR_CFG_NVM_OBJECT_EN) && (0 == PAR_CFG_NVM_EN)
#error "Parameter settings invalid: object persistence requires PAR_CFG_NVM_EN = 1!"
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (0 == PAR_CFG_NVM_EN) */

#if (1 == PAR_CFG_NVM_OBJECT_EN) && (0 == PAR_CFG_OBJECT_TYPES_ENABLED)
#error "Parameter settings invalid: object persistence requires at least one object type enabled!"
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (0 == PAR_CFG_OBJECT_TYPES_ENABLED) */

#if (1 == PAR_CFG_NVM_OBJECT_EN) && (0 == PAR_CFG_ENABLE_ID)
#error "Parameter settings invalid: object persistence requires PAR_CFG_ENABLE_ID = 1!"
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (0 == PAR_CFG_ENABLE_ID) */

/**
 * @brief Reject removed object persistence migration configuration.
 *
 * @details Generic object block migration is intentionally unsupported because
 * flash-safe relocation requires product-specific transaction storage,
 * power-loss recovery, and erase-block planning. Use fixed or dedicated object
 * placement for released products that must keep object values stable.
 */
#if defined(PAR_CFG_NVM_OBJECT_MIGRATION_EN) || \
    defined(PAR_CFG_NVM_OBJECT_MIGRATION_SRC_ADDR)
#error "Object persistence migration is not supported; use fixed or dedicated placement!"
#endif /* defined(PAR_CFG_NVM_OBJECT_MIGRATION_EN) || defined(PAR_CFG_NVM_OBJECT_MIGRATION_SRC_ADDR) */

#if (1 == PAR_CFG_NVM_OBJECT_EN) &&                                       \
    (PAR_CFG_NVM_OBJECT_STORE_MODE != PAR_CFG_NVM_OBJECT_STORE_SHARED) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE != PAR_CFG_NVM_OBJECT_STORE_DEDICATED)
#error "Parameter settings invalid: PAR_CFG_NVM_OBJECT_STORE_MODE must be SHARED or DEDICATED!"
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (PAR_CFG_NVM_OBJECT_STORE_MODE != PAR_CFG_NVM_OBJECT_STORE_SHARED) && (PAR_CFG_NVM_OBJECT_STORE_MODE != PAR_CFG_NVM_OBJECT_STORE_DEDICATED) */

#if (1 == PAR_CFG_NVM_OBJECT_EN) &&                                           \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) &&     \
    (PAR_CFG_NVM_OBJECT_ADDR_MODE != PAR_CFG_NVM_OBJECT_ADDR_AFTER_SCALAR) && \
    (PAR_CFG_NVM_OBJECT_ADDR_MODE != PAR_CFG_NVM_OBJECT_ADDR_FIXED)
#error "Parameter settings invalid: PAR_CFG_NVM_OBJECT_ADDR_MODE must be AFTER_SCALAR or FIXED!"
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) && (PAR_CFG_NVM_OBJECT_ADDR_MODE != PAR_CFG_NVM_OBJECT_ADDR_AFTER_SCALAR) && (PAR_CFG_NVM_OBJECT_ADDR_MODE != PAR_CFG_NVM_OBJECT_ADDR_FIXED) */

#if (1 == PAR_CFG_NVM_OBJECT_EN) &&                                       \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) && \
    (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_FIXED) &&    \
    (0U == PAR_CFG_NVM_OBJECT_FIXED_ADDR)
#error "Parameter settings invalid: fixed object persistence mode requires PAR_CFG_NVM_OBJECT_FIXED_ADDR != 0!"
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) && (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_FIXED) && (0U == PAR_CFG_NVM_OBJECT_FIXED_ADDR) */

#if (0 == PAR_CFG_ENABLE_ID) && (1 == PAR_CFG_ENABLE_RUNTIME_ID_DUP_CHECK)
#error "Parameter settings invalid: runtime duplicate-ID diagnostics require PAR_CFG_ENABLE_ID = 1!"
#endif /* (0 == PAR_CFG_ENABLE_ID) && (1 == PAR_CFG_ENABLE_RUNTIME_ID_DUP_CHECK) */

#if (0 == PAR_CFG_ENABLE_ID) && (1 == PAR_CFG_ENABLE_RUNTIME_ID_HASH_COLLISION_CHECK)
#error "Parameter settings invalid: runtime ID hash-collision diagnostics require PAR_CFG_ENABLE_ID = 1!"
#endif /* (0 == PAR_CFG_ENABLE_ID) && (1 == PAR_CFG_ENABLE_RUNTIME_ID_HASH_COLLISION_CHECK) */

#if (PAR_CFG_LAYOUT_SOURCE != PAR_CFG_LAYOUT_COMPILE_SCAN) && (PAR_CFG_LAYOUT_SOURCE != PAR_CFG_LAYOUT_SCRIPT)
#error "Parameter settings invalid: PAR_CFG_LAYOUT_SOURCE must be PAR_CFG_LAYOUT_COMPILE_SCAN or PAR_CFG_LAYOUT_SCRIPT!"
#endif /* (PAR_CFG_LAYOUT_SOURCE != PAR_CFG_LAYOUT_COMPILE_SCAN) && (PAR_CFG_LAYOUT_SOURCE != PAR_CFG_LAYOUT_SCRIPT) */

#define PAR_UINT16_MAX (65535u)

#endif /* !defined(_PAR_CFG_CHECKS_H_) */
