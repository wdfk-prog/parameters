/**
 * @file test_par_config_smoke.c
 * @brief Provide a small runtime smoke test for legal feature-off matrices.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include "test_host_common.h"

/** @brief Verify core scalar APIs remain usable in a legal feature-off build. */
static bool test_config_scalar_smoke(void)
{
    uint8_t value = 0U;

    TEST_ASSERT_OK(par_init());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 1U);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 4U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Entrypoint for feature-off configuration smoke tests. */
int main(void)
{
    static const par_host_test_case_t cases[] = {
        { "config_scalar_smoke", test_config_scalar_smoke },
    };

    return par_host_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
}
