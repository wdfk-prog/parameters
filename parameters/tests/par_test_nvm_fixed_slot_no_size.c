/**
 * @file par_test_nvm_fixed_slot_no_size.c
 * @brief Provide layout-aware manual MSH helpers for fixed-slot-no-size scalar NVM validation.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "par_test.h"

#if defined(RT_USING_FINSH)
#include <finsh.h>
#endif /* defined(RT_USING_FINSH) */

#include "par.h"
#include "par_cfg_table_api.h"
#include "par_layout.h"
#include "par_nvm.h"
#include "par_nvm_layout.h"
#include "par_store_backend.h"

#if defined(AUTOGEN_PM_TEST_NVM_FIXED_SLOT_NO_SIZE)
#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_SCALAR_EN)
#if (1 == PAR_CFG_NVM_BACKEND_RTT_AT24CXX_EN)
#if (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE)

/**
 * @brief Serialized scalar NVM header size for the current scalar core format.
 */
#define T_HEADER_SIZE (0x0CU)

/**
 * @brief CRC byte offset inside one serialized fixed-slot-no-size record.
 */
#define T_RECORD_CRC_OFFSET (2U)

/**
 * @brief Payload start offset inside one serialized fixed-slot-no-size record.
 */
#define T_RECORD_PAYLOAD_OFFSET (3U)

/**
 * @brief Size descriptor offset inside one serialized fixed-slot-no-size record.
 */
#define T_RECORD_SIZE_OFFSET (0U)

/**
 * @brief Print one line from the fixed-slot-no-size MSH helper.
 */
#define T_PRINT(...) par_test_print(__VA_ARGS__)

/**
 * @brief Compile-time assertion helper for host and RT-Thread builds.
 */
#define T_STATIC_ASSERT(name, condition) typedef char name[(condition) ? 1 : -1]

T_STATIC_ASSERT(t_header_size_matches_core,
                (T_HEADER_SIZE == (sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t))));

/**
 * @brief Describe one persistent scalar slot resolved from the live parameter table.
 */
typedef struct
{
    par_num_t par_num;        /**< Live parameter number. */
    uint16_t id;              /**< External parameter ID, or 0 when IDs are disabled. */
    const char *name;         /**< Optional parameter name. */
    uint32_t record_offset;   /**< Backend-relative serialized record offset. */
    uint32_t record_size;     /**< Serialized record size in bytes. */
    uint8_t payload_size;     /**< Natural scalar payload size in bytes. */
} t_slot_info_t;

/**
 * @brief Convert one parameter status to a short diagnostic string.
 * @param status Parameter status value.
 * @return Constant status string.
 */
static const char *t_status_str(const par_status_t status)
{
    switch (status)
    {
    case ePAR_OK:
        return "OK";
    case ePAR_ERROR_INIT:
        return "ERROR INIT";
    case ePAR_ERROR_NVM:
        return "ERROR NVM";
    case ePAR_ERROR_CRC:
        return "ERROR CRC";
    case ePAR_ERROR_PARAM:
        return "ERROR PARAM";
    case ePAR_ERROR_TABLE_ID:
        return "ERROR TABLE ID";
    default:
        return "ERROR";
    }
}

/**
 * @brief Return true when a parameter status contains no error bits.
 * @param status Parameter status value.
 * @return true when no error bit is present.
 */
static bool t_status_ok(const par_status_t status)
{
    return (((uint32_t)status & (uint32_t)ePAR_STATUS_ERROR_MASK) == 0U);
}

/**
 * @brief Parse one uint32 command argument.
 * @param text Input string, accepted in decimal or 0x-prefixed hexadecimal form.
 * @param[out] p_value Parsed value.
 * @return true on success.
 */
static bool t_parse_u32(const char *text, uint32_t * const p_value)
{
    char *end = NULL;
    unsigned long value;

    if ((NULL == text) || (NULL == p_value) || ('\0' == text[0]) || ('-' == text[0]))
    {
        return false;
    }

    errno = 0;
    value = strtoul(text, &end, 0);
    if ((0 != errno) || (end == text) || ('\0' != *end) || (value > (unsigned long)UINT32_MAX))
    {
        return false;
    }

    *p_value = (uint32_t)value;
    return true;
}

/**
 * @brief Resolve and initialize the active storage backend.
 * @param[out] pp_store Active backend API.
 * @return Operation status.
 */
static par_status_t t_get_store(const par_store_backend_api_t ** const pp_store)
{
    par_status_t status;
    const par_store_backend_api_t *store = NULL;
    bool is_init = false;

    if (NULL == pp_store)
    {
        return ePAR_ERROR_PARAM;
    }

    *pp_store = NULL;
    status = par_store_backend_bind();
    if (ePAR_OK != status)
    {
        return ePAR_ERROR_INIT;
    }

    store = par_store_backend_get_api();
    if ((NULL == store) || (NULL == store->init) || (NULL == store->is_init) || (NULL == store->read) ||
        (NULL == store->write) || (NULL == store->sync))
    {
        return ePAR_ERROR_INIT;
    }

    store->is_init(&is_init);
    if (false == is_init)
    {
        status = store->init();
        if (ePAR_OK != status)
        {
            return ePAR_ERROR_INIT;
        }
    }

    *pp_store = store;
    return ePAR_OK;
}

/**
 * @brief Fill a runtime persistent-slot-to-parameter map from the live table.
 * @param[out] p_map Persistent slot map.
 * @return true when every compiled persistent scalar slot is mapped.
 */
static bool t_build_slot_map(par_num_t * const p_map)
{
    bool seen[PAR_PERSIST_SLOT_MAP_CAPACITY] = { false };
    uint32_t par_num;
    uint32_t mapped = 0U;

    if (NULL == p_map)
    {
        return false;
    }

    for (uint32_t i = 0U; i < (uint32_t)PAR_PERSIST_SLOT_MAP_CAPACITY; i++)
    {
        p_map[i] = 0U;
    }

    for (par_num = 0U; par_num < (uint32_t)ePAR_NUM_OF; par_num++)
    {
        const par_cfg_t *cfg = par_cfg_get((par_num_t)par_num);
        if ((NULL != cfg) && (true == par_test_type_is_scalar(cfg->type)) && (true == cfg->persistent) &&
            (cfg->persist_idx < (uint16_t)PAR_PERSISTENT_COMPILE_COUNT))
        {
            if (true == seen[cfg->persist_idx])
            {
                return false;
            }

            seen[cfg->persist_idx] = true;
            p_map[cfg->persist_idx] = (par_num_t)par_num;
            mapped++;
        }
    }

    return (mapped == (uint32_t)PAR_PERSISTENT_COMPILE_COUNT);
}

/**
 * @brief Resolve a persistent slot to layout-aware address and parameter metadata.
 * @param slot Persistent scalar slot index.
 * @param[out] p_info Resolved slot information.
 * @return true when the slot is valid and resolved.
 */
static bool t_get_slot_info(const uint32_t slot, t_slot_info_t * const p_info)
{
    par_num_t slot_map[PAR_PERSIST_SLOT_MAP_CAPACITY];
    const par_nvm_layout_api_t *layout = NULL;
    par_num_t par_num;
    const par_cfg_t *cfg;

    if ((NULL == p_info) || (slot >= (uint32_t)PAR_PERSISTENT_COMPILE_COUNT) || (false == t_build_slot_map(slot_map)))
    {
        return false;
    }

    layout = par_nvm_layout_init();
    if ((NULL == layout) || (NULL == layout->addr_from_persist_idx) || (NULL == layout->record_size_from_par_num))
    {
        return false;
    }

    par_num = slot_map[slot];
    cfg = par_cfg_get(par_num);
    if ((NULL == cfg) || (false == par_test_type_is_scalar(cfg->type)) || (false == cfg->persistent))
    {
        return false;
    }

    p_info->par_num = par_num;
#if (1 == PAR_CFG_ENABLE_ID)
    p_info->id = cfg->id;
#else
    p_info->id = 0U;
#endif /* (1 == PAR_CFG_ENABLE_ID) */
#if (1 == PAR_CFG_ENABLE_NAME)
    p_info->name = (NULL != cfg->name) ? cfg->name : "";
#else
    p_info->name = "";
#endif /* (1 == PAR_CFG_ENABLE_NAME) */
    p_info->record_offset = layout->addr_from_persist_idx(T_HEADER_SIZE, (uint16_t)slot, slot_map);
    p_info->record_size = layout->record_size_from_par_num(par_num);
    p_info->payload_size = par_nvm_layout_payload_size_from_par_num(par_num);
    return true;
}

/**
 * @brief XOR one byte inside the scalar NVM backend window and verify readback.
 * @param offset Backend-relative byte offset inside the parameter NVM window.
 * @param mask XOR mask.
 * @param[out] p_old_value Original byte value.
 * @param[out] p_new_value New byte value.
 * @return Operation status.
 */
static par_status_t t_flip_u8(const uint32_t offset,
                              const uint8_t mask,
                              uint8_t * const p_old_value,
                              uint8_t * const p_new_value)
{
    const par_store_backend_api_t *store = NULL;
    uint8_t old_value = 0U;
    uint8_t new_value = 0U;
    par_status_t status;

    if ((NULL == p_old_value) || (NULL == p_new_value))
    {
        return ePAR_ERROR_PARAM;
    }

    status = t_get_store(&store);
    if (ePAR_OK != status)
    {
        return status;
    }

    status = store->read(offset, 1U, &old_value);
    if (ePAR_OK != status)
    {
        return status;
    }

    new_value = (uint8_t)(old_value ^ mask);
    status = store->write(offset, 1U, &new_value);
    if (ePAR_OK == status)
    {
        status = store->sync();
    }
    if (ePAR_OK == status)
    {
        uint8_t readback = 0U;
        status = store->read(offset, 1U, &readback);
        if ((ePAR_OK == status) && (readback != new_value))
        {
            status = ePAR_ERROR_NVM;
        }
    }

    if (ePAR_OK == status)
    {
        *p_old_value = old_value;
        *p_new_value = new_value;
    }

    return status;
}

/**
 * @brief Print command usage.
 */
static void t_print_usage(void)
{
    T_PRINT("usage:\n");
    T_PRINT("  par_nvm_fslot_no_size info\n");
    T_PRINT("  par_nvm_fslot_no_size slot <slot>\n");
    T_PRINT("  par_nvm_fslot_no_size corrupt_slot_crc <slot> [mask]\n");
    T_PRINT("  par_nvm_fslot_no_size corrupt_payload <slot> <payload_offset> [mask]\n");
}

/**
 * @brief Print the compiled fixed-slot-no-size NVM layout summary.
 * @return Shell command status.
 */
static int t_cmd_info(void)
{
    const par_store_backend_api_t *store = NULL;
    par_status_t status = t_get_store(&store);
    uint32_t scalar_bytes = T_HEADER_SIZE;
    uint32_t resolved_slots = 0U;

    for (uint32_t slot = 0U; slot < (uint32_t)PAR_PERSISTENT_COMPILE_COUNT; slot++)
    {
        t_slot_info_t info;
        if (true == t_get_slot_info(slot, &info))
        {
            scalar_bytes += info.record_size;
            resolved_slots++;
        }
    }

    T_PRINT("layout=fixed_slot_no_size\n");
    T_PRINT("backend=%s status=%s(0x%04X)\n",
            ((NULL != store) && (NULL != store->name)) ? store->name : "unknown",
            t_status_str(status),
            (unsigned)status);
    T_PRINT("header_size=0x%02X\n", (unsigned)T_HEADER_SIZE);
    T_PRINT("first_slot_offset=0x%02X\n", (unsigned)T_HEADER_SIZE);
    T_PRINT("record_crc_offset=%u\n", (unsigned)T_RECORD_CRC_OFFSET);
    T_PRINT("record_payload_offset=%u\n", (unsigned)T_RECORD_PAYLOAD_OFFSET);
    T_PRINT("persistent_count=%u\n", (unsigned)PAR_PERSISTENT_COMPILE_COUNT);
    T_PRINT("resolved_slots=%lu\n", (unsigned long)resolved_slots);
    T_PRINT("scalar_image_size=0x%08lX\n", (unsigned long)scalar_bytes);
#ifdef PAR_CFG_RTT_AT24_BASE_ADDR
    T_PRINT("at24_window_base=0x%08lX\n", (unsigned long)PAR_CFG_RTT_AT24_BASE_ADDR);
#endif /* defined(PAR_CFG_RTT_AT24_BASE_ADDR) */
#ifdef PAR_CFG_RTT_AT24_SIZE
    T_PRINT("at24_window_size=0x%08lX\n", (unsigned long)PAR_CFG_RTT_AT24_SIZE);
#endif /* defined(PAR_CFG_RTT_AT24_SIZE) */
    if ((uint32_t)PAR_PERSISTENT_COMPILE_COUNT != resolved_slots)
    {
        T_PRINT("ERR scalar persistent slot map invalid, resolved=%lu expected=%u\n",
                (unsigned long)resolved_slots,
                (unsigned)PAR_PERSISTENT_COMPILE_COUNT);
        return -1;
    }

    return t_status_ok(status) ? 0 : -1;
}

/**
 * @brief Print one fixed-slot-no-size slot address calculation.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Shell command status.
 */
static int t_cmd_slot(const int argc, char **argv)
{
    uint32_t slot = 0U;
    t_slot_info_t info;

    if ((argc < 3) || (false == t_parse_u32(argv[2], &slot)))
    {
        T_PRINT("ERR invalid slot\n");
        return -1;
    }
    if (false == t_get_slot_info(slot, &info))
    {
        T_PRINT("ERR slot out of range or scalar persistent slot map invalid, max=%u\n", (unsigned)PAR_PERSISTENT_COMPILE_COUNT);
        return -1;
    }

    T_PRINT("slot=%lu offset=0x%08lX crc_offset=0x%08lX payload_offset=0x%08lX record_size=%lu payload_size=%u\n",
            (unsigned long)slot,
            (unsigned long)info.record_offset,
            (unsigned long)(info.record_offset + T_RECORD_CRC_OFFSET),
            (unsigned long)(info.record_offset + T_RECORD_PAYLOAD_OFFSET),
            (unsigned long)info.record_size,
            (unsigned)info.payload_size);
    T_PRINT("par_num=%u id=%u name=%s\n", (unsigned)info.par_num, (unsigned)info.id, info.name);
    return 0;
}

/**
 * @brief Corrupt the CRC byte of one fixed-slot-no-size slot.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Shell command status.
 */
static int t_cmd_corrupt_slot_crc(const int argc, char **argv)
{
    uint32_t slot = 0U;
    uint32_t mask = 0x01U;
    uint8_t old_value = 0U;
    uint8_t new_value = 0U;
    par_status_t status;
    t_slot_info_t info;

    if ((argc < 3) || (false == t_parse_u32(argv[2], &slot)) || ((argc > 3) && (false == t_parse_u32(argv[3], &mask))) ||
        (mask > 0xFFU))
    {
        T_PRINT("ERR usage: par_nvm_fslot_no_size corrupt_slot_crc <slot> [mask]\n");
        return -1;
    }
    if (false == t_get_slot_info(slot, &info))
    {
        T_PRINT("ERR slot out of range or scalar persistent slot map invalid, max=%u\n", (unsigned)PAR_PERSISTENT_COMPILE_COUNT);
        return -1;
    }

    status = t_flip_u8(info.record_offset + T_RECORD_CRC_OFFSET, (uint8_t)mask, &old_value, &new_value);
    T_PRINT("corrupt_slot_crc slot=%lu crc_offset=0x%08lX old=0x%02X new=0x%02X status=%s(0x%04X)\n",
            (unsigned long)slot,
            (unsigned long)(info.record_offset + T_RECORD_CRC_OFFSET),
            (unsigned)old_value,
            (unsigned)new_value,
            t_status_str(status),
            (unsigned)status);
    T_PRINT("manual next step: reboot and check slot CRC recovery logs\n");
    return t_status_ok(status) ? 0 : -1;
}

/**
 * @brief Corrupt one serialized payload byte of a fixed-slot-no-size slot.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Shell command status.
 */
static int t_cmd_corrupt_payload(const int argc, char **argv)
{
    uint32_t slot = 0U;
    uint32_t payload_offset = 0U;
    uint32_t mask = 0x01U;
    uint8_t old_value = 0U;
    uint8_t new_value = 0U;
    par_status_t status;
    t_slot_info_t info;

    if ((argc < 4) || (false == t_parse_u32(argv[2], &slot)) || (false == t_parse_u32(argv[3], &payload_offset)) ||
        ((argc > 4) && (false == t_parse_u32(argv[4], &mask))) || (mask > 0xFFU))
    {
        T_PRINT("ERR usage: par_nvm_fslot_no_size corrupt_payload <slot> <payload_offset> [mask]\n");
        return -1;
    }
    if (false == t_get_slot_info(slot, &info))
    {
        T_PRINT("ERR slot out of range or scalar persistent slot map invalid, max=%u\n", (unsigned)PAR_PERSISTENT_COMPILE_COUNT);
        return -1;
    }
    if (payload_offset >= (uint32_t)info.payload_size)
    {
        T_PRINT("ERR payload_offset out of range, payload_size=%u\n", (unsigned)info.payload_size);
        return -1;
    }

    status = t_flip_u8(info.record_offset + T_RECORD_PAYLOAD_OFFSET + payload_offset, (uint8_t)mask, &old_value, &new_value);
    T_PRINT("corrupt_payload slot=%lu payload_offset=%lu byte_offset=0x%08lX old=0x%02X new=0x%02X status=%s(0x%04X)\n",
            (unsigned long)slot,
            (unsigned long)payload_offset,
            (unsigned long)(info.record_offset + T_RECORD_PAYLOAD_OFFSET + payload_offset),
            (unsigned)old_value,
            (unsigned)new_value,
            t_status_str(status),
            (unsigned)status);
    T_PRINT("manual next step: reboot and check CRC or value recovery logs\n");
    return t_status_ok(status) ? 0 : -1;
}

/**
 * @brief MSH entry point for fixed-slot-no-size NVM manual tests.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 on success; otherwise a negative error code.
 */
int par_test_nvm_fslot_no_size_exec(int argc, char **argv)
{
    if (argc < 2)
    {
        t_print_usage();
        return -1;
    }

    if (0 == strcmp(argv[1], "info"))
    {
        return t_cmd_info();
    }
    if (0 == strcmp(argv[1], "slot"))
    {
        return t_cmd_slot(argc, argv);
    }
    if (0 == strcmp(argv[1], "corrupt_slot_crc"))
    {
        return t_cmd_corrupt_slot_crc(argc, argv);
    }
    if (0 == strcmp(argv[1], "corrupt_payload"))
    {
        return t_cmd_corrupt_payload(argc, argv);
    }
    t_print_usage();
    return -1;
}

#if defined(RT_USING_FINSH)
/**
 * @brief MSH wrapper for fixed-slot-no-size NVM manual tests.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 on success; otherwise a negative error code.
 */
static int par_test_nvm_fslot_no_size_msh(int argc, char **argv)
{
    par_test_bind_rt_console();
    return par_test_nvm_fslot_no_size_exec(argc, argv);
}

/**
 * @brief Option-completion IDs for par_nvm_fslot_no_size subcommands.
 */
typedef enum
{
    T_OPT_INFO = 1,       /**< Print layout information. */
    T_OPT_SLOT,           /**< Print one persistent slot. */
    T_OPT_CORRUPT_CRC,    /**< Corrupt one slot CRC byte. */
    T_OPT_CORRUPT_PAYLOAD /**< Corrupt one payload byte. */
} t_opt_id_t;

CMD_OPTIONS_NODE_START(par_test_nvm_fslot_no_size_msh)
CMD_OPTIONS_NODE(T_OPT_INFO, info, print layout information)
CMD_OPTIONS_NODE(T_OPT_SLOT, slot, print one persistent slot)
CMD_OPTIONS_NODE(T_OPT_CORRUPT_CRC, corrupt_slot_crc, corrupt one slot CRC)
CMD_OPTIONS_NODE(T_OPT_CORRUPT_PAYLOAD, corrupt_payload, corrupt one payload byte)
CMD_OPTIONS_NODE_END
MSH_CMD_EXPORT_ALIAS(par_test_nvm_fslot_no_size_msh, par_nvm_fslot_no_size, fixed-slot-no-size NVM manual tests, optenable);
#endif /* defined(RT_USING_FINSH) */

#endif /* (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE) */
#endif /* (1 == PAR_CFG_NVM_BACKEND_RTT_AT24CXX_EN) */
#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_SCALAR_EN) */
#endif /* defined(AUTOGEN_PM_TEST_NVM_FIXED_SLOT_NO_SIZE) */
