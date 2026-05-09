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

/** @brief Reset shell output capture buffer. */
static void shell_capture_reset(void)
{
    g_shell_capture[0] = '\0';
    g_shell_capture_used = 0U;
}

/** @brief Return true when captured shell output contains text. */
static bool shell_capture_contains(const char *needle)
{
    return (NULL != strstr(g_shell_capture, needle));
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
    };

    return par_host_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
}
