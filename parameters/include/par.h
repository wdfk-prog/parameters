/**
 * @file par.h
 * @brief Aggregate the public device-parameter API headers.
 * @author Ziga Miklosic
 * @version V3.0.1
 * @date 2026-01-29
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-01-29 V3.0.1  Ziga Miklosic first version
 */
/**
 * @addtogroup PARAMETERS_API
 * @{ <!-- BEGIN GROUP -->
 */

#ifndef _PAR_H_
#define _PAR_H_

#include "par_types.h"
#include "par_core_api.h"
#include "par_scalar_api.h"
#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
#include "par_object_api.h"
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */
#include "par_meta_api.h"
#if (1 == PAR_CFG_NVM_EN)
#include "par_nvm_api.h"
#endif /* (1 == PAR_CFG_NVM_EN) */
#include "par_registration_api.h"

/**
 * @} <!-- END GROUP -->
 */

#endif /* !defined(_PAR_H_) */
