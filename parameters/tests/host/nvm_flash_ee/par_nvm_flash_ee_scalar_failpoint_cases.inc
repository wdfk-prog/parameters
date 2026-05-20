static bool test_nvm_obj_n_save_unchanged_skips_backend_write(void)
{
    uint8_t before[HOST_FLASH_SIZE];
    uint8_t after[HOST_FLASH_SIZE];
    const uint8_t *store_image = NULL;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"stable", 6U));
    store_image = host_object_active_image();
    memcpy(before, store_image, sizeof(before));
    TEST_ASSERT_OK(par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"stable", 6U));
    memcpy(after, store_image, sizeof(after));
    TEST_ASSERT(0 == memcmp(before, after, sizeof(before)));
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 6U);
    TEST_ASSERT(strcmp(str_buf, "stable") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify embedded-NUL string n-save rejects input without mutation. */
static bool test_nvm_str_embedded_nul_n_save_rejected_without_mutation(void)
{
    const uint8_t bad_payload[5] = { 'b', 'a', '\0', 'd', '!' };
    uint8_t before[HOST_FLASH_SIZE];
    uint8_t after[HOST_FLASH_SIZE];
    const uint8_t *store_image = NULL;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"stable", 6U));
    store_image = host_object_active_image();
    memcpy(before, store_image, sizeof(before));
    TEST_ASSERT_STATUS(par_set_obj_n_save(ePAR_TEST_STR,
                                           bad_payload,
                                           (uint16_t)sizeof(bad_payload)),
                       ePAR_ERROR_VALUE);
    memcpy(after, store_image, sizeof(after));
    TEST_ASSERT(0 == memcmp(before, after, sizeof(before)));
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 6U);
    TEST_ASSERT(strcmp(str_buf, "stable") == 0);
    TEST_ASSERT_OK(par_deinit());

    memset(str_buf, 0, sizeof(str_buf));
    len = 0U;
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 6U);
    TEST_ASSERT(strcmp(str_buf, "stable") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Save a committed scalar, then fail a save-all rewrite in a child. */
static bool host_flash_child_fail_save_all_after_commit(const int program_fail_after)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    g_fail_program_after = program_fail_after;
    TEST_ASSERT((par_save_all() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    return true;
}

/** @brief Verify failed bulk rewrites do not publish the partially written live value. */
static bool test_flash_ee_save_all_failpoint_preserves_last_committed_scalar(void)
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
            host_child_exit_from_result(host_flash_child_fail_save_all_after_commit(fail_after_cases[i]));
        }
        TEST_ASSERT(host_child_exit_is_success(child_pid));

        child_pid = fork();
        TEST_ASSERT(child_pid >= 0);
        if (0 == child_pid)
        {
            host_child_exit_from_result(host_flash_child_verify_mode_value(4U));
        }
        TEST_ASSERT(host_child_exit_is_success(child_pid));
    }

    return true;
}



/** @brief Save a committed scalar, then fail save-clean with a dirty live value. */
static bool host_flash_child_fail_save_clean_after_commit(const int erase_fail_after,
                                                          const int program_fail_after)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save_all());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    g_fail_erase_after = erase_fail_after;
    g_fail_program_after = program_fail_after;
    TEST_ASSERT((par_save_clean() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    return true;
}

/**
 * @brief Execute a forked failpoint attempt and verify the old mode survives.
 * @param p_attempt Child callback that applies one failing save operation.
 * @param program_fail_after Program failpoint countdown passed to @p p_attempt.
 * @return true when the failed attempt preserves mode value 4 after restart.
 */
static bool host_flash_verify_mode_failpoint_case(
    bool (* const p_attempt)(const int),
    const int program_fail_after)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(p_attempt(program_fail_after));
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

/**
 * @brief Save a committed scalar, then fail save-clean at a selected program step.
 * @param program_fail_after Program failpoint countdown.
 * @return true when save-clean reports the injected backend error.
 */
static bool host_flash_child_fail_save_clean_program_after_commit(
    const int program_fail_after)
{
    return host_flash_child_fail_save_clean_after_commit(HOST_FLASH_FAIL_DISABLED,
                                                         program_fail_after);
}

/**
 * @brief Verify representative save APIs preserve the last full commit on
 *        early program failures.
 */
static bool test_flash_ee_representative_program_failpoint_sweep_preserves_last_commit(void)
{
    static bool (* const attempts[])(const int) = {
        host_flash_child_fail_save_after_commit,
        host_flash_child_fail_save_all_after_commit,
        host_flash_child_fail_save_clean_program_after_commit,
    };
    static const int program_fail_after_cases[] = { 0, 1 };

    for (size_t attempt = 0U;
         attempt < (sizeof(attempts) / sizeof(attempts[0]));
         attempt++)
    {
        for (size_t fail_idx = 0U;
             fail_idx < (sizeof(program_fail_after_cases) / sizeof(program_fail_after_cases[0]));
             fail_idx++)
        {
            TEST_ASSERT(host_flash_verify_mode_failpoint_case(
                attempts[attempt], program_fail_after_cases[fail_idx]));
        }
    }

    return true;
}

/** @brief Verify failed save-clean rewrites converge to the last full commit. */
static bool test_flash_ee_save_clean_failpoint_matrix_preserves_last_commit(void)
{
    static const struct
    {
        int erase_after;   /**< Erase failpoint countdown. */
        int program_after; /**< Program failpoint countdown. */
    } cases[] = {
        { HOST_FLASH_FAIL_DISABLED, 0 },
        { HOST_FLASH_FAIL_DISABLED, 1 },
    };

    for (size_t i = 0U; i < (sizeof(cases) / sizeof(cases[0])); i++)
    {
        pid_t child_pid;

        host_flash_reset_erased();
        child_pid = fork();
        TEST_ASSERT(child_pid >= 0);
        if (0 == child_pid)
        {
            host_child_exit_from_result(host_flash_child_fail_save_clean_after_commit(
                cases[i].erase_after, cases[i].program_after));
        }
        TEST_ASSERT(host_child_exit_is_success(child_pid));

        child_pid = fork();
        TEST_ASSERT(child_pid >= 0);
        if (0 == child_pid)
        {
            host_child_exit_from_result(host_flash_child_verify_mode_value(4U));
        }
        TEST_ASSERT(host_child_exit_is_success(child_pid));
    }

    return true;
}

/** @brief Fill the active bank, then fail a save-clean checkpoint erase. */
static bool host_flash_child_fail_save_clean_checkpoint_erase_after_commit(void)
{
    uint32_t bank_base = 0U;
    uint32_t next_offset = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));

    for (uint16_t attempt = 0U; attempt < 300U; attempt++)
    {
        TEST_ASSERT(host_flash_ee_find_active_bank(&bank_base, &next_offset));
        if ((next_offset + HOST_FLASH_EE_RECORD_SIZE) > HOST_FLASH_EE_BANK_SIZE)
        {
            break;
        }
        TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, (uint16_t)(attempt % 1000U)));
        TEST_ASSERT_OK(par_save(ePAR_TEST_U16));
    }

    TEST_ASSERT(host_flash_ee_find_active_bank(&bank_base, &next_offset));
    TEST_ASSERT((next_offset + HOST_FLASH_EE_RECORD_SIZE) > HOST_FLASH_EE_BANK_SIZE);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    g_fail_erase_after = 0;
    TEST_ASSERT((par_save_clean() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    return true;
}

/** @brief Verify save-clean checkpoint erase failure keeps the last commit. */
static bool test_flash_ee_save_clean_checkpoint_erase_failure_preserves_last_commit(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_fail_save_clean_checkpoint_erase_after_commit());
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

/** @brief Verify raw fake-flash read failpoints report errors without mutation. */
static bool test_flash_ee_port_read_failpoint_reports_error(void)
{
    uint8_t before[HOST_FLASH_PROGRAM_SIZE];
    uint8_t after[HOST_FLASH_PROGRAM_SIZE];
    uint8_t untouched[HOST_FLASH_PROGRAM_SIZE];
    const uint8_t payload[HOST_FLASH_PROGRAM_SIZE] = { 1U, 2U, 3U, 4U };

    host_flash_reset_erased();
    TEST_ASSERT_OK(par_store_flash_ee_native_port_init());
    TEST_ASSERT_OK(par_store_flash_ee_native_port_program(0U,
                                                          (uint32_t)sizeof(payload),
                                                          payload));
    TEST_ASSERT_OK(par_store_flash_ee_native_port_read(0U, (uint32_t)sizeof(before), before));
    memset(after, 0xA5, sizeof(after));
    memcpy(untouched, after, sizeof(untouched));
    g_fail_read_after = 0;
    TEST_ASSERT_STATUS(par_store_flash_ee_native_port_read(0U,
                                                           (uint32_t)sizeof(after),
                                                           after),
                       ePAR_ERROR);
    TEST_ASSERT(0 == memcmp(untouched, after, sizeof(after)));
    host_flash_clear_failpoints();
    TEST_ASSERT_OK(par_store_flash_ee_native_port_read(0U, (uint32_t)sizeof(after), after));
    TEST_ASSERT(0 == memcmp(before, after, sizeof(before)));
    TEST_ASSERT_OK(par_store_flash_ee_native_port_deinit());
    return true;
}

/** @brief Verify invalid Flash-EE geometry is rejected and later retry recovers. */
static bool test_flash_ee_init_rejects_invalid_geometry_and_recovers(void)
{
    const par_store_backend_api_t * const p_store = par_store_backend_flash_ee_get_api();
    host_flash_ee_geom_ctx_t geom;

    memset(&geom, 0, sizeof(geom));
    memset(geom.flash, 0xFF, sizeof(geom.flash));
    geom.region_size = HOST_FLASH_SIZE - 1U;
    geom.erase_size = HOST_FLASH_ERASE_SIZE;
    geom.program_size = HOST_FLASH_PROGRAM_SIZE;
    TEST_ASSERT_OK(par_store_backend_flash_ee_bind_port(&g_host_flash_ee_geom_port_api, &geom));
    TEST_ASSERT_STATUS(p_store->init(), ePAR_ERROR_INIT);
    TEST_ASSERT(par_store_backend_flash_ee_get_diag() == ePAR_STORE_FLASH_EE_DIAG_PORT_GEOMETRY);

    memset(geom.flash, 0xFF, sizeof(geom.flash));
    geom.region_size = HOST_FLASH_SIZE;
    geom.erase_size = HOST_FLASH_ERASE_SIZE;
    geom.program_size = HOST_FLASH_PROGRAM_SIZE;
    geom.is_init = false;
    TEST_ASSERT_OK(par_store_backend_flash_ee_bind_port(&g_host_flash_ee_geom_port_api, &geom));
    TEST_ASSERT_OK(p_store->init());
    TEST_ASSERT(par_store_backend_flash_ee_get_diag() == ePAR_STORE_FLASH_EE_DIAG_NONE);
    TEST_ASSERT_OK(p_store->deinit());
    TEST_ASSERT_OK(par_store_backend_bind());
    return true;
}

/** @brief Verify NVM wrapper APIs reject wrong type and before-init usage. */
static bool test_nvm_wrapper_negative_type_and_init_policies(void)
{
    uint16_t scalar_value = 123U;
    const uint8_t payload[3] = { 'b', 'a', 'd' };

    host_flash_reset_erased();
    if (par_is_init())
    {
        TEST_ASSERT_OK(par_deinit());
    }
    TEST_ASSERT_STATUS(par_save(ePAR_TEST_MODE), ePAR_ERROR_INIT);
    TEST_ASSERT_STATUS(par_save_all(), ePAR_ERROR_INIT);
    TEST_ASSERT_STATUS(par_save_clean(), ePAR_ERROR_INIT);

    TEST_ASSERT(init_module());
    TEST_ASSERT_STATUS(par_set_scalar_n_save(ePAR_TEST_STR, &scalar_value), ePAR_ERROR_TYPE);
    TEST_ASSERT_STATUS(par_set_obj_n_save(ePAR_TEST_MODE, payload, (uint16_t)sizeof(payload)), ePAR_ERROR_TYPE);
    TEST_ASSERT_STATUS(par_set_obj_n_save(ePAR_TEST_ARR_U16, payload, 3U), ePAR_ERROR_VALUE);
    TEST_ASSERT_STATUS(par_set_obj_n_save(ePAR_TEST_ARR_U32, payload, 6U), ePAR_ERROR_VALUE);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify saving a non-persistent row reports unsupported policy without image mutation. */
static bool test_nvm_save_non_persistent_is_noop_and_preserves_image(void)
{
    uint8_t before[HOST_FLASH_SIZE];
    uint8_t after[HOST_FLASH_SIZE];

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_i8(ePAR_TEST_I8, -5));
    memcpy(before, g_flash, sizeof(before));
    TEST_ASSERT_STATUS(par_save(ePAR_TEST_I8), ePAR_ERROR);
    memcpy(after, g_flash, sizeof(after));
    TEST_ASSERT(0 == memcmp(before, after, sizeof(before)));
    TEST_ASSERT_OK(par_deinit());
    return true;
}
