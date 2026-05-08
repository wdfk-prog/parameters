/**
 * @file par_nvm_scalar_store.h
 * @brief Declare scalar NVM backend and layout binding helpers.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-05-01
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#ifndef _PAR_NVM_SCALAR_STORE_H_
#define _PAR_NVM_SCALAR_STORE_H_

#include <stdbool.h>

#include "par_store_backend.h"
#include "par_nvm_layout.h"

#if (1 == PAR_CFG_NVM_EN)
/**
 * @brief Resolve and validate the selected scalar persisted-record layout.
 *
 * @details Call this only when at least one scalar persistent record is active.
 * Object-only dedicated persistence does not require a scalar layout adapter.
 *
 * @param[out] pp_layout Receives the selected scalar record-layout adapter.
 * @return Operation status.
 */
par_status_t par_nvm_scalar_layout_bind(const par_nvm_layout_api_t ** const pp_layout);

/**
 * @brief Resolve, validate, and initialize the scalar storage backend.
 *
 * @details Shared object persistence may use this backend even when scalar
 * parameter persistence itself is disabled. The caller owns the decision about
 * whether scalar records or shared object placement require scalar storage.
 *
 * @param[out] pp_store Receives the active scalar storage backend API.
 * @param[out] p_is_owner Receives whether this module initialized the backend.
 * @return Operation status.
 */
par_status_t par_nvm_scalar_store_init(const par_store_backend_api_t ** const pp_store,
                                       bool * const p_is_owner);

/**
 * @brief Deinitialize a module-owned scalar storage backend.
 *
 * @param p_store Active scalar storage backend API.
 * @param is_owner Whether this module initialized the backend.
 * @param p_context Log context used when deinitialization fails.
 * @return Operation status.
 */
par_status_t par_nvm_scalar_store_deinit(const par_store_backend_api_t * const p_store,
                                         const bool is_owner,
                                         const char * const p_context);
#endif /* (1 == PAR_CFG_NVM_EN) */

#endif /* !defined(_PAR_NVM_SCALAR_STORE_H_) */
