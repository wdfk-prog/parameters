/**
 * @file par_test_eeprom_host.c
 * @brief Validate parameter persistence with a host fake EEPROM backend.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#include "par_test.h"

#if defined(AUTOGEN_PM_TEST_USING_EEPROM_HOST) && (1 == PAR_CFG_NVM_EN) && !defined(PAR_HOST_BACKEND_FLASH_EE)

#include "par_def.h"
#include "par_host_fake_storage.h"
#include "par_store_backend.h"

#include <limits.h>

/**
 * @brief Convert a parameter status code to a printable host EEPROM string.
 * @param status Parameter status code.
 * @return Printable status string when debug strings are enabled; otherwise a stable fallback string.
 */
static const char *par_test_eeprom_host_status_str(par_status_t status)
{
#if (1 == PAR_CFG_DEBUG_EN)
    return par_get_status_str(status);
#else
    (void)status;
    return "status";
#endif /* (1 == PAR_CFG_DEBUG_EN) */
}

/**
 * @brief Reset the host fake EEPROM image and restart the parameter module from a clean image.
 * @param ctx Test execution context.
 * @return ePAR_TEST_PASS on success, otherwise ePAR_TEST_FAIL.
 */
static par_test_result_t par_test_eeprom_host_reset_image(par_test_context_t *ctx)
{
    const par_store_backend_api_t *p_api;
    par_status_t status;

    if (true == par_is_init())
    {
        status = par_deinit();
        PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "deinit_status=%s", par_test_eeprom_host_status_str(status));
    }

    status = par_store_backend_bind();
    PAR_TEST_ASSERT(ctx, ePAR_OK == status, "bind_status=%u", (unsigned)status);

    p_api = par_store_backend_get_api();
    PAR_TEST_ASSERT(ctx, NULL != p_api, "backend_api_null");
    PAR_TEST_ASSERT(ctx, NULL != p_api->init, "backend_init_null");
    PAR_TEST_ASSERT(ctx, NULL != p_api->deinit, "backend_deinit_null");

    status = p_api->init();
    PAR_TEST_ASSERT(ctx, ePAR_OK == status, "backend_init_status=%u", (unsigned)status);

    status = par_host_fake_storage_reset_image();
    if (ePAR_OK != status)
    {
        (void)p_api->deinit();
        PAR_TEST_FAIL(ctx, "reset_image_status=%u", (unsigned)status);
    }

    status = p_api->deinit();
    PAR_TEST_ASSERT(ctx, ePAR_OK == status, "backend_deinit_status=%u", (unsigned)status);
    return ePAR_TEST_PASS;
}

/**
 * @brief Restore and save one persistent scalar value after a test mutation.
 * @param par_num Parameter number.
 * @param value Original scalar value.
 * @return Operation status.
 */
static par_status_t par_test_eeprom_host_restore_original(par_num_t par_num, const par_type_t *value)
{
    par_status_t status;

    status = par_test_set_scalar_fast(par_num, value);
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        return status;
    }

    return par_save(par_num);
}

/**
 * @brief Find a mutable writable scalar that is not marked persistent.
 * @param par_num Output parameter number.
 * @param original Output current scalar value.
 * @param alternate Output alternate scalar value.
 * @return true when a matching scalar is found; otherwise false.
 */
static bool par_test_eeprom_host_find_mutable_nonpersistent_writable_scalar(par_num_t *par_num,
                                                                           par_type_t *original,
                                                                           par_type_t *alternate)
{
    par_num_t it;

    if ((NULL == par_num) || (NULL == original) || (NULL == alternate))
    {
        return false;
    }

    for (it = 0U; it < (par_num_t)ePAR_NUM_OF; it++)
    {
        const par_cfg_t *cfg = par_get_config(it);
        par_status_t status;

        if ((NULL == cfg) || (false == par_test_type_is_scalar(cfg->type)) ||
            (false == par_test_cfg_is_writable(cfg)) || (true == cfg->persistent))
        {
            continue;
        }

        status = par_test_read_scalar(it, original);
        if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
        {
            continue;
        }

        if (false == par_test_make_alternate_scalar(it, original, alternate))
        {
            continue;
        }

        *par_num = it;
        return true;
    }

    return false;
}

#if (1 == PAR_CFG_ENABLE_ID)
/**
 * @brief Find an external parameter ID that is not present in the generated table.
 * @param p_id Output invalid ID.
 * @return true when an unmapped ID was found; otherwise false.
 */
static bool par_test_eeprom_host_find_invalid_id(uint16_t *p_id)
{
    uint32_t candidate;

    if (NULL == p_id)
    {
        return false;
    }

    for (candidate = 0UL; candidate <= (uint32_t)UINT16_MAX; candidate++)
    {
        par_num_t resolved = 0U;

        if (ePAR_OK != par_get_num_by_id((uint16_t)candidate, &resolved))
        {
            *p_id = (uint16_t)candidate;
            return true;
        }
    }

    return false;
}
#endif /* (1 == PAR_CFG_ENABLE_ID) */

/**
 * @brief Validate that the host fake EEPROM-backed NVM path can initialize.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_eeprom_host_backend_available(par_test_context_t *ctx)
{
    par_status_t status;

    if (ePAR_TEST_PASS != par_test_eeprom_host_reset_image(ctx))
    {
        return ePAR_TEST_FAIL;
    }

    status = par_init();
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "status=%s", par_test_eeprom_host_status_str(status));
    PAR_TEST_ASSERT(ctx, true == par_is_init(), "par_is_init=false");
    return ePAR_TEST_PASS;
}

/**
 * @brief Validate one save and reboot-like reload cycle through host fake EEPROM.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_eeprom_host_save_restore_scalar(par_test_context_t *ctx)
{
    par_num_t par_num = 0U;
    const par_cfg_t *cfg;
    par_type_t original = { 0 };
    par_type_t alternate = { 0 };
    par_type_t readback = { 0 };
    par_status_t status;

    if (ePAR_TEST_PASS != par_test_eeprom_host_reset_image(ctx))
    {
        return ePAR_TEST_FAIL;
    }

    status = par_init();
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "init_status=%s", par_test_eeprom_host_status_str(status));

    if (false == par_test_find_mutable_writable_scalar(true, &par_num, &original, &alternate))
    {
        PAR_TEST_SKIP(ctx, "no_mutable_persistent_writable_scalar");
    }

    cfg = par_get_config(par_num);
    PAR_TEST_ASSERT(ctx, NULL != cfg, "par=%u cfg=null", (unsigned)par_num);

    status = par_test_set_scalar(par_num, &alternate);
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_eeprom_host_restore_original(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u set_status=%s", (unsigned)par_num, par_test_eeprom_host_status_str(status));
    }

    status = par_save(par_num);
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_eeprom_host_restore_original(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u save_status=%s", (unsigned)par_num, par_test_eeprom_host_status_str(status));
    }

    status = par_deinit();
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_eeprom_host_restore_original(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u deinit_status=%s", (unsigned)par_num, par_test_eeprom_host_status_str(status));
    }

    status = par_init();
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        PAR_TEST_FAIL(ctx, "par=%u reinit_status=%s", (unsigned)par_num, par_test_eeprom_host_status_str(status));
    }

    status = par_test_read_scalar(par_num, &readback);
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_eeprom_host_restore_original(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u reload_read_status=%s", (unsigned)par_num, par_test_eeprom_host_status_str(status));
    }

    if (false == par_test_scalar_equal(cfg->type, &alternate, &readback))
    {
        (void)par_test_eeprom_host_restore_original(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u reload_mismatch", (unsigned)par_num);
    }

    status = par_test_eeprom_host_restore_original(par_num, &original);
    PAR_TEST_ASSERT(ctx,
                    PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u restore_status=%s",
                    (unsigned)par_num,
                    par_test_eeprom_host_status_str(status));
    return ePAR_TEST_PASS;
}

/**
 * @brief Validate one save-by-ID and reboot-like reload cycle through host fake EEPROM.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_eeprom_host_save_restore_scalar_by_id(par_test_context_t *ctx)
{
#if (1 == PAR_CFG_ENABLE_ID)
    par_num_t par_num = 0U;
    const par_cfg_t *cfg;
    uint16_t id = 0U;
    par_type_t original = { 0 };
    par_type_t alternate = { 0 };
    par_type_t readback = { 0 };
    par_status_t status;

    if (ePAR_TEST_PASS != par_test_eeprom_host_reset_image(ctx))
    {
        return ePAR_TEST_FAIL;
    }

    status = par_init();
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "init_status=%s", par_test_eeprom_host_status_str(status));

    if (false == par_test_find_mutable_writable_scalar(true, &par_num, &original, &alternate))
    {
        PAR_TEST_SKIP(ctx, "no_mutable_persistent_writable_scalar");
    }

    cfg = par_get_config(par_num);
    PAR_TEST_ASSERT(ctx, NULL != cfg, "par=%u cfg=null", (unsigned)par_num);

    status = par_get_id_by_num(par_num, &id);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u get_id_status=%s", (unsigned)par_num, par_test_eeprom_host_status_str(status));

    status = par_test_set_scalar(par_num, &alternate);
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_eeprom_host_restore_original(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u set_status=%s", (unsigned)par_num, par_test_eeprom_host_status_str(status));
    }

    status = par_save_by_id(id);
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_eeprom_host_restore_original(par_num, &original);
        PAR_TEST_FAIL(ctx, "id=%u save_by_id_status=%s", (unsigned)id, par_test_eeprom_host_status_str(status));
    }

    status = par_deinit();
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_eeprom_host_restore_original(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u deinit_status=%s", (unsigned)par_num, par_test_eeprom_host_status_str(status));
    }

    status = par_init();
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        PAR_TEST_FAIL(ctx, "par=%u reinit_status=%s", (unsigned)par_num, par_test_eeprom_host_status_str(status));
    }

    status = par_test_read_scalar(par_num, &readback);
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_eeprom_host_restore_original(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u reload_read_status=%s", (unsigned)par_num, par_test_eeprom_host_status_str(status));
    }

    if (false == par_test_scalar_equal(cfg->type, &alternate, &readback))
    {
        (void)par_test_eeprom_host_restore_original(par_num, &original);
        PAR_TEST_FAIL(ctx, "id=%u reload_mismatch", (unsigned)id);
    }

    status = par_test_eeprom_host_restore_original(par_num, &original);
    PAR_TEST_ASSERT(ctx,
                    PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u restore_status=%s",
                    (unsigned)par_num,
                    par_test_eeprom_host_status_str(status));
    return ePAR_TEST_PASS;
#else
    PAR_TEST_SKIP(ctx, "id_support_disabled");
#endif /* (1 == PAR_CFG_ENABLE_ID) */
}

/**
 * @brief Validate that saving a non-persistent scalar is rejected or unavailable.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_eeprom_host_save_rejects_nonpersistent_scalar(par_test_context_t *ctx)
{
    par_num_t par_num = 0U;
    par_type_t original = { 0 };
    par_type_t alternate = { 0 };
    par_status_t status;

    if (ePAR_TEST_PASS != par_test_eeprom_host_reset_image(ctx))
    {
        return ePAR_TEST_FAIL;
    }

    status = par_init();
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "init_status=%s", par_test_eeprom_host_status_str(status));

    if (false == par_test_eeprom_host_find_mutable_nonpersistent_writable_scalar(&par_num, &original, &alternate))
    {
        PAR_TEST_SKIP(ctx, "no_mutable_nonpersistent_writable_scalar");
    }

    status = par_save(par_num);
    PAR_TEST_ASSERT(ctx, false == PAR_TEST_STATUS_HAS_NO_ERROR(status), "par=%u unexpected_save_success", (unsigned)par_num);
    (void)original;
    (void)alternate;
    return ePAR_TEST_PASS;
}

/**
 * @brief Validate that save-by-ID rejects an unmapped external ID.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_eeprom_host_save_by_id_rejects_invalid_id(par_test_context_t *ctx)
{
#if (1 == PAR_CFG_ENABLE_ID)
    uint16_t invalid_id = 0U;
    par_status_t status;

    if (ePAR_TEST_PASS != par_test_eeprom_host_reset_image(ctx))
    {
        return ePAR_TEST_FAIL;
    }

    status = par_init();
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "init_status=%s", par_test_eeprom_host_status_str(status));

    if (false == par_test_eeprom_host_find_invalid_id(&invalid_id))
    {
        PAR_TEST_SKIP(ctx, "no_invalid_id_available");
    }

    status = par_save_by_id(invalid_id);
    PAR_TEST_ASSERT(ctx, false == PAR_TEST_STATUS_HAS_NO_ERROR(status), "id=%u unexpected_save_by_id_success", (unsigned)invalid_id);
    return ePAR_TEST_PASS;
#else
    PAR_TEST_SKIP(ctx, "id_support_disabled");
#endif /* (1 == PAR_CFG_ENABLE_ID) */
}

#else
/**
 * @brief Skip host EEPROM tests when the host fake EEPROM backend is unavailable.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_eeprom_host_backend_disabled(par_test_context_t *ctx)
{
    PAR_TEST_SKIP(ctx, "eeprom_host_backend_disabled");
}
#endif /* defined(AUTOGEN_PM_TEST_USING_EEPROM_HOST) && (1 == PAR_CFG_NVM_EN) && !defined(PAR_HOST_BACKEND_FLASH_EE) */

/**
 * @brief Host fake EEPROM validation case table.
 */
static const par_test_case_t g_par_test_eeprom_host_cases[] = {
#if defined(AUTOGEN_PM_TEST_USING_EEPROM_HOST) && (1 == PAR_CFG_NVM_EN) && !defined(PAR_HOST_BACKEND_FLASH_EE)
    { "backend_available", par_test_eeprom_host_backend_available, (uint32_t)ePAR_TEST_CASE_FLAG_DESTRUCTIVE },
    { "save_restore_scalar", par_test_eeprom_host_save_restore_scalar, (uint32_t)ePAR_TEST_CASE_FLAG_DESTRUCTIVE },
    { "save_restore_scalar_by_id", par_test_eeprom_host_save_restore_scalar_by_id, (uint32_t)ePAR_TEST_CASE_FLAG_DESTRUCTIVE },
    { "save_rejects_nonpersistent_scalar",
      par_test_eeprom_host_save_rejects_nonpersistent_scalar,
      (uint32_t)ePAR_TEST_CASE_FLAG_DESTRUCTIVE },
    { "save_by_id_rejects_invalid_id", par_test_eeprom_host_save_by_id_rejects_invalid_id, (uint32_t)ePAR_TEST_CASE_FLAG_DESTRUCTIVE },
#else
    { "backend_disabled", par_test_eeprom_host_backend_disabled, (uint32_t)ePAR_TEST_CASE_FLAG_NONE },
#endif /* defined(AUTOGEN_PM_TEST_USING_EEPROM_HOST) && (1 == PAR_CFG_NVM_EN) && !defined(PAR_HOST_BACKEND_FLASH_EE) */
};

const par_test_suite_t g_par_test_suite_eeprom_host = {
    .name = "eeprom_host",
    .cases = g_par_test_eeprom_host_cases,
    .case_count = (uint32_t)PAR_TEST_ARRAY_SIZE(g_par_test_eeprom_host_cases),
};
