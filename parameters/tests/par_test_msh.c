/**
 * @file par_test_msh.c
 * @brief Provide RT-Thread MSH wrappers for the reusable parameter test runner.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#include "par_test.h"

#include <stdarg.h>
#include <string.h>

#include <rtthread.h>
#ifdef RT_USING_FINSH
#include <finsh.h>
#endif /* defined(RT_USING_FINSH) */

/**
 * @brief Forward one parameter-test log line to RT-Thread console output.
 * @param p_ctx Opaque print context, currently unused.
 * @param fmt printf-style format string.
 * @param args printf-style argument list.
 */
static void par_test_msh_vprint(void *p_ctx, const char *fmt, va_list args)
{
    char line[192];

    (void)p_ctx;

    if (NULL == fmt)
    {
        return;
    }

    (void)rt_vsnprintf(line, sizeof(line), fmt, args);
    line[sizeof(line) - 1U] = '\0';
    rt_kprintf("%s", line);
}

/**
 * @brief Install the RT-Thread console printer for hardware test execution.
 */
void par_test_bind_rt_console(void)
{
    par_test_set_vprint(par_test_msh_vprint, NULL);
}

#if defined(RT_USING_FINSH)
/**
 * @brief MSH entry point for parameter runtime tests.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 when the selected suites have no failures; otherwise -1.
 */
static int par_test_msh(int argc, char **argv)
{
    const char *target = "all";
    par_test_summary_t summary;

    par_test_bind_rt_console();

    if (argc > 1)
    {
        target = argv[1];
    }

    if (0 == strcmp(target, "list"))
    {
        par_test_print_list();
        return 0;
    }

    summary = par_test_run_by_name(target);
    return (summary.fail == 0U) ? 0 : -1;
}
/**
 * @brief Option-completion IDs for par_test subcommands and suite names.
 */
typedef enum
{
    PAR_TEST_MSH_OPT_LIST = 1, /**< List compiled test suites. */
    PAR_TEST_MSH_OPT_ALL,      /**< Run all compiled test suites. */
    PAR_TEST_MSH_OPT_RAM,      /**< Run RAM configuration tests. */
    PAR_TEST_MSH_OPT_AT24,         /**< Run AT24CXX persistence tests. */
    PAR_TEST_MSH_OPT_FLASH_EE_FAL  /**< Run Flash EE real FAL partition tests. */
} par_test_msh_opt_id_t;

CMD_OPTIONS_NODE_START(par_test_msh)
CMD_OPTIONS_NODE(PAR_TEST_MSH_OPT_LIST, list, list compiled test suites)
CMD_OPTIONS_NODE(PAR_TEST_MSH_OPT_ALL, all, run all compiled suites)
#if defined(AUTOGEN_PM_TEST_USING_RAM_CONFIG)
CMD_OPTIONS_NODE(PAR_TEST_MSH_OPT_RAM, ram, run RAM configuration suite)
#endif /* defined(AUTOGEN_PM_TEST_USING_RAM_CONFIG) */
#if defined(AUTOGEN_PM_TEST_USING_AT24CXX)
CMD_OPTIONS_NODE(PAR_TEST_MSH_OPT_AT24, at24, run AT24CXX persistence suite)
#endif /* defined(AUTOGEN_PM_TEST_USING_AT24CXX) */
#if defined(AUTOGEN_PM_TEST_USING_FLASH_EE_FAL)
CMD_OPTIONS_NODE(PAR_TEST_MSH_OPT_FLASH_EE_FAL, flash_ee_fal, run Flash EE real FAL partition suite)
#endif /* defined(AUTOGEN_PM_TEST_USING_FLASH_EE_FAL) */
CMD_OPTIONS_NODE_END
MSH_CMD_EXPORT_ALIAS(par_test_msh, par_test, parameter manager runtime tests, optenable);
#endif /* defined(RT_USING_FINSH) */

#if defined(AUTOGEN_PM_TEST_AUTO_RUN)
/**
 * @brief Automatically run selected parameter tests after application init.
 * @return RT-Thread init status.
 */
static int par_test_auto_run_init(void)
{
    par_test_bind_rt_console();
    (void)par_test_run_by_name("all");
    return 0;
}
INIT_APP_EXPORT(par_test_auto_run_init);
#endif /* defined(AUTOGEN_PM_TEST_AUTO_RUN) */
