/**
 * @file par_test_flash_ee_fal.c
 * @brief Validate the Flash EE backend on a dedicated FAL partition.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#include "par_test.h"
#include "par_cfg.h"

#include <string.h>

#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_FAL_EN)
#include "par_store_backend.h"
#include "par_store_backend_flash_ee.h"

#include <fal.h>

/**
 * @brief Erased byte value expected from Flash EE logical erase operations.
 */
#define PAR_TEST_FLASH_EE_FAL_ERASE_VALUE  (0xFFu)

/**
 * @brief Number of bytes used by the real-FAL read/write smoke pattern.
 */
#define PAR_TEST_FLASH_EE_FAL_PATTERN_SIZE (64u)

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
 * @brief Fill one buffer with a deterministic byte pattern.
 * @param p_buf Destination buffer.
 * @param size Number of bytes to fill.
 * @param seed Pattern seed.
 */
static void par_test_flash_ee_fal_fill_pattern(uint8_t *p_buf, uint32_t size, uint8_t seed)
{
    uint32_t idx;

    for (idx = 0u; idx < size; ++idx)
    {
        p_buf[idx] = (uint8_t)(seed + (uint8_t)(idx * 17u));
    }
}

/**
 * @brief Return true when a buffer contains only erased bytes.
 * @param p_buf Buffer to inspect.
 * @param size Number of bytes to inspect.
 * @return true when every byte equals the logical erased value.
 */
static bool par_test_flash_ee_fal_is_erased(const uint8_t *p_buf, uint32_t size)
{
    uint32_t idx;

    for (idx = 0u; idx < size; ++idx)
    {
        if (PAR_TEST_FLASH_EE_FAL_ERASE_VALUE != p_buf[idx])
        {
            return false;
        }
    }

    return true;
}

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
 * @brief Write one full logical line with a deterministic pattern.
 * @param ctx Test execution context.
 * @param p_api Backend API table.
 * @param line_index Logical line index to write.
 * @param seed Pattern seed.
 * @param p_expected Optional output buffer receiving the written pattern.
 * @param p_step Failure-detail prefix.
 * @return ePAR_TEST_PASS on success, otherwise ePAR_TEST_FAIL.
 */
static par_test_result_t par_test_flash_ee_fal_write_line_pattern(par_test_context_t *ctx,
                                                                  const par_store_backend_api_t *p_api,
                                                                  uint32_t line_index,
                                                                  uint8_t seed,
                                                                  uint8_t *p_expected,
                                                                  const char *p_step)
{
    uint8_t line[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    par_status_t status;

    par_test_flash_ee_fal_fill_pattern(line, sizeof(line), seed);
    status = p_api->write(line_index * PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE, sizeof(line), line);
    if (ePAR_OK != status)
    {
        PAR_TEST_FAIL(ctx, "%s line=%lu write_status=%u diag=%s",
                      p_step,
                      (unsigned long)line_index,
                      (unsigned)status,
                      par_store_backend_flash_ee_get_diag_str(par_store_backend_flash_ee_get_diag()));
    }

    if (NULL != p_expected)
    {
        (void)memcpy(p_expected, line, sizeof(line));
    }

    return ePAR_TEST_PASS;
}

/**
 * @brief Read one full logical line and compare it with an expected pattern.
 * @param ctx Test execution context.
 * @param p_api Backend API table.
 * @param line_index Logical line index to read.
 * @param p_expected Expected line payload.
 * @param p_step Failure-detail prefix.
 * @return ePAR_TEST_PASS on success, otherwise ePAR_TEST_FAIL.
 */
static par_test_result_t par_test_flash_ee_fal_expect_line(par_test_context_t *ctx,
                                                           const par_store_backend_api_t *p_api,
                                                           uint32_t line_index,
                                                           const uint8_t *p_expected,
                                                           const char *p_step)
{
    uint8_t readback[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    par_status_t status;

    (void)memset(readback, 0, sizeof(readback));
    status = p_api->read(line_index * PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE, sizeof(readback), readback);
    if ((ePAR_OK != status) || (0 != memcmp(p_expected, readback, sizeof(readback))))
    {
        PAR_TEST_FAIL(ctx, "%s line=%lu read_status=%u",
                      p_step,
                      (unsigned long)line_index,
                      (unsigned)status);
    }

    return ePAR_TEST_PASS;
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
 * @brief Validate that the configured FAL partition can be bound and formatted.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_fal_bind_init(par_test_context_t *ctx)
{
    const par_store_backend_api_t *p_api;

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_reset_partition(ctx))
    {
        return ePAR_TEST_FAIL;
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_open(ctx, &p_api))
    {
        return ePAR_TEST_FAIL;
    }

    return par_test_flash_ee_fal_close(ctx, p_api);
}

/**
 * @brief Validate write/read data survives Flash EE backend reinitialization.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_fal_write_read_persists_after_reinit(par_test_context_t *ctx)
{
    const par_store_backend_api_t *p_api;
    uint8_t pattern[PAR_TEST_FLASH_EE_FAL_PATTERN_SIZE];
    uint8_t readback[PAR_TEST_FLASH_EE_FAL_PATTERN_SIZE];
    par_status_t status;

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_reset_partition(ctx))
    {
        return ePAR_TEST_FAIL;
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_open(ctx, &p_api))
    {
        return ePAR_TEST_FAIL;
    }

    par_test_flash_ee_fal_fill_pattern(pattern, sizeof(pattern), 0x24u);
    (void)memset(readback, 0, sizeof(readback));

    status = p_api->erase(0u, PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE);
    if (ePAR_OK != status)
    {
        (void)p_api->deinit();
        PAR_TEST_FAIL(ctx, "logical_erase_status=%u", (unsigned)status);
    }

    status = p_api->write(0u, sizeof(pattern), pattern);
    if (ePAR_OK != status)
    {
        (void)p_api->deinit();
        PAR_TEST_FAIL(ctx, "write_status=%u", (unsigned)status);
    }

    status = p_api->sync();
    if (ePAR_OK != status)
    {
        (void)p_api->deinit();
        PAR_TEST_FAIL(ctx, "sync_status=%u", (unsigned)status);
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_close(ctx, p_api))
    {
        return ePAR_TEST_FAIL;
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_open(ctx, &p_api))
    {
        return ePAR_TEST_FAIL;
    }

    status = p_api->read(0u, sizeof(readback), readback);
    if ((ePAR_OK != status) || (0 != memcmp(pattern, readback, sizeof(pattern))))
    {
        (void)p_api->deinit();
        PAR_TEST_FAIL(ctx, "reload_read_status=%u", (unsigned)status);
    }

    return par_test_flash_ee_fal_close(ctx, p_api);
}

/**
 * @brief Validate logical erase results survive Flash EE backend reinitialization.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_fal_erase_persists_after_reinit(par_test_context_t *ctx)
{
    const par_store_backend_api_t *p_api;
    uint8_t pattern[PAR_TEST_FLASH_EE_FAL_PATTERN_SIZE];
    uint8_t readback[PAR_TEST_FLASH_EE_FAL_PATTERN_SIZE];
    par_status_t status;

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_reset_partition(ctx))
    {
        return ePAR_TEST_FAIL;
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_open(ctx, &p_api))
    {
        return ePAR_TEST_FAIL;
    }

    par_test_flash_ee_fal_fill_pattern(pattern, sizeof(pattern), 0x73u);

    status = p_api->write(0u, sizeof(pattern), pattern);
    if (ePAR_OK != status)
    {
        (void)p_api->deinit();
        PAR_TEST_FAIL(ctx, "seed_write_status=%u", (unsigned)status);
    }

    status = p_api->erase(0u, sizeof(pattern));
    if (ePAR_OK != status)
    {
        (void)p_api->deinit();
        PAR_TEST_FAIL(ctx, "erase_status=%u", (unsigned)status);
    }

    status = p_api->sync();
    if (ePAR_OK != status)
    {
        (void)p_api->deinit();
        PAR_TEST_FAIL(ctx, "sync_status=%u", (unsigned)status);
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_close(ctx, p_api))
    {
        return ePAR_TEST_FAIL;
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_open(ctx, &p_api))
    {
        return ePAR_TEST_FAIL;
    }

    status = p_api->read(0u, sizeof(readback), readback);
    if ((ePAR_OK != status) || (false == par_test_flash_ee_fal_is_erased(readback, sizeof(readback))))
    {
        (void)p_api->deinit();
        PAR_TEST_FAIL(ctx, "reload_erase_status=%u", (unsigned)status);
    }

    return par_test_flash_ee_fal_close(ctx, p_api);
}


/**
 * @brief Validate real FAL rollover preserves newest lines and remains writable.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_fal_wrap_after_bank_full_preserves_latest(par_test_context_t *ctx)
{
    const par_store_backend_api_t *p_api;
    const struct fal_partition *p_part;
    uint8_t latest0[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    uint8_t stable1[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    uint8_t post_wrap2[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    uint32_t capacity;
    uint32_t iter;

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_reset_partition(ctx))
    {
        return ePAR_TEST_FAIL;
    }
    if (ePAR_TEST_PASS != par_test_flash_ee_fal_get_partition_info(ctx, &p_part, &capacity))
    {
        return ePAR_TEST_FAIL;
    }
    (void)p_part;

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_open(ctx, &p_api))
    {
        return ePAR_TEST_FAIL;
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_write_line_pattern(ctx, p_api, 1u, 0xA1u, stable1, "stable_seed"))
    {
        (void)p_api->deinit();
        return ePAR_TEST_FAIL;
    }

    for (iter = 0u; iter < (capacity + 3u); ++iter)
    {
        if (ePAR_TEST_PASS != par_test_flash_ee_fal_write_line_pattern(ctx, p_api, 0u, (uint8_t)(0x20u + iter), latest0, "wrap_fill"))
        {
            (void)p_api->deinit();
            return ePAR_TEST_FAIL;
        }
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_close(ctx, p_api))
    {
        return ePAR_TEST_FAIL;
    }
    if (ePAR_TEST_PASS != par_test_flash_ee_fal_open(ctx, &p_api))
    {
        return ePAR_TEST_FAIL;
    }

    if ((ePAR_TEST_PASS != par_test_flash_ee_fal_expect_line(ctx, p_api, 0u, latest0, "line0_after_wrap")) ||
        (ePAR_TEST_PASS != par_test_flash_ee_fal_expect_line(ctx, p_api, 1u, stable1, "line1_after_wrap")))
    {
        (void)p_api->deinit();
        return ePAR_TEST_FAIL;
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_write_line_pattern(ctx, p_api, 2u, 0xB2u, post_wrap2, "post_wrap_write"))
    {
        (void)p_api->deinit();
        return ePAR_TEST_FAIL;
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_close(ctx, p_api))
    {
        return ePAR_TEST_FAIL;
    }
    if (ePAR_TEST_PASS != par_test_flash_ee_fal_open(ctx, &p_api))
    {
        return ePAR_TEST_FAIL;
    }

    if ((ePAR_TEST_PASS != par_test_flash_ee_fal_expect_line(ctx, p_api, 0u, latest0, "line0_final")) ||
        (ePAR_TEST_PASS != par_test_flash_ee_fal_expect_line(ctx, p_api, 1u, stable1, "line1_final")) ||
        (ePAR_TEST_PASS != par_test_flash_ee_fal_expect_line(ctx, p_api, 2u, post_wrap2, "line2_final")))
    {
        (void)p_api->deinit();
        return ePAR_TEST_FAIL;
    }

    return par_test_flash_ee_fal_close(ctx, p_api);
}


/**
 * @brief Validate multiple real FAL rollover cycles preserve the latest logical image.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_fal_wrap_multiple_cycles_preserves_latest(par_test_context_t *ctx)
{
    const par_store_backend_api_t *p_api;
    const struct fal_partition *p_part;
    uint8_t latest0[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    uint8_t stable1[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    uint32_t capacity;
    uint32_t cycle;
    uint32_t iter;

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_reset_partition(ctx))
    {
        return ePAR_TEST_FAIL;
    }
    if (ePAR_TEST_PASS != par_test_flash_ee_fal_get_partition_info(ctx, &p_part, &capacity))
    {
        return ePAR_TEST_FAIL;
    }
    (void)p_part;

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_open(ctx, &p_api))
    {
        return ePAR_TEST_FAIL;
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_write_line_pattern(ctx, p_api, 1u, 0xC1u, stable1, "stable_seed"))
    {
        (void)p_api->deinit();
        return ePAR_TEST_FAIL;
    }

    for (cycle = 0u; cycle < 3u; ++cycle)
    {
        for (iter = 0u; iter < (capacity + 2u); ++iter)
        {
            if (ePAR_TEST_PASS != par_test_flash_ee_fal_write_line_pattern(ctx,
                                                                           p_api,
                                                                           0u,
                                                                           (uint8_t)(0x30u + cycle + iter),
                                                                           latest0,
                                                                           "multi_wrap_fill"))
            {
                (void)p_api->deinit();
                return ePAR_TEST_FAIL;
            }
        }

        if (ePAR_TEST_PASS != par_test_flash_ee_fal_close(ctx, p_api))
        {
            return ePAR_TEST_FAIL;
        }
        if (ePAR_TEST_PASS != par_test_flash_ee_fal_open(ctx, &p_api))
        {
            return ePAR_TEST_FAIL;
        }

        if ((ePAR_TEST_PASS != par_test_flash_ee_fal_expect_line(ctx, p_api, 0u, latest0, "cycle_line0_reload")) ||
            (ePAR_TEST_PASS != par_test_flash_ee_fal_expect_line(ctx, p_api, 1u, stable1, "cycle_line1_reload")))
        {
            (void)p_api->deinit();
            return ePAR_TEST_FAIL;
        }
    }

    return par_test_flash_ee_fal_close(ctx, p_api);
}

/**
 * @brief Validate a full real FAL bank is writable after reboot-like reinit.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_fal_full_bank_then_reinit_remains_writable(par_test_context_t *ctx)
{
    const par_store_backend_api_t *p_api;
    const struct fal_partition *p_part;
    uint8_t latest0[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    uint8_t recovery1[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    uint32_t capacity;
    uint32_t iter;

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_reset_partition(ctx))
    {
        return ePAR_TEST_FAIL;
    }
    if (ePAR_TEST_PASS != par_test_flash_ee_fal_get_partition_info(ctx, &p_part, &capacity))
    {
        return ePAR_TEST_FAIL;
    }
    (void)p_part;

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_open(ctx, &p_api))
    {
        return ePAR_TEST_FAIL;
    }

    for (iter = 0u; iter < capacity; ++iter)
    {
        if (ePAR_TEST_PASS != par_test_flash_ee_fal_write_line_pattern(ctx, p_api, 0u, (uint8_t)(0x40u + iter), latest0, "fill_exact"))
        {
            (void)p_api->deinit();
            return ePAR_TEST_FAIL;
        }
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_close(ctx, p_api))
    {
        return ePAR_TEST_FAIL;
    }
    if (ePAR_TEST_PASS != par_test_flash_ee_fal_open(ctx, &p_api))
    {
        return ePAR_TEST_FAIL;
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_expect_line(ctx, p_api, 0u, latest0, "full_line0_reload"))
    {
        (void)p_api->deinit();
        return ePAR_TEST_FAIL;
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_write_line_pattern(ctx, p_api, 1u, 0xE1u, recovery1, "full_recovery_write"))
    {
        (void)p_api->deinit();
        return ePAR_TEST_FAIL;
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_fal_close(ctx, p_api))
    {
        return ePAR_TEST_FAIL;
    }
    if (ePAR_TEST_PASS != par_test_flash_ee_fal_open(ctx, &p_api))
    {
        return ePAR_TEST_FAIL;
    }

    if ((ePAR_TEST_PASS != par_test_flash_ee_fal_expect_line(ctx, p_api, 0u, latest0, "full_line0_final")) ||
        (ePAR_TEST_PASS != par_test_flash_ee_fal_expect_line(ctx, p_api, 1u, recovery1, "full_line1_final")))
    {
        (void)p_api->deinit();
        return ePAR_TEST_FAIL;
    }

    return par_test_flash_ee_fal_close(ctx, p_api);
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
