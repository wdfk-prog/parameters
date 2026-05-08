/**
 * @file par_store_backend_flash_ee_fal.c
 * @brief Bind the generic flash-emulated EEPROM core to a FAL partition.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-04-14
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-04-14 1.0     wdfk-prog     first version
 */
#include "par_cfg.h"
#include "par_store_backend_flash_ee.h"

#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_FAL_EN)

#include <string.h>

#include <fal.h>

/**
 * @brief Hold the runtime context for the FAL flash-ee adapter.
 */
typedef struct
{
    const struct fal_partition *p_part;   /**< Bound FAL partition handle. */
    const struct fal_flash_dev *p_flash;  /**< Underlying flash device for the bound partition. */
    const char *partition_name;           /**< Configured FAL partition name. */
    bool is_init;                         /**< True after the adapter resolves the partition. */
} par_store_flash_ee_fal_ctx_t;

/**
 * @brief Store the singleton FAL adapter context.
 */
static par_store_flash_ee_fal_ctx_t g_par_store_flash_ee_fal_ctx = {
    .p_part = NULL,
    .p_flash = NULL,
    .partition_name = PAR_CFG_NVM_BACKEND_FLASH_EE_FAL_PARTITION_NAME,
    .is_init = false,
};

/**
 * @brief Initialize the FAL-backed flash port.
 *
 * @param[in,out] p_ctx Mutable FAL port context.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_fal_init(void *p_ctx)
{
    par_store_flash_ee_fal_ctx_t *p_fal_ctx = (par_store_flash_ee_fal_ctx_t *)p_ctx;

    if (NULL == p_fal_ctx)
    {
        return ePAR_ERROR_PARAM;
    }

    if (true == p_fal_ctx->is_init)
    {
        return ePAR_OK;
    }

    p_fal_ctx->p_part = fal_partition_find(p_fal_ctx->partition_name);
    if (NULL == p_fal_ctx->p_part)
    {
        PAR_ERR_PRINT("PAR_FLASH_EE_FAL: partition not found, name=%s", p_fal_ctx->partition_name);
        return ePAR_ERROR_INIT;
    }

    p_fal_ctx->p_flash = fal_flash_device_find(p_fal_ctx->p_part->flash_name);
    if (NULL == p_fal_ctx->p_flash)
    {
        PAR_ERR_PRINT("PAR_FLASH_EE_FAL: flash device not found, partition=%s flash=%s",
                      p_fal_ctx->partition_name,
                      p_fal_ctx->p_part->flash_name);
        p_fal_ctx->p_part = NULL;
        return ePAR_ERROR_INIT;
    }

    p_fal_ctx->is_init = true;
    return ePAR_OK;
}

/**
 * @brief Deinitialize the FAL-backed flash port.
 *
 * @param[in,out] p_ctx Mutable FAL port context.
 * @return ePAR_OK on success.
 */
static par_status_t par_store_flash_ee_fal_deinit(void *p_ctx)
{
    par_store_flash_ee_fal_ctx_t *p_fal_ctx = (par_store_flash_ee_fal_ctx_t *)p_ctx;

    if (NULL == p_fal_ctx)
    {
        return ePAR_ERROR_PARAM;
    }

    p_fal_ctx->p_part = NULL;
    p_fal_ctx->p_flash = NULL;
    p_fal_ctx->is_init = false;
    return ePAR_OK;
}

/**
 * @brief Report whether the FAL-backed flash port is initialized.
 *
 * @param[in] p_ctx Immutable FAL port context.
 * @param[out] p_is_init Receives the initialization state.
 */
static void par_store_flash_ee_fal_is_init(const void *p_ctx, bool *p_is_init)
{
    const par_store_flash_ee_fal_ctx_t *p_fal_ctx = (const par_store_flash_ee_fal_ctx_t *)p_ctx;

    if (NULL == p_is_init)
    {
        return;
    }

    *p_is_init = false;
    if (NULL != p_fal_ctx)
    {
        *p_is_init = p_fal_ctx->is_init;
    }
}

/**
 * @brief Read raw bytes from the FAL partition.
 *
 * @param[in] p_ctx Immutable FAL port context.
 * @param[in] addr Byte offset inside the bound partition.
 * @param[in] size Number of bytes to read.
 * @param[out] p_buf Destination buffer.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_fal_read(const void *p_ctx, uint32_t addr, uint32_t size, uint8_t *p_buf)
{
    const par_store_flash_ee_fal_ctx_t *p_fal_ctx = (const par_store_flash_ee_fal_ctx_t *)p_ctx;

    if ((NULL == p_fal_ctx) || (NULL == p_fal_ctx->p_part) || (NULL == p_buf))
    {
        return ePAR_ERROR_PARAM;
    }

    return ((int)size == fal_partition_read(p_fal_ctx->p_part, (long)addr, p_buf, (size_t)size)) ? ePAR_OK : ePAR_ERROR_NVM;
}

/**
 * @brief Program raw bytes into the FAL partition.
 *
 * @param[in] p_ctx Immutable FAL port context.
 * @param[in] addr Byte offset inside the bound partition.
 * @param[in] size Number of bytes to program.
 * @param[in] p_buf Source buffer.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_fal_program(const void *p_ctx, uint32_t addr, uint32_t size, const uint8_t *p_buf)
{
    const par_store_flash_ee_fal_ctx_t *p_fal_ctx = (const par_store_flash_ee_fal_ctx_t *)p_ctx;

    if ((NULL == p_fal_ctx) || (NULL == p_fal_ctx->p_part) || (NULL == p_buf))
    {
        return ePAR_ERROR_PARAM;
    }

    return ((int)size == fal_partition_write(p_fal_ctx->p_part, (long)addr, p_buf, (size_t)size)) ? ePAR_OK : ePAR_ERROR_NVM;
}

/**
 * @brief Erase bytes inside the FAL partition.
 *
 * @param[in] p_ctx Immutable FAL port context.
 * @param[in] addr Byte offset inside the bound partition.
 * @param[in] size Number of bytes to erase.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_fal_erase(const void *p_ctx, uint32_t addr, uint32_t size)
{
    const par_store_flash_ee_fal_ctx_t *p_fal_ctx = (const par_store_flash_ee_fal_ctx_t *)p_ctx;

    if ((NULL == p_fal_ctx) || (NULL == p_fal_ctx->p_part))
    {
        return ePAR_ERROR_PARAM;
    }

    return ((int)size == fal_partition_erase(p_fal_ctx->p_part, (long)addr, (size_t)size)) ? ePAR_OK : ePAR_ERROR_NVM;
}

/**
 * @brief Return the total FAL partition size.
 *
 * @param[in] p_ctx Immutable FAL port context.
 * @return Partition size in bytes.
 */
static uint32_t par_store_flash_ee_fal_get_region_size(const void *p_ctx)
{
    const par_store_flash_ee_fal_ctx_t *p_fal_ctx = (const par_store_flash_ee_fal_ctx_t *)p_ctx;
    return ((NULL != p_fal_ctx) && (NULL != p_fal_ctx->p_part)) ? (uint32_t)p_fal_ctx->p_part->len : 0u;
}

/**
 * @brief Return the FAL partition erase size.
 *
 * @param[in] p_ctx Immutable FAL port context.
 * @return Erase size in bytes.
 */
static uint32_t par_store_flash_ee_fal_get_erase_size(const void *p_ctx)
{
    const par_store_flash_ee_fal_ctx_t *p_fal_ctx = (const par_store_flash_ee_fal_ctx_t *)p_ctx;
    return ((NULL != p_fal_ctx) && (NULL != p_fal_ctx->p_flash)) ? (uint32_t)p_fal_ctx->p_flash->blk_size : 0u;
}

/**
 * @brief Return the FAL program size.
 *
 * @param[in] p_ctx Immutable FAL port context.
 * @return Program size in bytes.
 *
 * @note FAL does not expose one universal program granularity field. The
 * backend therefore uses the package configuration value.
 */
static uint32_t par_store_flash_ee_fal_get_program_size(const void *p_ctx)
{
    (void)p_ctx;
    return PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE;
}

/**
 * @brief Return the port name used by the FAL adapter.
 *
 * @param[in] p_ctx Immutable FAL port context.
 * @return Constant name string.
 */
static const char *par_store_flash_ee_fal_get_name(const void *p_ctx)
{
    const par_store_flash_ee_fal_ctx_t *p_fal_ctx = (const par_store_flash_ee_fal_ctx_t *)p_ctx;
    return ((NULL != p_fal_ctx) && (NULL != p_fal_ctx->partition_name)) ? p_fal_ctx->partition_name : "fal";
}

/**
 * @brief Expose the FAL-backed flash port operations to the generic core.
 */
static const par_store_flash_ee_port_api_t g_par_store_flash_ee_fal_port = {
    .init = par_store_flash_ee_fal_init,
    .deinit = par_store_flash_ee_fal_deinit,
    .is_init = par_store_flash_ee_fal_is_init,
    .read = par_store_flash_ee_fal_read,
    .program = par_store_flash_ee_fal_program,
    .erase = par_store_flash_ee_fal_erase,
    .get_region_size = par_store_flash_ee_fal_get_region_size,
    .get_erase_size = par_store_flash_ee_fal_get_erase_size,
    .get_program_size = par_store_flash_ee_fal_get_program_size,
    .get_name = par_store_flash_ee_fal_get_name,
};

/**
 * @brief Bind the FAL adapter to the generic flash-ee backend core.
 *
 * @return ePAR_OK on success, otherwise an error code from the generic core.
 */
par_status_t par_store_backend_bind(void)
{
    return par_store_backend_flash_ee_bind_port(&g_par_store_flash_ee_fal_port, &g_par_store_flash_ee_fal_ctx);
}

/**
 * @brief Return the generic flash-ee backend API after adapter binding.
 *
 * @return Generic flash-ee backend API table.
 */
const par_store_backend_api_t * par_store_backend_get_api(void)
{
    return par_store_backend_flash_ee_get_api();
}

#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_FAL_EN) */
