#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/** @brief Append a bad committed tail after a valid commit. */
static bool host_flash_child_append_bad_committed_tail_after_commit(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT(host_flash_ee_append_bad_committed_tail_record(0U));
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/** @brief Append a bad committed tail after two valid records for the same scalar. */
static bool host_flash_child_append_newer_bad_record_after_two_commits(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT(host_flash_ee_append_bad_committed_tail_record(0U));
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/**
 * @brief Verify a bad committed tail rebuilds defaults by current policy.
 * @details The current replay policy does not skip a bad committed tail and
 *          does not fall back to an older good record. Once the committed tail
 *          record fails validation, the image is treated as untrusted and the
 *          runtime is rebuilt from defaults.
 */
static bool test_flash_ee_bad_committed_tail_rebuilds_default_current_policy(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_append_bad_committed_tail_after_commit());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_verify_mode_value(1U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/**
 * @brief Verify a newer bad record rebuilds defaults by current policy.
 * @details This intentionally expects the default mode value after restart,
 *          even though older good records exist. Do not change this test to
 *          older-good fallback unless Flash-EE replay policy is changed and
 *          documented explicitly.
 */
static bool test_flash_ee_newer_bad_record_rebuilds_default_current_policy(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_append_newer_bad_record_after_two_commits());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_verify_mode_value(1U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/** @brief Save a scalar through a change callback and exit without deinit. */
static bool host_flash_child_callback_save_during_dispatch(void)
{
    TEST_ASSERT(init_module());
    g_nvm_callback_hits = 0U;
    par_register_on_change_cb(ePAR_TEST_MODE, on_nvm_scalar_change_save);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT(g_nvm_callback_hits == 1U);
    return true;
}

/** @brief Verify current callback-time save policy survives a hard process restart. */
static bool test_flash_ee_callback_save_during_dispatch_current_policy(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_callback_save_during_dispatch());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_verify_mode_value(4U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}

/** @brief Save all live values from a callback and exit without deinit. */
static bool host_flash_child_callback_save_all_during_dispatch(void)
{
    TEST_ASSERT(init_module());
    g_nvm_callback_hits = 0U;
    par_register_on_change_cb(ePAR_TEST_MODE, on_nvm_scalar_change_save_all);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 7U));
    TEST_ASSERT(g_nvm_callback_hits == 1U);
    return true;
}

/** @brief Verify current callback-time save-all policy survives a hard process restart. */
static bool test_flash_ee_callback_save_all_during_dispatch_current_policy(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_callback_save_all_during_dispatch());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_verify_mode_value(7U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}

/** @brief Save-clean from a callback and exit without deinit. */
static bool host_flash_child_callback_save_clean_during_dispatch(void)
{
    TEST_ASSERT(init_module());
    g_nvm_callback_hits = 0U;
    par_register_on_change_cb(ePAR_TEST_MODE, on_nvm_scalar_change_save_clean);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 8U));
    TEST_ASSERT(g_nvm_callback_hits == 1U);
    return true;
}

/** @brief Verify current callback-time save-clean policy survives a hard process restart. */
static bool test_flash_ee_callback_save_clean_during_dispatch_current_policy(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_callback_save_clean_during_dispatch());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_verify_mode_value(8U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/** @brief Deinitialize from a change callback and exit with the module down. */
static bool host_flash_child_callback_deinit_during_dispatch(void)
{
    TEST_ASSERT(init_module());
    g_nvm_callback_hits = 0U;
    par_register_on_change_cb(ePAR_TEST_MODE, on_nvm_scalar_change_deinit);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 6U));
    TEST_ASSERT(g_nvm_callback_hits == 1U);
    TEST_ASSERT(!par_is_init());
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/** @brief Verify callback-time deinit does not persist through the current policy. */
static bool test_flash_ee_callback_deinit_during_dispatch_current_policy(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_callback_deinit_during_dispatch());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_verify_mode_value(1U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
