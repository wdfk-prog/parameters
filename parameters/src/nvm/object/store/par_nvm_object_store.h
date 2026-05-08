/**
 * @file par_nvm_object_store.h
 * @brief Declare object persistence storage-target adapter APIs.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-05-01
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#ifndef _PAR_NVM_OBJECT_STORE_H_
#define _PAR_NVM_OBJECT_STORE_H_

#include <stdbool.h>

#include "par.h"
#include "par_store_backend.h"

#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
/**
 * @brief Return whether the selected object store needs the scalar backend.
 *
 * @return true when object records share the scalar backend address space.
 */
bool par_nvm_object_store_uses_scalar_backend(void);

/**
 * @brief Initialize the selected object persistence storage target.
 *
 * @param p_shared_store Active scalar backend used only by shared-store mode.
 * @return Status of operation.
 */
par_status_t par_nvm_object_store_init(const par_store_backend_api_t * const p_shared_store);

/**
 * @brief Deinitialize the selected object persistence storage target.
 *
 * @return Status of operation.
 */
par_status_t par_nvm_object_store_deinit(void);

/**
 * @brief Return the active object storage backend.
 *
 * @return Active object backend API pointer, or NULL when object storage has not been initialized.
 */
const par_store_backend_api_t *par_nvm_object_store_get_api(void);
#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

#endif /* !defined(_PAR_NVM_OBJECT_STORE_H_) */
