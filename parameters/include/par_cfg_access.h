/**
 * @file par_cfg_access.h
 * @brief Provide parameter access, role-policy, and description-check defaults.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#ifndef _PAR_CFG_ACCESS_H_
#define _PAR_CFG_ACCESS_H_

#include "par_cfg_features.h"
/**
 * @brief Enable/Disable parameter access metadata.
 */
#ifndef PAR_CFG_ENABLE_ACCESS
#define PAR_CFG_ENABLE_ACCESS (1)
#endif /* !defined(PAR_CFG_ENABLE_ACCESS) */

/**
 * @brief Enable/Disable optional external role-policy metadata and enforcement.
 */
#ifndef PAR_CFG_ENABLE_ROLE_POLICY
#define PAR_CFG_ENABLE_ROLE_POLICY (0)
#endif /* !defined(PAR_CFG_ENABLE_ROLE_POLICY) */

/**
 * @brief Enable/Disable description check.
 *
 * @note Default follows PAR_CFG_ENABLE_DESC.
 */
#ifndef PAR_CFG_ENABLE_DESC_CHECK
#define PAR_CFG_ENABLE_DESC_CHECK (PAR_CFG_ENABLE_DESC)
#endif /* !defined(PAR_CFG_ENABLE_DESC_CHECK) */

#endif /* !defined(_PAR_CFG_ACCESS_H_) */
