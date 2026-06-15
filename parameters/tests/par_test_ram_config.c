/**
 * @file par_test_ram_config.c
 * @brief Validate RAM-mode parameter behavior and optional feature switches.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#include "par_test.h"

#include <string.h>

#include "par_def.h"

/**
 * @brief Return a status string when debug strings are compiled in.
 * @param status Parameter API status value.
 * @return Printable status name.
 */
static const char *par_test_ram_status_str(const par_status_t status)
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
static bool par_test_ram_find_invalid_id(uint16_t *p_id)
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

#if !defined(AUTOGEN_PM_TEST_TABLE_ROWS)
/**
 * @brief Pick a deterministic scalar row for RAM feature tests.
 * @param type Expected scalar type.
 * @param par_num Output parameter number.
 * @return true when a matching writable scalar with an alternate value exists.
 */
static bool par_test_ram_find_scalar_type(const par_type_list_t type, par_num_t *par_num)
{
    par_num_t it;

    if (NULL == par_num)
    {
        return false;
    }

    for (it = 0U; it < (par_num_t)ePAR_NUM_OF; it++)
    {
        const par_cfg_t *cfg = par_get_config(it);
        par_type_t original = { 0 };
        par_type_t alternate = { 0 };

        if ((NULL == cfg) || (cfg->type != type) || (false == par_test_cfg_is_writable(cfg)))
        {
            continue;
        }

        if (false == PAR_TEST_STATUS_HAS_NO_ERROR(par_test_read_scalar(it, &original)))
        {
            continue;
        }

        if (false == par_test_make_alternate_scalar(it, &original, &alternate))
        {
            continue;
        }

        *par_num = it;
        return true;
    }

    return false;
}
#endif /* !defined(AUTOGEN_PM_TEST_TABLE_ROWS) */

/**
 * @brief Validate one scalar default restore and restore the original value.
 * @details Reads the original value, applies par_set_to_default(), verifies
 *          the readback matches the configured default, and restores the
 *          original runtime value before returning.
 * @param ctx Test execution context.
 * @param par_num Parameter number.
 * @return Case result.
 */
static par_test_result_t par_test_ram_check_scalar_default(par_test_context_t *ctx, const par_num_t par_num)
{
    const par_cfg_t *cfg = par_get_config(par_num);
    par_type_t original = { 0 };
    par_type_t readback = { 0 };
    par_type_t def = { 0 };
    par_status_t status;

    PAR_TEST_ASSERT(ctx, NULL != cfg, "par=%u cfg=null", (unsigned)par_num);

    status = par_test_read_scalar(par_num, &original);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u read_status=%s", (unsigned)par_num, par_test_ram_status_str(status));

    status = par_get_scalar_default(par_num, &def);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u default_status=%s", (unsigned)par_num, par_test_ram_status_str(status));

    status = par_set_to_default(par_num);
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u set_default_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    }

    status = par_test_read_scalar(par_num, &readback);
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u default_read_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    }

    if (false == par_test_scalar_equal(cfg->type, &def, &readback))
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u default_mismatch", (unsigned)par_num);
    }

    status = par_test_set_scalar_fast(par_num, &original);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u restore_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    return ePAR_TEST_PASS;
}

/**
 * @brief Validate scalar default restore behavior for RAM storage.
 * @details Initializes the parameter module, walks every compiled scalar row,
 *          verifies each scalar can be restored to its configured default, and
 *          requires at least one scalar row to be checked.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_ram_default_readback(par_test_context_t *ctx)
{
    par_num_t par_num;
    uint32_t checked = 0U;
    par_status_t status = par_test_ensure_parameter_init();

    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "init_status=%s", par_test_ram_status_str(status));

    /* Case step: exercise every compiled scalar row. Expected result is that
     * each row can be forced to default, read back as default, and restored.
     */
    for (par_num = 0U; par_num < (par_num_t)ePAR_NUM_OF; par_num++)
    {
        const par_cfg_t *cfg = par_get_config(par_num);

        if ((NULL == cfg) || (false == par_test_type_is_scalar(cfg->type)))
        {
            continue;
        }

        if (ePAR_TEST_PASS != par_test_ram_check_scalar_default(ctx, par_num))
        {
            return ePAR_TEST_FAIL;
        }
        checked++;
    }

    PAR_TEST_ASSERT(ctx, checked > 0U, "no_scalar_parameter");
    return ePAR_TEST_PASS;
}

/**
 * @brief Validate one scalar set/get/restore path.
 * @details Captures the original value, generates a valid alternate value,
 *          writes the alternate through the normal setter, verifies getter
 *          readback equality, and restores the original value.
 * @param ctx Test execution context.
 * @param par_num Parameter number.
 * @return Case result.
 */
static par_test_result_t par_test_ram_check_scalar_set_get(par_test_context_t *ctx, const par_num_t par_num)
{
    const par_cfg_t *cfg = par_get_config(par_num);
    par_type_t original = { 0 };
    par_type_t alternate = { 0 };
    par_type_t readback = { 0 };
    par_status_t status;

    PAR_TEST_ASSERT(ctx, NULL != cfg, "par=%u cfg=null", (unsigned)par_num);

    status = par_test_read_scalar(par_num, &original);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u read_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    PAR_TEST_ASSERT(ctx, true == par_test_make_alternate_scalar(par_num, &original, &alternate),
                    "par=%u alternate_unavailable", (unsigned)par_num);

    status = par_test_set_scalar(par_num, &alternate);
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u set_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    }

    status = par_test_read_scalar(par_num, &readback);
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u readback_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    }

    if (false == par_test_scalar_equal(cfg->type, &alternate, &readback))
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u readback_mismatch", (unsigned)par_num);
    }

    status = par_test_set_scalar_fast(par_num, &original);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u restore_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    return ePAR_TEST_PASS;
}

/**
 * @brief Validate RAM setter/getter paths for writable scalar parameters.
 * @details Selects a writable scalar row, verifies that a valid in-range
 *          alternate value can be written and read back unchanged, and restores
 *          the original value after the check.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_ram_scalar_set_get_restore(par_test_context_t *ctx)
{
    par_num_t par_num = 0U;
    par_status_t status = par_test_ensure_parameter_init();

    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "init_status=%s", par_test_ram_status_str(status));

#if defined(AUTOGEN_PM_TEST_TABLE_ROWS)
    /* Case step: use the dedicated writable U8 row. Expected result is a
     * successful valid write, matching readback, and original-value restore.
     */
    (void)par_num;
    return par_test_ram_check_scalar_set_get(ctx, (par_num_t)ePAR_TEST_U8_RW);
#else
    if (false == par_test_find_mutable_writable_scalar(false, &par_num, &(par_type_t){ 0 }, &(par_type_t){ 0 }))
    {
        PAR_TEST_SKIP(ctx, "no_mutable_writable_scalar");
    }
    return par_test_ram_check_scalar_set_get(ctx, par_num);
#endif /* defined(AUTOGEN_PM_TEST_TABLE_ROWS) */
}

/**
 * @brief Validate out-of-range scalar writes are limited to configured bounds.
 * @details Selects a writable scalar row with range support, writes values
 *          below and above the configured range, expects ePAR_WAR_LIMITED, and
 *          verifies storage is clamped to the lower and upper limits.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_ram_scalar_range_limit(par_test_context_t *ctx)
{
#if (1 == PAR_CFG_ENABLE_RANGE)
    par_num_t par_num = 0U;
    const par_cfg_t *cfg;
    par_type_t original = { 0 };
    par_type_t out = { 0 };
    par_type_t limit = { 0 };
    par_type_t readback = { 0 };
    par_status_t status;

    status = par_test_ensure_parameter_init();
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "init_status=%s", par_test_ram_status_str(status));

#if defined(AUTOGEN_PM_TEST_TABLE_ROWS)
    par_num = (par_num_t)ePAR_TEST_U8_RW;
#else
    if (false == par_test_find_writable_scalar(false, &par_num))
    {
        PAR_TEST_SKIP(ctx, "no_writable_scalar");
    }
#endif /* defined(AUTOGEN_PM_TEST_TABLE_ROWS) */

    cfg = par_get_config(par_num);
    PAR_TEST_ASSERT(ctx, (NULL != cfg) && (true == par_test_type_is_scalar(cfg->type)),
                    "par=%u scalar_unavailable", (unsigned)par_num);

    /* Case step: preserve the original value before injecting out-of-range
     * inputs. Expected result is that cleanup can restore this value.
     */
    status = par_test_read_scalar(par_num, &original);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u read_status=%s", (unsigned)par_num, par_test_ram_status_str(status));

    /* Case step: write below the lower range limit. Expected result is
     * ePAR_WAR_LIMITED and readback equal to the lower limit.
     */
    PAR_TEST_ASSERT(ctx, true == par_test_make_below_range_scalar(par_num, &out),
                    "par=%u below_value_unavailable", (unsigned)par_num);
    PAR_TEST_ASSERT(ctx, true == par_test_get_range_limit_scalar(par_num, false, &limit),
                    "par=%u lower_limit_unavailable", (unsigned)par_num);

    status = par_test_set_scalar(par_num, &out);
    if (ePAR_WAR_LIMITED != status)
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u below_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    }

    status = par_test_read_scalar(par_num, &readback);
    if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) ||
        (false == par_test_scalar_equal(cfg->type, &limit, &readback)))
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u below_limit_mismatch status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    }

    /* Case step: write above the upper range limit. Expected result is
     * ePAR_WAR_LIMITED and readback equal to the upper limit.
     */
    PAR_TEST_ASSERT(ctx, true == par_test_make_above_range_scalar(par_num, &out),
                    "par=%u above_value_unavailable", (unsigned)par_num);
    PAR_TEST_ASSERT(ctx, true == par_test_get_range_limit_scalar(par_num, true, &limit),
                    "par=%u upper_limit_unavailable", (unsigned)par_num);

    status = par_test_set_scalar(par_num, &out);
    if (ePAR_WAR_LIMITED != status)
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u above_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    }

    status = par_test_read_scalar(par_num, &readback);
    if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) ||
        (false == par_test_scalar_equal(cfg->type, &limit, &readback)))
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u above_limit_mismatch status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    }

    status = par_test_set_scalar_fast(par_num, &original);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u restore_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    return ePAR_TEST_PASS;
#else
    PAR_TEST_SKIP(ctx, "range_disabled");
#endif /* (1 == PAR_CFG_ENABLE_RANGE) */
}

/**
 * @brief Validate optional name, unit, description, and description-check APIs.
 * @details Selects one parameter row, verifies enabled metadata getters return
 *          the same name, unit, and description pointers as the configuration,
 *          and checks description validity when the validator is enabled.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_ram_metadata_strings(par_test_context_t *ctx)
{
#if (1 == PAR_CFG_ENABLE_NAME) || (1 == PAR_CFG_ENABLE_UNIT) || (1 == PAR_CFG_ENABLE_DESC)
    par_num_t par_num = 0U;
    const par_cfg_t *cfg;

#if defined(AUTOGEN_PM_TEST_TABLE_ROWS)
    par_num = (par_num_t)ePAR_TEST_U8_RW;
#else
    for (par_num = 0U; par_num < (par_num_t)ePAR_NUM_OF; par_num++)
    {
        cfg = par_get_config(par_num);
        if (NULL != cfg)
        {
            break;
        }
    }
#endif /* defined(AUTOGEN_PM_TEST_TABLE_ROWS) */

    cfg = par_get_config(par_num);
    PAR_TEST_ASSERT(ctx, NULL != cfg, "metadata_row_unavailable");

    /* Case step: compare enabled metadata getters against the compiled row.
     * Expected result is pointer equality and valid description text.
     */
#if (1 == PAR_CFG_ENABLE_NAME)
    PAR_TEST_ASSERT(ctx, (NULL != par_get_name(par_num)) && (par_get_name(par_num) == cfg->name),
                    "par=%u name_mismatch", (unsigned)par_num);
#endif /* (1 == PAR_CFG_ENABLE_NAME) */
#if (1 == PAR_CFG_ENABLE_UNIT)
    PAR_TEST_ASSERT(ctx, (NULL != par_get_unit(par_num)) && (par_get_unit(par_num) == cfg->unit),
                    "par=%u unit_mismatch", (unsigned)par_num);
#endif /* (1 == PAR_CFG_ENABLE_UNIT) */
#if (1 == PAR_CFG_ENABLE_DESC)
    PAR_TEST_ASSERT(ctx, (NULL != par_get_desc(par_num)) && (par_get_desc(par_num) == cfg->desc),
                    "par=%u desc_mismatch", (unsigned)par_num);
#if (1 == PAR_CFG_ENABLE_DESC_CHECK)
    PAR_TEST_ASSERT(ctx, true == par_port_is_desc_valid(cfg->desc),
                    "par=%u desc_invalid", (unsigned)par_num);
#endif /* (1 == PAR_CFG_ENABLE_DESC_CHECK) */
#endif /* (1 == PAR_CFG_ENABLE_DESC) */
    return ePAR_TEST_PASS;
#else
    PAR_TEST_SKIP(ctx, "metadata_strings_disabled");
#endif /* (1 == PAR_CFG_ENABLE_NAME) || (1 == PAR_CFG_ENABLE_UNIT) || (1 == PAR_CFG_ENABLE_DESC) */
}

/**
 * @brief Validate external ID lookup and scalar by-ID accessors.
 * @details Resolves the selected parameter number to an external ID and back,
 *          writes an alternate scalar value through the by-ID setter, verifies
 *          by-ID getter readback, verifies invalid ID lookup/read/write errors
 *          do not modify the selected row, and restores the original value.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_ram_id_accessors(par_test_context_t *ctx)
{
#if (1 == PAR_CFG_ENABLE_ID)
    par_num_t par_num = 0U;
    par_num_t resolved = 0U;
    uint16_t id = 0U;
    uint16_t invalid_id = 0U;
    par_type_t original = { 0 };
    par_type_t alternate = { 0 };
    par_type_t readback = { 0 };
    par_status_t status;

    status = par_test_ensure_parameter_init();
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "init_status=%s", par_test_ram_status_str(status));

#if defined(AUTOGEN_PM_TEST_TABLE_ROWS)
    par_num = (par_num_t)ePAR_TEST_U16_RW;
#else
    if (false == par_test_find_mutable_writable_scalar(false, &par_num, &original, &alternate))
    {
        PAR_TEST_SKIP(ctx, "no_mutable_writable_scalar");
    }
#endif /* defined(AUTOGEN_PM_TEST_TABLE_ROWS) */

    /* Case step: resolve parameter number to ID and back. Expected result is
     * a stable bijection for the selected row.
     */
    status = par_get_id_by_num(par_num, &id);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u get_id_status=%s", (unsigned)par_num, par_test_ram_status_str(status));

    status = par_get_num_by_id(id, &resolved);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "id=%u get_num_status=%s", (unsigned)id, par_test_ram_status_str(status));
    PAR_TEST_ASSERT(ctx, resolved == par_num, "id=%u par=%u resolved=%u", (unsigned)id, (unsigned)par_num, (unsigned)resolved);

    status = par_test_read_scalar(par_num, &original);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u read_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    PAR_TEST_ASSERT(ctx, true == par_test_make_alternate_scalar(par_num, &original, &alternate),
                    "par=%u alternate_unavailable", (unsigned)par_num);

    /* Case step: access the same scalar through ID-based APIs. Expected
     * result is successful write and matching by-ID readback.
     */
    status = par_set_scalar_by_id(id, &alternate);
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "id=%u set_by_id_status=%s", (unsigned)id, par_test_ram_status_str(status));
    }

    status = par_get_scalar_by_id(id, &readback);
    if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) ||
        (false == par_test_scalar_equal(par_get_type(par_num), &alternate, &readback)))
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "id=%u get_by_id_mismatch status=%s", (unsigned)id, par_test_ram_status_str(status));
    }

    /* Case step: probe an unmapped external ID. Expected result is lookup,
     * read, and write failure without changing the selected scalar row.
     */
    PAR_TEST_ASSERT(ctx, true == par_test_ram_find_invalid_id(&invalid_id), "invalid_id_unavailable");

    status = par_get_num_by_id(invalid_id, &resolved);
    if (ePAR_ERROR != status)
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "invalid_id=%u get_num_status=%s", (unsigned)invalid_id, par_test_ram_status_str(status));
    }

    status = par_get_scalar_by_id(invalid_id, &readback);
    if (ePAR_ERROR != status)
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "invalid_id=%u get_by_id_status=%s", (unsigned)invalid_id, par_test_ram_status_str(status));
    }

    status = par_set_scalar_by_id(invalid_id, &original);
    if (ePAR_ERROR != status)
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "invalid_id=%u set_by_id_status=%s", (unsigned)invalid_id, par_test_ram_status_str(status));
    }

    status = par_test_read_scalar(par_num, &readback);
    if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) ||
        (false == par_test_scalar_equal(par_get_type(par_num), &alternate, &readback)))
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "invalid_id=%u polluted_value status=%s", (unsigned)invalid_id, par_test_ram_status_str(status));
    }

    status = par_test_set_scalar_fast(par_num, &original);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u restore_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    return ePAR_TEST_PASS;
#else
    PAR_TEST_SKIP(ctx, "id_disabled");
#endif /* (1 == PAR_CFG_ENABLE_ID) */
}

/**
 * @brief Validate read-only access policy blocks normal writes.
 * @details Selects a read-only scalar row, prepares a valid alternate value,
 *          attempts a normal write, and verifies the API rejects it with
 *          ePAR_ERROR_ACCESS while preserving the original value.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_ram_access_policy(par_test_context_t *ctx)
{
#if (1 == PAR_CFG_ENABLE_ACCESS)
    par_num_t par_num = 0U;
    par_type_t original = { 0 };
    par_type_t alternate = { 0 };
    par_type_t readback = { 0 };
    par_status_t status;

    status = par_test_ensure_parameter_init();
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "init_status=%s", par_test_ram_status_str(status));

#if defined(AUTOGEN_PM_TEST_TABLE_ROWS)
    par_num = (par_num_t)ePAR_TEST_U8_RO;
#else
    for (par_num = 0U; par_num < (par_num_t)ePAR_NUM_OF; par_num++)
    {
        const par_cfg_t *cfg = par_get_config(par_num);
        if ((NULL != cfg) && (true == par_test_type_is_scalar(cfg->type)) &&
            (false == par_test_cfg_is_writable(cfg)))
        {
            break;
        }
    }
    if (par_num >= (par_num_t)ePAR_NUM_OF)
    {
        PAR_TEST_SKIP(ctx, "no_readonly_scalar");
    }
#endif /* defined(AUTOGEN_PM_TEST_TABLE_ROWS) */

    /* Case step: try a valid value on a read-only row. Expected result is an
     * access error, proving the failure is policy-related, not value-related.
     */
    status = par_test_read_scalar(par_num, &original);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u read_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    PAR_TEST_ASSERT(ctx, true == par_test_make_alternate_scalar(par_num, &original, &alternate),
                    "par=%u alternate_unavailable", (unsigned)par_num);

    status = par_test_set_scalar(par_num, &alternate);
    PAR_TEST_ASSERT(ctx, ePAR_ERROR_ACCESS == status,
                    "par=%u write_status=%s", (unsigned)par_num, par_test_ram_status_str(status));

    status = par_test_read_scalar(par_num, &readback);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u readback_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    PAR_TEST_ASSERT(ctx, true == par_test_scalar_equal(par_get_type(par_num), &original, &readback),
                    "par=%u readonly_value_polluted", (unsigned)par_num);
    return ePAR_TEST_PASS;
#else
    PAR_TEST_SKIP(ctx, "access_disabled");
#endif /* (1 == PAR_CFG_ENABLE_ACCESS) */
}

#if (1 == PAR_CFG_ENABLE_TYPE_F32)
/**
 * @brief Validate an F32 scalar set/get/restore path.
 * @details Selects an F32 scalar row and uses the generic scalar helper to
 *          verify valid write, F32 readback equality, restoration, and when
 *          range metadata is enabled, exact lower and upper boundary writes.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_ram_f32_scalar(par_test_context_t *ctx)
{
    par_num_t par_num = 0U;
    par_status_t status = par_test_ensure_parameter_init();

    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "init_status=%s", par_test_ram_status_str(status));

#if defined(AUTOGEN_PM_TEST_TABLE_ROWS)
    /* Case step: use the dedicated F32 row. Expected result is the same valid
     * write, readback, and restore behavior as other scalar types.
     */
    par_num = (par_num_t)ePAR_TEST_F32_RW;
#else
    if (false == par_test_ram_find_scalar_type(ePAR_TYPE_F32, &par_num))
    {
        PAR_TEST_SKIP(ctx, "no_f32_scalar");
    }
#endif /* defined(AUTOGEN_PM_TEST_TABLE_ROWS) */

#if (1 == PAR_CFG_ENABLE_RANGE)
    /* Case step: write exact F32 range boundaries. Expected result is that
     * min and max values are accepted without limit warnings and read back
     * unchanged, then the original runtime value is restored.
     */
    {
        const par_cfg_t *cfg = par_get_config(par_num);
        par_type_t original = { 0 };
        par_type_t lower = { 0 };
        par_type_t upper = { 0 };
        par_type_t readback = { 0 };

        PAR_TEST_ASSERT(ctx, (NULL != cfg) && (ePAR_TYPE_F32 == cfg->type),
                        "par=%u f32_unavailable", (unsigned)par_num);
        status = par_test_read_scalar(par_num, &original);
        PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                        "par=%u read_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
        PAR_TEST_ASSERT(ctx, true == par_test_get_range_limit_scalar(par_num, false, &lower),
                        "par=%u lower_limit_unavailable", (unsigned)par_num);
        PAR_TEST_ASSERT(ctx, true == par_test_get_range_limit_scalar(par_num, true, &upper),
                        "par=%u upper_limit_unavailable", (unsigned)par_num);

        status = par_test_set_scalar(par_num, &lower);
        if (ePAR_OK != status)
        {
            (void)par_test_set_scalar_fast(par_num, &original);
            PAR_TEST_FAIL(ctx, "par=%u f32_lower_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
        }
        status = par_test_read_scalar(par_num, &readback);
        if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) ||
            (false == par_test_scalar_equal(cfg->type, &lower, &readback)))
        {
            (void)par_test_set_scalar_fast(par_num, &original);
            PAR_TEST_FAIL(ctx, "par=%u f32_lower_mismatch status=%s", (unsigned)par_num, par_test_ram_status_str(status));
        }

        status = par_test_set_scalar(par_num, &upper);
        if (ePAR_OK != status)
        {
            (void)par_test_set_scalar_fast(par_num, &original);
            PAR_TEST_FAIL(ctx, "par=%u f32_upper_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
        }
        status = par_test_read_scalar(par_num, &readback);
        if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) ||
            (false == par_test_scalar_equal(cfg->type, &upper, &readback)))
        {
            (void)par_test_set_scalar_fast(par_num, &original);
            PAR_TEST_FAIL(ctx, "par=%u f32_upper_mismatch status=%s", (unsigned)par_num, par_test_ram_status_str(status));
        }

        status = par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                        "par=%u f32_restore_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    }
#endif /* (1 == PAR_CFG_ENABLE_RANGE) */

    return par_test_ram_check_scalar_set_get(ctx, par_num);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */

#if (1 == PAR_CFG_ENABLE_CHANGE_CALLBACK)
/**
 * @brief Change-callback invocation counter used by one test case.
 */
static uint32_t g_par_test_change_count;

/**
 * @brief Old value observed by the change-callback case.
 */
static par_type_t g_par_test_change_old;

/**
 * @brief New value observed by the change-callback case.
 */
static par_type_t g_par_test_change_new;

/**
 * @brief Record one scalar change callback invocation.
 * @param par_num Parameter number.
 * @param new_val New scalar value.
 * @param old_val Previous scalar value.
 */
static void par_test_ram_on_change_cb(const par_num_t par_num, const par_type_t new_val, const par_type_t old_val)
{
    (void)par_num;
    g_par_test_change_count++;
    g_par_test_change_new = new_val;
    g_par_test_change_old = old_val;
}

/**
 * @brief Validate scalar on-change callback dispatch in RAM mode.
 * @details Registers a change callback for a writable scalar, writes a valid
 *          alternate value, verifies exactly one callback reports the expected
 *          old and new values, unregisters it, and restores the original value.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_ram_change_callback(par_test_context_t *ctx)
{
    par_num_t par_num = 0U;
    const par_cfg_t *cfg;
    par_type_t original = { 0 };
    par_type_t alternate = { 0 };
    par_status_t status;

    status = par_test_ensure_parameter_init();
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "init_status=%s", par_test_ram_status_str(status));

#if defined(AUTOGEN_PM_TEST_TABLE_ROWS)
    par_num = (par_num_t)ePAR_TEST_U32_RW;
    status = par_test_read_scalar(par_num, &original);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u read_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    PAR_TEST_ASSERT(ctx, true == par_test_make_alternate_scalar(par_num, &original, &alternate),
                    "par=%u alternate_unavailable", (unsigned)par_num);
#else
    if (false == par_test_find_mutable_writable_scalar(false, &par_num, &original, &alternate))
    {
        PAR_TEST_SKIP(ctx, "no_mutable_writable_scalar");
    }
#endif /* defined(AUTOGEN_PM_TEST_TABLE_ROWS) */

    cfg = par_get_config(par_num);
    PAR_TEST_ASSERT(ctx, NULL != cfg, "par=%u cfg=null", (unsigned)par_num);

    /* Case step: register a callback and clear captured state. Expected
     * result is one callback carrying the original and alternate values.
     */
    g_par_test_change_count = 0U;
    (void)memset(&g_par_test_change_old, 0, sizeof(g_par_test_change_old));
    (void)memset(&g_par_test_change_new, 0, sizeof(g_par_test_change_new));
    par_register_on_change_cb(par_num, par_test_ram_on_change_cb);

    status = par_test_set_scalar(par_num, &alternate);
    par_register_on_change_cb(par_num, NULL);
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u set_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    }

    /* Case step: validate callback observations. Expected result is exactly
     * one event with old value equal to original and new value equal to write.
     */
    if ((1U != g_par_test_change_count) ||
        (false == par_test_scalar_equal(cfg->type, &original, &g_par_test_change_old)) ||
        (false == par_test_scalar_equal(cfg->type, &alternate, &g_par_test_change_new)))
    {
        (void)par_test_set_scalar_fast(par_num, &original);
        PAR_TEST_FAIL(ctx, "par=%u callback_mismatch count=%u", (unsigned)par_num, (unsigned)g_par_test_change_count);
    }

    status = par_test_set_scalar_fast(par_num, &original);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u restore_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    return ePAR_TEST_PASS;
}
#endif /* (1 == PAR_CFG_ENABLE_CHANGE_CALLBACK) */

#if (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION)
/**
 * @brief Parameter number under validation callback test.
 */
static par_num_t g_par_test_validation_par;

/**
 * @brief Scalar type under validation callback test.
 */
static par_type_list_t g_par_test_validation_type;

/**
 * @brief Rejected scalar value under validation callback test.
 */
static par_type_t g_par_test_validation_reject;

/**
 * @brief Reject exactly one scalar value during validation callback testing.
 * @param par_num Parameter number.
 * @param val Candidate scalar value.
 * @return false for the configured rejected value; otherwise true.
 */
static bool par_test_ram_validation_cb(const par_num_t par_num, const par_type_t val)
{
    if (par_num != g_par_test_validation_par)
    {
        return true;
    }

    return (false == par_test_scalar_equal(g_par_test_validation_type, &val, &g_par_test_validation_reject));
}

/**
 * @brief Validate scalar runtime validation callback rejection.
 * @details Registers a validation callback that rejects one prepared alternate
 *          value, verifies the setter returns ePAR_ERROR_VALUE, and confirms
 *          the rejected value was not committed to storage.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_ram_runtime_validation(par_test_context_t *ctx)
{
    par_num_t par_num = 0U;
    const par_cfg_t *cfg;
    par_type_t original = { 0 };
    par_type_t alternate = { 0 };
    par_type_t readback = { 0 };
    par_status_t status;

    status = par_test_ensure_parameter_init();
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "init_status=%s", par_test_ram_status_str(status));

#if defined(AUTOGEN_PM_TEST_TABLE_ROWS)
    par_num = (par_num_t)ePAR_TEST_I16_RW;
    status = par_test_read_scalar(par_num, &original);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u read_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    PAR_TEST_ASSERT(ctx, true == par_test_make_alternate_scalar(par_num, &original, &alternate),
                    "par=%u alternate_unavailable", (unsigned)par_num);
#else
    if (false == par_test_find_mutable_writable_scalar(false, &par_num, &original, &alternate))
    {
        PAR_TEST_SKIP(ctx, "no_mutable_writable_scalar");
    }
#endif /* defined(AUTOGEN_PM_TEST_TABLE_ROWS) */

    cfg = par_get_config(par_num);
    PAR_TEST_ASSERT(ctx, NULL != cfg, "par=%u cfg=null", (unsigned)par_num);

    /* Case step: reject the prepared alternate value through validation.
     * Expected result is ePAR_ERROR_VALUE and unchanged stored data.
     */
    g_par_test_validation_par = par_num;
    g_par_test_validation_type = cfg->type;
    g_par_test_validation_reject = alternate;
    par_register_validation(par_num, par_test_ram_validation_cb);

    status = par_test_set_scalar(par_num, &alternate);
    par_register_validation(par_num, NULL);
    PAR_TEST_ASSERT(ctx, ePAR_ERROR_VALUE == status,
                    "par=%u validation_status=%s", (unsigned)par_num, par_test_ram_status_str(status));

    status = par_test_read_scalar(par_num, &readback);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u readback_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    PAR_TEST_ASSERT(ctx, true == par_test_scalar_equal(cfg->type, &original, &readback),
                    "par=%u rejected_value_committed", (unsigned)par_num);
    return ePAR_TEST_PASS;
}
#endif /* (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION) */

#if (1 == PAR_CFG_ENABLE_RESET_ALL_RAW)
/**
 * @brief Validate raw reset-all restores defaults from RAM mirror storage.
 * @details Writes a valid alternate value to a scalar row, runs raw reset-all,
 *          verifies the selected row reads back its configured default, and
 *          restores the original value after the check.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_ram_reset_all_raw(par_test_context_t *ctx)
{
    par_num_t par_num = 0U;
    const par_cfg_t *cfg;
    par_type_t original = { 0 };
    par_type_t alternate = { 0 };
    par_type_t readback = { 0 };
    par_type_t def = { 0 };
    par_status_t status;

    status = par_test_ensure_parameter_init();
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "init_status=%s", par_test_ram_status_str(status));

#if defined(AUTOGEN_PM_TEST_TABLE_ROWS)
    par_num = (par_num_t)ePAR_TEST_I32_RW;
    status = par_test_read_scalar(par_num, &original);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u read_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    PAR_TEST_ASSERT(ctx, true == par_test_make_alternate_scalar(par_num, &original, &alternate),
                    "par=%u alternate_unavailable", (unsigned)par_num);
#else
    if (false == par_test_find_mutable_writable_scalar(false, &par_num, &original, &alternate))
    {
        PAR_TEST_SKIP(ctx, "no_mutable_writable_scalar");
    }
#endif /* defined(AUTOGEN_PM_TEST_TABLE_ROWS) */

    cfg = par_get_config(par_num);
    PAR_TEST_ASSERT(ctx, NULL != cfg, "par=%u cfg=null", (unsigned)par_num);

    /* Case step: prime storage away from default before raw reset. Expected
     * result is that reset-all restores the configured default value.
     */
    status = par_test_set_scalar_fast(par_num, &alternate);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u prime_status=%s", (unsigned)par_num, par_test_ram_status_str(status));

    status = par_get_scalar_default(par_num, &def);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u default_status=%s", (unsigned)par_num, par_test_ram_status_str(status));

    status = par_reset_all_to_default_raw();
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "reset_raw_status=%s", par_test_ram_status_str(status));

    status = par_test_read_scalar(par_num, &readback);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u readback_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    PAR_TEST_ASSERT(ctx, true == par_test_scalar_equal(cfg->type, &def, &readback),
                    "par=%u raw_default_mismatch", (unsigned)par_num);

    status = par_test_set_scalar_fast(par_num, &original);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "par=%u restore_status=%s", (unsigned)par_num, par_test_ram_status_str(status));
    return ePAR_TEST_PASS;
}
#endif /* (1 == PAR_CFG_ENABLE_RESET_ALL_RAW) */

#if defined(AUTOGEN_PM_TEST_OBJECT_ROWS) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
/**
 * @brief Validate a STR object parameter set/get/default path.
 * @details Preserves the original string, writes short, maximum-length, and
 *          empty valid strings, verifies readback content and length, verifies
 *          the compiled default string, rejects an over-length write, confirms
 *          the rejected write does not modify the committed payload, and
 *          restores the original string payload.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_ram_object_string(par_test_context_t *ctx)
{
#if (1 == PAR_CFG_ENABLE_TYPE_STR) && defined(AUTOGEN_PM_TEST_OBJECT_ROWS)
    char original[16] = { 0 };
    char readback[16] = { 0 };
    char def[16] = { 0 };
    const char max_str[] = "12345678";
    const char over_str[] = "123456789";
    uint16_t original_len = 0U;
    uint16_t readback_len = 0U;
    uint16_t def_len = 0U;
    par_status_t status = par_test_ensure_parameter_init();

    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "init_status=%s", par_test_ram_status_str(status));

    /* Case step: preserve original STR data and write "ci". Expected result
     * is readback content "ci" with length 2.
     */
    status = par_get_str((par_num_t)ePAR_TEST_STR_RW, original, (uint16_t)sizeof(original), &original_len);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "str_read_status=%s", par_test_ram_status_str(status));

    status = par_set_str((par_num_t)ePAR_TEST_STR_RW, "ci");
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_set_str((par_num_t)ePAR_TEST_STR_RW, original);
        PAR_TEST_FAIL(ctx, "str_set_status=%s", par_test_ram_status_str(status));
    }

    status = par_get_str((par_num_t)ePAR_TEST_STR_RW, readback, (uint16_t)sizeof(readback), &readback_len);
    if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) || (0 != strcmp(readback, "ci")) || (2U != readback_len))
    {
        (void)par_set_str((par_num_t)ePAR_TEST_STR_RW, original);
        PAR_TEST_FAIL(ctx, "str_readback_mismatch status=%s len=%u", par_test_ram_status_str(status), (unsigned)readback_len);
    }

    /* Case step: query generated STR default. Expected result is default
     * content "ap" with length 2.
     */
    status = par_get_default_str((par_num_t)ePAR_TEST_STR_RW, def, (uint16_t)sizeof(def), &def_len);
    if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) || (0 != strcmp(def, "ap")) || (2U != def_len))
    {
        (void)par_set_str((par_num_t)ePAR_TEST_STR_RW, original);
        PAR_TEST_FAIL(ctx, "str_default_mismatch status=%s len=%u", par_test_ram_status_str(status), (unsigned)def_len);
    }

    /* Case step: write the maximum accepted STR length. Expected result is
     * readback content "12345678" with length 8.
     */
    status = par_set_str((par_num_t)ePAR_TEST_STR_RW, max_str);
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_set_str((par_num_t)ePAR_TEST_STR_RW, original);
        PAR_TEST_FAIL(ctx, "str_max_set_status=%s", par_test_ram_status_str(status));
    }

    status = par_get_str((par_num_t)ePAR_TEST_STR_RW, readback, (uint16_t)sizeof(readback), &readback_len);
    if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) || (0 != strcmp(readback, max_str)) || (8U != readback_len))
    {
        (void)par_set_str((par_num_t)ePAR_TEST_STR_RW, original);
        PAR_TEST_FAIL(ctx, "str_max_readback_mismatch status=%s len=%u", par_test_ram_status_str(status), (unsigned)readback_len);
    }

    /* Case step: reject one byte beyond the maximum STR length. Expected
     * result is ePAR_ERROR_VALUE and the prior max-length value remains live.
     */
    status = par_set_str((par_num_t)ePAR_TEST_STR_RW, over_str);
    if (ePAR_ERROR_VALUE != status)
    {
        (void)par_set_str((par_num_t)ePAR_TEST_STR_RW, original);
        PAR_TEST_FAIL(ctx, "str_over_status=%s", par_test_ram_status_str(status));
    }

    status = par_get_str((par_num_t)ePAR_TEST_STR_RW, readback, (uint16_t)sizeof(readback), &readback_len);
    if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) || (0 != strcmp(readback, max_str)) || (8U != readback_len))
    {
        (void)par_set_str((par_num_t)ePAR_TEST_STR_RW, original);
        PAR_TEST_FAIL(ctx, "str_over_polluted status=%s len=%u", par_test_ram_status_str(status), (unsigned)readback_len);
    }

    /* Case step: write an empty string allowed by min=0. Expected result is
     * an empty readback payload with length 0.
     */
    status = par_set_str((par_num_t)ePAR_TEST_STR_RW, "");
    if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
    {
        (void)par_set_str((par_num_t)ePAR_TEST_STR_RW, original);
        PAR_TEST_FAIL(ctx, "str_empty_set_status=%s", par_test_ram_status_str(status));
    }

    status = par_get_str((par_num_t)ePAR_TEST_STR_RW, readback, (uint16_t)sizeof(readback), &readback_len);
    if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) || (0 != strcmp(readback, "")) || (0U != readback_len))
    {
        (void)par_set_str((par_num_t)ePAR_TEST_STR_RW, original);
        PAR_TEST_FAIL(ctx, "str_empty_readback_mismatch status=%s len=%u", par_test_ram_status_str(status), (unsigned)readback_len);
    }

    status = par_set_str((par_num_t)ePAR_TEST_STR_RW, original);
    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status),
                    "str_restore_status=%s", par_test_ram_status_str(status));
    (void)original_len;
    return ePAR_TEST_PASS;
#else
    PAR_TEST_SKIP(ctx, "str_test_row_disabled");
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) && defined(AUTOGEN_PM_TEST_OBJECT_ROWS) */
}

/**
 * @brief Validate byte and array object parameter set/get/default paths.
 * @details For each enabled BYTES, ARR_U8, ARR_U16, and ARR_U32 object row,
 *          preserves the original payload, writes a test payload, verifies
 *          readback and count, rejects below-size and above-size writes,
 *          verifies rejected writes do not change the committed payload,
 *          verifies the compiled default, and restores the original payload.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_ram_object_arrays(par_test_context_t *ctx)
{
#if defined(AUTOGEN_PM_TEST_OBJECT_ROWS)
    par_status_t status = par_test_ensure_parameter_init();

    PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "init_status=%s", par_test_ram_status_str(status));

#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
    /* BYTES subcase: write four bytes, reject 3-byte and 5-byte
     * payloads without state pollution, verify default bytes, then restore the
     * original byte payload.
     */
    {
        uint8_t original[4] = { 0 };
        uint8_t bytes[4] = { 0x10U, 0x20U, 0x30U, 0x40U };
        uint8_t short_bytes[3] = { 0xA0U, 0xA1U, 0xA2U };
        uint8_t long_bytes[5] = { 0xB0U, 0xB1U, 0xB2U, 0xB3U, 0xB4U };
        uint8_t readback[4] = { 0 };
        uint8_t def[4] = { 0 };
        uint16_t len = 0U;

        status = par_get_bytes((par_num_t)ePAR_TEST_BYTES_RW, original, (uint16_t)sizeof(original), &len);
        PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status) && (4U == len),
                        "bytes_original_status=%s len=%u", par_test_ram_status_str(status), (unsigned)len);
        status = par_set_bytes((par_num_t)ePAR_TEST_BYTES_RW, bytes, (uint16_t)sizeof(bytes));
        PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "bytes_set_status=%s", par_test_ram_status_str(status));
        status = par_get_bytes((par_num_t)ePAR_TEST_BYTES_RW, readback, (uint16_t)sizeof(readback), &len);
        if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) || (4U != len) || (0 != memcmp(bytes, readback, 4U)))
        {
            (void)par_set_bytes((par_num_t)ePAR_TEST_BYTES_RW, original, (uint16_t)sizeof(original));
            PAR_TEST_FAIL(ctx, "bytes_readback_mismatch status=%s len=%u", par_test_ram_status_str(status), (unsigned)len);
        }
        status = par_set_bytes((par_num_t)ePAR_TEST_BYTES_RW, short_bytes, (uint16_t)sizeof(short_bytes));
        if (ePAR_ERROR_VALUE != status)
        {
            (void)par_set_bytes((par_num_t)ePAR_TEST_BYTES_RW, original, (uint16_t)sizeof(original));
            PAR_TEST_FAIL(ctx, "bytes_short_status=%s", par_test_ram_status_str(status));
        }
        status = par_get_bytes((par_num_t)ePAR_TEST_BYTES_RW, readback, (uint16_t)sizeof(readback), &len);
        if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) || (4U != len) || (0 != memcmp(bytes, readback, 4U)))
        {
            (void)par_set_bytes((par_num_t)ePAR_TEST_BYTES_RW, original, (uint16_t)sizeof(original));
            PAR_TEST_FAIL(ctx, "bytes_short_polluted status=%s len=%u", par_test_ram_status_str(status), (unsigned)len);
        }
        status = par_set_bytes((par_num_t)ePAR_TEST_BYTES_RW, long_bytes, (uint16_t)sizeof(long_bytes));
        if (ePAR_ERROR_VALUE != status)
        {
            (void)par_set_bytes((par_num_t)ePAR_TEST_BYTES_RW, original, (uint16_t)sizeof(original));
            PAR_TEST_FAIL(ctx, "bytes_long_status=%s", par_test_ram_status_str(status));
        }
        status = par_get_bytes((par_num_t)ePAR_TEST_BYTES_RW, readback, (uint16_t)sizeof(readback), &len);
        if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) || (4U != len) || (0 != memcmp(bytes, readback, 4U)))
        {
            (void)par_set_bytes((par_num_t)ePAR_TEST_BYTES_RW, original, (uint16_t)sizeof(original));
            PAR_TEST_FAIL(ctx, "bytes_long_polluted status=%s len=%u", par_test_ram_status_str(status), (unsigned)len);
        }
        status = par_get_default_bytes((par_num_t)ePAR_TEST_BYTES_RW, def, (uint16_t)sizeof(def), &len);
        if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) || (4U != len) ||
            (0 != memcmp(def, (const uint8_t[]){ 0x00U, 0x11U, 0x22U, 0x33U }, 4U)))
        {
            (void)par_set_bytes((par_num_t)ePAR_TEST_BYTES_RW, original, (uint16_t)sizeof(original));
            PAR_TEST_FAIL(ctx, "bytes_default_mismatch status=%s len=%u", par_test_ram_status_str(status), (unsigned)len);
        }
        status = par_set_bytes((par_num_t)ePAR_TEST_BYTES_RW, original, (uint16_t)sizeof(original));
        PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "bytes_restore_status=%s", par_test_ram_status_str(status));
    }
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
    /* ARR_U8 subcase: write four elements, reject 3-element and 5-element
     * writes without state pollution, verify default elements, then restore the
     * original array payload.
     */
    {
        uint8_t original[4] = { 0 };
        uint8_t arr[4] = { 9U, 8U, 7U, 6U };
        uint8_t short_arr[3] = { 1U, 3U, 5U };
        uint8_t long_arr[5] = { 2U, 4U, 6U, 8U, 10U };
        uint8_t readback[4] = { 0 };
        uint8_t def[4] = { 0 };
        uint16_t count = 0U;

        status = par_get_arr_u8((par_num_t)ePAR_TEST_ARR_U8_RW, original, 4U, &count);
        PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status) && (4U == count),
                        "arr_u8_original_status=%s count=%u", par_test_ram_status_str(status), (unsigned)count);
        status = par_set_arr_u8((par_num_t)ePAR_TEST_ARR_U8_RW, arr, 4U);
        PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "arr_u8_set_status=%s", par_test_ram_status_str(status));
        status = par_get_arr_u8((par_num_t)ePAR_TEST_ARR_U8_RW, readback, 4U, &count);
        if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) || (4U != count) || (0 != memcmp(arr, readback, 4U)))
        {
            (void)par_set_arr_u8((par_num_t)ePAR_TEST_ARR_U8_RW, original, 4U);
            PAR_TEST_FAIL(ctx, "arr_u8_readback_mismatch status=%s count=%u", par_test_ram_status_str(status), (unsigned)count);
        }
        status = par_set_arr_u8((par_num_t)ePAR_TEST_ARR_U8_RW, short_arr, 3U);
        if (ePAR_ERROR_VALUE != status)
        {
            (void)par_set_arr_u8((par_num_t)ePAR_TEST_ARR_U8_RW, original, 4U);
            PAR_TEST_FAIL(ctx, "arr_u8_short_status=%s", par_test_ram_status_str(status));
        }
        status = par_get_arr_u8((par_num_t)ePAR_TEST_ARR_U8_RW, readback, 4U, &count);
        if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) || (4U != count) || (0 != memcmp(arr, readback, 4U)))
        {
            (void)par_set_arr_u8((par_num_t)ePAR_TEST_ARR_U8_RW, original, 4U);
            PAR_TEST_FAIL(ctx, "arr_u8_short_polluted status=%s count=%u", par_test_ram_status_str(status), (unsigned)count);
        }
        status = par_set_arr_u8((par_num_t)ePAR_TEST_ARR_U8_RW, long_arr, 5U);
        if (ePAR_ERROR_VALUE != status)
        {
            (void)par_set_arr_u8((par_num_t)ePAR_TEST_ARR_U8_RW, original, 4U);
            PAR_TEST_FAIL(ctx, "arr_u8_long_status=%s", par_test_ram_status_str(status));
        }
        status = par_get_arr_u8((par_num_t)ePAR_TEST_ARR_U8_RW, readback, 4U, &count);
        if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) || (4U != count) || (0 != memcmp(arr, readback, 4U)))
        {
            (void)par_set_arr_u8((par_num_t)ePAR_TEST_ARR_U8_RW, original, 4U);
            PAR_TEST_FAIL(ctx, "arr_u8_long_polluted status=%s count=%u", par_test_ram_status_str(status), (unsigned)count);
        }
        status = par_get_default_arr_u8((par_num_t)ePAR_TEST_ARR_U8_RW, def, 4U, &count);
        if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) || (4U != count) ||
            (0 != memcmp(def, (const uint8_t[]){ 1U, 2U, 3U, 4U }, 4U)))
        {
            (void)par_set_arr_u8((par_num_t)ePAR_TEST_ARR_U8_RW, original, 4U);
            PAR_TEST_FAIL(ctx, "arr_u8_default_mismatch status=%s count=%u", par_test_ram_status_str(status), (unsigned)count);
        }
        status = par_set_arr_u8((par_num_t)ePAR_TEST_ARR_U8_RW, original, 4U);
        PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "arr_u8_restore_status=%s", par_test_ram_status_str(status));
    }
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
    /* ARR_U16 subcase: write three elements, reject 2-element and 4-element
     * writes without state pollution, verify default elements, then restore the
     * original array payload.
     */
    {
        uint16_t original[3] = { 0 };
        uint16_t arr[3] = { 300U, 200U, 100U };
        uint16_t short_arr[2] = { 11U, 22U };
        uint16_t long_arr[4] = { 44U, 33U, 22U, 11U };
        uint16_t readback[3] = { 0 };
        uint16_t def[3] = { 0 };
        uint16_t count = 0U;

        status = par_get_arr_u16((par_num_t)ePAR_TEST_ARR_U16_RW, original, 3U, &count);
        PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status) && (3U == count),
                        "arr_u16_original_status=%s count=%u", par_test_ram_status_str(status), (unsigned)count);
        status = par_set_arr_u16((par_num_t)ePAR_TEST_ARR_U16_RW, arr, 3U);
        PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "arr_u16_set_status=%s", par_test_ram_status_str(status));
        status = par_get_arr_u16((par_num_t)ePAR_TEST_ARR_U16_RW, readback, 3U, &count);
        if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) || (3U != count) || (0 != memcmp(arr, readback, sizeof(arr))))
        {
            (void)par_set_arr_u16((par_num_t)ePAR_TEST_ARR_U16_RW, original, 3U);
            PAR_TEST_FAIL(ctx, "arr_u16_readback_mismatch status=%s count=%u", par_test_ram_status_str(status), (unsigned)count);
        }
        status = par_set_arr_u16((par_num_t)ePAR_TEST_ARR_U16_RW, short_arr, 2U);
        if (ePAR_ERROR_VALUE != status)
        {
            (void)par_set_arr_u16((par_num_t)ePAR_TEST_ARR_U16_RW, original, 3U);
            PAR_TEST_FAIL(ctx, "arr_u16_short_status=%s", par_test_ram_status_str(status));
        }
        status = par_get_arr_u16((par_num_t)ePAR_TEST_ARR_U16_RW, readback, 3U, &count);
        if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) || (3U != count) || (0 != memcmp(arr, readback, sizeof(arr))))
        {
            (void)par_set_arr_u16((par_num_t)ePAR_TEST_ARR_U16_RW, original, 3U);
            PAR_TEST_FAIL(ctx, "arr_u16_short_polluted status=%s count=%u", par_test_ram_status_str(status), (unsigned)count);
        }
        status = par_set_arr_u16((par_num_t)ePAR_TEST_ARR_U16_RW, long_arr, 4U);
        if (ePAR_ERROR_VALUE != status)
        {
            (void)par_set_arr_u16((par_num_t)ePAR_TEST_ARR_U16_RW, original, 3U);
            PAR_TEST_FAIL(ctx, "arr_u16_long_status=%s", par_test_ram_status_str(status));
        }
        status = par_get_arr_u16((par_num_t)ePAR_TEST_ARR_U16_RW, readback, 3U, &count);
        if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) || (3U != count) || (0 != memcmp(arr, readback, sizeof(arr))))
        {
            (void)par_set_arr_u16((par_num_t)ePAR_TEST_ARR_U16_RW, original, 3U);
            PAR_TEST_FAIL(ctx, "arr_u16_long_polluted status=%s count=%u", par_test_ram_status_str(status), (unsigned)count);
        }
        status = par_get_default_arr_u16((par_num_t)ePAR_TEST_ARR_U16_RW, def, 3U, &count);
        if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) || (3U != count) ||
            (0 != memcmp(def, (const uint16_t[]){ 100U, 200U, 300U }, sizeof(def))))
        {
            (void)par_set_arr_u16((par_num_t)ePAR_TEST_ARR_U16_RW, original, 3U);
            PAR_TEST_FAIL(ctx, "arr_u16_default_mismatch status=%s count=%u", par_test_ram_status_str(status), (unsigned)count);
        }
        status = par_set_arr_u16((par_num_t)ePAR_TEST_ARR_U16_RW, original, 3U);
        PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "arr_u16_restore_status=%s", par_test_ram_status_str(status));
    }
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
    /* ARR_U32 subcase: write two elements, reject 1-element and 3-element
     * writes without state pollution, verify default elements, then restore the
     * original array payload.
     */
    {
        uint32_t original[2] = { 0 };
        uint32_t arr[2] = { 4000UL, 3000UL };
        uint32_t short_arr[1] = { 11UL };
        uint32_t long_arr[3] = { 33UL, 22UL, 11UL };
        uint32_t readback[2] = { 0 };
        uint32_t def[2] = { 0 };
        uint16_t count = 0U;

        status = par_get_arr_u32((par_num_t)ePAR_TEST_ARR_U32_RW, original, 2U, &count);
        PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status) && (2U == count),
                        "arr_u32_original_status=%s count=%u", par_test_ram_status_str(status), (unsigned)count);
        status = par_set_arr_u32((par_num_t)ePAR_TEST_ARR_U32_RW, arr, 2U);
        PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "arr_u32_set_status=%s", par_test_ram_status_str(status));
        status = par_get_arr_u32((par_num_t)ePAR_TEST_ARR_U32_RW, readback, 2U, &count);
        if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) || (2U != count) || (0 != memcmp(arr, readback, sizeof(arr))))
        {
            (void)par_set_arr_u32((par_num_t)ePAR_TEST_ARR_U32_RW, original, 2U);
            PAR_TEST_FAIL(ctx, "arr_u32_readback_mismatch status=%s count=%u", par_test_ram_status_str(status), (unsigned)count);
        }
        status = par_set_arr_u32((par_num_t)ePAR_TEST_ARR_U32_RW, short_arr, 1U);
        if (ePAR_ERROR_VALUE != status)
        {
            (void)par_set_arr_u32((par_num_t)ePAR_TEST_ARR_U32_RW, original, 2U);
            PAR_TEST_FAIL(ctx, "arr_u32_short_status=%s", par_test_ram_status_str(status));
        }
        status = par_get_arr_u32((par_num_t)ePAR_TEST_ARR_U32_RW, readback, 2U, &count);
        if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) || (2U != count) || (0 != memcmp(arr, readback, sizeof(arr))))
        {
            (void)par_set_arr_u32((par_num_t)ePAR_TEST_ARR_U32_RW, original, 2U);
            PAR_TEST_FAIL(ctx, "arr_u32_short_polluted status=%s count=%u", par_test_ram_status_str(status), (unsigned)count);
        }
        status = par_set_arr_u32((par_num_t)ePAR_TEST_ARR_U32_RW, long_arr, 3U);
        if (ePAR_ERROR_VALUE != status)
        {
            (void)par_set_arr_u32((par_num_t)ePAR_TEST_ARR_U32_RW, original, 2U);
            PAR_TEST_FAIL(ctx, "arr_u32_long_status=%s", par_test_ram_status_str(status));
        }
        status = par_get_arr_u32((par_num_t)ePAR_TEST_ARR_U32_RW, readback, 2U, &count);
        if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) || (2U != count) || (0 != memcmp(arr, readback, sizeof(arr))))
        {
            (void)par_set_arr_u32((par_num_t)ePAR_TEST_ARR_U32_RW, original, 2U);
            PAR_TEST_FAIL(ctx, "arr_u32_long_polluted status=%s count=%u", par_test_ram_status_str(status), (unsigned)count);
        }
        status = par_get_default_arr_u32((par_num_t)ePAR_TEST_ARR_U32_RW, def, 2U, &count);
        if ((false == PAR_TEST_STATUS_HAS_NO_ERROR(status)) || (2U != count) ||
            (0 != memcmp(def, (const uint32_t[]){ 1000UL, 2000UL }, sizeof(def))))
        {
            (void)par_set_arr_u32((par_num_t)ePAR_TEST_ARR_U32_RW, original, 2U);
            PAR_TEST_FAIL(ctx, "arr_u32_default_mismatch status=%s count=%u", par_test_ram_status_str(status), (unsigned)count);
        }
        status = par_set_arr_u32((par_num_t)ePAR_TEST_ARR_U32_RW, original, 2U);
        PAR_TEST_ASSERT(ctx, PAR_TEST_STATUS_HAS_NO_ERROR(status), "arr_u32_restore_status=%s", par_test_ram_status_str(status));
    }
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */
    return ePAR_TEST_PASS;
#else
    PAR_TEST_SKIP(ctx, "object_test_rows_disabled");
#endif /* defined(AUTOGEN_PM_TEST_OBJECT_ROWS) */
}
#endif /* defined(AUTOGEN_PM_TEST_OBJECT_ROWS) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

/**
 * @brief RAM validation case table.
 */
static const par_test_case_t g_par_test_ram_cases[] = {
    { "default_readback", par_test_ram_default_readback, (uint32_t)ePAR_TEST_CASE_FLAG_NONE },
    { "scalar_set_get_restore", par_test_ram_scalar_set_get_restore, (uint32_t)ePAR_TEST_CASE_FLAG_NONE },
    { "scalar_range_limit", par_test_ram_scalar_range_limit, (uint32_t)ePAR_TEST_CASE_FLAG_NONE },
    { "metadata_strings", par_test_ram_metadata_strings, (uint32_t)ePAR_TEST_CASE_FLAG_NONE },
    { "id_accessors", par_test_ram_id_accessors, (uint32_t)ePAR_TEST_CASE_FLAG_NONE },
    { "access_policy", par_test_ram_access_policy, (uint32_t)ePAR_TEST_CASE_FLAG_NONE },
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
    { "f32_scalar", par_test_ram_f32_scalar, (uint32_t)ePAR_TEST_CASE_FLAG_NONE },
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */
#if (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION)
    { "runtime_validation", par_test_ram_runtime_validation, (uint32_t)ePAR_TEST_CASE_FLAG_NONE },
#endif /* (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION) */
#if (1 == PAR_CFG_ENABLE_CHANGE_CALLBACK)
    { "change_callback", par_test_ram_change_callback, (uint32_t)ePAR_TEST_CASE_FLAG_NONE },
#endif /* (1 == PAR_CFG_ENABLE_CHANGE_CALLBACK) */
#if (1 == PAR_CFG_ENABLE_RESET_ALL_RAW)
    { "reset_all_raw", par_test_ram_reset_all_raw, (uint32_t)ePAR_TEST_CASE_FLAG_NONE },
#endif /* (1 == PAR_CFG_ENABLE_RESET_ALL_RAW) */
#if defined(AUTOGEN_PM_TEST_OBJECT_ROWS) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
    { "object_string", par_test_ram_object_string, (uint32_t)ePAR_TEST_CASE_FLAG_NONE },
    { "object_arrays", par_test_ram_object_arrays, (uint32_t)ePAR_TEST_CASE_FLAG_NONE },
#endif /* defined(AUTOGEN_PM_TEST_OBJECT_ROWS) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */
};

const par_test_suite_t g_par_test_suite_ram = {
    .name = "ram",
    .cases = g_par_test_ram_cases,
    .case_count = (uint32_t)PAR_TEST_ARRAY_SIZE(g_par_test_ram_cases),
};
