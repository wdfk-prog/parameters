#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED)
/** @brief Verify a reloaded scalar/object pair matches expected committed values. */
static bool host_flash_verify_mode_and_str_value(const uint8_t expected_mode,
                                                 const char *const expected_str)
{
    uint8_t mode = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_clear_failpoints();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &mode));
    TEST_ASSERT(mode == expected_mode);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == (uint16_t)strlen(expected_str));
    TEST_ASSERT(0 == strcmp(str_buf, expected_str));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Save a shared object with a configurable program failpoint. */
static bool host_flash_child_shared_obj_n_save_fail_after_commit(const int program_fail_after)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "old"));
    TEST_ASSERT_OK(par_save_all());
    g_fail_program_after = program_fail_after;
    TEST_ASSERT((par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"new", 3U) &
                 ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    return true;
}

/** @brief Verify shared object n-save program failpoints preserve last commit. */
static bool test_nvm_shared_obj_n_save_program_failpoint_matrix_preserves_last_commit(void)
{
    static const int fail_after_cases[] = { 0, 1 };

    for (size_t i = 0U; i < (sizeof(fail_after_cases) / sizeof(fail_after_cases[0])); i++)
    {
        pid_t child_pid;

        host_flash_reset_erased();
        child_pid = fork();
        TEST_ASSERT(child_pid >= 0);
        if (0 == child_pid)
        {
            host_child_exit_from_result(
                host_flash_child_shared_obj_n_save_fail_after_commit(fail_after_cases[i]));
        }
        TEST_ASSERT(host_child_exit_is_success(child_pid));

        child_pid = fork();
        TEST_ASSERT(child_pid >= 0);
        if (0 == child_pid)
        {
            host_child_exit_from_result(host_flash_verify_mode_and_str_value(4U, "old"));
        }
        TEST_ASSERT(host_child_exit_is_success(child_pid));
    }

    return true;
}

/** @brief Save mixed values, fail twice, and keep the last complete mixed commit. */
static bool test_nvm_shared_repeated_mixed_failures_converge_to_last_commit(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "old"));
    TEST_ASSERT_OK(par_save_all());
    TEST_ASSERT_OK(par_deinit());

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        TEST_ASSERT(init_module());
        TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
        TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "new"));
        g_fail_program_after = 0;
        TEST_ASSERT((par_save_all() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
        host_child_exit_from_result(true);
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        TEST_ASSERT(init_module());
        TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 6U));
        TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "bad"));
        g_fail_program_after = 1;
        TEST_ASSERT((par_save_all() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
        host_child_exit_from_result(true);
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_verify_mode_and_str_value(4U, "old"));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}

/** @brief Verify deterministic mixed scalar/object save-restart steps match a model. */
static bool test_nvm_shared_seeded_mixed_restarts_match_model(void)
{
    static const struct
    {
        uint8_t mode;       /**< Expected scalar mode after the step. */
        const char *str;    /**< Expected string object after the step. */
        bool save_all;      /**< Persist with save-all instead of per-item saves. */
    } steps[] = {
        { 2U, "aa", false },
        { 5U, "bbb", true },
        { 8U, "c", false },
        { 3U, "model", true },
        { 6U, "d", false },
        { 9U, "ee", true },
        { 4U, "fff", false },
        { 7U, "gggg", true },
        { 1U, "h", false },
        { 10U, "ii", true },
        { 2U, "jjj", false },
        { 5U, "final", true },
    };

    host_flash_reset_erased();
    for (size_t i = 0U; i < (sizeof(steps) / sizeof(steps[0])); i++)
    {
        TEST_ASSERT(init_module());
        TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, steps[i].mode));
        TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, steps[i].str));
        if (steps[i].save_all)
        {
            TEST_ASSERT_OK(par_save_all());
        }
        else
        {
            TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
            TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
        }
        TEST_ASSERT_OK(par_deinit());
        TEST_ASSERT(host_flash_verify_mode_and_str_value(steps[i].mode, steps[i].str));
    }
    return true;
}

/**
 * @brief Advance a deterministic host model pseudo-random seed.
 * @param p_seed Seed updated in place.
 * @return Next pseudo-random value.
 */
static uint32_t host_model_next_seed(uint32_t *const p_seed)
{
    *p_seed = ((*p_seed * 1103515245UL) + 12345UL);
    return *p_seed;
}

/**
 * @brief Generate a deterministic bounded object string for model tests.
 * @param seed Pseudo-random seed used to derive characters.
 * @param p_out Destination string buffer.
 * @param out_size Destination buffer size in bytes.
 */
static bool host_model_make_str(uint32_t seed, char *const p_out, const size_t out_size)
{
    const size_t len = (size_t)((seed % 6UL) + 1UL);

    TEST_ASSERT(out_size > len);
    for (size_t idx = 0U; idx < len; idx++)
    {
        seed = ((seed * 1664525UL) + 1013904223UL);
        p_out[idx] = (char)('a' + (char)((seed >> 24U) % 26UL));
    }
    p_out[len] = '\0';
    return true;
}

/** @brief Verify a larger deterministic mixed scalar/object random model. */
static bool test_nvm_shared_seeded_mixed_random_restarts_match_model(void)
{
    uint32_t seed = 0x13572468UL;

    host_flash_reset_erased();
    for (size_t step = 0U; step < 24U; step++)
    {
        char expected_str[9] = { 0 };
        const uint8_t expected_mode = (uint8_t)((host_model_next_seed(&seed) % 10UL) + 1UL);
        const bool save_all = ((host_model_next_seed(&seed) & 1UL) != 0UL);
        const bool object_first = ((host_model_next_seed(&seed) & 2UL) != 0UL);

        TEST_ASSERT(host_model_make_str(host_model_next_seed(&seed), expected_str, sizeof(expected_str)));
        TEST_ASSERT(init_module());
        TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, expected_mode));
        TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, expected_str));
        if (save_all)
        {
            TEST_ASSERT_OK(par_save_all());
        }
        else if (object_first)
        {
            TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
            TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
        }
        else
        {
            TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
            TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
        }
        TEST_ASSERT_OK(par_deinit());
        TEST_ASSERT(host_flash_verify_mode_and_str_value(expected_mode, expected_str));
    }
    return true;
}

#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (1 == PAR_CFG_ENABLE_TYPE_STR) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED)
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/**
 * @brief Update an object while its validation callback persists current state.
 * @param validation Object validation callback to install.
 * @return true when the child scenario completes.
 */
static bool host_flash_child_object_validation_save_policy(
    const pf_par_obj_validation_t validation)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "old"));
    TEST_ASSERT_OK(par_save_all());
    g_nvm_callback_hits = 0U;
    par_register_obj_validation(ePAR_TEST_STR, validation);
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "new"));
    TEST_ASSERT(g_nvm_callback_hits == 1U);
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/** @brief Verify object validation-time save persists pre-validation state. */
static bool test_nvm_object_validation_save_during_dispatch_current_policy(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_object_validation_save_policy(
            on_nvm_object_validation_save));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_verify_mode_and_str_value(4U, "old"));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/** @brief Verify object validation-time save-all persists pre-validation state. */
static bool test_nvm_object_validation_save_all_during_dispatch_current_policy(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_object_validation_save_policy(
            on_nvm_object_validation_save_all));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_verify_mode_and_str_value(4U, "old"));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/** @brief Verify object validation-time save-clean persists pre-validation state. */
static bool test_nvm_object_validation_save_clean_during_dispatch_current_policy(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_object_validation_save_policy(
            on_nvm_object_validation_save_clean));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_verify_mode_and_str_value(4U, "old"));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (1 == PAR_CFG_ENABLE_TYPE_STR) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) */


#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) */
