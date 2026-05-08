/**
 * @file par_nvm_object_store_dedicated.c
 * @brief Implement dedicated object persistence storage-target binding.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-05-01
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include "par_nvm_object_store.h"

#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_OBJECT_EN) && \
    (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED)

#include "par_if.h"

/**
 * @brief Ownership guard for the dedicated object storage backend.
 */
static bool gb_par_nvm_object_store_owner = false;

/**
 * @brief Active dedicated object storage backend.
 */
static const par_store_backend_api_t *gp_par_nvm_object_store = NULL;

/**
 * @brief Bind or prepare the dedicated object-storage backend.
 *
 * @details The default weak implementation fails intentionally so integrators
 * cannot enable dedicated object storage without wiring the object media or
 * partition. A port that supports this mode shall provide a strong override.
 *
 * @return ePAR_ERROR_INIT because no dedicated object backend is wired by default.
 */
PAR_PORT_WEAK par_status_t par_object_store_backend_bind(void)
{
    return ePAR_ERROR_INIT;
}

/**
 * @brief Return the dedicated object-storage backend API.
 *
 * @details The default weak implementation returns NULL. Dedicated object
 * storage ports shall override this function and return a complete backend API
 * for the object media or partition.
 *
 * @return NULL in the default implementation.
 */
PAR_PORT_WEAK const par_store_backend_api_t *par_object_store_backend_get_api(void)
{
    return NULL;
}

/**
 * @brief Return whether this object store uses the scalar backend.
 *
 * @return false for this compiled storage-target adapter.
 */
bool par_nvm_object_store_uses_scalar_backend(void)
{
    return false;
}

/**
 * @brief Initialize dedicated object persistence storage.
 *
 * @param p_shared_store Unused scalar storage backend pointer.
 * @return ePAR_OK on success, otherwise an initialization error.
 */
par_status_t par_nvm_object_store_init(const par_store_backend_api_t * const p_shared_store)
{
    par_status_t status = ePAR_OK;
    bool is_nvm_init = false;

    (void)p_shared_store;

    PAR_DBG_PRINT("PAR_NVM: resolving dedicated object storage backend");
    status = par_object_store_backend_bind();
    if (ePAR_OK != status)
    {
        PAR_ERR_PRINT("PAR_NVM: object backend bind failed, err=%u", (unsigned)status);
        status = ePAR_ERROR_INIT;
    }

    if (ePAR_OK == status)
    {
        gp_par_nvm_object_store = par_object_store_backend_get_api();
    }

    if ((ePAR_OK == status) &&
        ((NULL == gp_par_nvm_object_store) || (NULL == gp_par_nvm_object_store->init) ||
         (NULL == gp_par_nvm_object_store->deinit) || (NULL == gp_par_nvm_object_store->is_init) ||
         (NULL == gp_par_nvm_object_store->read) || (NULL == gp_par_nvm_object_store->write) ||
         (NULL == gp_par_nvm_object_store->erase) || (NULL == gp_par_nvm_object_store->sync)))
    {
        PAR_ERR_PRINT("PAR_NVM: no valid dedicated object storage backend is wired");
        status = ePAR_ERROR_INIT;
    }

    if (ePAR_OK == status)
    {
        gp_par_nvm_object_store->is_init(&is_nvm_init);
    }

    if ((ePAR_OK == status) && (false == is_nvm_init))
    {
        const par_status_t store_status = gp_par_nvm_object_store->init();
        if (ePAR_OK != store_status)
        {
            status = ePAR_ERROR_INIT;
            PAR_ERR_PRINT("PAR_NVM: object backend init failed, err=%u", (unsigned)store_status);
        }
        else
        {
            gb_par_nvm_object_store_owner = true;
            PAR_DBG_PRINT("PAR_NVM: dedicated object backend initialized by object module");
        }
    }
    else if (ePAR_OK == status)
    {
        PAR_DBG_PRINT("PAR_NVM: reusing already-initialized dedicated object backend");
    }

    return status;
}

/**
 * @brief Deinitialize a module-owned dedicated object backend.
 *
 * @return ePAR_OK on success, otherwise ePAR_ERROR.
 */
par_status_t par_nvm_object_store_deinit(void)
{
    par_status_t status = ePAR_OK;

    if ((true == gb_par_nvm_object_store_owner) &&
        (NULL != gp_par_nvm_object_store) && (NULL != gp_par_nvm_object_store->deinit))
    {
        const par_status_t store_status = gp_par_nvm_object_store->deinit();
        if (ePAR_OK != store_status)
        {
            status = ePAR_ERROR;
            PAR_ERR_PRINT("PAR_NVM: object backend deinit failed, err=%u", (unsigned)store_status);
        }
    }

    gb_par_nvm_object_store_owner = false;
    gp_par_nvm_object_store = NULL;

    return status;
}

/**
 * @brief Return the active dedicated object storage backend.
 *
 * @return Active dedicated backend API, or NULL when not initialized.
 */
const par_store_backend_api_t *par_nvm_object_store_get_api(void)
{
    return gp_par_nvm_object_store;
}

#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED) */
