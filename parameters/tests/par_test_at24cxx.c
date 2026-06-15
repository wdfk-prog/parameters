/**
 * @file par_test_at24cxx.c
 * @brief Validate AT24CXX-backed parameter persistence through public APIs.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#include "par_test.h"

#include "par_def.h"

/**
 * @brief Convert a parameter status code to a printable string for AT24 tests.
 * @param status Parameter status code.
 * @return Printable status string when debug strings are enabled; otherwise a
 *         stable fallback string.
 */
static const char *par_test_at24_status_str(par_status_t status)
{
#if (1 == PAR_CFG_DEBUG_EN)
    return par_get_status_str(status);
#else
    (void)status;
    return "status";
#endif /* (1 == PAR_CFG_DEBUG_EN) */
}


#if (1 == PAR_CFG_ENABLE_ID)
/**
 * @brief Find an external ID that is not mapped by the compiled table.
 * @param p_id Output invalid external ID.
 * @return true when an unmapped ID was found; otherwise false.
 */
static bool par_test_at24_find_invalid_id(uint16_t *p_id)
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

#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_RTT_AT24CXX_EN)

/**
 * @brief Find a mutable writable scalar that is not marked persistent.
 * @param par_num Output parameter number.
 * @param original Output current scalar value.
 * @param alternate Output alternate scalar value.
 * @return true when a matching scalar is found; otherwise false.
 */
static bool par_test_at24_find_mutable_nonpersistent_writable_scalar(par_num_t *par_num,
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

/**
 * @brief Validate that the AT24CXX-backed NVM path can initialize.
 * @details Ensures the parameter module initializes successfully with the
 *          AT24CXX backend compiled in and verifies the public initialized
 *          state flag is set.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_at24_backend_available(par_test_context_t *ctx)
{
    par_status_t status = par_test_ensure_parameter_init();

    /* Case step: initialize the parameter module with the AT24 backend.
     * Expected result is successful initialization and par_is_init() true.
     */
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "status=%s", par_test_at24_status_str(status));
    PAR_TEST_ASSERT(ctx, true == par_is_init(), "par_is_init=false");
    return ePAR_TEST_PASS;
}

/**
 * @brief Restore and save one persistent scalar value after a test mutation.
 * @details Writes the preserved scalar value back to RAM storage and saves it
 *          to NVM so the persistence test does not leave AT24 contents mutated.
 * @param par_num Parameter number.
 * @param value Original scalar value.
 * @return Operation status.
 */
static par_status_t par_test_at24_restore_original(par_num_t par_num, const par_type_t *value)
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
 * @brief Validate one save and reboot-like reload cycle through AT24CXX NVM.
 * @details Selects a persistent writable scalar, writes and saves a valid
 *          alternate value to AT24CXX, deinitializes and reinitializes the
 *          parameter module, verifies the value reloads from NVM, and restores
 *          the original value to both RAM and NVM.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_at24_save_restore_scalar(par_test_context_t *ctx)
{
    par_num_t par_num = 0U;
    const par_cfg_t *cfg;
    par_type_t original = { 0 };
    par_type_t alternate = { 0 };
    par_type_t readback = { 0 };
    par_status_t status;

    status = par_test_ensure_parameter_init();
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "init_status=%s", par_test_at24_status_str(status));

    /* Case step: select a persistent writable scalar and preserve its
     * original value. Expected result is a valid alternate value to save.
     */
    if (false == par_test_find_mutable_writable_scalar(true, &par_num, &original, &alternate))
    {
        PAR_TEST_SKIP(ctx, "no_mutable_persistent_writable_scalar");
    }

    cfg = par_get_config(par_num);
    PAR_TEST_ASSERT(ctx, NULL != cfg, "par=%u cfg=null", (unsigned)par_num);

    /* Case step: write the alternate value in RAM and save it to AT24CXX.
     * Expected result is that both setter and par_save() succeed.
     */
    status = par_test_set_scalar(par_num, &alternate);
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_at24_restore_original(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u set_status=%s", (unsigned)par_num, par_test_at24_status_str(status));
    }

    status = par_save(par_num);
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_at24_restore_original(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u save_status=%s", (unsigned)par_num, par_test_at24_status_str(status));
    }

    /* Case step: simulate a reboot-like reload by deinitializing and
     * reinitializing the parameter module.
     */
    status = par_deinit();
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_at24_restore_original(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u deinit_status=%s", (unsigned)par_num, par_test_at24_status_str(status));
    }

    status = par_init();
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        PAR_TEST_FAIL(ctx, "par=%u reinit_status=%s", (unsigned)par_num, par_test_at24_status_str(status));
    }

    /* Case step: read the scalar after reinit. Expected result is that the
     * alternate value was reloaded from AT24CXX-backed NVM.
     */
    status = par_test_read_scalar(par_num, &readback);
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_at24_restore_original(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u reload_read_status=%s", (unsigned)par_num, par_test_at24_status_str(status));
    }

    if (false == par_test_scalar_equal(cfg->type, &alternate, &readback))
    {
        (void)par_test_at24_restore_original(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u reload_mismatch", (unsigned)par_num);
    }

    /* Case step: restore the original value to RAM and NVM so the test does
     * not leave persistent board data changed.
     */
    status = par_test_at24_restore_original(par_num, &original);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "par=%u restore_status=%s", (unsigned)par_num, par_test_at24_status_str(status));
    return ePAR_TEST_PASS;
}

/**
 * @brief Validate one save-by-ID and reboot-like reload cycle through AT24CXX NVM.
 * @details Selects a persistent writable scalar, writes a valid alternate
 *          value, saves it through the external-ID API, reinitializes the
 *          module, verifies the value reloads from AT24CXX, and restores the
 *          original value to both RAM and NVM.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_at24_save_restore_scalar_by_id(par_test_context_t *ctx)
{
#if (1 == PAR_CFG_ENABLE_ID)
    par_num_t par_num = 0U;
    const par_cfg_t *cfg;
    uint16_t id = 0U;
    par_type_t original = { 0 };
    par_type_t alternate = { 0 };
    par_type_t readback = { 0 };
    par_status_t status;

    status = par_test_ensure_parameter_init();
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "init_status=%s", par_test_at24_status_str(status));

    if (false == par_test_find_mutable_writable_scalar(true, &par_num, &original, &alternate))
    {
        PAR_TEST_SKIP(ctx, "no_mutable_persistent_writable_scalar");
    }

    cfg = par_get_config(par_num);
    PAR_TEST_ASSERT(ctx, NULL != cfg, "par=%u cfg=null", (unsigned)par_num);

    status = par_get_id_by_num(par_num, &id);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u get_id_status=%s", (unsigned)par_num, par_test_at24_status_str(status));

    /* Case step: write the alternate value in RAM and save by external ID.
     * Expected result is that the ID mapping and save path both succeed.
     */
    status = par_test_set_scalar(par_num, &alternate);
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_at24_restore_original(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u set_status=%s", (unsigned)par_num, par_test_at24_status_str(status));
    }

    status = par_save_by_id(id);
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_at24_restore_original(par_num, &original);
        PAR_TEST_FAIL(ctx, "id=%u save_by_id_status=%s", (unsigned)id, par_test_at24_status_str(status));
    }

    /* Case step: simulate a reboot-like reload. Expected result is that the
     * alternate value saved by ID is restored from AT24CXX-backed NVM.
     */
    status = par_deinit();
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_at24_restore_original(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u deinit_status=%s", (unsigned)par_num, par_test_at24_status_str(status));
    }

    status = par_init();
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        PAR_TEST_FAIL(ctx, "par=%u reinit_status=%s", (unsigned)par_num, par_test_at24_status_str(status));
    }

    status = par_test_read_scalar(par_num, &readback);
    if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) ||
        (false == par_test_scalar_equal(cfg->type, &alternate, &readback)))
    {
        (void)par_test_at24_restore_original(par_num, &original);
        PAR_TEST_FAIL(ctx, "id=%u reload_by_id_mismatch status=%s", (unsigned)id, par_test_at24_status_str(status));
    }

    status = par_test_at24_restore_original(par_num, &original);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u restore_status=%s", (unsigned)par_num, par_test_at24_status_str(status));
    return ePAR_TEST_PASS;
#else
    PAR_TEST_SKIP(ctx, "id_disabled");
#endif /* (1 == PAR_CFG_ENABLE_ID) */
}

/**
 * @brief Validate that AT24CXX save rejects non-persistent scalar parameters.
 * @details Writes an alternate non-persistent scalar value, verifies
 *          par_save() returns the expected error without changing live RAM
 *          state, and restores the original RAM value for test isolation.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_at24_save_rejects_nonpersistent_scalar(par_test_context_t *ctx)
{
    par_num_t par_num = 0U;
    const par_cfg_t *cfg;
    par_type_t original = { 0 };
    par_type_t alternate = { 0 };
    par_type_t readback = { 0 };
    par_status_t status;

    status = par_test_ensure_parameter_init();
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "init_status=%s", par_test_at24_status_str(status));

    if (false == par_test_at24_find_mutable_nonpersistent_writable_scalar(&par_num, &original, &alternate))
    {
        PAR_TEST_SKIP(ctx, "no_mutable_nonpersistent_writable_scalar");
    }

    cfg = par_get_config(par_num);
    PAR_TEST_ASSERT(ctx, NULL != cfg, "par=%u cfg=null", (unsigned)par_num);

    /* Case step: write an alternate non-persistent value and try to save it.
     * Expected result is a save error while the live RAM value stays intact.
     */
    status = par_test_set_scalar(par_num, &alternate);
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u set_status=%s", (unsigned)par_num, par_test_at24_status_str(status));
    }

    status = par_save(par_num);
    if (ePAR_ERROR != status)
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u nonpersistent_save_status=%s", (unsigned)par_num, par_test_at24_status_str(status));
    }

    status = par_test_read_scalar(par_num, &readback);
    if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) ||
        (false == par_test_scalar_equal(cfg->type, &alternate, &readback)))
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u nonpersistent_save_polluted status=%s", (unsigned)par_num, par_test_at24_status_str(status));
    }

    /* Case step: restore the original RAM value for test isolation.
     * Expected result is that the failed non-persistent save did not require
     * any NVM rollback and the live RAM value can be restored normally.
     */
    status = par_test_set_scalar_fast(par_num, &original);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u restore_status=%s", (unsigned)par_num, par_test_at24_status_str(status));
    return ePAR_TEST_PASS;
}

/**
 * @brief Validate that AT24CXX save-by-ID rejects unmapped external IDs.
 * @details Finds an unmapped external ID, verifies par_save_by_id() returns
 *          the expected error, and verifies a selected persistent scalar value
 *          is not changed by the failed save request.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_at24_save_by_id_rejects_invalid_id(par_test_context_t *ctx)
{
#if (1 == PAR_CFG_ENABLE_ID)
    par_num_t par_num = 0U;
    uint16_t invalid_id = 0U;
    par_type_t original = { 0 };
    par_type_t alternate = { 0 };
    par_type_t readback = { 0 };
    par_status_t status;

    status = par_test_ensure_parameter_init();
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "init_status=%s", par_test_at24_status_str(status));

    if (false == par_test_find_mutable_writable_scalar(true, &par_num, &original, &alternate))
    {
        PAR_TEST_SKIP(ctx, "no_mutable_persistent_writable_scalar");
    }

    PAR_TEST_ASSERT(ctx, true == par_test_at24_find_invalid_id(&invalid_id), "invalid_id_unavailable");

    /* Case step: write a known live value, then attempt save-by-ID with an
     * unmapped ID. Expected result is ePAR_ERROR and no live-value mutation.
     */
    status = par_test_set_scalar(par_num, &alternate);
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u set_status=%s", (unsigned)par_num, par_test_at24_status_str(status));
    }

    status = par_save_by_id(invalid_id);
    if (ePAR_ERROR != status)
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "invalid_id=%u save_by_id_status=%s", (unsigned)invalid_id, par_test_at24_status_str(status));
    }

    status = par_test_read_scalar(par_num, &readback);
    if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) ||
        (false == par_test_scalar_equal(par_get_type(par_num), &alternate, &readback)))
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "invalid_id=%u polluted_value status=%s", (unsigned)invalid_id, par_test_at24_status_str(status));
    }

    status = par_test_set_scalar_fast(par_num, &original);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u restore_status=%s", (unsigned)par_num, par_test_at24_status_str(status));
    return ePAR_TEST_PASS;
#else
    PAR_TEST_SKIP(ctx, "id_disabled");
#endif /* (1 == PAR_CFG_ENABLE_ID) */
}


#else
/**
 * @brief Skip AT24CXX tests when the compiled backend is unavailable.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_at24_backend_available(par_test_context_t *ctx)
{
    PAR_TEST_SKIP(ctx, "at24_backend_disabled");
}

/**
 * @brief Skip AT24CXX persistence tests when the compiled backend is unavailable.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_at24_save_restore_scalar(par_test_context_t *ctx)
{
    PAR_TEST_SKIP(ctx, "at24_backend_disabled");
}

/**
 * @brief Skip AT24CXX save-by-ID persistence tests when the backend is unavailable.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_at24_save_restore_scalar_by_id(par_test_context_t *ctx)
{
    PAR_TEST_SKIP(ctx, "at24_backend_disabled");
}

/**
 * @brief Skip AT24CXX non-persistent save tests when the backend is unavailable.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_at24_save_rejects_nonpersistent_scalar(par_test_context_t *ctx)
{
    PAR_TEST_SKIP(ctx, "at24_backend_disabled");
}

/**
 * @brief Skip AT24CXX invalid-ID save tests when the backend is unavailable.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_at24_save_by_id_rejects_invalid_id(par_test_context_t *ctx)
{
    PAR_TEST_SKIP(ctx, "at24_backend_disabled");
}


#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_RTT_AT24CXX_EN) */

/**
 * @brief AT24CXX validation case table.
 */
static const par_test_case_t g_par_test_at24_cases[] = {
    { "backend_available", par_test_at24_backend_available, (uint32_t)ePAR_TEST_CASE_FLAG_NONE },
    { "save_restore_scalar", par_test_at24_save_restore_scalar, (uint32_t)ePAR_TEST_CASE_FLAG_NONE },
    { "save_restore_scalar_by_id", par_test_at24_save_restore_scalar_by_id, (uint32_t)ePAR_TEST_CASE_FLAG_NONE },
    { "save_rejects_nonpersistent_scalar", par_test_at24_save_rejects_nonpersistent_scalar, (uint32_t)ePAR_TEST_CASE_FLAG_NONE },
    { "save_by_id_rejects_invalid_id", par_test_at24_save_by_id_rejects_invalid_id, (uint32_t)ePAR_TEST_CASE_FLAG_NONE },
};

const par_test_suite_t g_par_test_suite_at24 = {
    .name = "at24",
    .cases = g_par_test_at24_cases,
    .case_count = (uint32_t)PAR_TEST_ARRAY_SIZE(g_par_test_at24_cases),
};
