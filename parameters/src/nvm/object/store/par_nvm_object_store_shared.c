/**
 * @file par_nvm_object_store_shared.c
 * @brief Implement shared scalar/object persistence storage-target binding.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-05-01
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include "par_nvm_object_store.h"

#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_OBJECT_EN) && \
    (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED)

#include "par_if.h"

/**
 * @brief Active shared object storage backend alias.
 */
static const par_store_backend_api_t *gp_par_nvm_object_store = NULL;

/**
 * @brief Return whether this object store uses the scalar backend.
 *
 * @return true for this compiled storage-target adapter.
 */
bool par_nvm_object_store_uses_scalar_backend(void)
{
    return true;
}

/**
 * @brief Initialize shared object persistence storage.
 *
 * @param p_shared_store Active scalar storage backend to alias.
 * @return ePAR_OK on success, otherwise ePAR_ERROR_INIT.
 */
par_status_t par_nvm_object_store_init(const par_store_backend_api_t * const p_shared_store)
{
    if (NULL == p_shared_store)
    {
        return ePAR_ERROR_INIT;
    }

    gp_par_nvm_object_store = p_shared_store;
    return ePAR_OK;
}

/**
 * @brief Release the shared object storage alias.
 *
 * @return ePAR_OK after clearing the active alias.
 */
par_status_t par_nvm_object_store_deinit(void)
{
    gp_par_nvm_object_store = NULL;
    return ePAR_OK;
}

/**
 * @brief Return the active shared object storage backend.
 *
 * @return Active scalar backend alias, or NULL when not initialized.
 */
const par_store_backend_api_t *par_nvm_object_store_get_api(void)
{
    return gp_par_nvm_object_store;
}

#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) */
