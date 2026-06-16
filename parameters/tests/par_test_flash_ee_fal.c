/**
 * @file par_test_flash_ee_fal.c
 * @brief Validate the Flash EE backend on a dedicated FAL partition.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#include "par_test.h"
#include "par_cfg.h"

#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_FAL_EN)

#include "par_store_backend.h"
#include "par_store_backend_flash_ee.h"
#include "par_test_flash_ee_common.h"

#include <fal.h>

/**
 * @brief Persistent bank header size used by the Flash EE on-flash format.
 */
#define PAR_TEST_FLASH_EE_FAL_HEADER_SIZE      (64u)

/**
 * @brief Append-record metadata size used by the Flash EE on-flash format.
 */
#define PAR_TEST_FLASH_EE_FAL_RECORD_META_SIZE (12u)

/**
 * @brief Align a compile-time value upward to the requested granularity.
 */
#define PAR_TEST_FLASH_EE_FAL_ALIGN_UP(value, align) ((((value) + (align) - 1u) / (align)) * (align))

/**
 * @brief Offset of the record metadata block in one append record.
 */
#define PAR_TEST_FLASH_EE_FAL_RECORD_META_OFFSET \
    PAR_TEST_FLASH_EE_FAL_ALIGN_UP(PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE, PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE)

/**
 * @brief Offset of the commit unit in one append record.
 */
#define PAR_TEST_FLASH_EE_FAL_RECORD_COMMIT_OFFSET \
    PAR_TEST_FLASH_EE_FAL_ALIGN_UP(PAR_TEST_FLASH_EE_FAL_RECORD_META_OFFSET + PAR_TEST_FLASH_EE_FAL_RECORD_META_SIZE, \
                                   PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE)

/**
 * @brief Total append-record size in bytes for the configured Flash EE geometry.
 */
#define PAR_TEST_FLASH_EE_FAL_RECORD_SIZE \
    (PAR_TEST_FLASH_EE_FAL_RECORD_COMMIT_OFFSET + PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE)

/**
 * @brief Find the configured FAL partition and verify it has enough capacity.
 * @param ctx Test execution context.
 * @param pp_part Receives the partition on success.
 * @param p_capacity Receives append-record capacity per Flash EE bank on success.
 * @return ePAR_TEST_PASS on success, otherwise ePAR_TEST_FAIL.
 */
static par_test_result_t par_test_flash_ee_fal_get_partition_info(par_test_context_t *ctx,
                                                                  const struct fal_partition **pp_part,
                                                                  uint32_t *p_capacity)
{
    const struct fal_partition *p_part;
    uint32_t bank_size;
    uint32_t capacity;
    uint32_t line_count;

    if ((NULL == pp_part) || (NULL == p_capacity))
    {
        PAR_TEST_FAIL(ctx, "partition_info_output_null");
    }

    p_part = fal_partition_find(PAR_CFG_NVM_BACKEND_FLASH_EE_FAL_PARTITION_NAME);
    if (NULL == p_part)
    {
        PAR_TEST_FAIL(ctx, "partition_not_found=%s", PAR_CFG_NVM_BACKEND_FLASH_EE_FAL_PARTITION_NAME);
    }

    bank_size = (uint32_t)p_part->len / 2u;
    line_count = PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE / PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE;
    if ((0u != ((uint32_t)p_part->len % 2u)) || (bank_size <= PAR_TEST_FLASH_EE_FAL_HEADER_SIZE))
    {
        PAR_TEST_FAIL(ctx,
                      "partition_geometry_invalid len=%lu bank=%lu logical=%lu",
                      (unsigned long)p_part->len,
                      (unsigned long)bank_size,
                      (unsigned long)PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE);
    }

    capacity = (bank_size - PAR_TEST_FLASH_EE_FAL_HEADER_SIZE) / PAR_TEST_FLASH_EE_FAL_RECORD_SIZE;
    if (capacity <= line_count)
    {
        PAR_TEST_FAIL(ctx,
                      "partition_geometry_invalid len=%lu bank=%lu capacity=%lu line_count=%lu logical=%lu",
                      (unsigned long)p_part->len,
                      (unsigned long)bank_size,
                      (unsigned long)capacity,
                      (unsigned long)line_count,
                      (unsigned long)PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE);
    }

    *pp_part = p_part;
    *p_capacity = capacity;
    return ePAR_TEST_PASS;
}

/**
 * @brief Erase the configured FAL partition before one destructive test case.
 * @param ctx Test execution context.
 * @return ePAR_TEST_PASS on success, otherwise ePAR_TEST_FAIL.
 */
static par_test_result_t par_test_flash_ee_fal_reset_partition(par_test_context_t *ctx)
{
    const struct fal_partition *p_part;
    uint32_t capacity;

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_get_partition_info(ctx, &p_part, &capacity))
    {
        return ePAR_TEST_FAIL;
    }
    (void)capacity;

    if ((int)p_part->len != fal_partition_erase(p_part, 0, p_part->len))
    {
        PAR_TEST_FAIL(ctx, "partition_erase_failed part=%s len=%lu",
                      PAR_CFG_NVM_BACKEND_FLASH_EE_FAL_PARTITION_NAME,
                      (unsigned long)p_part->len);
    }

    return ePAR_TEST_PASS;
}

/**
 * @brief Return FAL append-record capacity for the common Flash EE tests.
 * @param ctx Test execution context.
 * @param p_capacity Receives append-record capacity per bank.
 * @return ePAR_TEST_PASS on success, otherwise ePAR_TEST_FAIL.
 */
static par_test_result_t par_test_flash_ee_fal_get_capacity(par_test_context_t *ctx, uint32_t *p_capacity)
{
    const struct fal_partition *p_part;

    if (NULL == p_capacity)
    {
        PAR_TEST_FAIL(ctx, "capacity_output_null");
    }

    return par_test_flash_ee_fal_get_partition_info(ctx, &p_part, p_capacity);
}

/**
 * @brief Bind and initialize the configured FAL-backed Flash EE backend.
 * @param ctx Test execution context.
 * @param pp_api Receives the initialized backend API on success.
 * @return ePAR_TEST_PASS on success, otherwise ePAR_TEST_FAIL.
 */
static par_test_result_t par_test_flash_ee_fal_open(par_test_context_t *ctx, const par_store_backend_api_t **pp_api)
{
    const par_store_backend_api_t *p_api;
    par_status_t status;
    bool is_init = false;

    if (NULL == pp_api)
    {
        PAR_TEST_FAIL(ctx, "api_output_null");
    }

    *pp_api = NULL;

    status = par_store_backend_bind();
    if (ePAR_OK != status)
    {
        PAR_TEST_FAIL(ctx,
                      "bind_status=%u diag=%s partition=%s",
                      (unsigned)status,
                      par_store_backend_flash_ee_get_diag_str(par_store_backend_flash_ee_get_diag()),
                      PAR_CFG_NVM_BACKEND_FLASH_EE_FAL_PARTITION_NAME);
    }

    p_api = par_store_backend_get_api();
    if ((NULL == p_api) || (NULL == p_api->init) || (NULL == p_api->deinit) ||
        (NULL == p_api->read) || (NULL == p_api->write) || (NULL == p_api->erase) || (NULL == p_api->sync))
    {
        PAR_TEST_FAIL(ctx, "backend_api_incomplete");
    }

    status = p_api->init();
    if (ePAR_OK != status)
    {
        PAR_TEST_FAIL(ctx,
                      "init_status=%u diag=%s partition=%s",
                      (unsigned)status,
                      par_store_backend_flash_ee_get_diag_str(par_store_backend_flash_ee_get_diag()),
                      PAR_CFG_NVM_BACKEND_FLASH_EE_FAL_PARTITION_NAME);
    }

    if (NULL != p_api->is_init)
    {
        p_api->is_init(&is_init);
        if (false == is_init)
        {
            (void)p_api->deinit();
            PAR_TEST_FAIL(ctx, "backend_not_initialized");
        }
    }

    *pp_api = p_api;
    return ePAR_TEST_PASS;
}

/**
 * @brief Deinitialize the configured Flash EE backend.
 * @param ctx Test execution context.
 * @param p_api Backend API.
 * @return ePAR_TEST_PASS on success, otherwise ePAR_TEST_FAIL.
 */
static par_test_result_t par_test_flash_ee_fal_close(par_test_context_t *ctx, const par_store_backend_api_t *p_api)
{
    par_status_t status;

    if ((NULL == p_api) || (NULL == p_api->deinit))
    {
        PAR_TEST_FAIL(ctx, "close_api_invalid");
    }

    status = p_api->deinit();
    if (ePAR_OK != status)
    {
        PAR_TEST_FAIL(ctx, "deinit_status=%u", (unsigned)status);
    }

    return ePAR_TEST_PASS;
}

/**
 * @brief FAL-backed Flash EE operations consumed by common test flows.
 */
static const par_test_flash_ee_ops_t g_par_test_flash_ee_fal_ops = {
    .name = "flash_ee_fal",
    .reset = par_test_flash_ee_fal_reset_partition,
    .open = par_test_flash_ee_fal_open,
    .close = par_test_flash_ee_fal_close,
    .get_capacity = par_test_flash_ee_fal_get_capacity,
};

/**
 * @brief Validate that the configured FAL partition can be bound and formatted.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_fal_bind_init(par_test_context_t *ctx)
{
    return par_test_flash_ee_common_bind_init(ctx, &g_par_test_flash_ee_fal_ops);
}

/**
 * @brief Validate write/read data survives Flash EE backend reinitialization.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_fal_write_read_persists_after_reinit(par_test_context_t *ctx)
{
    return par_test_flash_ee_common_write_read_persists_after_reinit(ctx, &g_par_test_flash_ee_fal_ops);
}

/**
 * @brief Validate logical erase results survive Flash EE backend reinitialization.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_fal_erase_persists_after_reinit(par_test_context_t *ctx)
{
    return par_test_flash_ee_common_erase_persists_after_reinit(ctx, &g_par_test_flash_ee_fal_ops);
}

/**
 * @brief Validate real FAL rollover preserves newest lines and remains writable.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_fal_wrap_after_bank_full_preserves_latest(par_test_context_t *ctx)
{
    return par_test_flash_ee_common_wrap_after_bank_full_preserves_latest(ctx, &g_par_test_flash_ee_fal_ops);
}

/**
 * @brief Validate multiple real FAL rollover cycles preserve the latest logical image.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_fal_wrap_multiple_cycles_preserves_latest(par_test_context_t *ctx)
{
    return par_test_flash_ee_common_wrap_multiple_cycles_preserves_latest(ctx, &g_par_test_flash_ee_fal_ops);
}

/**
 * @brief Validate a full real FAL bank is writable after reboot-like reinit.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_fal_full_bank_then_reinit_remains_writable(par_test_context_t *ctx)
{
    return par_test_flash_ee_common_full_bank_then_reinit_remains_writable(ctx, &g_par_test_flash_ee_fal_ops);
}

#else
/**
 * @brief Skip real-FAL Flash EE tests when the FAL port is unavailable.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_fal_backend_disabled(par_test_context_t *ctx)
{
    PAR_TEST_SKIP(ctx, "flash_ee_fal_backend_disabled");
}
#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_FAL_EN) */

/**
 * @brief Case flag used for tests that rewrite the configured FAL partition.
 */
#define PAR_TEST_FLASH_EE_FAL_CASE_FLAGS ((uint32_t)ePAR_TEST_CASE_FLAG_DESTRUCTIVE)

/**
 * @brief Flash EE real-FAL validation case table.
 */
static const par_test_case_t g_par_test_flash_ee_fal_cases[] = {
#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_FAL_EN)
    { "bind_init", par_test_flash_ee_fal_bind_init, PAR_TEST_FLASH_EE_FAL_CASE_FLAGS },
    { "write_read_persists_after_reinit", par_test_flash_ee_fal_write_read_persists_after_reinit, PAR_TEST_FLASH_EE_FAL_CASE_FLAGS },
    { "erase_persists_after_reinit", par_test_flash_ee_fal_erase_persists_after_reinit, PAR_TEST_FLASH_EE_FAL_CASE_FLAGS },
    { "wrap_after_bank_full_preserves_latest",
      par_test_flash_ee_fal_wrap_after_bank_full_preserves_latest,
      PAR_TEST_FLASH_EE_FAL_CASE_FLAGS },
    { "wrap_multiple_cycles_preserves_latest",
      par_test_flash_ee_fal_wrap_multiple_cycles_preserves_latest,
      PAR_TEST_FLASH_EE_FAL_CASE_FLAGS },
    { "full_bank_then_reinit_remains_writable",
      par_test_flash_ee_fal_full_bank_then_reinit_remains_writable,
      PAR_TEST_FLASH_EE_FAL_CASE_FLAGS },
#else
    { "backend_disabled", par_test_flash_ee_fal_backend_disabled, (uint32_t)ePAR_TEST_CASE_FLAG_NONE },
#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_FAL_EN) */
};

const par_test_suite_t g_par_test_suite_flash_ee_fal = {
    .name = "flash_ee_fal",
    .cases = g_par_test_flash_ee_fal_cases,
    .case_count = (uint32_t)PAR_TEST_ARRAY_SIZE(g_par_test_flash_ee_fal_cases),
};
