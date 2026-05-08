/**
 * @file par_nvm_scalar_store.c
 * @brief Implement scalar NVM backend and layout binding helpers.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-05-01
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include "par_nvm_scalar_store.h"

#if (1 == PAR_CFG_NVM_EN)

#include "par_cfg.h"
#include "par_if.h"

/**
 * @brief Bind or prepare the active parameter-storage backend.
 *
 * @details This weak default keeps existing backends source-compatible when
 * they do not need an explicit pre-bind step. Backends that must attach one
 * concrete port or device context should provide a strong override.
 *
 * @return ePAR_OK because the default implementation has no work to perform.
 */
PAR_PORT_WEAK par_status_t par_store_backend_bind(void)
{
    return ePAR_OK;
}

/**
 * @brief Return the active scalar parameter-storage backend API.
 *
 * @details The default weak implementation returns NULL so builds that use
 * only a dedicated object backend do not need to link a scalar backend port.
 * Scalar-backed configurations shall provide a strong override.
 *
 * @return NULL in the default implementation.
 */
PAR_PORT_WEAK const par_store_backend_api_t *par_store_backend_get_api(void)
{
    return NULL;
}

/**
 * @brief Return whether a scalar persisted-record layout adapter is complete.
 *
 * @param p_layout Candidate layout adapter.
 * @return true when all required scalar layout callbacks are present.
 */
static bool par_nvm_scalar_layout_api_is_valid(const par_nvm_layout_api_t * const p_layout)
{
    return ((NULL != p_layout) && (NULL != p_layout->record_size_from_par_num) &&
            (NULL != p_layout->addr_from_persist_idx) && (NULL != p_layout->populate_data_obj) &&
            (NULL != p_layout->read) && (NULL != p_layout->write) &&
            (NULL != p_layout->validate_loaded_obj) && (NULL != p_layout->get_error_stored_id) &&
            (NULL != p_layout->check_compat)
#if (1 == PAR_CFG_NVM_WRITE_VERIFY_EN)
            && (NULL != p_layout->data_obj_matches)
#endif /* (1 == PAR_CFG_NVM_WRITE_VERIFY_EN) */
           );
}

/**
 * @brief Return whether a scalar storage backend API is complete.
 *
 * @param p_store Candidate scalar storage backend API.
 * @return true when all required scalar backend callbacks are present.
 */
static bool par_nvm_scalar_store_api_is_valid(const par_store_backend_api_t * const p_store)
{
    return ((NULL != p_store) && (NULL != p_store->init) && (NULL != p_store->deinit) &&
            (NULL != p_store->is_init) && (NULL != p_store->read) &&
            (NULL != p_store->write) && (NULL != p_store->erase) && (NULL != p_store->sync));
}

/**
 * @brief Resolve and validate the selected scalar persisted-record layout.
 *
 * @param[out] pp_layout Receives the selected scalar record-layout adapter.
 * @return Operation status.
 */
par_status_t par_nvm_scalar_layout_bind(const par_nvm_layout_api_t ** const pp_layout)
{
    PAR_ASSERT(NULL != pp_layout);
    if (NULL == pp_layout)
    {
        return ePAR_ERROR_PARAM;
    }

#if (1 == PAR_CFG_NVM_SCALAR_EN)
    *pp_layout = par_nvm_layout_init();
    if (false == par_nvm_scalar_layout_api_is_valid(*pp_layout))
    {
        *pp_layout = NULL;
        PAR_ERR_PRINT("PAR_NVM: no valid persisted-record layout adapter is wired");
        return ePAR_ERROR_INIT;
    }

    return ePAR_OK;
#else
    *pp_layout = NULL;
    return ePAR_ERROR_INIT;
#endif /* (1 == PAR_CFG_NVM_SCALAR_EN) */
}

/**
 * @brief Resolve, validate, and initialize the scalar storage backend.
 *
 * @param[out] pp_store Receives the active scalar storage backend API.
 * @param[out] p_is_owner Receives whether this module initialized the backend.
 * @return Operation status.
 */
par_status_t par_nvm_scalar_store_init(const par_store_backend_api_t ** const pp_store,
                                       bool * const p_is_owner)
{
    par_status_t status = ePAR_OK;
    bool is_nvm_init = false;

    PAR_ASSERT((NULL != pp_store) && (NULL != p_is_owner));
    if ((NULL == pp_store) || (NULL == p_is_owner))
    {
        return ePAR_ERROR_PARAM;
    }

    *pp_store = NULL;
    *p_is_owner = false;

    PAR_DBG_PRINT("PAR_NVM: resolving scalar storage backend");
    status = par_store_backend_bind();
    if (ePAR_OK != status)
    {
        PAR_ERR_PRINT("PAR_NVM: scalar backend bind failed, err=%u", (unsigned)status);
        status = ePAR_ERROR_INIT;
    }

    if (ePAR_OK == status)
    {
        *pp_store = par_store_backend_get_api();
    }

    if ((ePAR_OK == status) && (false == par_nvm_scalar_store_api_is_valid(*pp_store)))
    {
        PAR_ERR_PRINT("PAR_NVM: no valid scalar parameter storage backend is wired");
        status = ePAR_ERROR_INIT;
    }

    if (ePAR_OK == status)
    {
        (*pp_store)->is_init(&is_nvm_init);
    }

    if ((ePAR_OK == status) && (false == is_nvm_init))
    {
        const par_status_t store_status = (*pp_store)->init();
        if (ePAR_OK != store_status)
        {
            status = ePAR_ERROR_INIT;
            PAR_ERR_PRINT("PAR_NVM: scalar backend init failed, err=%u", (unsigned)store_status);
        }
        else
        {
            *p_is_owner = true;
            PAR_DBG_PRINT("PAR_NVM: scalar backend initialized by parameter module");
        }
    }
    else if (ePAR_OK == status)
    {
        PAR_DBG_PRINT("PAR_NVM: reusing already-initialized scalar storage backend");
    }

    if (ePAR_OK != status)
    {
        *pp_store = NULL;
        *p_is_owner = false;
    }

    return status;
}

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
                                         const char * const p_context)
{
    par_status_t status = ePAR_OK;

    if ((true == is_owner) && (NULL != p_store) && (NULL != p_store->deinit))
    {
        const par_status_t store_status = p_store->deinit();
        if (ePAR_OK != store_status)
        {
            status = ePAR_ERROR;
            PAR_ERR_PRINT("PAR_NVM: scalar backend deinit failed during %s, err=%u",
                          (NULL != p_context) ? p_context : "cleanup",
                          (unsigned)store_status);
        }
    }

    return status;
}

#endif /* (1 == PAR_CFG_NVM_EN) */
