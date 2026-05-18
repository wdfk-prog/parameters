/**
 * @file test_par_shell_tool.c
 * @brief Exercise MSH parameter command parsing and output formatting.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include "test_host_common.h"
#include "rtthread.h"

#include <stdarg.h>

/** @brief Captured shell output buffer size. */
#define SHELL_CAPTURE_SIZE (8192U)

/** @brief Captured shell output buffer. */
static char g_shell_capture[SHELL_CAPTURE_SIZE];
/** @brief Number of bytes currently used in g_shell_capture. */
static size_t g_shell_capture_used;
/** @brief One-shot shell heap-allocation failure switch. */
static bool g_shell_malloc_fail_once;

/** @brief Reset shell output capture buffer. */
static void shell_capture_reset(void)
{
    g_shell_capture[0] = '\0';
    g_shell_capture_used = 0U;
    g_shell_malloc_fail_once = false;
}

/** @brief Return true when captured shell output contains text. */
static bool shell_capture_contains(const char *needle)
{
    return (NULL != strstr(g_shell_capture, needle));
}

/**
 * @brief Return true when captured shell output exactly matches text.
 * @param expected Null-terminated text to compare against the capture buffer.
 * @return true when @p expected exactly matches the capture buffer.
 */
static bool shell_capture_equals(const char *expected)
{
    return (0 == strcmp(g_shell_capture, expected));
}

/** @brief Append formatted text to the shell capture buffer. */
int rt_kprintf(const char *fmt, ...)
{
    int written;
    va_list args;
    size_t free_size = (g_shell_capture_used < SHELL_CAPTURE_SIZE) ?
                       (SHELL_CAPTURE_SIZE - g_shell_capture_used) : 0U;

    if (0U == free_size)
    {
        return 0;
    }

    va_start(args, fmt);
    written = vsnprintf(&g_shell_capture[g_shell_capture_used], free_size, fmt, args);
    va_end(args);

    if (written < 0)
    {
        return written;
    }
    if ((size_t)written >= free_size)
    {
        g_shell_capture_used = SHELL_CAPTURE_SIZE - 1U;
    }
    else
    {
        g_shell_capture_used += (size_t)written;
    }

    return written;
}

/** @brief Host stub for RT-Thread snprintf. */
int rt_snprintf(char *buf, rt_size_t size, const char *fmt, ...)
{
    int written;
    va_list args;

    va_start(args, fmt);
    written = vsnprintf(buf, size, fmt, args);
    va_end(args);
    return written;
}

/** @brief Host stub for RT-Thread heap allocation. */
void *rt_malloc(rt_size_t size)
{
    if (g_shell_malloc_fail_once)
    {
        g_shell_malloc_fail_once = false;
        return NULL;
    }

    return malloc(size);
}

/** @brief Host stub for RT-Thread heap free. */
void rt_free(void *ptr)
{
    free(ptr);
}

#include "../../../port/par_shell_tool.c"

/** @brief Initialize the parameter module for one shell test case. */
static bool init_module(void)
{
    if (par_is_init())
    {
        TEST_ASSERT_OK(par_deinit());
    }

    TEST_ASSERT_OK(par_init());
    shell_capture_reset();
    return true;
}

/** @brief Run the static shell command dispatcher with an argv vector. */
static void run_shell(int argc, char **argv)
{
    shell_capture_reset();
    par_msh(argc, argv);
}

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

/** @brief Verify role list separators and duplicate tokens preserve the same mask. */
static bool test_msh_role_combination_and_duplicate_token_policy(void)
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

/** @brief Entrypoint for MSH shell host tests. */
int main(void)
{
    static const par_host_test_case_t cases[] = {
        { "msh_help_and_unknown_command", test_msh_help_and_unknown_command },
        { "msh_info_prints_table_summary", test_msh_info_prints_table_summary },
        { "msh_get_set_scalar_by_id", test_msh_get_set_scalar_by_id },
        { "msh_set_compact_and_malformed_value", test_msh_set_compact_and_malformed_value },
        { "msh_rejects_invalid_id_and_range", test_msh_rejects_invalid_id_and_range },
        { "msh_role_commands_gate_scalar_access", test_msh_role_commands_gate_scalar_access },
        { "msh_get_object_str_output", test_msh_get_object_str_output },
        { "msh_json_prints_escaped_table", test_msh_json_prints_escaped_table },
        { "msh_def_restores_default", test_msh_def_restores_default },
        { "msh_def_all_restores_defaults", test_msh_def_all_restores_defaults },
        { "msh_get_object_bytes_hex_output", test_msh_get_object_bytes_hex_output },
        { "msh_set_object_row_is_rejected_without_mutation", test_msh_set_object_row_is_rejected_without_mutation },
        { "msh_set_object_compact_syntax_rejected_without_mutation", test_msh_set_object_compact_syntax_rejected_without_mutation },
        { "msh_role_commands_gate_object_access", test_msh_role_commands_gate_object_access },
        { "msh_get_object_array_outputs_are_stable", test_msh_get_object_array_outputs_are_stable },
        { "msh_get_object_alloc_failure_reports_error", test_msh_get_object_alloc_failure_reports_error },
        { "msh_set_all_scalar_widths_and_float", test_msh_set_all_scalar_widths_and_float },
        { "msh_set_rejects_overflow_and_negative_unsigned", test_msh_set_rejects_overflow_and_negative_unsigned },
        { "msh_parser_whitespace_and_empty_token_matrix", test_msh_parser_whitespace_and_empty_token_matrix },
        { "msh_role_combination_and_duplicate_token_policy", test_msh_role_combination_and_duplicate_token_policy },
        { "msh_json_escapes_control_chars", test_msh_json_escapes_control_chars },
        { "msh_set_rejects_numeric_trailing_garbage_without_mutation", test_msh_set_rejects_numeric_trailing_garbage_without_mutation },
        { "msh_set_rejects_nonfinite_float_without_mutation", test_msh_set_rejects_nonfinite_float_without_mutation },
        { "msh_role_rejects_empty_and_mixed_separator_tokens", test_msh_role_rejects_empty_and_mixed_separator_tokens },
        { "msh_role_rejects_unknown_token_without_role_change", test_msh_role_rejects_unknown_token_without_role_change },
    };

    return par_host_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
}
