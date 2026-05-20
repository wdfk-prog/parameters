/**
 * @brief Return the active object NVM image used by the current profile.
 * @return Pointer to the fake flash image that persists object writes.
 */
static const uint8_t *host_object_active_image(void)
{
#if (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED)
    return g_object_flash;
#else
    return g_flash;
#endif /* (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED) */
}

#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
/** @brief Verify object NVM block size stays within the active profile region. */
static bool test_nvm_object_region_profile_bounds(void)
{
    const uint32_t block_size = par_nvm_object_get_block_size();

    TEST_ASSERT(block_size > 0U);
#if (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) &&     (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_FIXED)
    TEST_ASSERT(block_size <= PAR_CFG_NVM_OBJECT_REGION_SIZE);
#endif /* (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) && (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_FIXED) */
    return true;
}
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

#if defined(PAR_HOST_TEST_OBJECT_ONLY) || \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED)
/** @brief Reinitialize the module and verify an object string value. */
static bool host_verify_reloaded_object_str_value(const char * const expected)
{
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    TEST_ASSERT(NULL != expected);
    if (par_is_init())
    {
        TEST_ASSERT_OK(par_deinit());
    }
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == (uint16_t)strlen(expected));
    TEST_ASSERT(0 == strcmp(str_buf, expected));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

#endif /* defined(PAR_HOST_TEST_OBJECT_ONLY) || (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED) */

#if defined(PAR_HOST_TEST_OBJECT_ONLY)
/** @brief Verify object-only persistence saves and reloads object rows. */
static bool test_nvm_object_only_save_reload_preserves_object(void)
{
    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"obj", 3U));
    TEST_ASSERT_OK(par_deinit());
    TEST_ASSERT(host_verify_reloaded_object_str_value("obj"));
    return true;
}
#endif /* defined(PAR_HOST_TEST_OBJECT_ONLY) */

#if defined(PAR_HOST_TEST_FIXED_OBJECT_INVALID)
/** @brief Verify invalid fixed object placement rejects NVM initialization. */
static bool test_nvm_fixed_object_invalid_rejects_init(void)
{
    par_status_t status;

    host_flash_reset_erased();
    status = par_init();
    TEST_ASSERT((status & ePAR_ERROR_NVM) != 0U);
    if (par_is_init())
    {
        (void)par_deinit();
    }
    return true;
}
#endif /* defined(PAR_HOST_TEST_FIXED_OBJECT_INVALID) */

#if (1 == PAR_CFG_NVM_OBJECT_WRITE_VERIFY_EN) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED)
/**
 * @brief Save one object, then corrupt the physical writeback candidate.
 * @details Dedicated object writes update the backend before write-verify
 * reports the mismatch. The failed save therefore leaves a corrupt persisted
 * record, and reload must reject it and restore the table default
 * instead of the previously saved value.
 */
static bool host_flash_child_write_verify_corrupt_object_save(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"old", 3U));
#if (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED)
    g_object_corrupt_write_after = 0;
#else
    g_corrupt_program_after = 0;
#endif /* (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED) */
    TEST_ASSERT((par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"new", 3U) & ePAR_ERROR_NVM) != 0U);
    host_flash_clear_failpoints();
    TEST_ASSERT(host_verify_reloaded_object_str_value("ap"));
    return true;
}

/** @brief Verify object write verification rejects physical readback mismatch. */
static bool test_nvm_object_write_verify_detects_payload_mismatch(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_write_verify_corrupt_object_save());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}


/** @brief Save one object while the verify-read path reports a backend error. */
static bool host_flash_child_write_verify_object_read_error_save(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"old", 3U));
    g_object_fail_read_after = 0;
    TEST_ASSERT((par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"new", 3U) & ePAR_ERROR_NVM) != 0U);
    host_flash_clear_failpoints();
    TEST_ASSERT(host_verify_reloaded_object_str_value("new"));
    return true;
}

/** @brief Verify object write verification reports readback transport errors. */
static bool test_nvm_object_write_verify_read_error_is_reported(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_write_verify_object_read_error_save());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}
#endif /* (1 == PAR_CFG_NVM_OBJECT_WRITE_VERIFY_EN) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED) */


#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED)
/**
 * @brief Verify a reloaded scalar/object pair matches expected committed values.
 * @param expected_mode Expected scalar mode value.
 * @param expected_str Expected string object value.
 * @return true when the reloaded values match.
 */
static bool host_flash_verify_mode_and_str_value(const uint8_t expected_mode,
                                                 const char * const expected_str);

/** @brief Save mixed dirty scalar/object values and fail a selected backend program. */
static bool host_flash_child_shared_save_all_fail_mixed_values(const int program_fail_after)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "old"));
    TEST_ASSERT_OK(par_save_all());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "new"));
    g_fail_program_after = program_fail_after;
    TEST_ASSERT((par_save_all() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    return true;
}

/** @brief Verify failed shared save-all does not load corrupt mixed values. */
static bool test_nvm_shared_save_all_failpoint_preserves_last_committed_values(void)
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
                host_flash_child_shared_save_all_fail_mixed_values(fail_after_cases[i]));
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
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) */
