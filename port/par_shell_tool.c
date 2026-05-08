/**
 * @file par_shell_tool.c
 * @brief Provide RT-Thread MSH helpers for parameter inspection and control.
 * @author wdfk-prog
 * @version 1.1
 * @date 2026-04-23
 *
 * @copyright Copyright (c) 2026
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-04-21 1.0     wdfk-prog     first version
 * 2026-04-23 1.1     wdfk-prog     add Doxygen comments for shell helpers
 */
#include <rtthread.h>
#ifdef RT_USING_FINSH
#include <finsh.h>
#endif /* defined(RT_USING_FINSH) */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "par.h"

#if defined(AUTOGEN_PM_USING_MSH_TOOL) && defined(RT_USING_FINSH)

/**
 * @brief Fixed decimal scale used when formatting float values.
 */
#define PAR_SHELL_F32_SCALE (1000000UL)

/**
 * @brief Object payload display requires the RT-Thread heap allocator.
 */
#if defined(AUTOGEN_PM_MSH_CMD_GET_OBJECT) && defined(RT_USING_HEAP)
#define PAR_SHELL_OBJECT_GET_ENABLED (1)
#else
#define PAR_SHELL_OBJECT_GET_ENABLED (0)
#endif /* defined(AUTOGEN_PM_MSH_CMD_GET_OBJECT) && defined(RT_USING_HEAP) */

/**
 * @brief Resolved shell target identified by external ID and internal number.
 */
typedef struct
{
    uint16_t id;       /**< External parameter identifier. */
    par_num_t par_num; /**< Internal parameter number. */
} par_shell_target_t;

#if defined(AUTOGEN_PM_MSH_CMD_JSON)
/**
 * @brief Convert one parameter type to a shell-facing string token.
 * @param type Parameter type.
 * @return Constant string for the type.
 */
static const char *par_shell_type_str(const par_type_list_t type)
{
    switch (type)
    {
    case ePAR_TYPE_U8:
        return "u8";
    case ePAR_TYPE_U16:
        return "u16";
    case ePAR_TYPE_U32:
        return "u32";
    case ePAR_TYPE_I8:
        return "i8";
    case ePAR_TYPE_I16:
        return "i16";
    case ePAR_TYPE_I32:
        return "i32";
    case ePAR_TYPE_F32:
        return "f32";
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
    case ePAR_TYPE_STR:
        return "str";
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
    case ePAR_TYPE_BYTES:
        return "bytes";
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
    case ePAR_TYPE_ARR_U8:
        return "arr_u8";
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
    case ePAR_TYPE_ARR_U16:
        return "arr_u16";
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
    case ePAR_TYPE_ARR_U32:
        return "arr_u32";
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */
    default:
        return "unknown";
    }
}
#endif /* defined(AUTOGEN_PM_MSH_CMD_JSON) */

#if defined(AUTOGEN_PM_MSH_CMD_INFO)
/**
 * @brief Convert one parameter type to its numeric shell representation.
 * @param type Parameter type.
 * @return Unsigned numeric type code.
 */
static unsigned int par_shell_type_num(const par_type_list_t type)
{
    return (unsigned int)type;
}
#endif /* defined(AUTOGEN_PM_MSH_CMD_INFO) */

#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
/**
 * @brief Check whether one parameter type uses object storage.
 * @param type Parameter type.
 * @return true when the type is an object-backed parameter.
 */
static bool par_shell_type_is_object(const par_type_list_t type)
{
    switch (type)
    {
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
    case ePAR_TYPE_STR:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
    case ePAR_TYPE_BYTES:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
    case ePAR_TYPE_ARR_U8:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
    case ePAR_TYPE_ARR_U16:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
    case ePAR_TYPE_ARR_U32:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */
        return true;

    default:
        return false;
    }
}
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

#if (1 == PAR_CFG_ENABLE_ACCESS)
#if (defined(AUTOGEN_PM_MSH_CMD_INFO) || defined(AUTOGEN_PM_MSH_CMD_JSON))
/**
 * @brief Convert access flags to a short shell string.
 * @param access Access flags.
 * @return "rw", "ro", "wo", or "none".
 */
static const char *par_shell_access_str(const par_access_t access)
{
    const bool can_read = (0U != ((uint32_t)access & (uint32_t)ePAR_ACCESS_READ));
    const bool can_write = (0U != ((uint32_t)access & (uint32_t)ePAR_ACCESS_WRITE));

    if (can_read && can_write)
    {
        return "rw";
    }
    if (can_read)
    {
        return "ro";
    }
    if (can_write)
    {
        return "wo";
    }
    return "none";
}
#endif /* (defined(AUTOGEN_PM_MSH_CMD_INFO) || defined(AUTOGEN_PM_MSH_CMD_JSON)) */
#endif /* (1 == PAR_CFG_ENABLE_ACCESS) */

#if (1 == PAR_CFG_ENABLE_ROLE_POLICY)
/**
 * @brief Active shell role mask used for access checks.
 */
static par_role_t gs_par_shell_roles = ePAR_ROLE_PUBLIC;

/**
 * @brief Append one role token to a bounded shell string.
 * @param buf Destination buffer.
 * @param buf_size Destination buffer size.
 * @param[in,out] p_used Number of bytes already stored in the buffer.
 * @param token Role token to append.
 */
static void par_shell_append_role_token(char *buf,
                                        const rt_size_t buf_size,
                                        rt_size_t *const p_used,
                                        const char *token)
{
    int written;
    const rt_size_t free_size = (buf_size > *p_used) ? (buf_size - *p_used) : 0U;

    if (free_size == 0U)
    {
        buf[buf_size - 1U] = '\0';
        return;
    }

    written = rt_snprintf(buf + *p_used,
                          free_size,
                          "%s%s",
                          (*p_used > 0U) ? "|" : "",
                          token);
    if (written < 0)
    {
        buf[*p_used] = '\0';
        return;
    }

    if ((rt_size_t)written >= free_size)
    {
        *p_used = buf_size - 1U;
        buf[*p_used] = '\0';
    }
    else
    {
        *p_used += (rt_size_t)written;
    }
}

/**
 * @brief Format one role mask as a printable string.
 * @param roles Role mask.
 * @param buf Destination buffer.
 * @param buf_size Destination buffer size.
 * @return Pointer to the formatted role string.
 */
static const char *par_shell_roles_to_cstr(const par_role_t roles, char *buf, const rt_size_t buf_size)
{
    rt_size_t used = 0U;

    if ((buf == RT_NULL) || (buf_size == 0U))
    {
        return "";
    }

    buf[0] = '\0';

    if (ePAR_ROLE_NONE == roles)
    {
        rt_snprintf(buf, buf_size, "none");
        return buf;
    }

    if (0U != ((uint32_t)roles & (uint32_t)ePAR_ROLE_PUBLIC))
    {
        par_shell_append_role_token(buf, buf_size, &used, "public");
    }
    if (0U != ((uint32_t)roles & (uint32_t)ePAR_ROLE_SERVICE))
    {
        par_shell_append_role_token(buf, buf_size, &used, "service");
    }
    if (0U != ((uint32_t)roles & (uint32_t)ePAR_ROLE_DEVELOPER))
    {
        par_shell_append_role_token(buf, buf_size, &used, "developer");
    }
    if (0U != ((uint32_t)roles & (uint32_t)ePAR_ROLE_MANUFACTURING))
    {
        par_shell_append_role_token(buf, buf_size, &used, "manufacturing");
    }

    return buf;
}

/**
 * @brief Translate one role token to a role bit mask.
 * @param token Input role token.
 * @param[out] p_role Parsed role mask.
 * @return true on success.
 */
static bool par_shell_role_name_to_mask(const char *token, par_role_t *const p_role)
{
    if ((token == RT_NULL) || (p_role == RT_NULL))
    {
        return false;
    }

    if (strcmp(token, "none") == 0)
    {
        *p_role = ePAR_ROLE_NONE;
        return true;
    }
    if (strcmp(token, "all") == 0)
    {
        *p_role = ePAR_ROLE_ALL;
        return true;
    }
    if (strcmp(token, "public") == 0)
    {
        *p_role = ePAR_ROLE_PUBLIC;
        return true;
    }
    if (strcmp(token, "service") == 0)
    {
        *p_role = ePAR_ROLE_SERVICE;
        return true;
    }
    if ((strcmp(token, "developer") == 0) || (strcmp(token, "dev") == 0))
    {
        *p_role = ePAR_ROLE_DEVELOPER;
        return true;
    }
    if ((strcmp(token, "manufacturing") == 0) || (strcmp(token, "mfg") == 0))
    {
        *p_role = ePAR_ROLE_MANUFACTURING;
        return true;
    }

    return false;
}

/**
 * @brief Parse a role-expression string.
 * @param str Input role list.
 * @param[out] p_roles Parsed role mask.
 * @return true on success.
 */
static bool par_shell_parse_roles(const char *str, par_role_t *const p_roles)
{
    char buf[64];
    char *token;
    par_role_t roles = ePAR_ROLE_NONE;
    par_role_t role = ePAR_ROLE_NONE;

    if ((str == RT_NULL) || (p_roles == RT_NULL) || (str[0] == '\0'))
    {
        return false;
    }

    rt_strncpy(buf, str, sizeof(buf) - 1U);
    buf[sizeof(buf) - 1U] = '\0';

    for (token = buf; *token != '\0'; token++)
    {
        if ((*token == '|') || (*token == ',') || (*token == '+'))
        {
            *token = ' ';
        }
    }

    token = strtok(buf, " ");
    while (token != RT_NULL)
    {
        if (false == par_shell_role_name_to_mask(token, &role))
        {
            return false;
        }

        if (role == ePAR_ROLE_ALL)
        {
            roles = ePAR_ROLE_ALL;
        }
        else if (role == ePAR_ROLE_NONE)
        {
            if (roles != ePAR_ROLE_NONE)
            {
                return false;
            }
        }
        else
        {
            roles = (par_role_t)((uint32_t)roles | (uint32_t)role);
        }

        token = strtok(RT_NULL, " ");
    }

    *p_roles = roles;
    return true;
}

/**
 * @brief Get the current shell role mask.
 * @return Active shell role mask.
 */
static par_role_t par_shell_get_roles(void)
{
    return gs_par_shell_roles;
}

/**
 * @brief Check whether the current shell roles can read one parameter.
 * @param par_num Parameter number.
 * @return true when read access is granted.
 */
static bool par_shell_can_read(const par_num_t par_num)
{
    return par_can_read(par_num, par_shell_get_roles());
}

/**
 * @brief Check whether the current shell roles can write one parameter.
 * @param par_num Parameter number.
 * @return true when write access is granted.
 */
static bool par_shell_can_write(const par_num_t par_num)
{
    return par_can_write(par_num, par_shell_get_roles());
}
#endif /* (1 == PAR_CFG_ENABLE_ROLE_POLICY) */

/**
 * @brief Resolve an optional shell group name for one parameter.
 * @param par_num Parameter number.
 * @return Group name, or RT_NULL when grouping is unused.
 */
PAR_PORT_WEAK const char *par_port_get_shell_group(const par_num_t par_num)
{
    RT_UNUSED(par_num);
    return RT_NULL;
}

#if defined(AUTOGEN_PM_MSH_CMD_INFO)
/**
 * @brief Decide whether a new group marker shall be printed.
 * @param current_group Current group name.
 * @param last_group Previously printed group name.
 * @return true when the marker shall be emitted.
 */
static bool par_shell_should_print_group_marker(const char *current_group, const char *last_group)
{
    if ((current_group == RT_NULL) || (current_group[0] == '\0'))
    {
        return false;
    }

    if (last_group == RT_NULL)
    {
        return true;
    }

    return (strcmp(current_group, last_group) != 0);
}
#endif /* defined(AUTOGEN_PM_MSH_CMD_INFO) */

#if defined(AUTOGEN_PM_MSH_CMD_INFO)
/**
 * @brief Print one group marker when the group changes.
 * @param par_num Parameter number.
 * @param last_group Previously printed group name.
 * @return Current group name, or RT_NULL when no group is printed.
 */
static const char *par_shell_print_group_marker(const par_num_t par_num, const char *last_group)
{
    const char *group_name = par_port_get_shell_group(par_num);

    if ((group_name == RT_NULL) || (group_name[0] == '\0'))
    {
        return RT_NULL;
    }

    if (par_shell_should_print_group_marker(group_name, last_group))
    {
        rt_kprintf(":%s\n", group_name);
    }

    return group_name;
}
#endif /* defined(AUTOGEN_PM_MSH_CMD_INFO) */

#if (defined(AUTOGEN_PM_MSH_CMD_INFO) || defined(AUTOGEN_PM_MSH_CMD_GET) || defined(AUTOGEN_PM_MSH_CMD_SET) || defined(AUTOGEN_PM_MSH_CMD_JSON))
/**
 * @brief Format one float value for shell output.
 * @param buf Destination buffer.
 * @param buf_size Destination buffer size.
 * @param value Float value.
 */
static void par_shell_f32_to_str(char *buf, const rt_size_t buf_size, float32_t value)
{
    const char *dot;

    if ((buf == RT_NULL) || (buf_size == 0U))
    {
        return;
    }

    rt_snprintf(buf, buf_size, "%.6f", (double)value);

    dot = strchr(buf, '.');
    if (dot != RT_NULL)
    {
        char *tail = buf + strlen(buf) - 1;
        while ((tail > dot) && (*tail == '0'))
        {
            *tail = '\0';
            tail--;
        }
        if (*tail == '.')
        {
            *tail = '\0';
        }
    }
}
#endif /* (defined(AUTOGEN_PM_MSH_CMD_INFO) || defined(AUTOGEN_PM_MSH_CMD_GET) || defined(AUTOGEN_PM_MSH_CMD_SET) || defined(AUTOGEN_PM_MSH_CMD_JSON)) */

#if (defined(AUTOGEN_PM_MSH_CMD_INFO) || defined(AUTOGEN_PM_MSH_CMD_GET) || defined(AUTOGEN_PM_MSH_CMD_SET) || defined(AUTOGEN_PM_MSH_CMD_JSON))
/**
 * @brief Format one scalar parameter value as a string.
 * @param type Parameter type.
 * @param p_value Typed parameter value.
 * @param buf Destination buffer.
 * @param buf_size Destination buffer size.
 * @return Pointer to the formatted string.
 */
static const char *par_shell_value_to_cstr(const par_type_list_t type, const par_type_t *const p_value, char *buf, const rt_size_t buf_size)
{
    if ((p_value == RT_NULL) || (buf == RT_NULL) || (buf_size == 0U))
    {
        return "";
    }

    switch (type)
    {
    case ePAR_TYPE_U8:
        rt_snprintf(buf, buf_size, "%u", (unsigned int)p_value->u8);
        break;

    case ePAR_TYPE_U16:
        rt_snprintf(buf, buf_size, "%u", (unsigned int)p_value->u16);
        break;

    case ePAR_TYPE_U32:
        rt_snprintf(buf, buf_size, "%lu", (unsigned long)p_value->u32);
        break;

    case ePAR_TYPE_I8:
        rt_snprintf(buf, buf_size, "%d", (int)p_value->i8);
        break;

    case ePAR_TYPE_I16:
        rt_snprintf(buf, buf_size, "%d", (int)p_value->i16);
        break;

    case ePAR_TYPE_I32:
        rt_snprintf(buf, buf_size, "%ld", (long)p_value->i32);
        break;

    case ePAR_TYPE_F32:
        par_shell_f32_to_str(buf, buf_size, p_value->f32);
        break;

    default:
        rt_snprintf(buf, buf_size, "?");
        break;
    }

    return buf;
}
#endif /* (defined(AUTOGEN_PM_MSH_CMD_INFO) || defined(AUTOGEN_PM_MSH_CMD_GET) || defined(AUTOGEN_PM_MSH_CMD_SET) || defined(AUTOGEN_PM_MSH_CMD_JSON)) */

#if (defined(AUTOGEN_PM_MSH_CMD_INFO) || defined(AUTOGEN_PM_MSH_CMD_JSON))
/**
 * @brief Format one default value for shell output.
 * @param p_cfg Parameter configuration.
 * @param buf Destination buffer.
 * @param buf_size Destination buffer size.
 * @return Pointer to the formatted string.
 */
static const char *par_shell_default_to_cstr(const par_cfg_t *const p_cfg, char *buf, const rt_size_t buf_size)
{
#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
    if ((p_cfg != RT_NULL) && (true == par_shell_type_is_object(p_cfg->type)))
    {
        rt_snprintf(buf, buf_size, "<object,len=%u,cap=%u>",
                    (unsigned int)p_cfg->value_cfg.object.def.len,
                    (unsigned int)p_cfg->value_cfg.object.range.max_len);
        return buf;
    }
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

    return par_shell_value_to_cstr(p_cfg->type, &p_cfg->value_cfg.scalar.def, buf, buf_size);
}

#if (1 == PAR_CFG_ENABLE_RANGE)
/**
 * @brief Format one minimum limit for shell output.
 * @param p_cfg Parameter configuration.
 * @param buf Destination buffer.
 * @param buf_size Destination buffer size.
 * @return Pointer to the formatted string.
 */
static const char *par_shell_min_to_cstr(const par_cfg_t *const p_cfg, char *buf, const rt_size_t buf_size)
{
#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
    if ((p_cfg != RT_NULL) && (true == par_shell_type_is_object(p_cfg->type)))
    {
        rt_snprintf(buf, buf_size, "%u", (unsigned int)p_cfg->value_cfg.object.range.min_len);
        return buf;
    }
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

    return par_shell_value_to_cstr(p_cfg->type, &p_cfg->value_cfg.scalar.range.min, buf, buf_size);
}

/**
 * @brief Format one maximum limit for shell output.
 * @param p_cfg Parameter configuration.
 * @param buf Destination buffer.
 * @param buf_size Destination buffer size.
 * @return Pointer to the formatted string.
 */
static const char *par_shell_max_to_cstr(const par_cfg_t *const p_cfg, char *buf, const rt_size_t buf_size)
{
#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
    if ((p_cfg != RT_NULL) && (true == par_shell_type_is_object(p_cfg->type)))
    {
        rt_snprintf(buf, buf_size, "%u", (unsigned int)p_cfg->value_cfg.object.range.max_len);
        return buf;
    }
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

    return par_shell_value_to_cstr(p_cfg->type, &p_cfg->value_cfg.scalar.range.max, buf, buf_size);
}
#endif /* (1 == PAR_CFG_ENABLE_RANGE) */
#endif /* (defined(AUTOGEN_PM_MSH_CMD_INFO) || defined(AUTOGEN_PM_MSH_CMD_JSON)) */

#if defined(AUTOGEN_PM_MSH_CMD_JSON)
/**
 * @brief Print one JSON-escaped string to the shell.
 * @param str Source string.
 */
static void par_shell_json_print_escaped(const char *str)
{
    const unsigned char *p = (const unsigned char *)((str != RT_NULL) ? str : "");

    while (*p != '\0')
    {
        switch (*p)
        {
        case '"':
            rt_kprintf("\\\"");
            break;
        case '\\':
            rt_kprintf("\\\\");
            break;
        case '\b':
            rt_kprintf("\\b");
            break;
        case '\f':
            rt_kprintf("\\f");
            break;
        case '\n':
            rt_kprintf("\\n");
            break;
        case '\r':
            rt_kprintf("\\r");
            break;
        case '\t':
            rt_kprintf("\\t");
            break;
        default:
            if (*p < 0x20U)
            {
                rt_kprintf("\\u%04x", (unsigned int)*p);
            }
            else
            {
                rt_kprintf("%c", *p);
            }
            break;
        }
        p++;
    }
}
#endif /* defined(AUTOGEN_PM_MSH_CMD_JSON) */

#if (defined(AUTOGEN_PM_MSH_CMD_GET) || defined(AUTOGEN_PM_MSH_CMD_SET) || defined(AUTOGEN_PM_MSH_CMD_DEF) || defined(AUTOGEN_PM_MSH_CMD_DEF_ALL) || defined(AUTOGEN_PM_MSH_CMD_SAVE) || defined(AUTOGEN_PM_MSH_CMD_SAVE_CLEAN))
/**
 * @brief Print a normalized shell status line.
 * @param action Action name.
 * @param status Parameter status code.
 */
static void par_shell_print_status(const char *action, const par_status_t status)
{
    const char *level = ((status & ePAR_STATUS_ERROR_MASK) != 0U) ? "ERR" : "WAR";

    if (status == ePAR_OK)
    {
        rt_kprintf("OK, %s\n", action);
    }
    else
    {
        rt_kprintf("%s, %s status=0x%04X\n", level, action, (unsigned int)status);
    }
}
#endif /* (defined(AUTOGEN_PM_MSH_CMD_GET) || defined(AUTOGEN_PM_MSH_CMD_SET) || defined(AUTOGEN_PM_MSH_CMD_DEF) || defined(AUTOGEN_PM_MSH_CMD_DEF_ALL) || defined(AUTOGEN_PM_MSH_CMD_SAVE) || defined(AUTOGEN_PM_MSH_CMD_SAVE_CLEAN)) */

#if (defined(AUTOGEN_PM_MSH_CMD_INFO) || defined(AUTOGEN_PM_MSH_CMD_JSON))
/**
 * @brief Format one non-OK status for shell output.
 * @param status Parameter status code.
 * @param buf Destination buffer.
 * @param buf_size Destination buffer size.
 * @return Pointer to the formatted string.
 */
static const char *par_shell_status_to_cstr(const par_status_t status, char *buf, const rt_size_t buf_size)
{
    if ((buf == RT_NULL) || (buf_size == 0U))
    {
        return "ERR";
    }

    rt_snprintf(buf, buf_size, "ERR(0x%04X)", (unsigned int)status);
    return buf;
}
#endif /* (defined(AUTOGEN_PM_MSH_CMD_INFO) || defined(AUTOGEN_PM_MSH_CMD_JSON)) */

#if ((defined(AUTOGEN_PM_MSH_CMD_INFO) || defined(AUTOGEN_PM_MSH_CMD_JSON)) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED))
/**
 * @brief Format object metadata for table-style shell output.
 * @details The current payload length is only printed when the caller already
 * passed the same read boundary used by `par_get_obj_len()`. Capacity is printed
 * with the same success path so denied reads do not expose object metadata.
 * @param par_num Parameter number.
 * @param status Read status returned by the object length read boundary.
 * @param p_value Typed value storage; `u16` carries the current payload length.
 * @param buf Destination buffer.
 * @param buf_size Destination buffer size.
 * @return Pointer to the formatted string.
 */
static const char *par_shell_object_value_to_cstr(const par_num_t par_num,
                                                  const par_status_t status,
                                                  const par_type_t *const p_value,
                                                  char *buf,
                                                  const rt_size_t buf_size)
{
    uint16_t capacity = 0U;
    par_status_t cap_status;

    if (status != ePAR_OK)
    {
        return par_shell_status_to_cstr(status, buf, buf_size);
    }
    if ((p_value == RT_NULL) || (buf == RT_NULL) || (buf_size == 0U))
    {
        return "";
    }

    cap_status = par_get_obj_capacity(par_num, &capacity);
    if (cap_status != ePAR_OK)
    {
        return par_shell_status_to_cstr(cap_status, buf, buf_size);
    }

    rt_snprintf(buf, buf_size, "<object,len=%u,cap=%u>",
                (unsigned int)p_value->u16,
                (unsigned int)capacity);
    return buf;
}
#endif /* ((defined(AUTOGEN_PM_MSH_CMD_INFO) || defined(AUTOGEN_PM_MSH_CMD_JSON)) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED)) */

#if (defined(AUTOGEN_PM_MSH_CMD_INFO) || defined(AUTOGEN_PM_MSH_CMD_JSON))
/**
 * @brief Format the current runtime value for shell output.
 * @details Non-OK read status is reported before object metadata formatting
 * so role-denied object rows are shown consistently with scalar rows.
 * @param par_num Parameter number.
 * @param type Parameter type.
 * @param status Read status.
 * @param p_value Typed parameter value.
 * @param buf Destination buffer.
 * @param buf_size Destination buffer size.
 * @return Pointer to the formatted string.
 */
static const char *par_shell_current_value_to_cstr(const par_num_t par_num,
                                                   const par_type_list_t type,
                                                   const par_status_t status,
                                                   const par_type_t *const p_value,
                                                   char *buf,
                                                   const rt_size_t buf_size)
{
    RT_UNUSED(par_num);

    if (status != ePAR_OK)
    {
        return par_shell_status_to_cstr(status, buf, buf_size);
    }

#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
    if (true == par_shell_type_is_object(type))
    {
        return par_shell_object_value_to_cstr(par_num, status, p_value, buf, buf_size);
    }
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

    return par_shell_value_to_cstr(type, p_value, buf, buf_size);
}
#endif /* (defined(AUTOGEN_PM_MSH_CMD_INFO) || defined(AUTOGEN_PM_MSH_CMD_JSON)) */

#if ((defined(AUTOGEN_PM_MSH_CMD_GET) || defined(AUTOGEN_PM_MSH_CMD_SET) || defined(AUTOGEN_PM_MSH_CMD_DEF)) && (1 == PAR_CFG_ENABLE_ID))
/**
 * @brief Parse one unsigned 16-bit integer from shell input.
 * @param str Input string.
 * @param[out] p_value Parsed value.
 * @return true on success.
 */
static bool par_shell_parse_u16(const char *str, uint16_t *const p_value)
{
    char *end = RT_NULL;
    unsigned long value;

    if ((str == RT_NULL) || (p_value == RT_NULL))
    {
        return false;
    }

    value = strtoul(str, &end, 0);
    if ((end == str) || (*end != '\0') || (value > 0xFFFFUL))
    {
        return false;
    }

    *p_value = (uint16_t)value;
    return true;
}
#endif /* ((defined(AUTOGEN_PM_MSH_CMD_GET) || defined(AUTOGEN_PM_MSH_CMD_SET) || defined(AUTOGEN_PM_MSH_CMD_DEF)) && (1 == PAR_CFG_ENABLE_ID)) */

#if (defined(AUTOGEN_PM_MSH_CMD_SET) && (1 == PAR_CFG_ENABLE_ID))
/**
 * @brief Parse one unsigned 32-bit integer from shell input.
 * @param str Input string.
 * @param[out] p_value Parsed value.
 * @return true on success.
 */
static bool par_shell_parse_u32(const char *str, uint32_t *const p_value)
{
    char *end = RT_NULL;
    unsigned long value;

    if ((str == RT_NULL) || (p_value == RT_NULL))
    {
        return false;
    }

    while ((*str == ' ') || (*str == '\t') || (*str == '\n') || (*str == '\r') || (*str == '\f') || (*str == '\v'))
    {
        str++;
    }

    if (str[0] == '-')
    {
        return false;
    }

    errno = 0;
    value = strtoul(str, &end, 0);
    if ((end == str) || (*end != '\0') || (errno == ERANGE) || (value > ((unsigned long)UINT32_MAX)))
    {
        return false;
    }

    *p_value = (uint32_t)value;
    return true;
}
#endif /* (defined(AUTOGEN_PM_MSH_CMD_SET) && (1 == PAR_CFG_ENABLE_ID)) */

#if (defined(AUTOGEN_PM_MSH_CMD_SET) && (1 == PAR_CFG_ENABLE_ID))
/**
 * @brief Parse one signed 32-bit integer from shell input.
 * @param str Input string.
 * @param[out] p_value Parsed value.
 * @return true on success.
 */
static bool par_shell_parse_i32(const char *str, int32_t *const p_value)
{
    char *end = RT_NULL;
    long value;

    if ((str == RT_NULL) || (p_value == RT_NULL))
    {
        return false;
    }

    errno = 0;
    value = strtol(str, &end, 0);
    if ((end == str) || (*end != '\0') || (errno == ERANGE) || (value < (long)INT32_MIN) || (value > (long)INT32_MAX))
    {
        return false;
    }

    *p_value = (int32_t)value;
    return true;
}
#endif /* (defined(AUTOGEN_PM_MSH_CMD_SET) && (1 == PAR_CFG_ENABLE_ID)) */

#if (defined(AUTOGEN_PM_MSH_CMD_GET) && (1 == PAR_CFG_ENABLE_ID) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED))
/**
 * @brief Read one parameter value or validate object readability for shell output.
 * @param par_num Parameter number.
 * @param[out] p_value Destination typed value for scalar rows.
 * @return Parameter status code.
 */
static par_status_t par_shell_get_value(const par_num_t par_num, par_type_t *const p_value);
#endif /* (defined(AUTOGEN_PM_MSH_CMD_GET) && (1 == PAR_CFG_ENABLE_ID) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED)) */

#if (defined(AUTOGEN_PM_MSH_CMD_GET) && (1 == PAR_CFG_ENABLE_ID) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (1 == PAR_SHELL_OBJECT_GET_ENABLED))
/**
 * @brief Allocate a temporary object payload buffer for shell output.
 * @param size Buffer size in bytes.
 * @return Allocated buffer pointer, or RT_NULL on allocation failure.
 */
static void *par_shell_object_alloc(const rt_size_t size)
{
    return rt_malloc(size);
}

/**
 * @brief Free a temporary object payload buffer allocated by the shell.
 * @param p_buf Buffer pointer returned by par_shell_object_alloc().
 */
static void par_shell_object_free(void *p_buf)
{
    if (p_buf == RT_NULL)
    {
        return;
    }

    rt_free(p_buf);
}

/**
 * @brief Print a shell-escaped string value.
 * @param p_str NUL-terminated string to print.
 */
static void par_shell_print_quoted_str(const char *p_str)
{
    const unsigned char *p = (const unsigned char *)((p_str != RT_NULL) ? p_str : "");

    rt_kprintf("\"");
    while (*p != '\0')
    {
        switch (*p)
        {
        case '\"':
            rt_kprintf("\\\"");
            break;
        case '\\':
            rt_kprintf("\\\\");
            break;
        case '\n':
            rt_kprintf("\\n");
            break;
        case '\r':
            rt_kprintf("\\r");
            break;
        case '\t':
            rt_kprintf("\\t");
            break;
        default:
            if (*p < 0x20U)
            {
                rt_kprintf("\\x%02X", (unsigned int)*p);
            }
            else
            {
                rt_kprintf("%c", *p);
            }
            break;
        }
        p++;
    }
    rt_kprintf("\"");
}

/**
 * @brief Print a byte payload as contiguous hexadecimal text.
 * @param p_data Byte buffer.
 * @param len Number of bytes to print.
 */
static void par_shell_print_hex_bytes(const uint8_t *p_data, const uint16_t len)
{
    uint16_t i;

    rt_kprintf("hex:");
    for (i = 0U; i < len; i++)
    {
        rt_kprintf("%02X", (unsigned int)p_data[i]);
    }
}

/**
 * @brief Print an unsigned 8-bit array payload.
 * @param p_data Array buffer.
 * @param count Number of elements to print.
 */
static void par_shell_print_arr_u8(const uint8_t *p_data, const uint16_t count)
{
    uint16_t i;

    rt_kprintf("[");
    for (i = 0U; i < count; i++)
    {
        rt_kprintf("%s%u", (i > 0U) ? "," : "", (unsigned int)p_data[i]);
    }
    rt_kprintf("]");
}

/**
 * @brief Print an unsigned 16-bit array payload.
 * @param p_data Array buffer.
 * @param count Number of elements to print.
 */
static void par_shell_print_arr_u16(const uint16_t *p_data, const uint16_t count)
{
    uint16_t i;

    rt_kprintf("[");
    for (i = 0U; i < count; i++)
    {
        rt_kprintf("%s%u", (i > 0U) ? "," : "", (unsigned int)p_data[i]);
    }
    rt_kprintf("]");
}

/**
 * @brief Print an unsigned 32-bit array payload.
 * @param p_data Array buffer.
 * @param count Number of elements to print.
 */
static void par_shell_print_arr_u32(const uint32_t *p_data, const uint16_t count)
{
    uint16_t i;

    rt_kprintf("[");
    for (i = 0U; i < count; i++)
    {
        rt_kprintf("%s%lu", (i > 0U) ? "," : "", (unsigned long)p_data[i]);
    }
    rt_kprintf("]");
}

#if (1 == PAR_CFG_ENABLE_TYPE_STR)
/**
 * @brief Print one string object parameter payload for `par get`.
 * @details The buffer is sized from configured capacity, so the typed getter
 * can copy one consistent core-side object state without a prior length read.
 * @param par_num Parameter number.
 * @param capacity Configured string payload capacity in bytes.
 * @return Operation status.
 */
static par_status_t par_shell_print_get_str(const par_num_t par_num, const uint16_t capacity)
{
    char *p_buf;
    uint16_t out_len = 0U;
    par_status_t status;

    if (capacity == UINT16_MAX)
    {
        return ePAR_ERROR_PARAM;
    }

    p_buf = (char *)par_shell_object_alloc((rt_size_t)capacity + 1U);
    if (p_buf == RT_NULL)
    {
        return ePAR_ERROR;
    }

    status = par_get_str(par_num, p_buf, (uint16_t)(capacity + 1U), &out_len);
    if (status == ePAR_OK)
    {
        rt_kprintf("OK,PAR_GET=");
        par_shell_print_quoted_str(p_buf);
        rt_kprintf("\n");
    }

    par_shell_object_free(p_buf);
    return status;
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */

#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
/**
 * @brief Print one raw byte object parameter payload for `par get`.
 * @details The buffer is sized from configured capacity, so the typed getter
 * can copy one consistent core-side object state without a prior length read.
 * @param par_num Parameter number.
 * @param capacity Configured payload capacity in bytes.
 * @return Operation status.
 */
static par_status_t par_shell_print_get_bytes(const par_num_t par_num, const uint16_t capacity)
{
    uint8_t *p_buf = RT_NULL;
    uint16_t out_len = 0U;
    par_status_t status;

    if (capacity > 0U)
    {
        p_buf = (uint8_t *)par_shell_object_alloc((rt_size_t)capacity);
        if (p_buf == RT_NULL)
        {
            return ePAR_ERROR;
        }
    }

    status = par_get_bytes(par_num, p_buf, capacity, &out_len);
    if ((status == ePAR_OK) && (out_len > 0U) && (p_buf == RT_NULL))
    {
        status = ePAR_ERROR;
    }
    if (status == ePAR_OK)
    {
        rt_kprintf("OK,PAR_GET=");
        par_shell_print_hex_bytes(p_buf, out_len);
        rt_kprintf("\n");
    }

    par_shell_object_free(p_buf);
    return status;
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */

#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
/**
 * @brief Print one unsigned 8-bit array object parameter for `par get`.
 * @details The buffer is sized from configured capacity, so the typed getter
 * can copy one consistent core-side object state without a prior length read.
 * @param par_num Parameter number.
 * @param capacity Configured payload capacity in bytes.
 * @return Operation status.
 */
static par_status_t par_shell_print_get_arr_u8(const par_num_t par_num, const uint16_t capacity)
{
    uint8_t *p_buf = RT_NULL;
    uint16_t out_count = 0U;
    par_status_t status;

    if (capacity > 0U)
    {
        p_buf = (uint8_t *)par_shell_object_alloc((rt_size_t)capacity);
        if (p_buf == RT_NULL)
        {
            return ePAR_ERROR;
        }
    }

    status = par_get_arr_u8(par_num, p_buf, capacity, &out_count);
    if ((status == ePAR_OK) && (out_count > 0U) && (p_buf == RT_NULL))
    {
        status = ePAR_ERROR;
    }
    if (status == ePAR_OK)
    {
        rt_kprintf("OK,PAR_GET=");
        par_shell_print_arr_u8(p_buf, out_count);
        rt_kprintf("\n");
    }

    par_shell_object_free(p_buf);
    return status;
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */

#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
/**
 * @brief Print one unsigned 16-bit array object parameter for `par get`.
 * @details The buffer is sized from configured capacity, so the typed getter
 * can copy one consistent core-side object state without a prior length read.
 * @param par_num Parameter number.
 * @param capacity Configured payload capacity in bytes.
 * @return Operation status.
 */
static par_status_t par_shell_print_get_arr_u16(const par_num_t par_num, const uint16_t capacity)
{
    uint16_t *p_buf = RT_NULL;
    uint16_t count;
    uint16_t out_count = 0U;
    par_status_t status;

    if ((capacity % (uint16_t)sizeof(uint16_t)) != 0U)
    {
        return ePAR_ERROR;
    }

    count = (uint16_t)(capacity / (uint16_t)sizeof(uint16_t));
    if (capacity > 0U)
    {
        p_buf = (uint16_t *)par_shell_object_alloc((rt_size_t)capacity);
        if (p_buf == RT_NULL)
        {
            return ePAR_ERROR;
        }
    }

    status = par_get_arr_u16(par_num, p_buf, count, &out_count);
    if ((status == ePAR_OK) && (out_count > 0U) && (p_buf == RT_NULL))
    {
        status = ePAR_ERROR;
    }
    if (status == ePAR_OK)
    {
        rt_kprintf("OK,PAR_GET=");
        par_shell_print_arr_u16(p_buf, out_count);
        rt_kprintf("\n");
    }

    par_shell_object_free(p_buf);
    return status;
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */

#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
/**
 * @brief Print one unsigned 32-bit array object parameter for `par get`.
 * @details The buffer is sized from configured capacity, so the typed getter
 * can copy one consistent core-side object state without a prior length read.
 * @param par_num Parameter number.
 * @param capacity Configured payload capacity in bytes.
 * @return Operation status.
 */
static par_status_t par_shell_print_get_arr_u32(const par_num_t par_num, const uint16_t capacity)
{
    uint32_t *p_buf = RT_NULL;
    uint16_t count;
    uint16_t out_count = 0U;
    par_status_t status;

    if ((capacity % (uint16_t)sizeof(uint32_t)) != 0U)
    {
        return ePAR_ERROR;
    }

    count = (uint16_t)(capacity / (uint16_t)sizeof(uint32_t));
    if (capacity > 0U)
    {
        p_buf = (uint32_t *)par_shell_object_alloc((rt_size_t)capacity);
        if (p_buf == RT_NULL)
        {
            return ePAR_ERROR;
        }
    }

    status = par_get_arr_u32(par_num, p_buf, count, &out_count);
    if ((status == ePAR_OK) && (out_count > 0U) && (p_buf == RT_NULL))
    {
        status = ePAR_ERROR;
    }
    if (status == ePAR_OK)
    {
        rt_kprintf("OK,PAR_GET=");
        par_shell_print_arr_u32(p_buf, out_count);
        rt_kprintf("\n");
    }

    par_shell_object_free(p_buf);
    return status;
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */

/**
 * @brief Print one object parameter payload for the shell get command.
 * @details The shell allocates from configured capacity and then lets the
 * typed getter copy the current object value under the core read boundary.
 * This avoids a separate current-length read before payload copy.
 * @param par_num Parameter number.
 * @param type Parameter type.
 * @return Operation status.
 */
static par_status_t par_shell_print_object_get(const par_num_t par_num, const par_type_list_t type)
{
    uint16_t capacity = 0U;
    par_status_t status;

#if (1 == PAR_CFG_ENABLE_ROLE_POLICY)
    if (false == par_shell_can_read(par_num))
    {
        return ePAR_ERROR_ACCESS;
    }
#endif /* (1 == PAR_CFG_ENABLE_ROLE_POLICY) */

    status = par_get_obj_capacity(par_num, &capacity);
    if (status != ePAR_OK)
    {
        return status;
    }

    switch (type)
    {
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
    case ePAR_TYPE_STR:
        return par_shell_print_get_str(par_num, capacity);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
    case ePAR_TYPE_BYTES:
        return par_shell_print_get_bytes(par_num, capacity);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
    case ePAR_TYPE_ARR_U8:
        return par_shell_print_get_arr_u8(par_num, capacity);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
    case ePAR_TYPE_ARR_U16:
        return par_shell_print_get_arr_u16(par_num, capacity);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
    case ePAR_TYPE_ARR_U32:
        return par_shell_print_get_arr_u32(par_num, capacity);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */
    default:
        return ePAR_ERROR_TYPE;
    }
}
#endif /* (defined(AUTOGEN_PM_MSH_CMD_GET) && (1 == PAR_CFG_ENABLE_ID) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (1 == PAR_SHELL_OBJECT_GET_ENABLED)) */

#if ((defined(AUTOGEN_PM_MSH_CMD_GET) || defined(AUTOGEN_PM_MSH_CMD_SET) || defined(AUTOGEN_PM_MSH_CMD_DEF)) && (1 == PAR_CFG_ENABLE_ID))
/**
 * @brief Resolve one shell target from the external parameter ID.
 * @param id_str Parameter ID string.
 * @param[out] p_target Resolved shell target.
 * @return true on success.
 */
static bool par_shell_resolve_target(const char *id_str, par_shell_target_t *const p_target)
{
    if ((id_str == RT_NULL) || (p_target == RT_NULL))
    {
        return false;
    }

    if (!par_shell_parse_u16(id_str, &p_target->id))
    {
        return false;
    }

    return (par_get_num_by_id(p_target->id, &p_target->par_num) == ePAR_OK);
}
#endif /* ((defined(AUTOGEN_PM_MSH_CMD_GET) || defined(AUTOGEN_PM_MSH_CMD_SET) || defined(AUTOGEN_PM_MSH_CMD_DEF)) && (1 == PAR_CFG_ENABLE_ID)) */

#if (defined(AUTOGEN_PM_MSH_CMD_INFO) || defined(AUTOGEN_PM_MSH_CMD_GET) || defined(AUTOGEN_PM_MSH_CMD_SET) || defined(AUTOGEN_PM_MSH_CMD_JSON))
/**
 * @brief Read one parameter value or validate object readability for shell output.
 * @details Scalar rows populate `p_value`. Object rows do not expose their live
 * payload through scalar shell buffers; they return the status from the object
 * read boundary so role or access denial is preserved for `info` and `json`.
 * @param par_num Parameter number.
 * @param[out] p_value Destination typed value for scalar rows.
 * @return Parameter status code.
 */
static par_status_t par_shell_get_value(const par_num_t par_num, par_type_t *const p_value)
{
    const par_type_list_t type = par_get_type(par_num);

    if (p_value == RT_NULL)
    {
        return ePAR_ERROR_PARAM;
    }

#if (1 == PAR_CFG_ENABLE_ROLE_POLICY)
    if (false == par_shell_can_read(par_num))
    {
        return ePAR_ERROR_ACCESS;
    }
#endif /* (1 == PAR_CFG_ENABLE_ROLE_POLICY) */

    switch (type)
    {
    case ePAR_TYPE_U8:
        return par_get_scalar(par_num, &p_value->u8);
    case ePAR_TYPE_U16:
        return par_get_scalar(par_num, &p_value->u16);
    case ePAR_TYPE_U32:
        return par_get_scalar(par_num, &p_value->u32);
    case ePAR_TYPE_I8:
        return par_get_scalar(par_num, &p_value->i8);
    case ePAR_TYPE_I16:
        return par_get_scalar(par_num, &p_value->i16);
    case ePAR_TYPE_I32:
        return par_get_scalar(par_num, &p_value->i32);
    case ePAR_TYPE_F32:
        return par_get_scalar(par_num, &p_value->f32);
#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
    case ePAR_TYPE_STR:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
    case ePAR_TYPE_BYTES:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
    case ePAR_TYPE_ARR_U8:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
    case ePAR_TYPE_ARR_U16:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
    case ePAR_TYPE_ARR_U32:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */
    {
        par_status_t status;
        uint16_t object_len = 0U;

        status = par_get_obj_len(par_num, &object_len);
        if (status == ePAR_OK)
        {
            p_value->u16 = object_len;
        }
        return status;
    }
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */
    default:
        return ePAR_ERROR_TYPE;
    }
}
#endif /* (defined(AUTOGEN_PM_MSH_CMD_INFO) || defined(AUTOGEN_PM_MSH_CMD_GET) || defined(AUTOGEN_PM_MSH_CMD_SET) || defined(AUTOGEN_PM_MSH_CMD_JSON)) */

#if (defined(AUTOGEN_PM_MSH_CMD_SET) && (1 == PAR_CFG_ENABLE_ID))
/**
 * @brief Parse one scalar parameter value from shell input.
 * @param str Input string.
 * @param type Parameter type.
 * @param[out] p_value Parsed typed value.
 * @return true on success.
 */
static bool par_shell_parse_value(const char *str, const par_type_list_t type, par_type_t *const p_value)
{
    char *end = RT_NULL;
    unsigned long uval;
    long ival;

    if ((str == RT_NULL) || (p_value == RT_NULL))
    {
        return false;
    }

    switch (type)
    {
    case ePAR_TYPE_U8:
        uval = strtoul(str, &end, 0);
        if ((end == str) || (*end != '\0') || (uval > 0xFFUL))
            return false;
        p_value->u8 = (uint8_t)uval;
        return true;

    case ePAR_TYPE_U16:
        uval = strtoul(str, &end, 0);
        if ((end == str) || (*end != '\0') || (uval > 0xFFFFUL))
            return false;
        p_value->u16 = (uint16_t)uval;
        return true;

    case ePAR_TYPE_U32:
        return par_shell_parse_u32(str, &p_value->u32);

    case ePAR_TYPE_I8:
        ival = strtol(str, &end, 0);
        if ((end == str) || (*end != '\0') || (ival < -128L) || (ival > 127L))
            return false;
        p_value->i8 = (int8_t)ival;
        return true;

    case ePAR_TYPE_I16:
        ival = strtol(str, &end, 0);
        if ((end == str) || (*end != '\0') || (ival < -32768L) || (ival > 32767L))
            return false;
        p_value->i16 = (int16_t)ival;
        return true;

    case ePAR_TYPE_I32:
        return par_shell_parse_i32(str, &p_value->i32);

    case ePAR_TYPE_F32:
        p_value->f32 = strtof(str, &end);
        if ((end == str) || (*end != '\0'))
            return false;
        return true;

    default:
        return false;
    }
}
#endif /* (defined(AUTOGEN_PM_MSH_CMD_SET) && (1 == PAR_CFG_ENABLE_ID)) */

#if (defined(AUTOGEN_PM_MSH_CMD_SET) && (1 == PAR_CFG_ENABLE_ID))
/**
 * @brief Parse arguments for the shell set command.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @param[out] p_target Resolved shell target.
 * @param[out] p_value_str Pointer to the scalar value string.
 * @param pair_buf Mutable storage used when the `id,value` form is provided.
 * @param pair_buf_size Size of pair_buf in bytes.
 * @return true on success.
 */
static bool par_shell_parse_set_args(const int argc,
                                     char **argv,
                                     par_shell_target_t *const p_target,
                                     const char **const p_value_str,
                                     char *const pair_buf,
                                     const rt_size_t pair_buf_size)
{
    char *comma;

    if ((p_target == RT_NULL) || (p_value_str == RT_NULL) || (pair_buf == RT_NULL) || (pair_buf_size == 0U))
    {
        return false;
    }

    if (argc >= 4)
    {
        if (!par_shell_resolve_target(argv[2], p_target))
        {
            return false;
        }
        *p_value_str = argv[3];
        return true;
    }

    if (argc == 3)
    {
        rt_strncpy(pair_buf, argv[2], pair_buf_size - 1U);
        pair_buf[pair_buf_size - 1U] = '\0';
        comma = strchr(pair_buf, ',');
        if (comma == RT_NULL)
        {
            return false;
        }

        *comma = '\0';
        if (!par_shell_resolve_target(pair_buf, p_target))
        {
            return false;
        }
        *p_value_str = comma + 1;
        return true;
    }

    return false;
}
#endif /* (defined(AUTOGEN_PM_MSH_CMD_SET) && (1 == PAR_CFG_ENABLE_ID)) */

/**
 * @brief Print shell usage help.
 */
static void par_shell_print_usage(void)
{
    rt_kprintf("Usage: par <subcmd> [args]\n");
    rt_kprintf("  help                    Show this help\n");
#if defined(AUTOGEN_PM_MSH_CMD_INFO)
    rt_kprintf("  info                    Print parameter table\n");
#endif /* defined(AUTOGEN_PM_MSH_CMD_INFO) */
#if defined(AUTOGEN_PM_MSH_CMD_GET) && (1 == PAR_CFG_ENABLE_ID)
    rt_kprintf("  get <id>                Get scalar value or object payload by ID\n");
#endif /* defined(AUTOGEN_PM_MSH_CMD_GET) && (1 == PAR_CFG_ENABLE_ID) */
#if defined(AUTOGEN_PM_MSH_CMD_SET) && (1 == PAR_CFG_ENABLE_ID)
    rt_kprintf("  set <id> <value>        Set scalar parameter value by ID\n");
    rt_kprintf("  set <id>,<value>        Alternate compact syntax\n");
    rt_kprintf("  note: object rows use typed firmware APIs only\n");
#endif /* defined(AUTOGEN_PM_MSH_CMD_SET) && (1 == PAR_CFG_ENABLE_ID) */
#if defined(AUTOGEN_PM_MSH_CMD_DEF) && (1 == PAR_CFG_ENABLE_ID)
    rt_kprintf("  def <id>                Restore one parameter to default\n");
#endif /* defined(AUTOGEN_PM_MSH_CMD_DEF) && (1 == PAR_CFG_ENABLE_ID) */
#if defined(AUTOGEN_PM_MSH_CMD_DEF_ALL)
    rt_kprintf("  def_all                 Restore all parameters to default\n");
#endif /* defined(AUTOGEN_PM_MSH_CMD_DEF_ALL) */
#if defined(AUTOGEN_PM_MSH_CMD_SAVE) && (1 == PAR_CFG_NVM_EN)
    rt_kprintf("  save                    Save all persistent parameters\n");
#endif /* defined(AUTOGEN_PM_MSH_CMD_SAVE) && (1 == PAR_CFG_NVM_EN) */
#if defined(AUTOGEN_PM_MSH_CMD_SAVE_CLEAN) && (1 == PAR_CFG_NVM_EN)
    rt_kprintf("  save_clean              Rewrite managed parameter NVM area\n");
#endif /* defined(AUTOGEN_PM_MSH_CMD_SAVE_CLEAN) && (1 == PAR_CFG_NVM_EN) */
#if defined(AUTOGEN_PM_MSH_CMD_JSON)
    rt_kprintf("  json                    Export parameter table as JSON\n");
#endif /* defined(AUTOGEN_PM_MSH_CMD_JSON) */
}

#if defined(AUTOGEN_PM_MSH_CMD_INFO)
/**
 * @brief Print the parameter table in CSV-like text form.
 */
static void par_shell_cmd_info(void)
{
    par_num_t par_num;
    const char *last_group = RT_NULL;

#if (1 == PAR_CFG_ENABLE_ROLE_POLICY)
    rt_kprintf(";ID,Name,Value,Def,Min,Max,Unit,Type,Access,ReadRoles,WriteRoles,Persistance,Description\n");
#else
    rt_kprintf(";ID,Name,Value,Def,Min,Max,Unit,Type,Access,Persistance,Description\n");
#endif /* (1 == PAR_CFG_ENABLE_ROLE_POLICY) */
    rt_kprintf(": \n");

    for (par_num = 0U; par_num < ePAR_NUM_OF; par_num++)
    {
        const par_cfg_t *cfg = par_get_config(par_num);
        par_type_t value = {0};
        par_status_t value_status;
        char cur_buf[32];
        char def_buf[32];
        char min_buf[32];
        char max_buf[32];
        uint16_t id = 0U;
        const char *name = "";
        const char *unit = "";
        const char *desc = "";
        const char *access = "none";
#if (1 == PAR_CFG_ENABLE_ROLE_POLICY)
        char read_roles_buf[48];
        char write_roles_buf[48];
        const char *read_roles = "none";
        const char *write_roles = "none";
#endif /* (1 == PAR_CFG_ENABLE_ROLE_POLICY) */
        unsigned int persistent = 0U;

        if (cfg == RT_NULL)
        {
            continue;
        }

        value_status = par_shell_get_value(par_num, &value);

#if (1 == PAR_CFG_ENABLE_ID)
        id = cfg->id;
#endif /* (1 == PAR_CFG_ENABLE_ID) */
#if (1 == PAR_CFG_ENABLE_NAME)
        if (cfg->name != RT_NULL)
        {
            name = cfg->name;
        }
#endif /* (1 == PAR_CFG_ENABLE_NAME) */
#if (1 == PAR_CFG_ENABLE_UNIT)
        if (cfg->unit != RT_NULL)
        {
            unit = cfg->unit;
        }
#endif /* (1 == PAR_CFG_ENABLE_UNIT) */
#if (1 == PAR_CFG_ENABLE_DESC)
        if (cfg->desc != RT_NULL)
        {
            desc = cfg->desc;
        }
#endif /* (1 == PAR_CFG_ENABLE_DESC) */
#if (1 == PAR_CFG_ENABLE_ACCESS)
        access = par_shell_access_str(cfg->access);
#endif /* (1 == PAR_CFG_ENABLE_ACCESS) */
#if (1 == PAR_CFG_ENABLE_ROLE_POLICY)
        read_roles = par_shell_roles_to_cstr(cfg->read_roles, read_roles_buf, sizeof(read_roles_buf));
        write_roles = par_shell_roles_to_cstr(cfg->write_roles, write_roles_buf, sizeof(write_roles_buf));
#endif /* (1 == PAR_CFG_ENABLE_ROLE_POLICY) */
#if (1 == PAR_CFG_NVM_EN)
        persistent = cfg->persistent ? 1U : 0U;
#endif /* (1 == PAR_CFG_NVM_EN) */

        last_group = par_shell_print_group_marker(par_num, last_group);

        rt_kprintf("%u,%s,%s,%s,",
                   (unsigned int)id,
                   name,
                   par_shell_current_value_to_cstr(par_num, cfg->type, value_status, &value, cur_buf, sizeof(cur_buf)),
                   par_shell_default_to_cstr(cfg, def_buf, sizeof(def_buf)));
#if (1 == PAR_CFG_ENABLE_RANGE)
        rt_kprintf("%s,%s,",
                   par_shell_min_to_cstr(cfg, min_buf, sizeof(min_buf)),
                   par_shell_max_to_cstr(cfg, max_buf, sizeof(max_buf)));
#else
        rt_kprintf(",,");
#endif /* (1 == PAR_CFG_ENABLE_RANGE) */
#if (1 == PAR_CFG_ENABLE_ROLE_POLICY)
        rt_kprintf("%s,%u,%s,%s,%s,%u,%s\n",
                   unit,
                   par_shell_type_num(cfg->type),
                   access,
                   read_roles,
                   write_roles,
                   persistent,
                   desc);
#else
        rt_kprintf("%s,%u,%s,%u,%s\n",
                   unit,
                   par_shell_type_num(cfg->type),
                   access,
                   persistent,
                   desc);
#endif /* (1 == PAR_CFG_ENABLE_ROLE_POLICY) */
    }

    rt_kprintf(";END\n");
}
#endif /* defined(AUTOGEN_PM_MSH_CMD_INFO) */

#if defined(AUTOGEN_PM_MSH_CMD_GET) && (1 == PAR_CFG_ENABLE_ID)
/**
 * @brief Execute the shell get command.
 * @param argc Argument count.
 * @param argv Argument vector.
 */
static void par_shell_cmd_get(const int argc, char **argv)
{
    par_shell_target_t target;
    par_type_t value = {0};
    char value_buf[32];
    par_status_t status;

    if ((argc != 3) || !par_shell_resolve_target(argv[2], &target))
    {
        rt_kprintf("ERR, usage: par get <id>\n");
        return;
    }

#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
    if (true == par_shell_type_is_object(par_get_type(target.par_num)))
    {
#if (1 == PAR_SHELL_OBJECT_GET_ENABLED)
        status = par_shell_print_object_get(target.par_num, par_get_type(target.par_num));
        if (status != ePAR_OK)
        {
            par_shell_print_status("par_get_scalar", status);
        }
#else
        status = par_shell_get_value(target.par_num, &value);
        if (status != ePAR_OK)
        {
            par_shell_print_status("par_get_scalar", status);
        }
        else
        {
            rt_kprintf("ERR, object payload display requires AUTOGEN_PM_MSH_CMD_GET_OBJECT and RT_USING_HEAP\n");
        }
#endif /* (1 == PAR_SHELL_OBJECT_GET_ENABLED) */
        return;
    }
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

    status = par_shell_get_value(target.par_num, &value);
    if (status == ePAR_OK)
    {
        rt_kprintf("OK,PAR_GET=%s\n", par_shell_value_to_cstr(par_get_type(target.par_num), &value, value_buf, sizeof(value_buf)));
    }
    else
    {
        par_shell_print_status("par_get_scalar", status);
    }
}
#endif /* defined(AUTOGEN_PM_MSH_CMD_GET) && (1 == PAR_CFG_ENABLE_ID) */

#if defined(AUTOGEN_PM_MSH_CMD_SET) && (1 == PAR_CFG_ENABLE_ID)
/**
 * @brief Execute the shell set command.
 * @param argc Argument count.
 * @param argv Argument vector.
 */
static void par_shell_cmd_set(const int argc, char **argv)
{
    par_shell_target_t target;
    par_type_t value = {0};
    char value_buf[32];
    char pair_buf[96];
    const char *value_str = RT_NULL;
    par_status_t status;

    if (!par_shell_parse_set_args(argc, argv, &target, &value_str, pair_buf, sizeof(pair_buf)))
    {
        rt_kprintf("ERR, usage: par set <id> <value>\n");
        return;
    }

#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
    if (true == par_shell_type_is_object(par_get_type(target.par_num)))
    {
        rt_kprintf("ERR, object parameters do not support shell set\n");
        return;
    }
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

    if (!par_shell_parse_value(value_str, par_get_type(target.par_num), &value))
    {
        rt_kprintf("ERR, usage: par set <id> <value>\n");
        return;
    }

#if (1 == PAR_CFG_ENABLE_ROLE_POLICY)
    if (false == par_shell_can_write(target.par_num))
    {
        rt_kprintf("ERR, parameter is not writable for current shell roles\n");
        return;
    }
#elif (1 == PAR_CFG_ENABLE_ACCESS)
    if (0U == ((uint32_t)par_get_access(target.par_num) & (uint32_t)ePAR_ACCESS_WRITE))
    {
        rt_kprintf("ERR, parameter is not externally writable\n");
        return;
    }
#endif /* (1 == PAR_CFG_ENABLE_ROLE_POLICY) */

    switch (par_get_type(target.par_num))
    {
    case ePAR_TYPE_U8:
        status = par_set_scalar(target.par_num, &value.u8);
        break;
    case ePAR_TYPE_U16:
        status = par_set_scalar(target.par_num, &value.u16);
        break;
    case ePAR_TYPE_U32:
        status = par_set_scalar(target.par_num, &value.u32);
        break;
    case ePAR_TYPE_I8:
        status = par_set_scalar(target.par_num, &value.i8);
        break;
    case ePAR_TYPE_I16:
        status = par_set_scalar(target.par_num, &value.i16);
        break;
    case ePAR_TYPE_I32:
        status = par_set_scalar(target.par_num, &value.i32);
        break;
    case ePAR_TYPE_F32:
        status = par_set_scalar(target.par_num, &value.f32);
        break;
    default:
        status = ePAR_ERROR_TYPE;
        break;
    }

    if (status == ePAR_OK)
    {
        rt_kprintf("OK,PAR_SET=%s\n",
                   par_shell_value_to_cstr(par_get_type(target.par_num), &value, value_buf, sizeof(value_buf)));
    }
    else if (status == ePAR_WAR_LIMITED)
    {
        par_type_t applied_value = {0};

        if (par_shell_get_value(target.par_num, &applied_value) == ePAR_OK)
        {
            rt_kprintf("WAR,PAR_SET=%s\n",
                       par_shell_value_to_cstr(par_get_type(target.par_num), &applied_value, value_buf, sizeof(value_buf)));
        }
        rt_kprintf("WAR, value limited to configured range\n");
    }
    else
    {
        par_shell_print_status("par_set_scalar", status);
    }
}
#endif /* defined(AUTOGEN_PM_MSH_CMD_SET) && (1 == PAR_CFG_ENABLE_ID) */

#if (1 == PAR_CFG_ENABLE_ROLE_POLICY)
/**
 * @brief Execute the shell role-management command.
 * @param argc Argument count.
 * @param argv Argument vector.
 */
static void par_shell_cmd_role(const int argc, char **argv)
{
    char roles_buf[48];
    par_role_t roles = ePAR_ROLE_NONE;

    if (argc == 2)
    {
        rt_kprintf("OK,shell_roles=%s\n", par_shell_roles_to_cstr(par_shell_get_roles(), roles_buf, sizeof(roles_buf)));
        return;
    }

    if ((argc == 3) && (strcmp(argv[2], "clear") == 0))
    {
        gs_par_shell_roles = ePAR_ROLE_NONE;
        rt_kprintf("OK,shell_roles=%s\n", par_shell_roles_to_cstr(gs_par_shell_roles, roles_buf, sizeof(roles_buf)));
        return;
    }

    if ((argc == 4) && (strcmp(argv[2], "set") == 0))
    {
        if (false == par_shell_parse_roles(argv[3], &roles))
        {
            rt_kprintf("ERR, usage: par role set <public|service|developer|manufacturing|all|none>[|...]\n");
            return;
        }
        gs_par_shell_roles = roles;
        rt_kprintf("OK,shell_roles=%s\n", par_shell_roles_to_cstr(gs_par_shell_roles, roles_buf, sizeof(roles_buf)));
        return;
    }

    if ((argc == 4) && (strcmp(argv[2], "add") == 0))
    {
        if (false == par_shell_parse_roles(argv[3], &roles))
        {
            rt_kprintf("ERR, usage: par role add <public|service|developer|manufacturing>[|...]\n");
            return;
        }
        gs_par_shell_roles = (par_role_t)((uint32_t)gs_par_shell_roles | (uint32_t)roles);
        rt_kprintf("OK,shell_roles=%s\n", par_shell_roles_to_cstr(gs_par_shell_roles, roles_buf, sizeof(roles_buf)));
        return;
    }

    if ((argc == 4) && (strcmp(argv[2], "del") == 0))
    {
        if (false == par_shell_parse_roles(argv[3], &roles))
        {
            rt_kprintf("ERR, usage: par role del <public|service|developer|manufacturing>[|...]\n");
            return;
        }
        gs_par_shell_roles = (par_role_t)((uint32_t)gs_par_shell_roles & ~((uint32_t)roles));
        rt_kprintf("OK,shell_roles=%s\n", par_shell_roles_to_cstr(gs_par_shell_roles, roles_buf, sizeof(roles_buf)));
        return;
    }

    rt_kprintf("ERR, usage: par role [set|add|del <roles> | clear]\n");
}
#endif /* (1 == PAR_CFG_ENABLE_ROLE_POLICY) */

#if defined(AUTOGEN_PM_MSH_CMD_DEF) && (1 == PAR_CFG_ENABLE_ID)
/**
 * @brief Execute the shell default-restore command for one parameter.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @note Default restore is treated as a maintenance command and intentionally
 * uses the core default-restore path without shell write-role checks.
 */
static void par_shell_cmd_def(const int argc, char **argv)
{
    par_shell_target_t target;
    par_status_t status;

    if ((argc != 3) || !par_shell_resolve_target(argv[2], &target))
    {
        rt_kprintf("ERR, usage: par def <id>\n");
        return;
    }

    status = par_set_to_default(target.par_num);
    if (status == ePAR_OK)
    {
        rt_kprintf("OK, parameter %u set to default\n", (unsigned int)target.id);
    }
    else
    {
        par_shell_print_status("par_def", status);
    }
}
#endif /* defined(AUTOGEN_PM_MSH_CMD_DEF) && (1 == PAR_CFG_ENABLE_ID) */

#if defined(AUTOGEN_PM_MSH_CMD_DEF_ALL)
/**
 * @brief Execute the shell restore-all-defaults command.
 * @param argc Argument count.
 * @note Bulk default restore is treated as a maintenance command and
 * intentionally uses the core default-restore path without shell write-role
 * checks.
 */
static void par_shell_cmd_def_all(const int argc)
{
    par_status_t status;

    if (argc != 2)
    {
        rt_kprintf("ERR, usage: par def_all\n");
        return;
    }

    status = par_set_all_to_default();
    if (status == ePAR_OK)
    {
        rt_kprintf("OK, all parameters set to default\n");
    }
    else
    {
        par_shell_print_status("par_def_all", status);
    }
}
#endif /* defined(AUTOGEN_PM_MSH_CMD_DEF_ALL) */

#if defined(AUTOGEN_PM_MSH_CMD_SAVE) && (1 == PAR_CFG_NVM_EN)
/**
 * @brief Execute the shell save-all command.
 * @param argc Argument count.
 */
static void par_shell_cmd_save(const int argc)
{
    par_status_t status;

    if (argc != 2)
    {
        rt_kprintf("ERR, usage: par save\n");
        return;
    }

    status = par_save_all();
    if (status == ePAR_OK)
    {
        rt_kprintf("OK, parameters stored to NVM\n");
    }
    else
    {
        par_shell_print_status("par_save", status);
    }
}
#endif /* defined(AUTOGEN_PM_MSH_CMD_SAVE) && (1 == PAR_CFG_NVM_EN) */

#if defined(AUTOGEN_PM_MSH_CMD_SAVE_CLEAN) && (1 == PAR_CFG_NVM_EN)
/**
 * @brief Execute the shell clean-and-save command.
 * @param argc Argument count.
 */
static void par_shell_cmd_save_clean(const int argc)
{
    par_status_t status;

    if (argc != 2)
    {
        rt_kprintf("ERR, usage: par save_clean\n");
        return;
    }

    status = par_save_clean();
    if (status == ePAR_OK)
    {
        rt_kprintf("OK, parameter NVM area cleaned\n");
    }
    else
    {
        par_shell_print_status("par_save_clean", status);
    }
}
#endif /* defined(AUTOGEN_PM_MSH_CMD_SAVE_CLEAN) && (1 == PAR_CFG_NVM_EN) */

#if defined(AUTOGEN_PM_MSH_CMD_JSON)
/**
 * @brief Export the parameter table as JSON.
 */
static void par_shell_cmd_json(void)
{
    par_num_t par_num;
    bool first_item = true;

    rt_kprintf("{\"count\":%u,\"items\":[", (unsigned int)ePAR_NUM_OF);
    for (par_num = 0U; par_num < ePAR_NUM_OF; par_num++)
    {
        const par_cfg_t *cfg = par_get_config(par_num);
        par_type_t value = {0};
        par_status_t value_status;
        char cur_buf[32];
        char def_buf[32];
        char min_buf[32];
        char max_buf[32];
        uint16_t id = 0U;
        const char *name = "";
        const char *unit = "";
        const char *desc = "";
        const char *access = "na";
#if (1 == PAR_CFG_ENABLE_ROLE_POLICY)
        char read_roles_buf[48];
        char write_roles_buf[48];
        const char *read_roles = "none";
        const char *write_roles = "none";
#endif /* (1 == PAR_CFG_ENABLE_ROLE_POLICY) */
        unsigned int persistent = 0U;

        if (cfg == RT_NULL)
        {
            continue;
        }

        value_status = par_shell_get_value(par_num, &value);
#if (1 == PAR_CFG_ENABLE_ID)
        id = cfg->id;
#endif /* (1 == PAR_CFG_ENABLE_ID) */
#if (1 == PAR_CFG_ENABLE_NAME)
        if (cfg->name != RT_NULL)
        {
            name = cfg->name;
        }
#endif /* (1 == PAR_CFG_ENABLE_NAME) */
#if (1 == PAR_CFG_ENABLE_UNIT)
        if (cfg->unit != RT_NULL)
        {
            unit = cfg->unit;
        }
#endif /* (1 == PAR_CFG_ENABLE_UNIT) */
#if (1 == PAR_CFG_ENABLE_DESC)
        if (cfg->desc != RT_NULL)
        {
            desc = cfg->desc;
        }
#endif /* (1 == PAR_CFG_ENABLE_DESC) */
#if (1 == PAR_CFG_ENABLE_ACCESS)
        access = par_shell_access_str(cfg->access);
#endif /* (1 == PAR_CFG_ENABLE_ACCESS) */
#if (1 == PAR_CFG_ENABLE_ROLE_POLICY)
        read_roles = par_shell_roles_to_cstr(cfg->read_roles, read_roles_buf, sizeof(read_roles_buf));
        write_roles = par_shell_roles_to_cstr(cfg->write_roles, write_roles_buf, sizeof(write_roles_buf));
#endif /* (1 == PAR_CFG_ENABLE_ROLE_POLICY) */
#if (1 == PAR_CFG_NVM_EN)
        persistent = cfg->persistent ? 1U : 0U;
#endif /* (1 == PAR_CFG_NVM_EN) */

        if (false == first_item)
        {
            rt_kprintf(",");
        }
        first_item = false;
        rt_kprintf("{\"num\":%u,\"id\":%u,\"name\":\"", (unsigned int)par_num, (unsigned int)id);
        par_shell_json_print_escaped(name);
        rt_kprintf("\",\"type\":\"");
        par_shell_json_print_escaped(par_shell_type_str(cfg->type));
        rt_kprintf("\",\"access\":\"");
        par_shell_json_print_escaped(access);
#if (1 == PAR_CFG_ENABLE_ROLE_POLICY)
        rt_kprintf("\",\"read_roles\":\"");
        par_shell_json_print_escaped(read_roles);
        rt_kprintf("\",\"write_roles\":\"");
        par_shell_json_print_escaped(write_roles);
#endif /* (1 == PAR_CFG_ENABLE_ROLE_POLICY) */
        rt_kprintf("\",\"persistent\":%u,\"unit\":\"", persistent);
        par_shell_json_print_escaped(unit);
        rt_kprintf("\",\"desc\":\"");
        par_shell_json_print_escaped(desc);
        rt_kprintf("\",\"value\":\"");
        par_shell_json_print_escaped(par_shell_current_value_to_cstr(par_num, cfg->type, value_status, &value, cur_buf, sizeof(cur_buf)));
        rt_kprintf("\",\"default\":\"");
        par_shell_json_print_escaped(par_shell_default_to_cstr(cfg, def_buf, sizeof(def_buf)));
#if (1 == PAR_CFG_ENABLE_RANGE)
        rt_kprintf("\",\"min\":\"");
        par_shell_json_print_escaped(par_shell_min_to_cstr(cfg, min_buf, sizeof(min_buf)));
        rt_kprintf("\",\"max\":\"");
        par_shell_json_print_escaped(par_shell_max_to_cstr(cfg, max_buf, sizeof(max_buf)));
        rt_kprintf("\"}");
#else
        rt_kprintf("\",\"min\":\"\",\"max\":\"\"}");
#endif /* (1 == PAR_CFG_ENABLE_RANGE) */
    }
    rt_kprintf("]}\n");
}
#endif /* defined(AUTOGEN_PM_MSH_CMD_JSON) */

/**
 * @brief Dispatch the top-level parameter shell command.
 * @param argc Argument count.
 * @param argv Argument vector.
 */
static void par_msh(int argc, char **argv)
{
    if (argc < 2)
    {
        par_shell_print_usage();
        return;
    }

    if ((strcmp(argv[1], "help") == 0) || (strcmp(argv[1], "--help") == 0) || (strcmp(argv[1], "-h") == 0))
    {
        par_shell_print_usage();
        return;
    }

#if defined(AUTOGEN_PM_MSH_CMD_INFO)
    if (strcmp(argv[1], "info") == 0)
    {
        if (argc != 2)
        {
            rt_kprintf("ERR, usage: par info\n");
            return;
        }
        par_shell_cmd_info();
        return;
    }
#endif /* defined(AUTOGEN_PM_MSH_CMD_INFO) */

#if defined(AUTOGEN_PM_MSH_CMD_GET) && (1 == PAR_CFG_ENABLE_ID)
    if (strcmp(argv[1], "get") == 0)
    {
        par_shell_cmd_get(argc, argv);
        return;
    }
#endif /* defined(AUTOGEN_PM_MSH_CMD_GET) && (1 == PAR_CFG_ENABLE_ID) */

#if defined(AUTOGEN_PM_MSH_CMD_SET) && (1 == PAR_CFG_ENABLE_ID)
    if (strcmp(argv[1], "set") == 0)
    {
        par_shell_cmd_set(argc, argv);
        return;
    }
#endif /* defined(AUTOGEN_PM_MSH_CMD_SET) && (1 == PAR_CFG_ENABLE_ID) */

#if (1 == PAR_CFG_ENABLE_ROLE_POLICY)
    if (strcmp(argv[1], "role") == 0)
    {
        par_shell_cmd_role(argc, argv);
        return;
    }
#endif /* (1 == PAR_CFG_ENABLE_ROLE_POLICY) */

#if defined(AUTOGEN_PM_MSH_CMD_DEF) && (1 == PAR_CFG_ENABLE_ID)
    if (strcmp(argv[1], "def") == 0)
    {
        par_shell_cmd_def(argc, argv);
        return;
    }
#endif /* defined(AUTOGEN_PM_MSH_CMD_DEF) && (1 == PAR_CFG_ENABLE_ID) */

#if defined(AUTOGEN_PM_MSH_CMD_DEF_ALL)
    if (strcmp(argv[1], "def_all") == 0)
    {
        par_shell_cmd_def_all(argc);
        return;
    }
#endif /* defined(AUTOGEN_PM_MSH_CMD_DEF_ALL) */

#if defined(AUTOGEN_PM_MSH_CMD_SAVE) && (1 == PAR_CFG_NVM_EN)
    if (strcmp(argv[1], "save") == 0)
    {
        par_shell_cmd_save(argc);
        return;
    }
#endif /* defined(AUTOGEN_PM_MSH_CMD_SAVE) && (1 == PAR_CFG_NVM_EN) */

#if defined(AUTOGEN_PM_MSH_CMD_SAVE_CLEAN) && (1 == PAR_CFG_NVM_EN)
    if (strcmp(argv[1], "save_clean") == 0)
    {
        par_shell_cmd_save_clean(argc);
        return;
    }
#endif /* defined(AUTOGEN_PM_MSH_CMD_SAVE_CLEAN) && (1 == PAR_CFG_NVM_EN) */

#if defined(AUTOGEN_PM_MSH_CMD_JSON)
    if (strcmp(argv[1], "json") == 0)
    {
        if (argc != 2)
        {
            rt_kprintf("ERR, usage: par json\n");
            return;
        }
        par_shell_cmd_json();
        return;
    }
#endif /* defined(AUTOGEN_PM_MSH_CMD_JSON) */

    rt_kprintf("ERR, unknown subcmd: %s\n", argv[1]);
    par_shell_print_usage();
}

MSH_CMD_EXPORT_ALIAS(par_msh, par, parameter manager shell tool)

#endif /* defined(AUTOGEN_PM_USING_MSH_TOOL) && defined(RT_USING_FINSH) */
