static bool init_module(void)
{
    if (par_is_init())
    {
        TEST_ASSERT_OK(par_deinit());
    }

    TEST_ASSERT_OK(par_init());
    return true;
}

/**
 * @brief Run the included shell command dispatcher with an argv vector.
 * @param argc Argument count.
 * @param argv Argument vector.
 */
static void run_nvm_shell(int argc, char **argv)
{
    nvm_shell_capture_reset();
    par_msh(argc, argv);
}

/** @brief Verify first boot on erased flash restores and writes defaults. */
static bool test_nvm_first_boot_formats_and_restores_defaults(void)
{
    uint8_t value = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 1U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify saved scalar values reload as their last committed values. */
static bool test_nvm_save_reload_preserves_last_committed_scalar_values(void)
{
    uint8_t u8 = 0U;
    uint16_t u16 = 0U;
    uint32_t u32 = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, 456U));
    TEST_ASSERT_OK(par_set_u32(ePAR_TEST_U32, 98765UL));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_save(ePAR_TEST_U16));
    TEST_ASSERT_OK(par_save(ePAR_TEST_U32));
    TEST_ASSERT_OK(par_deinit());

    host_flash_clear_failpoints();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT_OK(par_get_u32(ePAR_TEST_U32, &u32));
    TEST_ASSERT(u8 == 4U);
    TEST_ASSERT(u16 == 456U);
    TEST_ASSERT(u32 == 98765UL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object values survive deinit/init cycles. */
static bool test_nvm_object_save_reload_shared_fixed_addr(void)
{
    char str_buf[9] = { 0 };
    uint8_t byte_buf[4] = { 0U };
    uint16_t len = 0U;
    const uint8_t payload[4] = { 9U, 8U, 7U, 6U };

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "nvm"));
    TEST_ASSERT_OK(par_set_bytes(ePAR_TEST_BYTES, payload, (uint16_t)sizeof(payload)));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    TEST_ASSERT_OK(par_save(ePAR_TEST_BYTES));
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 3U);
    TEST_ASSERT(strcmp(str_buf, "nvm") == 0);
    TEST_ASSERT_OK(par_get_bytes(ePAR_TEST_BYTES, byte_buf, sizeof(byte_buf), &len));
    TEST_ASSERT(len == 4U);
    TEST_ASSERT(memcmp(byte_buf, payload, sizeof(payload)) == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Save one committed value, then leave a failed live value unflushed. */
static bool host_flash_child_failed_save_without_deinit(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    g_fail_program_after = 0;
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    TEST_ASSERT((par_save(ePAR_TEST_MODE) & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    return true;
}

/** @brief Verify one value after a process-local backend reset. */
static bool host_flash_child_verify_mode_value(const uint8_t expected_value)
{
    uint8_t value = 0U;

    host_flash_clear_failpoints();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == expected_value);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify failed programming keeps the last committed value after hard reset. */
static bool test_flash_ee_failed_program_reload_preserves_last_committed_value(void)
{
    pid_t child_pid;

    host_flash_reset_erased();

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_failed_save_without_deinit());
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

/** @brief Verify failed programming is retried by graceful deinit and commits the live value. */
static bool test_flash_ee_failed_program_graceful_deinit_commits_live_value(void)
{
    uint8_t value = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    g_fail_program_after = 0;
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    TEST_ASSERT((par_save(ePAR_TEST_MODE) & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    host_flash_clear_failpoints();
    TEST_ASSERT_OK(par_deinit());

    host_flash_clear_failpoints();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 5U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify physical flash corruption rebuilds the default value. */
static bool test_flash_ee_corruption_rebuilds_default_value(void)
{
    uint8_t value = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 6U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_deinit());

    for (uint32_t addr = 0U; addr < 128U; addr += 11U)
    {
        host_flash_xor_byte(addr, 0x5AU);
    }

    host_flash_clear_failpoints();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 1U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object saves cannot corrupt committed scalar records. */
static bool test_nvm_object_updates_do_not_corrupt_scalar_values(void)
{
    uint8_t u8 = 0U;
    uint16_t u16 = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 7U));
    TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, 222U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_save(ePAR_TEST_U16));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "object"));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT(u8 == 7U);
    TEST_ASSERT(u16 == 222U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify scalar saves cannot corrupt committed object payloads. */
static bool test_nvm_scalar_updates_do_not_corrupt_object_values(void)
{
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "stable"));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    for (uint8_t value = 2U; value < 9U; value++)
    {
        TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, value));
        TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    }
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 6U);
    TEST_ASSERT(strcmp(str_buf, "stable") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify erase failpoints reject erase without modifying existing bytes. */
static bool test_flash_ee_failed_erase_preserves_existing_bytes(void)
{
    uint8_t before[HOST_FLASH_ERASE_SIZE];
    uint8_t after[HOST_FLASH_ERASE_SIZE];
    const uint8_t zeros[HOST_FLASH_PROGRAM_SIZE] = { 0U };

    host_flash_reset_erased();
    TEST_ASSERT_OK(par_store_flash_ee_native_port_init());
    TEST_ASSERT_OK(par_store_flash_ee_native_port_program(0U,
                                                          (uint32_t)sizeof(zeros),
                                                          zeros));
    TEST_ASSERT_OK(par_store_flash_ee_native_port_read(0U, (uint32_t)sizeof(before), before));
    g_fail_erase_after = 0;
    TEST_ASSERT_STATUS(par_store_flash_ee_native_port_erase(0U, HOST_FLASH_ERASE_SIZE),
                       ePAR_ERROR);
    host_flash_clear_failpoints();
    TEST_ASSERT_OK(par_store_flash_ee_native_port_read(0U, (uint32_t)sizeof(after), after));
    TEST_ASSERT(0 == memcmp(before, after, sizeof(before)));
    TEST_ASSERT_OK(par_store_flash_ee_native_port_deinit());
    return true;
}

/** @brief Verify repeated scalar updates still reload the last committed value. */
static bool test_flash_ee_many_updates_preserve_last_committed_value(void)
{
    uint8_t value = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    for (uint8_t next = 1U; next <= 10U; next++)
    {
        TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, next));
        TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    }
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 10U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Create an uncommitted Flash-EE tail record after a valid scalar commit. */
static bool host_flash_child_append_uncommitted_tail_after_commit(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT(host_flash_ee_append_uncommitted_tail_record(0U));
    return true;
}

/** @brief Verify an uncommitted Flash-EE tail record is ignored on restart. */
static bool test_flash_ee_record_commit_marker_corruption_ignores_partial_record(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_append_uncommitted_tail_after_commit());
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
