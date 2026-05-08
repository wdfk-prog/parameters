/**
 * @file par_store_backend_gel_nvm.c
 * @brief Adapt GeneralEmbeddedCLibraries/nvm to the packaged parameter-storage backend interface.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-03-29
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 *  @details This adapter is optional. Enable it only when the project includes
 * the GeneralEmbeddedCLibraries/nvm module and a valid PAR_CFG_NVM_REGION is
 * configured for parameter storage.
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-03-29 1.0     wdfk-prog     first version
 */
#include "par_cfg.h"
#include "par_store_backend.h"

#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_GEL_EN)
#include "middleware/nvm/nvm/src/nvm.h"

/**
 * @brief Verify the expected upstream NVM major/minor API.
 */
PAR_STATIC_ASSERT(gel_nvm_major_version_supported, (2 == NVM_VER_MAJOR));
PAR_STATIC_ASSERT(gel_nvm_minor_version_supported, (1 <= NVM_VER_MINOR));

/**
 * @brief Initialize the GeneralEmbeddedCLibraries NVM backend.
 *
 * @return ePAR_OK on success, otherwise an initialization error.
 */
static par_status_t par_store_gel_init(void)
{
    return (eNVM_OK == nvm_init()) ? ePAR_OK : ePAR_ERROR_INIT;
}

/**
 * @brief Deinitialize the GeneralEmbeddedCLibraries NVM backend.
 *
 * @return ePAR_OK on success, otherwise a backend error.
 */
static par_status_t par_store_gel_deinit(void)
{
    return (eNVM_OK == nvm_deinit()) ? ePAR_OK : ePAR_ERROR;
}

/**
 * @brief Report whether the GeneralEmbeddedCLibraries NVM backend is initialized.
 *
 * @param[out] p_is_init Receives the initialization state.
 */
static void par_store_gel_is_init(bool * const p_is_init)
{
    bool is_init = false;

    if (NULL == p_is_init)
    {
        return;
    }

    if (eNVM_OK == nvm_is_init(&is_init))
    {
        *p_is_init = is_init;
    }
    else
    {
        *p_is_init = false;
    }
}

/**
 * @brief Read raw bytes from the configured NVM region.
 *
 * @param[in] addr Byte offset inside the parameter NVM region.
 * @param[in] size Number of bytes to read.
 * @param[out] p_buf Destination buffer.
 * @return ePAR_OK on success, otherwise an NVM error.
 */
static par_status_t par_store_gel_read(const uint32_t addr, const uint32_t size, uint8_t * const p_buf)
{
    if (NULL == p_buf)
    {
        return ePAR_ERROR_PARAM;
    }

    return (eNVM_OK == nvm_read(PAR_CFG_NVM_REGION, addr, size, p_buf)) ? ePAR_OK : ePAR_ERROR_NVM;
}

/**
 * @brief Write raw bytes into the configured NVM region.
 *
 * @param[in] addr Byte offset inside the parameter NVM region.
 * @param[in] size Number of bytes to write.
 * @param[in] p_buf Source buffer.
 * @return ePAR_OK on success, otherwise an NVM error.
 */
static par_status_t par_store_gel_write(const uint32_t addr, const uint32_t size, const uint8_t * const p_buf)
{
    if (NULL == p_buf)
    {
        return ePAR_ERROR_PARAM;
    }

    return (eNVM_OK == nvm_write(PAR_CFG_NVM_REGION, addr, size, p_buf)) ? ePAR_OK : ePAR_ERROR_NVM;
}

/**
 * @brief Erase bytes inside the configured NVM region.
 *
 * @param[in] addr Byte offset inside the parameter NVM region.
 * @param[in] size Number of bytes to erase.
 * @return ePAR_OK on success, otherwise an NVM error.
 */
static par_status_t par_store_gel_erase(const uint32_t addr, const uint32_t size)
{
    return (eNVM_OK == nvm_erase(PAR_CFG_NVM_REGION, addr, size)) ? ePAR_OK : ePAR_ERROR_NVM;
}

/**
 * @brief Flush pending writes inside the configured NVM region.
 *
 * @return ePAR_OK on success, otherwise an NVM error.
 */
static par_status_t par_store_gel_sync(void)
{
    return (eNVM_OK == nvm_sync(PAR_CFG_NVM_REGION)) ? ePAR_OK : ePAR_ERROR_NVM;
}

/**
 * @brief Prepare the GeneralEmbeddedCLibraries adapter before API resolution.
 *
 * @details This backend has no extra bind step, so the function is a no-op.
 *
 * @return ePAR_OK.
 */
par_status_t par_store_backend_bind(void)
{
    return ePAR_OK;
}

/**
 * @brief Concrete GeneralEmbeddedCLibraries NVM backend API table.
 */
static const par_store_backend_api_t g_par_store_backend_gel =
{
    .init = par_store_gel_init,
    .deinit = par_store_gel_deinit,
    .is_init = par_store_gel_is_init,
    .read = par_store_gel_read,
    .write = par_store_gel_write,
    .erase = par_store_gel_erase,
    .sync = par_store_gel_sync,
    .name = "gel_nvm",
};

/**
 * @brief Return the GeneralEmbeddedCLibraries NVM backend API.
 *
 * @return Backend API table.
 */
const par_store_backend_api_t * par_store_backend_get_api(void)
{
    return &g_par_store_backend_gel;
}
#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_GEL_EN) */
