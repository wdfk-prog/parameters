/**
 * @file par_test_flash_ee_common.c
 * @brief Implement reusable Flash EE backend test flows for hardware and host CI.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#include "par_test_flash_ee_common.h"

#include "par_cfg.h"
#include "par_store_backend_flash_ee.h"

#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN)

#include <string.h>

/**
 * @brief Erased byte value expected from Flash EE logical erase operations.
 */
#define PAR_TEST_FLASH_EE_ERASE_VALUE  (0xFFu)

/**
 * @brief Number of bytes used by the read/write smoke pattern.
 */
#define PAR_TEST_FLASH_EE_PATTERN_SIZE (64u)

void par_test_flash_ee_fill_pattern(uint8_t *p_buf, uint32_t size, uint8_t seed)
{
    uint32_t idx;

    if (NULL == p_buf)
    {
        return;
    }

    for (idx = 0u; idx < size; ++idx)
    {
        p_buf[idx] = (uint8_t)(seed + (uint8_t)(idx * 17u));
    }
}

/**
 * @brief Fill a line with a deterministic pattern that carries a 32-bit tag.
 * @param p_buf Destination buffer.
 * @param size Number of bytes to fill.
 * @param tag Pattern tag encoded across the line.
 */
static void par_test_flash_ee_fill_tagged_pattern(uint8_t *p_buf, uint32_t size, uint32_t tag)
{
    uint32_t idx;

    if (NULL == p_buf)
    {
        return;
    }

    for (idx = 0u; idx < size; ++idx)
    {
        const uint32_t shift = (idx & 3u) * 8u;
        const uint8_t tag_byte = (uint8_t)((tag >> shift) & 0xFFu);

        p_buf[idx] = (uint8_t)(tag_byte + (uint8_t)(idx * 17u) + (uint8_t)(idx >> 2u));
    }
}

bool par_test_flash_ee_is_erased(const uint8_t *p_buf, uint32_t size)
{
    uint32_t idx;

    if (NULL == p_buf)
    {
        return false;
    }

    for (idx = 0u; idx < size; ++idx)
    {
        if (PAR_TEST_FLASH_EE_ERASE_VALUE != p_buf[idx])
        {
            return false;
        }
    }

    return true;
}

par_test_result_t par_test_flash_ee_write_line_pattern(par_test_context_t *ctx,
                                                       const par_store_backend_api_t *p_api,
                                                       uint32_t line_index,
                                                       uint32_t tag,
                                                       uint8_t *p_expected,
                                                       const char *p_step)
{
    uint8_t line[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    par_status_t status;

    if ((NULL == p_api) || (NULL == p_api->write))
    {
        PAR_TEST_FAIL(ctx, "%s api_invalid", (NULL != p_step) ? p_step : "write_line");
    }

    if (tag <= 0xFFu)
    {
        par_test_flash_ee_fill_pattern(line, sizeof(line), (uint8_t)tag);
    }
    else
    {
        par_test_flash_ee_fill_tagged_pattern(line, sizeof(line), tag);
    }

    status = p_api->write(line_index * PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE, sizeof(line), line);
    if (ePAR_OK != status)
    {
        PAR_TEST_FAIL(ctx, "%s line=%lu write_status=%u diag=%s",
                      (NULL != p_step) ? p_step : "write_line",
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

par_test_result_t par_test_flash_ee_expect_line(par_test_context_t *ctx,
                                                const par_store_backend_api_t *p_api,
                                                uint32_t line_index,
                                                const uint8_t *p_expected,
                                                const char *p_step)
{
    uint8_t readback[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    par_status_t status;

    if ((NULL == p_api) || (NULL == p_api->read) || (NULL == p_expected))
    {
        PAR_TEST_FAIL(ctx, "%s api_invalid", (NULL != p_step) ? p_step : "expect_line");
    }

    (void)memset(readback, 0, sizeof(readback));
    status = p_api->read(line_index * PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE, sizeof(readback), readback);
    if ((ePAR_OK != status) || (0 != memcmp(p_expected, readback, sizeof(readback))))
    {
        PAR_TEST_FAIL(ctx, "%s line=%lu read_status=%u",
                      (NULL != p_step) ? p_step : "expect_line",
                      (unsigned long)line_index,
                      (unsigned)status);
    }

    return ePAR_TEST_PASS;
}

/**
 * @brief Validate the platform operation table.
 * @param ctx Test context.
 * @param p_ops Platform operations.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_common_validate_ops(par_test_context_t *ctx, const par_test_flash_ee_ops_t *p_ops)
{
    if ((NULL == p_ops) || (NULL == p_ops->reset) || (NULL == p_ops->open) ||
        (NULL == p_ops->close) || (NULL == p_ops->get_capacity))
    {
        PAR_TEST_FAIL(ctx, "flash_ee_ops_invalid");
    }

    return ePAR_TEST_PASS;
}

par_test_result_t par_test_flash_ee_common_bind_init(par_test_context_t *ctx, const par_test_flash_ee_ops_t *p_ops)
{
    const par_store_backend_api_t *p_api;

    if (ePAR_TEST_PASS != par_test_flash_ee_common_validate_ops(ctx, p_ops))
    {
        return ePAR_TEST_FAIL;
    }

    if (ePAR_TEST_PASS != p_ops->reset(ctx))
    {
        return ePAR_TEST_FAIL;
    }

    if (ePAR_TEST_PASS != p_ops->open(ctx, &p_api))
    {
        return ePAR_TEST_FAIL;
    }

    return p_ops->close(ctx, p_api);
}

par_test_result_t par_test_flash_ee_common_write_read_persists_after_reinit(par_test_context_t *ctx,
                                                                           const par_test_flash_ee_ops_t *p_ops)
{
    const par_store_backend_api_t *p_api;
    uint8_t pattern[PAR_TEST_FLASH_EE_PATTERN_SIZE];
    uint8_t readback[PAR_TEST_FLASH_EE_PATTERN_SIZE];
    par_status_t status;

    if (ePAR_TEST_PASS != par_test_flash_ee_common_validate_ops(ctx, p_ops))
    {
        return ePAR_TEST_FAIL;
    }

    if ((ePAR_TEST_PASS != p_ops->reset(ctx)) || (ePAR_TEST_PASS != p_ops->open(ctx, &p_api)))
    {
        return ePAR_TEST_FAIL;
    }

    par_test_flash_ee_fill_pattern(pattern, sizeof(pattern), 0x24u);
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

    if ((ePAR_TEST_PASS != p_ops->close(ctx, p_api)) || (ePAR_TEST_PASS != p_ops->open(ctx, &p_api)))
    {
        return ePAR_TEST_FAIL;
    }

    status = p_api->read(0u, sizeof(readback), readback);
    if ((ePAR_OK != status) || (0 != memcmp(pattern, readback, sizeof(pattern))))
    {
        (void)p_api->deinit();
        PAR_TEST_FAIL(ctx, "reload_read_status=%u", (unsigned)status);
    }

    return p_ops->close(ctx, p_api);
}

par_test_result_t par_test_flash_ee_common_erase_persists_after_reinit(par_test_context_t *ctx,
                                                                       const par_test_flash_ee_ops_t *p_ops)
{
    const par_store_backend_api_t *p_api;
    uint8_t pattern[PAR_TEST_FLASH_EE_PATTERN_SIZE];
    uint8_t readback[PAR_TEST_FLASH_EE_PATTERN_SIZE];
    par_status_t status;

    if (ePAR_TEST_PASS != par_test_flash_ee_common_validate_ops(ctx, p_ops))
    {
        return ePAR_TEST_FAIL;
    }

    if ((ePAR_TEST_PASS != p_ops->reset(ctx)) || (ePAR_TEST_PASS != p_ops->open(ctx, &p_api)))
    {
        return ePAR_TEST_FAIL;
    }

    par_test_flash_ee_fill_pattern(pattern, sizeof(pattern), 0x73u);
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

    if ((ePAR_TEST_PASS != p_ops->close(ctx, p_api)) || (ePAR_TEST_PASS != p_ops->open(ctx, &p_api)))
    {
        return ePAR_TEST_FAIL;
    }

    status = p_api->read(0u, sizeof(readback), readback);
    if ((ePAR_OK != status) || (false == par_test_flash_ee_is_erased(readback, sizeof(readback))))
    {
        (void)p_api->deinit();
        PAR_TEST_FAIL(ctx, "reload_erase_status=%u", (unsigned)status);
    }

    return p_ops->close(ctx, p_api);
}

par_test_result_t par_test_flash_ee_common_wrap_after_bank_full_preserves_latest(par_test_context_t *ctx,
                                                                                const par_test_flash_ee_ops_t *p_ops)
{
    const par_store_backend_api_t *p_api;
    uint8_t latest0[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    uint8_t stable1[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    uint8_t post_wrap2[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    uint32_t capacity;
    uint32_t iter;

    if (ePAR_TEST_PASS != par_test_flash_ee_common_validate_ops(ctx, p_ops))
    {
        return ePAR_TEST_FAIL;
    }

    if ((ePAR_TEST_PASS != p_ops->reset(ctx)) ||
        (ePAR_TEST_PASS != p_ops->get_capacity(ctx, &capacity)) ||
        (ePAR_TEST_PASS != p_ops->open(ctx, &p_api)))
    {
        return ePAR_TEST_FAIL;
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_write_line_pattern(ctx, p_api, 1u, 0xA1u, stable1, "stable_seed"))
    {
        (void)p_api->deinit();
        return ePAR_TEST_FAIL;
    }

    for (iter = 0u; iter < (capacity + 3u); ++iter)
    {
        if (ePAR_TEST_PASS != par_test_flash_ee_write_line_pattern(ctx, p_api, 0u, 0x20000000u + iter, latest0, "wrap_fill"))
        {
            (void)p_api->deinit();
            return ePAR_TEST_FAIL;
        }
    }

    if ((ePAR_TEST_PASS != p_ops->close(ctx, p_api)) || (ePAR_TEST_PASS != p_ops->open(ctx, &p_api)))
    {
        return ePAR_TEST_FAIL;
    }

    if ((ePAR_TEST_PASS != par_test_flash_ee_expect_line(ctx, p_api, 0u, latest0, "line0_after_wrap")) ||
        (ePAR_TEST_PASS != par_test_flash_ee_expect_line(ctx, p_api, 1u, stable1, "line1_after_wrap")))
    {
        (void)p_api->deinit();
        return ePAR_TEST_FAIL;
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_write_line_pattern(ctx, p_api, 2u, 0xB2u, post_wrap2, "post_wrap_write"))
    {
        (void)p_api->deinit();
        return ePAR_TEST_FAIL;
    }

    if ((ePAR_TEST_PASS != p_ops->close(ctx, p_api)) || (ePAR_TEST_PASS != p_ops->open(ctx, &p_api)))
    {
        return ePAR_TEST_FAIL;
    }

    if ((ePAR_TEST_PASS != par_test_flash_ee_expect_line(ctx, p_api, 0u, latest0, "line0_final")) ||
        (ePAR_TEST_PASS != par_test_flash_ee_expect_line(ctx, p_api, 1u, stable1, "line1_final")) ||
        (ePAR_TEST_PASS != par_test_flash_ee_expect_line(ctx, p_api, 2u, post_wrap2, "line2_final")))
    {
        (void)p_api->deinit();
        return ePAR_TEST_FAIL;
    }

    return p_ops->close(ctx, p_api);
}

par_test_result_t par_test_flash_ee_common_wrap_multiple_cycles_preserves_latest(par_test_context_t *ctx,
                                                                                const par_test_flash_ee_ops_t *p_ops)
{
    const par_store_backend_api_t *p_api;
    uint8_t latest0[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    uint8_t stable1[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    uint32_t capacity;
    uint32_t cycle;
    uint32_t iter;

    if (ePAR_TEST_PASS != par_test_flash_ee_common_validate_ops(ctx, p_ops))
    {
        return ePAR_TEST_FAIL;
    }

    if ((ePAR_TEST_PASS != p_ops->reset(ctx)) ||
        (ePAR_TEST_PASS != p_ops->get_capacity(ctx, &capacity)) ||
        (ePAR_TEST_PASS != p_ops->open(ctx, &p_api)))
    {
        return ePAR_TEST_FAIL;
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_write_line_pattern(ctx, p_api, 1u, 0xC1u, stable1, "stable_seed"))
    {
        (void)p_api->deinit();
        return ePAR_TEST_FAIL;
    }

    for (cycle = 0u; cycle < 3u; ++cycle)
    {
        for (iter = 0u; iter < (capacity + 2u); ++iter)
        {
            if (ePAR_TEST_PASS != par_test_flash_ee_write_line_pattern(ctx,
                                                                       p_api,
                                                                       0u,
                                                                       0x30000000u + (cycle * (capacity + 2u)) + iter,
                                                                       latest0,
                                                                       "multi_wrap_fill"))
            {
                (void)p_api->deinit();
                return ePAR_TEST_FAIL;
            }
        }

        if ((ePAR_TEST_PASS != p_ops->close(ctx, p_api)) || (ePAR_TEST_PASS != p_ops->open(ctx, &p_api)))
        {
            return ePAR_TEST_FAIL;
        }

        if ((ePAR_TEST_PASS != par_test_flash_ee_expect_line(ctx, p_api, 0u, latest0, "cycle_line0_reload")) ||
            (ePAR_TEST_PASS != par_test_flash_ee_expect_line(ctx, p_api, 1u, stable1, "cycle_line1_reload")))
        {
            (void)p_api->deinit();
            return ePAR_TEST_FAIL;
        }
    }

    return p_ops->close(ctx, p_api);
}

par_test_result_t par_test_flash_ee_common_full_bank_then_reinit_remains_writable(par_test_context_t *ctx,
                                                                                 const par_test_flash_ee_ops_t *p_ops)
{
    const par_store_backend_api_t *p_api;
    uint8_t latest0[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    uint8_t recovery1[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    uint32_t capacity;
    uint32_t iter;

    if (ePAR_TEST_PASS != par_test_flash_ee_common_validate_ops(ctx, p_ops))
    {
        return ePAR_TEST_FAIL;
    }

    if ((ePAR_TEST_PASS != p_ops->reset(ctx)) ||
        (ePAR_TEST_PASS != p_ops->get_capacity(ctx, &capacity)) ||
        (ePAR_TEST_PASS != p_ops->open(ctx, &p_api)))
    {
        return ePAR_TEST_FAIL;
    }

    for (iter = 0u; iter < capacity; ++iter)
    {
        if (ePAR_TEST_PASS != par_test_flash_ee_write_line_pattern(ctx, p_api, 0u, 0x40000000u + iter, latest0, "fill_exact"))
        {
            (void)p_api->deinit();
            return ePAR_TEST_FAIL;
        }
    }

    if ((ePAR_TEST_PASS != p_ops->close(ctx, p_api)) || (ePAR_TEST_PASS != p_ops->open(ctx, &p_api)))
    {
        return ePAR_TEST_FAIL;
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_expect_line(ctx, p_api, 0u, latest0, "full_line0_reload"))
    {
        (void)p_api->deinit();
        return ePAR_TEST_FAIL;
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_write_line_pattern(ctx, p_api, 1u, 0xE1u, recovery1, "full_recovery_write"))
    {
        (void)p_api->deinit();
        return ePAR_TEST_FAIL;
    }

    if ((ePAR_TEST_PASS != p_ops->close(ctx, p_api)) || (ePAR_TEST_PASS != p_ops->open(ctx, &p_api)))
    {
        return ePAR_TEST_FAIL;
    }

    if ((ePAR_TEST_PASS != par_test_flash_ee_expect_line(ctx, p_api, 0u, latest0, "full_line0_final")) ||
        (ePAR_TEST_PASS != par_test_flash_ee_expect_line(ctx, p_api, 1u, recovery1, "full_line1_final")))
    {
        (void)p_api->deinit();
        return ePAR_TEST_FAIL;
    }

    return p_ops->close(ctx, p_api);
}

#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN) */
