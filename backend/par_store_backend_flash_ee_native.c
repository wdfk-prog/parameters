/**
 * @file par_store_backend_flash_ee_native.c
 * @brief Bind the generic flash-emulated EEPROM core to native flash hooks.
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

#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_NATIVE_EN)

/**
 * @brief Hold runtime geometry and state for the native flash-ee adapter.
 */
typedef struct
{
    bool is_init;         /**< True after the native hooks finish initialization. */
    uint32_t region_size; /**< Total persistence-region size reported by the hooks. */
    uint32_t erase_size;  /**< Physical erase granularity reported by the hooks. */
    uint32_t program_size; /**< Physical program granularity reported by the hooks. */
    const char *name;     /**< Diagnostic port name reported by the hooks. */
} par_store_flash_ee_native_ctx_t;

/**
 * @brief Weak default native-port deinitializer.
 *
 * @return ePAR_OK because the default implementation owns no resources.
 */
PAR_PORT_WEAK par_status_t par_store_flash_ee_native_port_deinit(void)
{
    return ePAR_OK;
}

/**
 * @brief Weak default native-port name hook.
 *
 * @return Default diagnostic name for the native adapter.
 */
PAR_PORT_WEAK const char *par_store_flash_ee_native_port_name(void)
{
    return "native";
}

/**
 * @brief Store the singleton native-adapter context.
 */
static par_store_flash_ee_native_ctx_t g_par_store_flash_ee_native_ctx = {
    .is_init = false,
    .region_size = 0u,
    .erase_size = 0u,
    .program_size = 0u,
    .name = "native",
};

/**
 * @brief Initialize the native-flash port.
 *
 * @param[in,out] p_ctx Mutable native-port context.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_native_init(void *p_ctx)
{
    par_store_flash_ee_native_ctx_t *p_native_ctx = (par_store_flash_ee_native_ctx_t *)p_ctx;
    par_status_t status = ePAR_OK;

    if (NULL == p_native_ctx)
    {
        return ePAR_ERROR_PARAM;
    }

    if (true == p_native_ctx->is_init)
    {
        return ePAR_OK;
    }

    status = par_store_flash_ee_native_port_init();
    if (ePAR_OK == status)
    {
        p_native_ctx->region_size = par_store_flash_ee_native_port_region_size();
        p_native_ctx->erase_size = par_store_flash_ee_native_port_erase_size();
        p_native_ctx->program_size = par_store_flash_ee_native_port_program_size();
        p_native_ctx->name = par_store_flash_ee_native_port_name();
        p_native_ctx->is_init = true;
    }

    return status;
}

/**
 * @brief Deinitialize the native-flash port.
 *
 * @param[in,out] p_ctx Mutable native-port context.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_native_deinit(void *p_ctx)
{
    par_store_flash_ee_native_ctx_t *p_native_ctx = (par_store_flash_ee_native_ctx_t *)p_ctx;

    if (NULL == p_native_ctx)
    {
        return ePAR_ERROR_PARAM;
    }

    p_native_ctx->is_init = false;
    return par_store_flash_ee_native_port_deinit();
}

/**
 * @brief Report whether the native-flash port is initialized.
 *
 * @param[in] p_ctx Immutable native-port context.
 * @param[out] p_is_init Receives the initialization state.
 */
static void par_store_flash_ee_native_is_init(const void *p_ctx, bool *p_is_init)
{
    const par_store_flash_ee_native_ctx_t *p_native_ctx = (const par_store_flash_ee_native_ctx_t *)p_ctx;

    if (NULL == p_is_init)
    {
        return;
    }

    *p_is_init = false;
    if (NULL != p_native_ctx)
    {
        *p_is_init = p_native_ctx->is_init;
    }
}

/**
 * @brief Read raw bytes through the native-flash hooks.
 *
 * @param[in] p_ctx Immutable native-port context.
 * @param[in] addr Byte offset inside the bound region.
 * @param[in] size Number of bytes to read.
 * @param[out] p_buf Destination buffer.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_native_read(const void *p_ctx, uint32_t addr, uint32_t size, uint8_t *p_buf)
{
    (void)p_ctx;
    return par_store_flash_ee_native_port_read(addr, size, p_buf);
}

/**
 * @brief Program raw bytes through the native-flash hooks.
 *
 * @param[in] p_ctx Immutable native-port context.
 * @param[in] addr Byte offset inside the bound region.
 * @param[in] size Number of bytes to program.
 * @param[in] p_buf Source buffer.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_native_program(const void *p_ctx, uint32_t addr, uint32_t size, const uint8_t *p_buf)
{
    (void)p_ctx;
    return par_store_flash_ee_native_port_program(addr, size, p_buf);
}

/**
 * @brief Erase bytes through the native-flash hooks.
 *
 * @param[in] p_ctx Immutable native-port context.
 * @param[in] addr Byte offset inside the bound region.
 * @param[in] size Number of bytes to erase.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_native_erase(const void *p_ctx, uint32_t addr, uint32_t size)
{
    (void)p_ctx;
    return par_store_flash_ee_native_port_erase(addr, size);
}

/**
 * @brief Return the native-flash region size.
 *
 * @param[in] p_ctx Immutable native-port context.
 * @return Region size in bytes.
 */
static uint32_t par_store_flash_ee_native_get_region_size(const void *p_ctx)
{
    const par_store_flash_ee_native_ctx_t *p_native_ctx = (const par_store_flash_ee_native_ctx_t *)p_ctx;
    return (NULL != p_native_ctx) ? p_native_ctx->region_size : 0u;
}

/**
 * @brief Return the native-flash erase size.
 *
 * @param[in] p_ctx Immutable native-port context.
 * @return Erase size in bytes.
 */
static uint32_t par_store_flash_ee_native_get_erase_size(const void *p_ctx)
{
    const par_store_flash_ee_native_ctx_t *p_native_ctx = (const par_store_flash_ee_native_ctx_t *)p_ctx;
    return (NULL != p_native_ctx) ? p_native_ctx->erase_size : 0u;
}

/**
 * @brief Return the native-flash program size.
 *
 * @param[in] p_ctx Immutable native-port context.
 * @return Program size in bytes.
 */
static uint32_t par_store_flash_ee_native_get_program_size(const void *p_ctx)
{
    const par_store_flash_ee_native_ctx_t *p_native_ctx = (const par_store_flash_ee_native_ctx_t *)p_ctx;
    return (NULL != p_native_ctx) ? p_native_ctx->program_size : 0u;
}

/**
 * @brief Return the native-flash port name.
 *
 * @param[in] p_ctx Immutable native-port context.
 * @return Constant name string.
 */
static const char *par_store_flash_ee_native_get_name(const void *p_ctx)
{
    const par_store_flash_ee_native_ctx_t *p_native_ctx = (const par_store_flash_ee_native_ctx_t *)p_ctx;
    return ((NULL != p_native_ctx) && (NULL != p_native_ctx->name)) ? p_native_ctx->name : "native";
}

/**
 * @brief Expose the native flash port operations to the generic core.
 */
static const par_store_flash_ee_port_api_t g_par_store_flash_ee_native_port = {
    .init = par_store_flash_ee_native_init,
    .deinit = par_store_flash_ee_native_deinit,
    .is_init = par_store_flash_ee_native_is_init,
    .read = par_store_flash_ee_native_read,
    .program = par_store_flash_ee_native_program,
    .erase = par_store_flash_ee_native_erase,
    .get_region_size = par_store_flash_ee_native_get_region_size,
    .get_erase_size = par_store_flash_ee_native_get_erase_size,
    .get_program_size = par_store_flash_ee_native_get_program_size,
    .get_name = par_store_flash_ee_native_get_name,
};

/**
 * @brief Bind the native adapter to the generic flash-ee backend core.
 *
 * @return ePAR_OK on success, otherwise an error code from the generic core.
 */
par_status_t par_store_backend_bind(void)
{
    return par_store_backend_flash_ee_bind_port(&g_par_store_flash_ee_native_port, &g_par_store_flash_ee_native_ctx);
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

#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_NATIVE_EN) */
