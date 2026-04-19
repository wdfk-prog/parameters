/**
 * @file par_store_backend_flash_ee_cfg.h
 * @brief Provide compile-time configuration defaults for the flash-ee backend.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-04-19
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-04-19 1.0     wdfk-prog     first version
 */
#ifndef _PAR_STORE_BACKEND_FLASH_EE_CFG_H_
#define _PAR_STORE_BACKEND_FLASH_EE_CFG_H_

#ifndef PAR_CFG_NVM_BACKEND_FLASH_EE_EN
#define PAR_CFG_NVM_BACKEND_FLASH_EE_EN (0)
#endif

#ifndef PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE
#define PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE (4096u)
#endif

#ifndef PAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE
#define PAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE (4096u)
#endif

#ifndef PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE
#define PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE (32u)
#endif

#ifndef PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE
#define PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE (8u)
#endif

#ifndef PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_FAL_EN
#define PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_FAL_EN (0)
#endif

#ifndef PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_NATIVE_EN
#define PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_NATIVE_EN (0)
#endif

#ifndef PAR_CFG_NVM_BACKEND_FLASH_EE_FAL_PARTITION_NAME
#define PAR_CFG_NVM_BACKEND_FLASH_EE_FAL_PARTITION_NAME "autogen_pm"
#endif

#ifndef PAR_CFG_NVM_BACKEND_FLASH_EE_VERSION
#define PAR_CFG_NVM_BACKEND_FLASH_EE_VERSION (1u)
#endif

#if (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN)
#if (0u == PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE)
#error "Parameter settings invalid: flash-ee line size must be greater than 0!"
#elif (0u == PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE)
#error "Parameter settings invalid: flash-ee logical size must be greater than 0!"
#elif (0u != (PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE % PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE))
#error "Parameter settings invalid: flash-ee logical size must be an integer multiple of the line size!"
#elif (0u == PAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE)
#error "Parameter settings invalid: flash-ee cache size must be greater than 0!"
#elif (0u != (PAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE % PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE))
#error "Parameter settings invalid: flash-ee cache size must be an integer multiple of the line size!"
#elif (0u == PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE)
#error "Parameter settings invalid: flash-ee program size must be greater than 0!"
#elif (0u != (64u % PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE))
#error "Parameter settings invalid: flash-ee program size must divide the 64-byte bank header exactly!"
#endif
#endif

#endif /* _PAR_STORE_BACKEND_FLASH_EE_CFG_H_ */
