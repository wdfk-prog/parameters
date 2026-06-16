/**
 * @file par_test_runner.c
 * @brief Provide reusable runtime-test dispatch for hardware and host harnesses.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#include "par_test.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Active test log printer.
 */
static par_test_vprint_fn_t g_par_test_vprint = NULL;

/**
 * @brief Opaque context passed to the active test log printer.
 */
static void *g_par_test_vprint_ctx = NULL;

/**
 * @brief Registered runtime-test suites.
 */
static const par_test_suite_t * const g_par_test_suites[] = {
#if defined(AUTOGEN_PM_TEST_USING_RAM_CONFIG)
    &g_par_test_suite_ram,
#endif /* defined(AUTOGEN_PM_TEST_USING_RAM_CONFIG) */
#if defined(AUTOGEN_PM_TEST_USING_AT24CXX)
    &g_par_test_suite_at24,
#endif /* defined(AUTOGEN_PM_TEST_USING_AT24CXX) */
#if defined(AUTOGEN_PM_TEST_USING_EEPROM_HOST)
    &g_par_test_suite_eeprom_host,
#endif /* defined(AUTOGEN_PM_TEST_USING_EEPROM_HOST) */
#if defined(AUTOGEN_PM_TEST_USING_FLASH_EE_FAL)
    &g_par_test_suite_flash_ee_fal,
#endif /* defined(AUTOGEN_PM_TEST_USING_FLASH_EE_FAL) */
#if defined(AUTOGEN_PM_TEST_USING_FLASH_EE_HOST)
    &g_par_test_suite_flash_ee_host,
#endif /* defined(AUTOGEN_PM_TEST_USING_FLASH_EE_HOST) */
    NULL,
};


/**
 * @brief Count compiled runtime-test suites excluding the sentinel entry.
 * @return Number of registered suites.
 */
uint32_t par_test_get_suite_count(void)
{
    uint32_t i;
    uint32_t count = 0U;

    for (i = 0U; i < (uint32_t)PAR_TEST_ARRAY_SIZE(g_par_test_suites); i++)
    {
        if (NULL != g_par_test_suites[i])
        {
            count++;
        }
    }

    return count;
}

void par_test_set_vprint(par_test_vprint_fn_t p_vprint, void *p_ctx)
{
    g_par_test_vprint = p_vprint;
    g_par_test_vprint_ctx = p_ctx;
}

void par_test_vprint(const char *fmt, va_list args)
{
    if (NULL == fmt)
    {
        return;
    }

    if (NULL != g_par_test_vprint)
    {
        g_par_test_vprint(g_par_test_vprint_ctx, fmt, args);
    }
    else
    {
        (void)vprintf(fmt, args);
    }
}

void par_test_print(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    par_test_vprint(fmt, args);
    va_end(args);
}

void par_test_set_detail(par_test_context_t *ctx, const char *fmt, ...)
{
    va_list args;

    if ((NULL == ctx) || (NULL == fmt))
    {
        return;
    }

    va_start(args, fmt);
    (void)vsnprintf(ctx->detail, sizeof(ctx->detail), fmt, args);
    va_end(args);
    ctx->detail[sizeof(ctx->detail) - 1U] = '\0';
}

par_status_t par_test_ensure_parameter_init(void)
{
    if (true == par_is_init())
    {
        return ePAR_OK;
    }

    return par_init();
}

/**
 * @brief Return true when the named filter selects a suite.
 * @param filter Requested suite name or "all".
 * @param suite Candidate suite descriptor.
 * @return true when the suite should run.
 */
static bool par_test_suite_selected(const char *filter, const par_test_suite_t *suite)
{
    if (NULL == suite)
    {
        return false;
    }

    if ((NULL == filter) || (0 == strcmp(filter, "all")))
    {
        return true;
    }

    return (0 == strcmp(filter, suite->name));
}

/**
 * @brief Accumulate one case result into a test summary.
 * @param summary Aggregate summary.
 * @param result Case result.
 */
static void par_test_summary_add(par_test_summary_t *summary, const par_test_result_t result)
{
    if (NULL == summary)
    {
        return;
    }

    summary->total++;
    switch (result)
    {
    case ePAR_TEST_PASS:
        summary->pass++;
        break;
    case ePAR_TEST_FAIL:
        summary->fail++;
        break;
    case ePAR_TEST_SKIP:
    default:
        summary->skip++;
        break;
    }
}

/**
 * @brief Print one case result in the stable CI log contract.
 * @param ctx Active case context.
 * @param result Case result.
 */
static void par_test_print_case_result(const par_test_context_t *ctx, const par_test_result_t result)
{
    const char *result_str = "SKIP";

    if (ePAR_TEST_PASS == result)
    {
        result_str = "PASS";
    }
    else if (ePAR_TEST_FAIL == result)
    {
        result_str = "FAIL";
    }

    if ((NULL != ctx) && ('\0' != ctx->detail[0]))
    {
        par_test_print("PAR_TEST_CASE %s suite=%s case=%s detail=%s\n",
                       result_str,
                       ctx->suite_name,
                       ctx->case_name,
                       ctx->detail);
    }
    else if (NULL != ctx)
    {
        par_test_print("PAR_TEST_CASE %s suite=%s case=%s\n",
                       result_str,
                       ctx->suite_name,
                       ctx->case_name);
    }
}

/**
 * @brief Run one suite and print its stable CI summary.
 * @param suite Suite descriptor.
 * @return Suite summary.
 */
par_test_summary_t par_test_run_suite(const par_test_suite_t *suite)
{
    par_test_summary_t summary = { 0U, 0U, 0U, 0U };
    uint32_t i;

    if ((NULL == suite) || (NULL == suite->cases))
    {
        return summary;
    }

    par_test_print("PAR_TEST_BEGIN suite=%s cases=%u\n", suite->name, (unsigned)suite->case_count);

    for (i = 0U; i < suite->case_count; i++)
    {
        par_test_result_t result = ePAR_TEST_FAIL;
        par_test_context_t ctx = { suite->name, suite->cases[i].name, { 0 } };

        if (NULL == suite->cases[i].run)
        {
            par_test_set_detail(&ctx, "missing_case_callback");
            result = ePAR_TEST_FAIL;
        }
        else if (0U != (suite->cases[i].flags & (uint32_t)ePAR_TEST_CASE_FLAG_DESTRUCTIVE))
        {
#if defined(AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE)
            result = suite->cases[i].run(&ctx);
#else
            par_test_set_detail(&ctx, "destructive_case_disabled");
            result = ePAR_TEST_SKIP;
#endif /* defined(AUTOGEN_PM_TEST_ALLOW_DESTRUCTIVE) */
        }
        else
        {
            result = suite->cases[i].run(&ctx);
        }

        par_test_summary_add(&summary, result);
        par_test_print_case_result(&ctx, result);

#if defined(AUTOGEN_PM_TEST_FAIL_FAST)
        if (summary.fail > 0U)
        {
            break;
        }
#endif /* defined(AUTOGEN_PM_TEST_FAIL_FAST) */
    }

    par_test_print("PAR_TEST_SUMMARY suite=%s total=%u pass=%u fail=%u skip=%u\n",
                   suite->name,
                   (unsigned)summary.total,
                   (unsigned)summary.pass,
                   (unsigned)summary.fail,
                   (unsigned)summary.skip);
    par_test_print("PAR_TEST_RESULT %s suite=%s\n", (summary.fail == 0U) ? "PASS" : "FAIL", suite->name);

    return summary;
}

const par_test_suite_t *par_test_get_suite(const uint32_t index)
{
    uint32_t i;
    uint32_t matched = 0U;

    for (i = 0U; i < PAR_TEST_ARRAY_SIZE(g_par_test_suites); i++)
    {
        if (NULL != g_par_test_suites[i])
        {
            if (matched == index)
            {
                return g_par_test_suites[i];
            }
            matched++;
        }
    }

    return NULL;
}

par_test_summary_t par_test_run_by_name(const char *name)
{
    par_test_summary_t total = { 0U, 0U, 0U, 0U };
    uint32_t i;
    bool matched = false;

    par_test_print("PAR_TEST_RUN target=%s suites=%u\n",
                   (NULL != name) ? name : "all",
                   (unsigned)par_test_get_suite_count());

    for (i = 0U; i < PAR_TEST_ARRAY_SIZE(g_par_test_suites); i++)
    {
        if (true == par_test_suite_selected(name, g_par_test_suites[i]))
        {
            par_test_summary_t suite_summary = par_test_run_suite(g_par_test_suites[i]);
            matched = true;
            total.total += suite_summary.total;
            total.pass += suite_summary.pass;
            total.fail += suite_summary.fail;
            total.skip += suite_summary.skip;
        }
    }

    if (false == matched)
    {
        par_test_print("PAR_TEST_RESULT FAIL suite=%s detail=unknown_suite\n", (NULL != name) ? name : "null");
        total.fail = 1U;
        total.total = 1U;
    }

    par_test_print("PAR_TEST_SUMMARY suite=all total=%u pass=%u fail=%u skip=%u\n",
                   (unsigned)total.total,
                   (unsigned)total.pass,
                   (unsigned)total.fail,
                   (unsigned)total.skip);
    par_test_print("PAR_TEST_RESULT %s suite=all\n", (total.fail == 0U) ? "PASS" : "FAIL");

    return total;
}

/**
 * @brief Print available parameter test suites.
 */
void par_test_print_list(void)
{
    uint32_t i;

    par_test_print("PAR_TEST_LIST_BEGIN suites=%u\n", (unsigned)par_test_get_suite_count());
    for (i = 0U; i < PAR_TEST_ARRAY_SIZE(g_par_test_suites); i++)
    {
        if (NULL != g_par_test_suites[i])
        {
            par_test_print("PAR_TEST_SUITE name=%s cases=%u\n",
                           g_par_test_suites[i]->name,
                           (unsigned)g_par_test_suites[i]->case_count);
        }
    }
    par_test_print("PAR_TEST_LIST_END\n");
}

