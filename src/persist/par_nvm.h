/**
 * @file par_nvm.h
 * @brief Declare non-volatile storage support for parameters.
 * @author Ziga Miklosic
 * @version V3.0.1
 * @date 2026-01-29
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-01-29 V3.0.1  Ziga Miklosic
 */

/**
 * @addtogroup PAR_NVM
 * @{ <!-- BEGIN GROUP -->
 */

#ifndef _PAR_NVM_H_
#define _PAR_NVM_H_
/**
 * @brief Include dependencies.
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "par.h"

#if (1 == PAR_CFG_NVM_EN)
/**
 * @brief Function declarations.
 */
/**
 * @brief Initialize parameter non-volatile storage support.
 * @return Operation status.
 */
par_status_t par_nvm_init(void);
/**
 * @brief Deinitialize parameter non-volatile storage support.
 * @return Operation status.
 */
par_status_t par_nvm_deinit(void);
/**
 * @brief Persist one parameter to non-volatile storage.
 * @param par_num Parameter number.
 * @param nvm_sync Synchronize the underlying NVM device after the write when true.
 * @return Operation status.
 */
par_status_t par_nvm_write(const par_num_t par_num, const bool nvm_sync);
/**
 * @brief Persist all persistent parameters to non-volatile storage.
 * @return Operation status.
 */
par_status_t par_nvm_write_all(void);
/**
 * @brief Erase all parameter data managed by the NVM backend.
 * @return Operation status.
 */
par_status_t par_nvm_reset_all(void);
/**
 * @brief Print the internal NVM lookup table for diagnostics.
 * @return Operation status.
 */
par_status_t par_nvm_print_nvm_lut(void);

#endif /* 1 == PAR_CFG_NVM_EN */
/**
 * @} <!-- END GROUP -->
 */

#endif /* _PAR_NVM_H_ */
