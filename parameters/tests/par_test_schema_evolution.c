/**
 * @file par_test_schema_evolution.c
 * @brief Provide RT-Thread MSH wrappers for NVM schema-evolution validation.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <rtthread.h>
#ifdef RT_USING_FINSH
#include <finsh.h>
#endif /* defined(RT_USING_FINSH) */

#include "schema_evolution/par_schema_evolution_core.h"

#if defined(AUTOGEN_PM_TEST_NVM_SCHEMA_EVOLUTION) && defined(RT_USING_FINSH)

#define PAR_TEST_SCHEMA_BUILD_ENABLED \
    ((1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_ENABLE_ID) && \
     (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED))

#if PAR_TEST_SCHEMA_BUILD_ENABLED

#define PAR_TEST_SCHEMA_MSH_LINE_SIZE (192U)

/**
 * @brief Print one schema-evolution line through the RT-Thread shell.
 * @param p_ctx Unused callback context.
 * @param p_fmt printf-like format string.
 * @param args Format argument list.
 */
static void par_nvm_schema_vprint(void *p_ctx, const char *p_fmt, va_list args)
{
    char line[PAR_TEST_SCHEMA_MSH_LINE_SIZE];

    (void)p_ctx;

    if (NULL == p_fmt)
    {
        return;
    }

    (void)vsnprintf(line, sizeof(line), p_fmt, args);
    rt_kprintf("%s", line);
}

/**
 * @brief Print schema-evolution helper usage.
 */
static void par_nvm_schema_usage(void)
{
    rt_kprintf("Usage:\n");
    rt_kprintf("  par_nvm_schema prepare\n");
    rt_kprintf("  par_nvm_schema dump\n");
    rt_kprintf("  par_nvm_schema verify base\n");
    rt_kprintf("  par_nvm_schema verify scalar_append\n");
    rt_kprintf("  par_nvm_schema verify object_append\n");
    rt_kprintf("  par_nvm_schema verify mixed_append\n");
    rt_kprintf("  par_nvm_schema verify scalar_append_object_rebuild\n");
    rt_kprintf("  par_nvm_schema verify object_rebuild\n");
    rt_kprintf("  par_nvm_schema verify scalar_rebuild\n");
    rt_kprintf("  par_nvm_schema verify scalar_rebuild_object_retain\n");
    rt_kprintf("  par_nvm_schema verify scalar_rebuild_object_default\n");
    rt_kprintf("  par_nvm_schema verify full_rebuild\n");
}

/**
 * @brief Convert a command argument to one schema-evolution verify mode.
 * @param p_name Command argument after "verify".
 * @param p_mode Output verification mode.
 * @return true when the argument is recognized.
 */
static bool par_nvm_schema_parse_verify_mode(const char * const p_name, par_schema_evolution_verify_mode_t * const p_mode)
{
    if ((NULL == p_name) || (NULL == p_mode))
    {
        return false;
    }

    if (0 == strcmp(p_name, "base"))
    {
        *p_mode = PAR_SCHEMA_EVOLUTION_VERIFY_BASE;
        return true;
    }
    if (0 == strcmp(p_name, "scalar_append"))
    {
        *p_mode = PAR_SCHEMA_EVOLUTION_VERIFY_SCALAR_APPEND;
        return true;
    }
    if (0 == strcmp(p_name, "object_append"))
    {
        *p_mode = PAR_SCHEMA_EVOLUTION_VERIFY_OBJECT_APPEND;
        return true;
    }
    if (0 == strcmp(p_name, "mixed_append"))
    {
        *p_mode = PAR_SCHEMA_EVOLUTION_VERIFY_MIXED_APPEND;
        return true;
    }
    if (0 == strcmp(p_name, "scalar_append_object_rebuild"))
    {
        *p_mode = PAR_SCHEMA_EVOLUTION_VERIFY_SCALAR_APPEND_OBJECT_REBUILD;
        return true;
    }
    if (0 == strcmp(p_name, "object_rebuild"))
    {
        *p_mode = PAR_SCHEMA_EVOLUTION_VERIFY_OBJECT_REBUILD;
        return true;
    }
    if (0 == strcmp(p_name, "scalar_rebuild"))
    {
        *p_mode = PAR_SCHEMA_EVOLUTION_VERIFY_SCALAR_REBUILD;
        return true;
    }
    if (0 == strcmp(p_name, "scalar_rebuild_object_retain"))
    {
        *p_mode = PAR_SCHEMA_EVOLUTION_VERIFY_SCALAR_REBUILD_OBJECT_RETAIN;
        return true;
    }
    if (0 == strcmp(p_name, "scalar_rebuild_object_default"))
    {
        *p_mode = PAR_SCHEMA_EVOLUTION_VERIFY_SCALAR_REBUILD_OBJECT_DEFAULT;
        return true;
    }
    if (0 == strcmp(p_name, "full_rebuild"))
    {
        *p_mode = PAR_SCHEMA_EVOLUTION_VERIFY_FULL_REBUILD;
        return true;
    }

    return false;
}

/**
 * @brief MSH entry for NVM schema-evolution acceptance helpers.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process-like return code, 0 on success.
 */
static int par_nvm_schema(int argc, char **argv)
{
    par_schema_evolution_verify_mode_t mode;

    par_schema_evolution_set_vprint(par_nvm_schema_vprint, NULL);

    if (argc < 2)
    {
        par_nvm_schema_usage();
        return -1;
    }

    if (0 == strcmp(argv[1], "prepare"))
    {
        return par_schema_evolution_prepare();
    }
    if (0 == strcmp(argv[1], "dump"))
    {
        return par_schema_evolution_dump();
    }
    if ((0 == strcmp(argv[1], "verify")) && (argc >= 3))
    {
        if (true == par_nvm_schema_parse_verify_mode(argv[2], &mode))
        {
            return par_schema_evolution_verify(mode);
        }
    }

    par_nvm_schema_usage();
    return -1;
}
/**
 * @brief Option-completion IDs for par_nvm_schema subcommands.
 */
typedef enum
{
    PAR_NVM_SCHEMA_OPT_PREPARE = 1,          /**< Prepare the V1 baseline image. */
    PAR_NVM_SCHEMA_OPT_DUMP,                 /**< Dump the active fixture map. */
    PAR_NVM_SCHEMA_OPT_VERIFY,               /**< Select verification mode. */
    PAR_NVM_SCHEMA_OPT_BASE,                 /**< Verify the V1 baseline image. */
    PAR_NVM_SCHEMA_OPT_SCALAR_APPEND,        /**< Verify scalar append behavior. */
    PAR_NVM_SCHEMA_OPT_OBJECT_APPEND,        /**< Verify object append behavior. */
    PAR_NVM_SCHEMA_OPT_MIXED_APPEND,         /**< Verify mixed scalar/object append behavior. */
    PAR_NVM_SCHEMA_OPT_SCALAR_APPEND_OBJECT_REBUILD, /**< Verify scalar append with object rebuild. */
    PAR_NVM_SCHEMA_OPT_OBJECT_REBUILD,       /**< Verify object rebuild behavior. */
    PAR_NVM_SCHEMA_OPT_SCALAR_REBUILD,       /**< Verify scalar rebuild behavior. */
    PAR_NVM_SCHEMA_OPT_SCALAR_REBUILD_OBJECT_RETAIN,  /**< Verify scalar rebuild with retained objects. */
    PAR_NVM_SCHEMA_OPT_SCALAR_REBUILD_OBJECT_DEFAULT, /**< Verify scalar rebuild with default objects. */
    PAR_NVM_SCHEMA_OPT_FULL_REBUILD          /**< Verify full rebuild behavior. */
} par_nvm_schema_opt_id_t;

CMD_OPTIONS_NODE_START(par_nvm_schema)
CMD_OPTIONS_NODE(PAR_NVM_SCHEMA_OPT_PREPARE, prepare, prepare V1 baseline NVM image)
CMD_OPTIONS_NODE(PAR_NVM_SCHEMA_OPT_DUMP, dump, dump active schema evolution fixture)
CMD_OPTIONS_NODE(PAR_NVM_SCHEMA_OPT_VERIFY, verify, select schema evolution verification mode)
CMD_OPTIONS_NODE(PAR_NVM_SCHEMA_OPT_BASE, base, verify prepared V1 baseline)
CMD_OPTIONS_NODE(PAR_NVM_SCHEMA_OPT_SCALAR_APPEND, scalar_append, verify scalar append)
CMD_OPTIONS_NODE(PAR_NVM_SCHEMA_OPT_OBJECT_APPEND, object_append, verify object append)
CMD_OPTIONS_NODE(PAR_NVM_SCHEMA_OPT_MIXED_APPEND, mixed_append, verify scalar and object append)
CMD_OPTIONS_NODE(PAR_NVM_SCHEMA_OPT_SCALAR_APPEND_OBJECT_REBUILD, scalar_append_object_rebuild, verify append object rebuild)
CMD_OPTIONS_NODE(PAR_NVM_SCHEMA_OPT_OBJECT_REBUILD, object_rebuild, verify object rebuild)
CMD_OPTIONS_NODE(PAR_NVM_SCHEMA_OPT_SCALAR_REBUILD, scalar_rebuild, verify scalar rebuild with flexible object result)
CMD_OPTIONS_NODE(PAR_NVM_SCHEMA_OPT_SCALAR_REBUILD_OBJECT_RETAIN, scalar_rebuild_object_retain, verify scalar rebuild retain)
CMD_OPTIONS_NODE(PAR_NVM_SCHEMA_OPT_SCALAR_REBUILD_OBJECT_DEFAULT, scalar_rebuild_object_default, verify scalar rebuild default)
CMD_OPTIONS_NODE(PAR_NVM_SCHEMA_OPT_FULL_REBUILD, full_rebuild, verify full schema rebuild)
CMD_OPTIONS_NODE_END
MSH_CMD_EXPORT(par_nvm_schema, NVM schema evolution acceptance helper, optenable);

#endif /* PAR_TEST_SCHEMA_BUILD_ENABLED */
#endif /* defined(AUTOGEN_PM_TEST_NVM_SCHEMA_EVOLUTION) && defined(RT_USING_FINSH) */
