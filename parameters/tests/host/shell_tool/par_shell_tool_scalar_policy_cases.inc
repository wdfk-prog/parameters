/** @brief Verify shell set accepts all scalar widths and base-prefixed numbers. */
static bool test_msh_set_all_scalar_widths_and_float(void)
{
    char *set_i8_args[] = { "par", "set", "2", "-5" };
    char *set_u16_args[] = { "par", "set", "3", "0x123" };
    char *set_u32_args[] = { "par", "set", "5", "12345" };
    char *set_f32_args[] = { "par", "set", "7", "2.5" };
    int8_t i8_value = 0;
    uint16_t u16_value = 0U;
    uint32_t u32_value = 0U;
    float32_t f32_value = 0.0f;

    TEST_ASSERT(init_module());
    run_shell(4, set_i8_args);
    TEST_ASSERT(shell_capture_contains("OK"));
    TEST_ASSERT_OK(par_get_i8(ePAR_TEST_I8, &i8_value));
    TEST_ASSERT(i8_value == -5);
    run_shell(4, set_u16_args);
    TEST_ASSERT(shell_capture_contains("OK"));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16_value));
    TEST_ASSERT(u16_value == 0x123U);
    run_shell(4, set_u32_args);
    TEST_ASSERT(shell_capture_contains("OK"));
    TEST_ASSERT_OK(par_get_u32(ePAR_TEST_U32, &u32_value));
    TEST_ASSERT(u32_value == 12345UL);
    run_shell(4, set_f32_args);
    TEST_ASSERT(shell_capture_contains("OK"));
    TEST_ASSERT_OK(par_get_f32(ePAR_TEST_F32, &f32_value));
    TEST_ASSERT((f32_value > 2.49f) && (f32_value < 2.51f));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify numeric parser failures leave scalar state unchanged. */
static bool test_msh_set_rejects_overflow_and_negative_unsigned(void)
{
    char *set_u8_args[] = { "par", "set", "1", "4" };
    char *u8_overflow_args[] = { "par", "set", "1", "999" };
    char *u8_negative_args[] = { "par", "set", "1", "-1" };
    char *u16_overflow_args[] = { "par", "set", "3", "70000" };
    uint8_t mode = 0U;
    uint16_t u16_value = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, 100U));
    run_shell(4, set_u8_args);
    TEST_ASSERT(shell_capture_contains("OK"));
    run_shell(4, u8_overflow_args);
    TEST_ASSERT(shell_capture_contains("ERR"));
    run_shell(4, u8_negative_args);
    TEST_ASSERT(shell_capture_contains("ERR"));
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &mode));
    TEST_ASSERT(mode == 4U);
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16_value));
    TEST_ASSERT(u16_value == 100U);
    run_shell(4, u16_overflow_args);
    TEST_ASSERT(shell_capture_contains("ERR"));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16_value));
    TEST_ASSERT(u16_value == 100U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify whitespace-tolerant compact syntax and empty values. */
static bool test_msh_parser_whitespace_and_empty_token_matrix(void)
{
    char *compact_space_args[] = { "par", "set", "1, 8" };
    char *empty_pair_args[] = { "par", "set", "1," };
    char *empty_value_args[] = { "par", "set", "1", "" };
    uint8_t mode = 0U;

    TEST_ASSERT(init_module());
    run_shell(3, compact_space_args);
    TEST_ASSERT(shell_capture_contains("OK"));
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &mode));
    TEST_ASSERT(mode == 8U);
    run_shell(3, empty_pair_args);
    TEST_ASSERT(shell_capture_contains("ERR"));
    run_shell(4, empty_value_args);
    TEST_ASSERT(shell_capture_contains("ERR"));
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &mode));
    TEST_ASSERT(mode == 8U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/** @brief Verify current role-token policy preserves the same mask. */
static bool test_msh_role_combination_and_duplicate_token_current_policy(void)
{
    char *role_set_combo_args[] = { "par", "role", "set", "public|service" };
    char *role_add_duplicate_args[] = { "par", "role", "add", "service" };
    char *role_query_args[] = { "par", "role" };

    TEST_ASSERT(init_module());
    run_shell(4, role_set_combo_args);
    TEST_ASSERT(shell_capture_contains("shell_roles=public|service"));
    run_shell(4, role_add_duplicate_args);
    TEST_ASSERT(shell_capture_contains("shell_roles=public|service"));
    run_shell(2, role_query_args);
    TEST_ASSERT(shell_capture_contains("shell_roles=public|service"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

/** @brief Verify JSON escaping covers quotes, backslashes, and control chars. */
static bool test_msh_json_escapes_control_chars(void)
{
    shell_capture_reset();
    par_shell_json_print_escaped("a\\\"b\n\t\x01");
    TEST_ASSERT(shell_capture_contains("a\\\\\\\"b\\n\\t\\u0001"));
    return true;
}

/** @brief Verify scalar numeric trailing garbage is rejected without mutation. */
static bool test_msh_set_rejects_numeric_trailing_garbage_without_mutation(void)
{
    char *set_u8_args[] = { "par", "set", "1", "4" };
    char *u8_garbage_args[] = { "par", "set", "1", "4abc" };
    char *u16_bad_hex_args[] = { "par", "set", "3", "0x" };
    char *i8_double_sign_args[] = { "par", "set", "2", "--1" };
    uint8_t mode = 0U;
    int8_t i8_value = 0;
    uint16_t u16_value = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_i8(ePAR_TEST_I8, -1));
    TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, 100U));
    run_shell(4, set_u8_args);
    TEST_ASSERT(shell_capture_contains("OK"));
    run_shell(4, u8_garbage_args);
    TEST_ASSERT(shell_capture_contains("ERR"));
    run_shell(4, u16_bad_hex_args);
    TEST_ASSERT(shell_capture_contains("ERR"));
    run_shell(4, i8_double_sign_args);
    TEST_ASSERT(shell_capture_contains("ERR"));
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &mode));
    TEST_ASSERT(mode == 4U);
    TEST_ASSERT_OK(par_get_i8(ePAR_TEST_I8, &i8_value));
    TEST_ASSERT(i8_value == -1);
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16_value));
    TEST_ASSERT(u16_value == 100U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify non-finite float tokens are rejected without mutation. */
static bool test_msh_set_rejects_nonfinite_float_without_mutation(void)
{
    char *set_f32_args[] = { "par", "set", "7", "2.5" };
    char *nan_args[] = { "par", "set", "7", "nan" };
    char *inf_args[] = { "par", "set", "7", "inf" };
    char *suffix_args[] = { "par", "set", "7", "1.0fxx" };
    float32_t f32_value = 0.0f;

    TEST_ASSERT(init_module());
    run_shell(4, set_f32_args);
    TEST_ASSERT(shell_capture_contains("OK"));
    run_shell(4, nan_args);
    TEST_ASSERT(shell_capture_contains("ERR"));
    run_shell(4, inf_args);
    TEST_ASSERT(shell_capture_contains("ERR"));
    run_shell(4, suffix_args);
    TEST_ASSERT(shell_capture_contains("ERR"));
    TEST_ASSERT_OK(par_get_f32(ePAR_TEST_F32, &f32_value));
    TEST_ASSERT((f32_value > 2.49f) && (f32_value < 2.51f));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify empty and mixed role separators reject invalid role tokens. */
static bool test_msh_role_rejects_empty_and_mixed_separator_tokens(void)
{
    char *role_set_public_args[] = { "par", "role", "set", "public" };
    char *role_set_empty_args[] = { "par", "role", "set", "" };
    char *role_set_empty_mid_args[] = { "par", "role", "set", "public||service" };
    char *role_set_spaced_empty_args[] = { "par", "role", "set", "public, ,service" };
    char *role_set_mixed_args[] = { "par", "role", "set", "public,service+developer" };
    char *role_query_args[] = { "par", "role" };

    TEST_ASSERT(init_module());
    run_shell(4, role_set_public_args);
    TEST_ASSERT(shell_capture_contains("shell_roles=public"));
    run_shell(4, role_set_empty_args);
    TEST_ASSERT(shell_capture_contains("ERR"));
    run_shell(4, role_set_empty_mid_args);
    TEST_ASSERT(shell_capture_contains("ERR"));
    run_shell(4, role_set_spaced_empty_args);
    TEST_ASSERT(shell_capture_contains("ERR"));
    run_shell(2, role_query_args);
    TEST_ASSERT(shell_capture_contains("shell_roles=public"));
    run_shell(4, role_set_mixed_args);
    TEST_ASSERT(shell_capture_contains("shell_roles=public|service|developer"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify malformed role commands preserve the previous role mask. */
static bool test_msh_role_rejects_unknown_token_without_role_change(void)
{
    char *role_set_args[] = { "par", "role", "set", "public" };
    char *role_bad_args[] = { "par", "role", "add", "root" };
    char *role_query_args[] = { "par", "role" };

    TEST_ASSERT(init_module());
    run_shell(4, role_set_args);
    TEST_ASSERT(shell_capture_contains("shell_roles=public"));
    run_shell(4, role_bad_args);
    TEST_ASSERT(shell_capture_contains("ERR"));
    run_shell(2, role_query_args);
    TEST_ASSERT(shell_capture_contains("shell_roles=public"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}
