/**
 * @file test_par_shell_feature_gate.c
 * @brief Exercise MSH command behavior when most shell subcommands are compiled out.
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

/**
 * @brief Return true when captured shell output contains text.
 * @param needle Null-terminated text to find.
 * @return true when @p needle is present in the capture buffer.
 */
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

/** @brief Initialize the parameter module for one shell feature-gate test case. */
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

/**
 * @brief Run the static shell command dispatcher with an argv vector.
 * @param argc Argument count passed to par_msh().
 * @param argv Argument vector passed to par_msh().
 */
static void run_shell(int argc, char **argv)
{
    shell_capture_reset();
    par_msh(argc, argv);
}


#if defined(PAR_HOST_TEST_SHELL_NO_GET)
/** @brief Verify disabling GET removes the command from help and dispatch. */
static bool test_msh_feature_gate_get_disabled_reports_unknown(void)
{
    char *help_args[] = { "par" };
    char *get_args[] = { "par", "get", "1" };

    TEST_ASSERT(init_module());
    run_shell(1, help_args);
    TEST_ASSERT(shell_capture_contains("Usage:"));
    TEST_ASSERT(!shell_capture_contains("get <id>"));
    run_shell(3, get_args);
    TEST_ASSERT(shell_capture_contains("ERR, unknown subcmd"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#elif defined(PAR_HOST_TEST_SHELL_NO_SAVE_JSON)
/** @brief Verify save and JSON commands can be removed while GET still works. */
static bool test_msh_feature_gate_save_json_disabled_get_still_works(void)
{
    char *help_args[] = { "par" };
    char *get_args[] = { "par", "get", "1" };
    char *save_args[] = { "par", "save" };
    char *json_args[] = { "par", "json" };

    TEST_ASSERT(init_module());
    run_shell(1, help_args);
    TEST_ASSERT(shell_capture_contains("Usage:"));
    TEST_ASSERT(shell_capture_contains("get <id>"));
    TEST_ASSERT(!shell_capture_contains("save"));
    TEST_ASSERT(!shell_capture_contains("json"));
    run_shell(3, get_args);
    TEST_ASSERT(0 == strcmp(g_shell_capture, "OK,PAR_GET=1\n"));
    run_shell(2, save_args);
    TEST_ASSERT(shell_capture_contains("ERR, unknown subcmd"));
    run_shell(2, json_args);
    TEST_ASSERT(shell_capture_contains("ERR, unknown subcmd"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#elif (0 == PAR_CFG_ENABLE_ID)
/** @brief Verify ID-disabled builds omit ID-based shell commands. */
static bool test_msh_feature_gate_id_disabled_omits_id_commands(void)
{
    char *help_args[] = { "par" };
    char *get_args[] = { "par", "get", "1" };
    char *set_args[] = { "par", "set", "1", "8" };

    TEST_ASSERT(init_module());
    run_shell(1, help_args);
    TEST_ASSERT(shell_capture_contains("Usage:"));
    TEST_ASSERT(!shell_capture_contains("get <id>"));
    TEST_ASSERT(!shell_capture_contains("set <id>"));
    run_shell(3, get_args);
    TEST_ASSERT(shell_capture_contains("ERR, unknown subcmd"));
    run_shell(4, set_args);
    TEST_ASSERT(shell_capture_contains("ERR, unknown subcmd"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#else
/** @brief Verify help only lists commands compiled into this feature profile. */
static bool test_msh_feature_gate_help_lists_only_compiled_commands(void)
{
    char *help_args[] = { "par" };

    TEST_ASSERT(init_module());
    run_shell(1, help_args);
    TEST_ASSERT(shell_capture_contains("Usage:"));
    TEST_ASSERT(shell_capture_contains("get <id>"));
    TEST_ASSERT(!shell_capture_contains("info"));
    TEST_ASSERT(!shell_capture_contains("set <id>"));
    TEST_ASSERT(!shell_capture_contains("json"));
    TEST_ASSERT(!shell_capture_contains("save"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify compiled-out subcommands are rejected by the dispatcher. */
static bool test_msh_feature_gate_disabled_commands_are_unknown(void)
{
    char *info_args[] = { "par", "info" };
    char *set_args[] = { "par", "set", "1", "8" };
    char *json_args[] = { "par", "json" };

    TEST_ASSERT(init_module());
    run_shell(2, info_args);
    TEST_ASSERT(shell_capture_contains("ERR, unknown subcmd"));
    run_shell(4, set_args);
    TEST_ASSERT(shell_capture_contains("ERR, unknown subcmd"));
    run_shell(2, json_args);
    TEST_ASSERT(shell_capture_contains("ERR, unknown subcmd"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify the remaining scalar get command still works. */
static bool test_msh_feature_gate_get_scalar_still_works(void)
{
    char *get_args[] = { "par", "get", "1" };

    TEST_ASSERT(init_module());
    run_shell(3, get_args);
    TEST_ASSERT(0 == strcmp(g_shell_capture, "OK,PAR_GET=1\n"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object get reports why object formatting is unavailable. */
static bool test_msh_feature_gate_object_get_requires_heap_command(void)
{
    char *get_args[] = { "par", "get", "9" };

    TEST_ASSERT(init_module());
    run_shell(3, get_args);
    TEST_ASSERT(shell_capture_contains("AUTOGEN_PM_MSH_CMD_GET_OBJECT"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

#endif /* defined(PAR_HOST_TEST_SHELL_NO_GET) */

/** @brief Entrypoint for shell feature-gate host tests. */
int main(void)
{
    static const par_host_test_case_t cases[] = {
#if defined(PAR_HOST_TEST_SHELL_NO_GET)
        { "msh_feature_gate_get_disabled_reports_unknown", test_msh_feature_gate_get_disabled_reports_unknown },
#elif defined(PAR_HOST_TEST_SHELL_NO_SAVE_JSON)
        { "msh_feature_gate_save_json_disabled_get_still_works", test_msh_feature_gate_save_json_disabled_get_still_works },
#elif (0 == PAR_CFG_ENABLE_ID)
        { "msh_feature_gate_id_disabled_omits_id_commands", test_msh_feature_gate_id_disabled_omits_id_commands },
#else
        { "msh_feature_gate_help_lists_only_compiled_commands", test_msh_feature_gate_help_lists_only_compiled_commands },
        { "msh_feature_gate_disabled_commands_are_unknown", test_msh_feature_gate_disabled_commands_are_unknown },
        { "msh_feature_gate_get_scalar_still_works", test_msh_feature_gate_get_scalar_still_works },
        { "msh_feature_gate_object_get_requires_heap_command", test_msh_feature_gate_object_get_requires_heap_command },
#endif /* defined(PAR_HOST_TEST_SHELL_NO_GET) */
    };

    return par_host_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
}
