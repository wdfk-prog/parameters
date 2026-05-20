/**
 * @file test_par_mutex_runtime.c
 * @brief Exercise mutex failure paths in public parameter APIs.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include "test_host_common.h"
#include "par_registration_api.h"

/** @brief Disabled value for host mutex acquire failpoint. */
#define HOST_MUTEX_FAIL_DISABLED (-1)

/** @brief Mutex acquire failpoint countdown; negative disables it. */
static int g_mutex_fail_after = HOST_MUTEX_FAIL_DISABLED;
/** @brief Count of successful fake mutex acquisitions. */
static unsigned g_mutex_acquire_count;
/** @brief Count of fake mutex releases. */
static unsigned g_mutex_release_count;
/** @brief Current fake recursive mutex lock depth. */
static unsigned g_mutex_lock_depth;
/** @brief Maximum fake recursive mutex lock depth seen by a test case. */
static unsigned g_mutex_max_lock_depth;
/** @brief Count of fake mutex release calls without a matching acquire. */
static unsigned g_mutex_release_underflow_count;
/** @brief Callback hit counter used by mutex-enabled reentrant tests. */
static unsigned g_mutex_reentrant_hits;
/** @brief Status returned by the callback's nested setter call. */
static par_status_t g_mutex_reentrant_set_status;

/** @brief Reset fake mutex state shared by mutex host test cases. */
static void host_mutex_reset(void)
{
    g_mutex_fail_after = HOST_MUTEX_FAIL_DISABLED;
    g_mutex_acquire_count = 0U;
    g_mutex_release_count = 0U;
    g_mutex_lock_depth = 0U;
    g_mutex_max_lock_depth = 0U;
    g_mutex_release_underflow_count = 0U;
    g_mutex_reentrant_hits = 0U;
    g_mutex_reentrant_set_status = ePAR_OK;
}

/**
 * @brief Host override for parameter mutex acquisition.
 * @param par_num Parameter number whose mutex is acquired.
 * @return ePAR_OK when the fake recursive mutex is acquired;
 *         ePAR_ERROR on failpoint.
 */
par_status_t par_if_aquire_mutex(const par_num_t par_num)
{
    (void)par_num;

    if (0 == g_mutex_fail_after)
    {
        return ePAR_ERROR;
    }
    if (g_mutex_fail_after > 0)
    {
        g_mutex_fail_after--;
    }

    g_mutex_acquire_count++;
    g_mutex_lock_depth++;
    if (g_mutex_lock_depth > g_mutex_max_lock_depth)
    {
        g_mutex_max_lock_depth = g_mutex_lock_depth;
    }
    return ePAR_OK;
}

/**
 * @brief Host override for parameter mutex release.
 * @param par_num Parameter number whose mutex is released.
 */
void par_if_release_mutex(const par_num_t par_num)
{
    (void)par_num;
    g_mutex_release_count++;
    if (g_mutex_lock_depth > 0U)
    {
        g_mutex_lock_depth--;
    }
    else
    {
        g_mutex_release_underflow_count++;
    }
}

#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/**
 * @brief Update another scalar from a callback while mutex hooks are enabled.
 * @param par_num Parameter number that changed.
 * @param new_val New scalar value.
 * @param old_val Previous scalar value.
 */
static void on_mutex_reentrant_update_other(const par_num_t par_num,
                                            const par_type_t new_val,
                                            const par_type_t old_val)
{
    (void)par_num;
    (void)new_val;
    (void)old_val;
    g_mutex_reentrant_hits++;
    g_mutex_reentrant_set_status = par_set_u16(ePAR_TEST_U16, 444U);
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

/**
 * @brief Initialize the parameter module for one mutex test case.
 * @return true when initialization succeeds.
 */
static bool init_module(void)
{
    if (par_is_init())
    {
        TEST_ASSERT_OK(par_deinit());
    }

    host_mutex_reset();
    TEST_ASSERT_OK(par_init());
    host_mutex_reset();
    return true;
}

/** @brief Verify scalar setters report mutex acquisition failures. */
static bool test_mutex_scalar_setter_failure_preserves_value(void)
{
    uint8_t value = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 2U));
    host_mutex_reset();
    g_mutex_fail_after = 0;
    TEST_ASSERT_STATUS(par_set_u8(ePAR_TEST_MODE, 3U), ePAR_ERROR_MUTEX);
    host_mutex_reset();
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 2U);
    TEST_ASSERT(g_mutex_acquire_count == 0U);
    TEST_ASSERT(g_mutex_release_count == 0U);
    TEST_ASSERT(g_mutex_lock_depth == 0U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object setters report mutex acquisition failures. */
static bool test_mutex_object_setter_failure_preserves_value(void)
{
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "old"));
    host_mutex_reset();
    g_mutex_fail_after = 0;
    TEST_ASSERT_STATUS(par_set_str(ePAR_TEST_STR, "new"), ePAR_ERROR_MUTEX);
    TEST_ASSERT(g_mutex_acquire_count == 0U);
    TEST_ASSERT(g_mutex_release_count == 0U);
    TEST_ASSERT(g_mutex_lock_depth == 0U);
    host_mutex_reset();
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 3U);
    TEST_ASSERT(strcmp(str_buf, "old") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object has-changed reports mutex acquisition failures. */
static bool test_mutex_object_has_changed_failure_reports_mutex(void)
{
    bool changed = false;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "changed"));
    host_mutex_reset();
    g_mutex_fail_after = 0;
    TEST_ASSERT_STATUS(par_has_changed(ePAR_TEST_STR, &changed), ePAR_ERROR_MUTEX);
    TEST_ASSERT(g_mutex_acquire_count == 0U);
    TEST_ASSERT(g_mutex_release_count == 0U);
    TEST_ASSERT(g_mutex_lock_depth == 0U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify default reset reports mutex acquisition failures. */
static bool test_mutex_default_reset_failure_reports_mutex(void)
{
    uint8_t value = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    host_mutex_reset();
    g_mutex_fail_after = 0;
    TEST_ASSERT_STATUS(par_set_to_default(ePAR_TEST_MODE), ePAR_ERROR_MUTEX);
    host_mutex_reset();
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 5U);
    TEST_ASSERT(g_mutex_acquire_count == 0U);
    TEST_ASSERT(g_mutex_release_count == 0U);
    TEST_ASSERT(g_mutex_lock_depth == 0U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/** @brief Verify current recursive-mutex policy permits cross-parameter callbacks. */
static bool test_mutex_reentrant_callback_updates_other_parameter_recursive_current_policy(void)
{
    uint16_t value16 = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8_fast(ePAR_TEST_MODE, 1U));
    par_register_on_change_cb(ePAR_TEST_MODE, on_mutex_reentrant_update_other);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 2U));
    TEST_ASSERT(g_mutex_reentrant_hits == 1U);
    TEST_ASSERT_OK(g_mutex_reentrant_set_status);
    TEST_ASSERT(g_mutex_max_lock_depth >= 2U);
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &value16));
    TEST_ASSERT(value16 == 444U);
    par_register_on_change_cb(ePAR_TEST_MODE, NULL);
    TEST_ASSERT(g_mutex_acquire_count == g_mutex_release_count);
    TEST_ASSERT(g_mutex_lock_depth == 0U);
    TEST_ASSERT(g_mutex_release_underflow_count == 0U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

/** @brief Entrypoint for mutex host runtime tests. */
int main(void)
{
    static const par_host_test_case_t cases[] = {
        { "mutex_scalar_setter_failure_preserves_value", test_mutex_scalar_setter_failure_preserves_value },
        { "mutex_object_setter_failure_preserves_value", test_mutex_object_setter_failure_preserves_value },
        { "mutex_object_has_changed_failure_reports_mutex", test_mutex_object_has_changed_failure_reports_mutex },
        { "mutex_default_reset_failure_reports_mutex", test_mutex_default_reset_failure_reports_mutex },
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
        { "mutex_reentrant_callback_updates_other_parameter_recursive_current_policy", test_mutex_reentrant_callback_updates_other_parameter_recursive_current_policy },
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
    };

    return par_host_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
}
