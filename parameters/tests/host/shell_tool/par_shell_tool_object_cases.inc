/** @brief Verify raw byte object get prints a stable hex payload. */
static bool test_msh_get_object_bytes_hex_output(void)
{
    char *args[] = { "par", "get", "10" };
    const uint8_t payload[4] = { 0xAAU, 0x55U, 0x00U, 0xFFU };

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_bytes(ePAR_TEST_BYTES, payload, (uint16_t)sizeof(payload)));
    run_shell(3, args);
    TEST_ASSERT(shell_capture_contains("hex:"));
    TEST_ASSERT(shell_capture_contains("AA"));
    TEST_ASSERT(shell_capture_contains("55"));
    TEST_ASSERT(shell_capture_contains("00"));
    TEST_ASSERT(shell_capture_contains("FF"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}


/** @brief Verify object set attempts are rejected and do not mutate payloads. */
static bool test_msh_set_object_row_is_rejected_without_mutation(void)
{
    char *set_args[] = { "par", "set", "9", "mutate" };
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "stable"));
    run_shell(4, set_args);
    TEST_ASSERT(shell_capture_contains("ERR"));
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 6U);
    TEST_ASSERT(strcmp(str_buf, "stable") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}


/** @brief Verify compact object set syntax is rejected without mutation. */
static bool test_msh_set_object_compact_syntax_rejected_without_mutation(void)
{
    char *set_args[] = { "par", "set", "9,mutate" };
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "stable"));
    run_shell(3, set_args);
    TEST_ASSERT(shell_capture_contains("ERR"));
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 6U);
    TEST_ASSERT(strcmp(str_buf, "stable") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify shell role gating applies to object reads. */
static bool test_msh_role_commands_gate_object_access(void)
{
    char *role_clear_args[] = { "par", "role", "clear" };
    char *role_set_public_args[] = { "par", "role", "set", "public" };
    char *get_args[] = { "par", "get", "9" };

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "role"));
    run_shell(3, role_clear_args);
    TEST_ASSERT(shell_capture_contains("shell_roles=none"));
    run_shell(3, get_args);
    TEST_ASSERT(shell_capture_contains("ERR"));
    run_shell(4, role_set_public_args);
    TEST_ASSERT(shell_capture_contains("shell_roles=public"));
    run_shell(3, get_args);
    TEST_ASSERT(shell_capture_contains("role"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object array get commands emit stable decimal elements. */
static bool test_msh_get_object_array_outputs_are_stable(void)
{
    char *get_arr8_args[] = { "par", "get", "11" };
    char *get_arr16_args[] = { "par", "get", "12" };
    char *get_arr32_args[] = { "par", "get", "13" };
    const uint8_t arr8[3] = { 9U, 8U, 7U };
    const uint16_t arr16[2] = { 300U, 400U };
    const uint32_t arr32[2] = { 3000UL, 4000UL };

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_arr_u8(ePAR_TEST_ARR_U8, arr8, 3U));
    run_shell(3, get_arr8_args);
    TEST_ASSERT(shell_capture_equals("OK,PAR_GET=[9,8,7]\n"));
    TEST_ASSERT_OK(par_set_arr_u16(ePAR_TEST_ARR_U16, arr16, 2U));
    run_shell(3, get_arr16_args);
    TEST_ASSERT(shell_capture_equals("OK,PAR_GET=[300,400]\n"));
    TEST_ASSERT_OK(par_set_arr_u32(ePAR_TEST_ARR_U32, arr32, 2U));
    run_shell(3, get_arr32_args);
    TEST_ASSERT(shell_capture_equals("OK,PAR_GET=[3000,4000]\n"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object get reports heap allocation failure without crashing. */
static bool test_msh_get_object_alloc_failure_reports_error(void)
{
    char *args[] = { "par", "get", "9" };

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "heap"));
    shell_capture_reset();
    g_shell_malloc_fail_once = true;
    par_msh(3, args);
    TEST_ASSERT(shell_capture_contains("ERR"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}
