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

#include "shell_tool/par_shell_tool_basic_cases.inc"
#include "shell_tool/par_shell_tool_object_cases.inc"
#include "shell_tool/par_shell_tool_scalar_policy_cases.inc"

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
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
        { "msh_role_combination_and_duplicate_token_current_policy", test_msh_role_combination_and_duplicate_token_current_policy },
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
        { "msh_json_escapes_control_chars", test_msh_json_escapes_control_chars },
        { "msh_set_rejects_numeric_trailing_garbage_without_mutation", test_msh_set_rejects_numeric_trailing_garbage_without_mutation },
        { "msh_set_rejects_nonfinite_float_without_mutation", test_msh_set_rejects_nonfinite_float_without_mutation },
        { "msh_role_rejects_empty_and_mixed_separator_tokens", test_msh_role_rejects_empty_and_mixed_separator_tokens },
        { "msh_role_rejects_unknown_token_without_role_change", test_msh_role_rejects_unknown_token_without_role_change },
    };

    return par_host_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
}
