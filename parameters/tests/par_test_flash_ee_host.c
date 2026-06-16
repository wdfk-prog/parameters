/**
 * @file par_test_flash_ee_host.c
 * @brief Validate Flash EE backend behavior on a host fake flash image.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#include "par_test.h"
#include "par_cfg.h"

/**
 * @brief Compile-time switch for host fake-flash Flash EE validation.
 */
#if defined(AUTOGEN_PM_TEST_USING_FLASH_EE_HOST)
#define PAR_TEST_FLASH_EE_HOST_SUITE_ENABLED \
    ((1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN) && \
     (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_NATIVE_EN))
#else
#define PAR_TEST_FLASH_EE_HOST_SUITE_ENABLED (0)
#endif /* defined(AUTOGEN_PM_TEST_USING_FLASH_EE_HOST) */

#if PAR_TEST_FLASH_EE_HOST_SUITE_ENABLED

#include "par_host_fake_storage.h"
#include "par_store_backend.h"
#include "par_store_backend_flash_ee.h"
#include "par_test_flash_ee_common.h"

#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Reset the host Flash EE fake partition image to erased state.
 * @return ePAR_OK on success, otherwise an error code.
 */
par_status_t par_host_flash_ee_reset_image(void);

/**
 * @brief Return the append-record capacity per host fake Flash EE bank.
 * @return Append-record capacity per bank.
 */
uint32_t par_host_flash_ee_bank_record_capacity(void);

/**
 * @brief Return whether a named environment variable matches an expected value.
 * @param p_name Environment variable name.
 * @param p_expected Expected value.
 * @return true when the value matches, otherwise false.
 */
static bool par_test_flash_ee_host_env_is(const char *p_name, const char *p_expected)
{
    const char *p_value;

    if ((NULL == p_name) || (NULL == p_expected))
    {
        return false;
    }

    p_value = getenv(p_name);
    return ((NULL != p_value) && (0 == strcmp(p_value, p_expected)));
}

/**
 * @brief Skip normal destructive flows while a two-process fault phase is selected.
 * @param ctx Test execution context.
 * @return ePAR_TEST_SKIP when a fault phase is selected, otherwise ePAR_TEST_PASS.
 */
static par_test_result_t par_test_flash_ee_host_skip_normal_during_fault_phase(par_test_context_t *ctx)
{
    if (NULL != getenv("PAR_HOST_FLASH_EE_FAULT_PHASE"))
    {
        PAR_TEST_SKIP(ctx, "fault_phase_selected");
    }

    return ePAR_TEST_PASS;
}

/**
 * @brief Reset the host fake flash image before one destructive test case.
 * @param ctx Test execution context.
 * @return ePAR_TEST_PASS on success, otherwise ePAR_TEST_FAIL.
 */
static par_test_result_t par_test_flash_ee_host_reset(par_test_context_t *ctx)
{
    par_status_t status;

    if (true == par_is_init())
    {
        status = par_deinit();
        if (false == PAR_TEST_STATUS_HAS_NO_ERROR(status))
        {
            PAR_TEST_FAIL(ctx, "par_deinit_status=%u", (unsigned)status);
        }
    }

    status = par_host_flash_ee_reset_image();

    if (ePAR_OK != status)
    {
        PAR_TEST_FAIL(ctx, "host_flash_reset_status=%u", (unsigned)status);
    }

    return ePAR_TEST_PASS;
}

/**
 * @brief Return host fake Flash EE append-record capacity.
 * @param ctx Test execution context.
 * @param p_capacity Receives append-record capacity per bank.
 * @return ePAR_TEST_PASS on success, otherwise ePAR_TEST_FAIL.
 */
static par_test_result_t par_test_flash_ee_host_get_capacity(par_test_context_t *ctx, uint32_t *p_capacity)
{
    if (NULL == p_capacity)
    {
        PAR_TEST_FAIL(ctx, "capacity_output_null");
    }

    *p_capacity = par_host_flash_ee_bank_record_capacity();
    return ePAR_TEST_PASS;
}

/**
 * @brief Bind and initialize the host fake-flash Flash EE backend.
 * @param ctx Test execution context.
 * @param pp_api Receives the initialized backend API on success.
 * @return ePAR_TEST_PASS on success, otherwise ePAR_TEST_FAIL.
 */
static par_test_result_t par_test_flash_ee_host_open(par_test_context_t *ctx, const par_store_backend_api_t **pp_api)
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
                      "bind_status=%u diag=%s",
                      (unsigned)status,
                      par_store_backend_flash_ee_get_diag_str(par_store_backend_flash_ee_get_diag()));
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
                      "init_status=%u diag=%s",
                      (unsigned)status,
                      par_store_backend_flash_ee_get_diag_str(par_store_backend_flash_ee_get_diag()));
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
 * @brief Deinitialize the host fake-flash Flash EE backend.
 * @param ctx Test execution context.
 * @param p_api Backend API.
 * @return ePAR_TEST_PASS on success, otherwise ePAR_TEST_FAIL.
 */
static par_test_result_t par_test_flash_ee_host_close(par_test_context_t *ctx, const par_store_backend_api_t *p_api)
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
 * @brief Host fake-flash Flash EE operations consumed by common test flows.
 */
static const par_test_flash_ee_ops_t g_par_test_flash_ee_host_ops = {
    .name = "flash_ee_host",
    .reset = par_test_flash_ee_host_reset,
    .open = par_test_flash_ee_host_open,
    .close = par_test_flash_ee_host_close,
    .get_capacity = par_test_flash_ee_host_get_capacity,
};

/**
 * @brief Validate that the host fake flash can be bound and formatted.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_host_bind_init(par_test_context_t *ctx)
{
    if (ePAR_TEST_PASS != par_test_flash_ee_host_skip_normal_during_fault_phase(ctx))
    {
        return ePAR_TEST_SKIP;
    }

    return par_test_flash_ee_common_bind_init(ctx, &g_par_test_flash_ee_host_ops);
}

/**
 * @brief Validate write/read data survives host Flash EE backend reinitialization.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_host_write_read_persists_after_reinit(par_test_context_t *ctx)
{
    if (ePAR_TEST_PASS != par_test_flash_ee_host_skip_normal_during_fault_phase(ctx))
    {
        return ePAR_TEST_SKIP;
    }

    return par_test_flash_ee_common_write_read_persists_after_reinit(ctx, &g_par_test_flash_ee_host_ops);
}

/**
 * @brief Validate logical erase survives host Flash EE backend reinitialization.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_host_erase_persists_after_reinit(par_test_context_t *ctx)
{
    if (ePAR_TEST_PASS != par_test_flash_ee_host_skip_normal_during_fault_phase(ctx))
    {
        return ePAR_TEST_SKIP;
    }

    return par_test_flash_ee_common_erase_persists_after_reinit(ctx, &g_par_test_flash_ee_host_ops);
}

/**
 * @brief Validate host fake-flash rollover preserves newest lines and remains writable.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_host_wrap_after_bank_full_preserves_latest(par_test_context_t *ctx)
{
    if (ePAR_TEST_PASS != par_test_flash_ee_host_skip_normal_during_fault_phase(ctx))
    {
        return ePAR_TEST_SKIP;
    }

    return par_test_flash_ee_common_wrap_after_bank_full_preserves_latest(ctx, &g_par_test_flash_ee_host_ops);
}

/**
 * @brief Validate multiple host fake-flash rollover cycles preserve the latest logical image.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_host_wrap_multiple_cycles_preserves_latest(par_test_context_t *ctx)
{
    if (ePAR_TEST_PASS != par_test_flash_ee_host_skip_normal_during_fault_phase(ctx))
    {
        return ePAR_TEST_SKIP;
    }

    return par_test_flash_ee_common_wrap_multiple_cycles_preserves_latest(ctx, &g_par_test_flash_ee_host_ops);
}

/**
 * @brief Validate a full host fake-flash bank is writable after reboot-like reinit.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_host_full_bank_then_reinit_remains_writable(par_test_context_t *ctx)
{
    if (ePAR_TEST_PASS != par_test_flash_ee_host_skip_normal_during_fault_phase(ctx))
    {
        return ePAR_TEST_SKIP;
    }

    return par_test_flash_ee_common_full_bank_then_reinit_remains_writable(ctx, &g_par_test_flash_ee_host_ops);
}

/**
 * @brief Seed a deterministic interrupted program operation and exit without a clean backend close.
 * @details This case is intended to run as the first process in a two-process
 *          power-loss simulation. Set PAR_HOST_FLASH_EE_FAULT_PHASE=seed.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_host_fault_program_prefix_seed(par_test_context_t *ctx)
{
    const par_store_backend_api_t *p_api;
    uint8_t stable0[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    uint8_t stable1[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    uint8_t update0[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    par_status_t status;

    if (false == par_test_flash_ee_host_env_is("PAR_HOST_FLASH_EE_FAULT_PHASE", "seed"))
    {
        PAR_TEST_SKIP(ctx, "fault_seed_phase_not_selected");
    }

    if ((ePAR_TEST_PASS != par_test_flash_ee_host_reset(ctx)) ||
        (ePAR_TEST_PASS != par_test_flash_ee_host_open(ctx, &p_api)))
    {
        return ePAR_TEST_FAIL;
    }

    if ((ePAR_TEST_PASS != par_test_flash_ee_write_line_pattern(ctx, p_api, 0u, 0x51u, stable0, "seed_line0")) ||
        (ePAR_TEST_PASS != par_test_flash_ee_write_line_pattern(ctx, p_api, 1u, 0x61u, stable1, "seed_line1")))
    {
        (void)p_api->deinit();
        return ePAR_TEST_FAIL;
    }
    (void)stable0;
    (void)stable1;

    status = p_api->sync();
    if (ePAR_OK != status)
    {
        (void)p_api->deinit();
        PAR_TEST_FAIL(ctx, "stable_sync_status=%u", (unsigned)status);
    }

    par_test_flash_ee_fill_pattern(update0, sizeof(update0), 0x71u);

    {
        jmp_buf powercut_jump;

        if (0 != setjmp(powercut_jump))
        {
            par_host_fake_storage_clear_powercut_jump();
            (void)par_host_fake_storage_sync();
            return ePAR_TEST_PASS;
        }

        par_host_fake_storage_set_powercut_jump(&powercut_jump);
        par_host_fake_storage_set_failpoint(ePAR_HOST_FAKE_STORAGE_OP_PROGRAM, 1u, 4u);
        status = p_api->write(0u, sizeof(update0), update0);
        par_host_fake_storage_clear_powercut_jump();
    }

    if (ePAR_OK == status)
    {
        (void)p_api->deinit();
        PAR_TEST_FAIL(ctx, "fault_program_unexpected_success");
    }

    (void)p_api->deinit();
    PAR_TEST_FAIL(ctx, "fault_program_returned_without_powercut status=%u", (unsigned)status);
}

/**
 * @brief Verify recovery after the interrupted program seed case.
 * @details This case is intended to run as the second process in a two-process
 *          power-loss simulation. Set PAR_HOST_FLASH_EE_FAULT_PHASE=recover.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_host_fault_program_prefix_recover(par_test_context_t *ctx)
{
    const par_store_backend_api_t *p_api;
    uint8_t stable0[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    uint8_t stable1[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    uint8_t recovery2[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];

    if (false == par_test_flash_ee_host_env_is("PAR_HOST_FLASH_EE_FAULT_PHASE", "recover"))
    {
        PAR_TEST_SKIP(ctx, "fault_recover_phase_not_selected");
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_host_open(ctx, &p_api))
    {
        return ePAR_TEST_FAIL;
    }

    par_test_flash_ee_fill_pattern(stable0, sizeof(stable0), 0x51u);
    par_test_flash_ee_fill_pattern(stable1, sizeof(stable1), 0x61u);

    if ((ePAR_TEST_PASS != par_test_flash_ee_expect_line(ctx, p_api, 0u, stable0, "line0_after_powercut")) ||
        (ePAR_TEST_PASS != par_test_flash_ee_expect_line(ctx, p_api, 1u, stable1, "line1_after_powercut")))
    {
        (void)p_api->deinit();
        return ePAR_TEST_FAIL;
    }

    if (ePAR_TEST_PASS != par_test_flash_ee_write_line_pattern(ctx, p_api, 2u, 0x81u, recovery2, "post_powercut_write"))
    {
        (void)p_api->deinit();
        return ePAR_TEST_FAIL;
    }

    return par_test_flash_ee_host_close(ctx, p_api);
}

#else
/**
 * @brief Skip host Flash EE tests when the host native backend is unavailable.
 * @param ctx Test execution context.
 * @return Case result.
 */
static par_test_result_t par_test_flash_ee_host_backend_disabled(par_test_context_t *ctx)
{
    PAR_TEST_SKIP(ctx, "flash_ee_host_backend_disabled");
}
#endif /* PAR_TEST_FLASH_EE_HOST_SUITE_ENABLED */

/**
 * @brief Case flag used for host fake-flash tests that rewrite the image file.
 */
#define PAR_TEST_FLASH_EE_HOST_CASE_FLAGS ((uint32_t)ePAR_TEST_CASE_FLAG_DESTRUCTIVE)

/**
 * @brief Flash EE host validation case table.
 */
static const par_test_case_t g_par_test_flash_ee_host_cases[] = {
#if PAR_TEST_FLASH_EE_HOST_SUITE_ENABLED
    { "bind_init", par_test_flash_ee_host_bind_init, PAR_TEST_FLASH_EE_HOST_CASE_FLAGS },
    { "write_read_persists_after_reinit", par_test_flash_ee_host_write_read_persists_after_reinit, PAR_TEST_FLASH_EE_HOST_CASE_FLAGS },
    { "erase_persists_after_reinit", par_test_flash_ee_host_erase_persists_after_reinit, PAR_TEST_FLASH_EE_HOST_CASE_FLAGS },
    { "wrap_after_bank_full_preserves_latest",
      par_test_flash_ee_host_wrap_after_bank_full_preserves_latest,
      PAR_TEST_FLASH_EE_HOST_CASE_FLAGS },
    { "wrap_multiple_cycles_preserves_latest",
      par_test_flash_ee_host_wrap_multiple_cycles_preserves_latest,
      PAR_TEST_FLASH_EE_HOST_CASE_FLAGS },
    { "full_bank_then_reinit_remains_writable",
      par_test_flash_ee_host_full_bank_then_reinit_remains_writable,
      PAR_TEST_FLASH_EE_HOST_CASE_FLAGS },
    { "fault_program_prefix_seed", par_test_flash_ee_host_fault_program_prefix_seed, PAR_TEST_FLASH_EE_HOST_CASE_FLAGS },
    { "fault_program_prefix_recover", par_test_flash_ee_host_fault_program_prefix_recover, PAR_TEST_FLASH_EE_HOST_CASE_FLAGS },
#else
    { "backend_disabled", par_test_flash_ee_host_backend_disabled, (uint32_t)ePAR_TEST_CASE_FLAG_NONE },
#endif /* PAR_TEST_FLASH_EE_HOST_SUITE_ENABLED */
};

const par_test_suite_t g_par_test_suite_flash_ee_host = {
    .name = "flash_ee_host",
    .cases = g_par_test_flash_ee_host_cases,
    .case_count = (uint32_t)PAR_TEST_ARRAY_SIZE(g_par_test_flash_ee_host_cases),
};
