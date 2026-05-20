static bool test_flash_ee_port_rejects_wrapped_ranges(void)
{
    uint8_t readback[4] = { 0U };
    const uint8_t payload[4] = { 1U, 2U, 3U, 4U };

    host_flash_reset_erased();
    TEST_ASSERT_OK(par_store_flash_ee_native_port_init());
    TEST_ASSERT_STATUS(par_store_flash_ee_native_port_read(UINT32_MAX - 1U,
                                                           (uint32_t)sizeof(readback),
                                                           readback),
                       ePAR_ERROR);
    TEST_ASSERT_STATUS(par_store_flash_ee_native_port_program(UINT32_MAX - 1U,
                                                              (uint32_t)sizeof(payload),
                                                              payload),
                       ePAR_ERROR);
    TEST_ASSERT_STATUS(par_store_flash_ee_native_port_erase(UINT32_MAX - 63U, 128U), ePAR_ERROR);
    TEST_ASSERT_OK(par_store_flash_ee_native_port_deinit());
    return true;
}

/** @brief Verify the fake flash port rejects invalid 0-to-1 programming. */
static bool test_flash_ee_program_one_to_zero_semantics(void)
{
    uint8_t readback[HOST_FLASH_PROGRAM_SIZE] = { 0U };
    uint8_t ones[HOST_FLASH_PROGRAM_SIZE];
    const uint8_t zeros[HOST_FLASH_PROGRAM_SIZE] = { 0U };

    memset(ones, 0xFF, sizeof(ones));
    host_flash_reset_erased();
    TEST_ASSERT_OK(par_store_flash_ee_native_port_init());
    TEST_ASSERT_OK(par_store_flash_ee_native_port_program(0U,
                                                          (uint32_t)sizeof(zeros),
                                                          zeros));
    TEST_ASSERT_STATUS(par_store_flash_ee_native_port_program(0U,
                                                              (uint32_t)sizeof(ones),
                                                              ones),
                       ePAR_ERROR);
    TEST_ASSERT_OK(par_store_flash_ee_native_port_read(0U,
                                                       (uint32_t)sizeof(readback),
                                                       readback));
    TEST_ASSERT(0 == memcmp(readback, zeros, sizeof(readback)));
    TEST_ASSERT_OK(par_store_flash_ee_native_port_deinit());
    return true;
}


/** @brief Verify a partial logical write preserves neighbor bytes in the line. */
static bool test_flash_ee_partial_line_write_preserves_neighbor_bytes(void)
{
    const par_store_backend_api_t * const p_store = par_store_backend_flash_ee_get_api();
    uint8_t initial[HOST_FLASH_EE_LINE_SIZE];
    uint8_t patch[2] = { 0xA5U, 0x5AU };
    uint8_t readback[HOST_FLASH_EE_LINE_SIZE];

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    for (uint32_t idx = 0U; idx < (uint32_t)sizeof(initial); idx++)
    {
        initial[idx] = (uint8_t)(idx + 1U);
    }
    TEST_ASSERT_OK(p_store->write(0U, (uint32_t)sizeof(initial), initial));
    TEST_ASSERT_OK(p_store->write(3U, (uint32_t)sizeof(patch), patch));
    TEST_ASSERT_OK(p_store->read(0U, (uint32_t)sizeof(readback), readback));
    initial[3] = patch[0];
    initial[4] = patch[1];
    TEST_ASSERT(0 == memcmp(readback, initial, sizeof(readback)));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify a logical write that crosses a cache window survives reload. */
static bool test_flash_ee_cross_cache_window_write_reload_roundtrip(void)
{
    const par_store_backend_api_t * const p_store = par_store_backend_flash_ee_get_api();
    const uint32_t addr = PAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE - 2U;
    const uint8_t payload[4] = { 0x11U, 0x22U, 0x33U, 0x44U };
    uint8_t readback[4] = { 0U };

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(p_store->write(addr, (uint32_t)sizeof(payload), payload));
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(p_store->read(addr, (uint32_t)sizeof(readback), readback));
    TEST_ASSERT(0 == memcmp(readback, payload, sizeof(readback)));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify a partial logical erase preserves bytes outside the range. */
static bool test_flash_ee_erase_partial_line_preserves_outside_range(void)
{
    const par_store_backend_api_t * const p_store = par_store_backend_flash_ee_get_api();
    uint8_t initial[HOST_FLASH_EE_LINE_SIZE];
    uint8_t expected[HOST_FLASH_EE_LINE_SIZE];
    uint8_t readback[HOST_FLASH_EE_LINE_SIZE];

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    for (uint32_t idx = 0U; idx < (uint32_t)sizeof(initial); idx++)
    {
        initial[idx] = (uint8_t)(0x80U + idx);
    }
    memcpy(expected, initial, sizeof(expected));
    expected[2] = 0xFFU;
    expected[3] = 0xFFU;
    expected[4] = 0xFFU;
    TEST_ASSERT_OK(p_store->write(0U, (uint32_t)sizeof(initial), initial));
    TEST_ASSERT_OK(p_store->erase(2U, 3U));
    TEST_ASSERT_OK(p_store->read(0U, (uint32_t)sizeof(readback), readback));
    TEST_ASSERT(0 == memcmp(readback, expected, sizeof(readback)));
    TEST_ASSERT_OK(par_deinit());
    return true;
}


/** @brief Verify raw fake-flash failpoint countdowns across read, program, and erase. */
static bool test_flash_ee_port_failpoint_countdown_matrix(void)
{
    const uint8_t payload[HOST_FLASH_PROGRAM_SIZE] = { 0x11U, 0x22U, 0x33U, 0x44U };
    const uint8_t second_payload[HOST_FLASH_PROGRAM_SIZE] = { 0x01U, 0x02U, 0x03U, 0x04U };
    uint8_t readback[HOST_FLASH_PROGRAM_SIZE] = { 0U };

    host_flash_reset_erased();
    TEST_ASSERT_OK(par_store_flash_ee_native_port_init());
    g_fail_program_after = 1;
    TEST_ASSERT_OK(par_store_flash_ee_native_port_program(0U,
                                                          (uint32_t)sizeof(payload),
                                                          payload));
    TEST_ASSERT_STATUS(par_store_flash_ee_native_port_program(HOST_FLASH_PROGRAM_SIZE,
                                                              (uint32_t)sizeof(second_payload),
                                                              second_payload),
                       ePAR_ERROR);
    host_flash_clear_failpoints();
    TEST_ASSERT_OK(par_store_flash_ee_native_port_read(0U,
                                                       (uint32_t)sizeof(readback),
                                                       readback));
    TEST_ASSERT(0 == memcmp(readback, payload, sizeof(readback)));

    g_fail_read_after = 1;
    TEST_ASSERT_OK(par_store_flash_ee_native_port_read(0U,
                                                       (uint32_t)sizeof(readback),
                                                       readback));
    TEST_ASSERT_STATUS(par_store_flash_ee_native_port_read(0U,
                                                           (uint32_t)sizeof(readback),
                                                           readback),
                       ePAR_ERROR);
    host_flash_clear_failpoints();

    g_fail_erase_after = 1;
    TEST_ASSERT_OK(par_store_flash_ee_native_port_erase(0U, HOST_FLASH_ERASE_SIZE));
    TEST_ASSERT_STATUS(par_store_flash_ee_native_port_erase(HOST_FLASH_ERASE_SIZE,
                                                            HOST_FLASH_ERASE_SIZE),
                       ePAR_ERROR);
    host_flash_clear_failpoints();
    TEST_ASSERT_OK(par_store_flash_ee_native_port_deinit());
    return true;
}

/** @brief Save a committed value, then fail a later save without graceful deinit. */
static bool host_flash_child_fail_save_after_commit(const int program_fail_after)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    g_fail_program_after = program_fail_after;
    TEST_ASSERT((par_save(ePAR_TEST_MODE) & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    return true;
}

/** @brief Verify early and mid-write program failpoints preserve last committed values. */
static bool test_flash_ee_program_failpoint_matrix_preserves_last_commit(void)
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
            host_child_exit_from_result(host_flash_child_fail_save_after_commit(fail_after_cases[i]));
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


/** @brief Verify repeated failed saves converge to the last complete commit. */
static bool test_flash_ee_repeated_failed_saves_converge_to_last_commit(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_fail_save_after_commit(0));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_verify_mode_value(4U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 6U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_deinit());

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        TEST_ASSERT(init_module());
        TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 7U));
        g_fail_program_after = 1;
        TEST_ASSERT((par_save(ePAR_TEST_MODE) & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
        host_child_exit_from_result(true);
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_verify_mode_value(6U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}


/** @brief Verify deterministic scalar save/restart operations match a small model. */
static bool test_flash_ee_seeded_scalar_restarts_match_model(void)
{
    static const uint8_t values[] = { 2U, 7U, 3U, 10U, 1U, 8U };
    uint8_t value = 0U;

    host_flash_reset_erased();
    for (size_t i = 0U; i < (sizeof(values) / sizeof(values[0])); i++)
    {
        TEST_ASSERT(init_module());
        TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, values[i]));
        if (0U == (i % 2U))
        {
            TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
        }
        else
        {
            TEST_ASSERT_OK(par_save_all());
        }
        TEST_ASSERT_OK(par_deinit());

        TEST_ASSERT(init_module());
        TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
        TEST_ASSERT(value == values[i]);
        TEST_ASSERT_OK(par_deinit());
    }
    return true;
}

#if (1 == PAR_CFG_NVM_WRITE_VERIFY_EN)
/** @brief Save a committed scalar, then corrupt the physical verify-readback candidate. */
static bool host_flash_child_write_verify_corrupt_scalar_save(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    g_corrupt_program_after = 0;
    TEST_ASSERT((par_save(ePAR_TEST_MODE) & ePAR_ERROR_NVM) != 0U);
    return true;
}

/** @brief Verify scalar write verification rejects physical readback mismatch. */
static bool test_nvm_scalar_write_verify_detects_readback_mismatch(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_write_verify_corrupt_scalar_save());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}


/** @brief Save a scalar while the verify-read path reports a backend error. */
static bool host_flash_child_write_verify_read_error_scalar_save(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    g_fail_read_after = 0;
    TEST_ASSERT((par_save(ePAR_TEST_MODE) & ePAR_ERROR_NVM) != 0U);
    return true;
}

/** @brief Verify scalar write verification reports readback transport errors but persists the completed write. */
static bool test_nvm_scalar_write_verify_read_error_is_reported(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_write_verify_read_error_scalar_save());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_verify_mode_value(5U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}
#endif /* (1 == PAR_CFG_NVM_WRITE_VERIFY_EN) */
