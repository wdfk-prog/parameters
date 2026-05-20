/** @brief Verify help and unknown command output paths. */
static bool test_msh_help_and_unknown_command(void)
{
    char *help_args[] = { "par" };
    char *bad_args[] = { "par", "unknown" };

    TEST_ASSERT(init_module());
    run_shell(1, help_args);
    TEST_ASSERT(shell_capture_contains("Usage:"));
    run_shell(2, bad_args);
    TEST_ASSERT(shell_capture_contains("ERR, unknown subcmd"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify info output contains table metadata. */
static bool test_msh_info_prints_table_summary(void)
{
    char *args[] = { "par", "info" };

    TEST_ASSERT(init_module());
    run_shell(2, args);
    TEST_ASSERT(shell_capture_contains("ID"));
    TEST_ASSERT(shell_capture_contains("Mode"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify scalar get/set by ID. */
static bool test_msh_get_set_scalar_by_id(void)
{
    char *set_args[] = { "par", "set", "1", "8" };
    char *get_args[] = { "par", "get", "1" };
    uint8_t value = 0U;

    TEST_ASSERT(init_module());
    run_shell(4, set_args);
    TEST_ASSERT(shell_capture_contains("OK"));
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 8U);
    run_shell(3, get_args);
    TEST_ASSERT(strcmp(g_shell_capture, "OK,PAR_GET=8\n") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify invalid ID and out-of-range value diagnostics. */
static bool test_msh_rejects_invalid_id_and_range(void)
{
    char *bad_id_args[] = { "par", "get", "65535" };
    char *bad_range_args[] = { "par", "set", "1", "99" };

    TEST_ASSERT(init_module());
    run_shell(3, bad_id_args);
    TEST_ASSERT(shell_capture_contains("ERR"));
    run_shell(4, bad_range_args);
    TEST_ASSERT(shell_capture_contains("WAR"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object get output when RT_USING_HEAP enables the path. */
static bool test_msh_get_object_str_output(void)
{
    char *args[] = { "par", "get", "9" };

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "shell"));
    run_shell(3, args);
    TEST_ASSERT(shell_capture_contains("shell"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify JSON command emits valid high-level fields. */
static bool test_msh_json_prints_escaped_table(void)
{
    char *args[] = { "par", "json" };

    TEST_ASSERT(init_module());
    run_shell(2, args);
    TEST_ASSERT(shell_capture_contains("{\"count\""));
    TEST_ASSERT(shell_capture_contains("\"name\":\"Mode\""));
    TEST_ASSERT(shell_capture_contains("\"type\":\"u8\""));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify shell role commands gate scalar read/write access. */
static bool test_msh_role_commands_gate_scalar_access(void)
{
    char *role_query_args[] = { "par", "role" };
    char *role_clear_args[] = { "par", "role", "clear" };
    char *role_set_public_args[] = { "par", "role", "set", "public" };
    char *role_add_service_args[] = { "par", "role", "add", "service" };
    char *role_del_public_args[] = { "par", "role", "del", "public" };
    char *get_args[] = { "par", "get", "1" };
    char *set_args[] = { "par", "set", "1", "4" };
    uint8_t value = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_to_default(ePAR_TEST_MODE));
    run_shell(2, role_query_args);
    TEST_ASSERT(shell_capture_contains("shell_roles=public"));
    run_shell(3, role_clear_args);
    TEST_ASSERT(shell_capture_contains("shell_roles=none"));
    run_shell(3, get_args);
    TEST_ASSERT(shell_capture_contains("ERR"));
    run_shell(4, set_args);
    TEST_ASSERT(shell_capture_contains("not writable"));
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 1U);
    run_shell(4, role_set_public_args);
    TEST_ASSERT(shell_capture_contains("shell_roles=public"));
    run_shell(4, set_args);
    TEST_ASSERT(shell_capture_contains("OK,PAR_SET=4"));
    run_shell(4, role_add_service_args);
    TEST_ASSERT(shell_capture_contains("shell_roles=public|service"));
    run_shell(4, role_del_public_args);
    TEST_ASSERT(shell_capture_contains("shell_roles=service"));
    run_shell(4, role_set_public_args);
    TEST_ASSERT(shell_capture_contains("shell_roles=public"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify default restore command works by ID. */
static bool test_msh_def_restores_default(void)
{
    char *args[] = { "par", "def", "1" };
    uint8_t value = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 6U));
    run_shell(3, args);
    TEST_ASSERT(shell_capture_contains("OK"));
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 1U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify compact set syntax and malformed values do not corrupt state. */
static bool test_msh_set_compact_and_malformed_value(void)
{
    char *compact_args[] = { "par", "set", "1,9" };
    char *bad_value_args[] = { "par", "set", "1", "abc" };
    uint8_t value = 0U;

    TEST_ASSERT(init_module());
    run_shell(3, compact_args);
    TEST_ASSERT(shell_capture_contains("OK"));
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 9U);
    run_shell(4, bad_value_args);
    TEST_ASSERT(shell_capture_contains("ERR"));
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 9U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify def_all restores multiple changed scalar and object values. */
static bool test_msh_def_all_restores_defaults(void)
{
    char *args[] = { "par", "def_all" };
    uint8_t value = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 6U));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "changed"));
    run_shell(2, args);
    TEST_ASSERT(shell_capture_contains("OK"));
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 1U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}
