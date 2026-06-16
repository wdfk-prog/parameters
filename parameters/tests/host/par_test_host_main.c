/**
 * @file par_test_host_main.c
 * @brief Provide the host executable entry for reusable parameter runtime tests.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#include "par_test.h"

#include <string.h>

/**
 * @brief Run parameter runtime tests from a host process.
 * @param argc Argument count.
 * @param argv Argument vector. argv[1] may be "list", "all", or a suite name.
 * @return 0 when selected tests pass, otherwise 1.
 */
int main(int argc, char **argv)
{
    const char *target = (argc > 1) ? argv[1] : "all";
    par_test_summary_t summary;

    if (0 == strcmp(target, "list"))
    {
        par_test_print_list();
        return 0;
    }

    summary = par_test_run_by_name(target);
    return (0U == summary.fail) ? 0 : 1;
}
