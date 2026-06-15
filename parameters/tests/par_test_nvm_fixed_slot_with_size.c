/**
 * @file par_test_nvm_fixed_slot_with_size.c
 * @brief Provide layout-aware manual MSH helpers for fixed-slot-with-size scalar NVM validation.
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

#if defined(AUTOGEN_PM_TEST_NVM_FIXED_SLOT_WITH_SIZE)
#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_SCALAR_EN)
#if (1 == PAR_CFG_NVM_BACKEND_RTT_AT24CXX_EN)
#if (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE)

/**
 * @brief Serialized scalar NVM header size for the current scalar core format.
 */
#define PAR_TEST_NVM_FSLOT_HEADER_SIZE (0x0CU)

/**
 * @brief Serialized fixed-slot-with-size record size.
 */
#define PAR_TEST_NVM_FSLOT_RECORD_SIZE (8U)

/**
 * @brief Offset of the first fixed-slot-with-size record after the header.
 */
#define PAR_TEST_NVM_FSLOT_FIRST_RECORD_OFFSET (PAR_TEST_NVM_FSLOT_HEADER_SIZE)

/**
 * @brief CRC byte offset inside one fixed-slot-with-size record.
 */
#define PAR_TEST_NVM_FSLOT_RECORD_CRC_OFFSET (3U)

/**
 * @brief Print one line from the fixed-slot-with-size MSH helper.
 */
#define PAR_TEST_NVM_FSLOT_PRINT(...) par_test_print(__VA_ARGS__)

/**
 * @brief Compile-time assertion helper for host and RT-Thread builds.
 */
#define PAR_TEST_NVM_FSLOT_STATIC_ASSERT(name, condition) typedef char name[(condition) ? 1 : -1]

PAR_TEST_NVM_FSLOT_STATIC_ASSERT(par_test_nvm_fslot_header_size_matches_core,
                                 (PAR_TEST_NVM_FSLOT_HEADER_SIZE ==
                                  (sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t))));
PAR_TEST_NVM_FSLOT_STATIC_ASSERT(par_test_nvm_fslot_record_size_matches_layout,
                                 (PAR_TEST_NVM_FSLOT_RECORD_SIZE == sizeof(par_nvm_layout_fixed_slot_with_size_record_t)));

/**
 * @brief Describe one persistent slot resolved from the live parameter table.
 */
typedef struct
{
    bool found;             /**< True when a live parameter owns the slot. */
    par_num_t par_num;      /**< Live parameter number. */
    uint16_t id;            /**< External parameter ID. */
    const char *name;       /**< Optional parameter name. */
} par_test_nvm_fslot_target_t;

/**
 * @brief Convert one parameter status to a short diagnostic string.
 * @param status Parameter status value.
 * @return Constant status string.
 */
static const char *par_test_nvm_fslot_status_str(const par_status_t status)
{
    switch (status)
    {
    case ePAR_OK:
        return "OK";
    case ePAR_ERROR:
        return "ERROR";
    case ePAR_ERROR_INIT:
        return "ERROR INIT";
    case ePAR_ERROR_NVM:
        return "ERROR NVM";
    case ePAR_ERROR_CRC:
        return "ERROR CRC";
    case ePAR_ERROR_TYPE:
        return "ERROR TYPE";
    case ePAR_ERROR_MUTEX:
        return "ERROR MUTEX";
    case ePAR_ERROR_VALUE:
        return "ERROR VALUE";
    case ePAR_ERROR_PARAM:
        return "ERROR PARAM";
    case ePAR_ERROR_PAR_NUM:
        return "ERROR PAR NUM";
    case ePAR_ERROR_ACCESS:
        return "ERROR ACCESS";
    case ePAR_ERROR_TABLE_ID:
        return "ERROR TABLE ID";
    default:
        return "MIXED";
    }
}

/**
 * @brief Return true when a parameter status contains no error bits.
 * @param status Parameter status value.
 * @return true when no error bit is present.
 */
static bool par_test_nvm_fslot_status_ok(const par_status_t status)
{
    return (((uint32_t)status & (uint32_t)ePAR_STATUS_ERROR_MASK) == 0U);
}

/**
 * @brief Parse one uint32 command argument.
 * @param text Input string, accepted in decimal or 0x-prefixed hexadecimal form.
 * @param[out] p_value Parsed value.
 * @return true on success.
 */
static bool par_test_nvm_fslot_parse_u32(const char *text, uint32_t * const p_value)
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
 * @brief Resolve and initialize the active storage backend for fixed-slot corruption.
 * @param[out] pp_store Active backend API.
 * @return Operation status.
 */
static par_status_t par_test_nvm_fslot_get_store(const par_store_backend_api_t ** const pp_store)
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
 * @brief Calculate one fixed-slot record start offset.
 * @param slot Persistent slot index.
 * @return Backend-relative record start offset.
 */
static uint32_t par_test_nvm_fslot_slot_offset(const uint32_t slot)
{
    return PAR_TEST_NVM_FSLOT_FIRST_RECORD_OFFSET + (slot * PAR_TEST_NVM_FSLOT_RECORD_SIZE);
}

/**
 * @brief Calculate one fixed-slot CRC byte offset.
 * @param slot Persistent slot index.
 * @return Backend-relative CRC byte offset.
 */
static uint32_t par_test_nvm_fslot_slot_crc_offset(const uint32_t slot)
{
    return par_test_nvm_fslot_slot_offset(slot) + PAR_TEST_NVM_FSLOT_RECORD_CRC_OFFSET;
}

/**
 * @brief Return true when a parameter type uses scalar storage.
 * @param type Parameter type.
 * @return true for scalar parameter types.
 */
static bool par_test_nvm_fslot_type_is_scalar(const par_type_list_t type)
{
    switch (type)
    {
    case ePAR_TYPE_U8:
    case ePAR_TYPE_U16:
    case ePAR_TYPE_U32:
    case ePAR_TYPE_I8:
    case ePAR_TYPE_I16:
    case ePAR_TYPE_I32:
    case ePAR_TYPE_F32:
        return true;
    default:
        return false;
    }
}

/**
 * @brief Resolve the live parameter entry that owns one persistent scalar slot.
 * @param slot Persistent slot index.
 * @param[out] p_target Resolved live parameter details.
 * @return true when a scalar parameter entry was found.
 */
static bool par_test_nvm_fslot_find_slot_target(const uint32_t slot, par_test_nvm_fslot_target_t * const p_target)
{
    uint32_t par_num;
    const uint32_t table_size = par_cfg_get_table_size();

    if (NULL == p_target)
    {
        return false;
    }

    p_target->found = false;
    p_target->par_num = 0U;
    p_target->id = 0U;
    p_target->name = "";

    for (par_num = 0U; par_num < table_size; par_num++)
    {
        const par_cfg_t *cfg = par_cfg_get((par_num_t)par_num);
        if ((NULL != cfg) && (true == par_test_nvm_fslot_type_is_scalar(cfg->type)) &&
            (true == cfg->persistent) && (cfg->persist_idx == (uint16_t)slot))
        {
            p_target->found = true;
            p_target->par_num = (par_num_t)par_num;
#if (1 == PAR_CFG_ENABLE_ID)
            p_target->id = cfg->id;
#else
            p_target->id = 0U;
#endif /* (1 == PAR_CFG_ENABLE_ID) */
#if (1 == PAR_CFG_ENABLE_NAME)
            p_target->name = (NULL != cfg->name) ? cfg->name : "";
#else
            p_target->name = "";
#endif /* (1 == PAR_CFG_ENABLE_NAME) */
            return true;
        }
    }

    return false;
}

/**
 * @brief XOR one byte inside the scalar NVM backend window and verify readback.
 * @param offset Backend-relative byte offset inside the parameter NVM window.
 * @param mask XOR mask.
 * @param[out] p_old_value Original byte value.
 * @param[out] p_new_value New byte value.
 * @return Operation status.
 */
static par_status_t par_test_nvm_fslot_flip_u8(const uint32_t offset,
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

    status = par_test_nvm_fslot_get_store(&store);
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
static void par_test_nvm_fslot_print_usage(void)
{
    PAR_TEST_NVM_FSLOT_PRINT("usage:\n");
    PAR_TEST_NVM_FSLOT_PRINT("  par_nvm_fslot info\n");
    PAR_TEST_NVM_FSLOT_PRINT("  par_nvm_fslot slot <slot>\n");
    PAR_TEST_NVM_FSLOT_PRINT("  par_nvm_fslot corrupt_slot_crc <slot>\n");
    PAR_TEST_NVM_FSLOT_PRINT("raw helper:\n");
    PAR_TEST_NVM_FSLOT_PRINT("  use par_nvm_raw dump|poke|flip|corrupt_header|lut for layout-neutral raw access\n");
}

/**
 * @brief Print the compiled fixed-slot-with-size NVM layout summary.
 * @return Shell command status.
 */
static int par_test_nvm_fslot_cmd_info(void)
{
    const par_store_backend_api_t *store = NULL;
    par_status_t status = par_test_nvm_fslot_get_store(&store);
    const uint32_t scalar_bytes = PAR_TEST_NVM_FSLOT_HEADER_SIZE +
                                  ((uint32_t)PAR_PERSISTENT_COMPILE_COUNT * PAR_TEST_NVM_FSLOT_RECORD_SIZE);

    PAR_TEST_NVM_FSLOT_PRINT("layout=fixed_slot_with_size\n");
    PAR_TEST_NVM_FSLOT_PRINT("backend=%s status=%s(0x%04X)\n",
                             ((NULL != store) && (NULL != store->name)) ? store->name : "unknown",
                             par_test_nvm_fslot_status_str(status),
                             (unsigned)status);
    PAR_TEST_NVM_FSLOT_PRINT("header_size=0x%02X\n", (unsigned)PAR_TEST_NVM_FSLOT_HEADER_SIZE);
    PAR_TEST_NVM_FSLOT_PRINT("first_slot_offset=0x%02X\n", (unsigned)PAR_TEST_NVM_FSLOT_FIRST_RECORD_OFFSET);
    PAR_TEST_NVM_FSLOT_PRINT("record_size=%u\n", (unsigned)PAR_TEST_NVM_FSLOT_RECORD_SIZE);
    PAR_TEST_NVM_FSLOT_PRINT("record_crc_offset=%u\n", (unsigned)PAR_TEST_NVM_FSLOT_RECORD_CRC_OFFSET);
    PAR_TEST_NVM_FSLOT_PRINT("persistent_count=%u\n", (unsigned)PAR_PERSISTENT_COMPILE_COUNT);
    PAR_TEST_NVM_FSLOT_PRINT("scalar_image_size=0x%08lX\n", (unsigned long)scalar_bytes);
#ifdef PAR_CFG_RTT_AT24_BASE_ADDR
    PAR_TEST_NVM_FSLOT_PRINT("at24_window_base=0x%08lX\n", (unsigned long)PAR_CFG_RTT_AT24_BASE_ADDR);
#endif /* defined(PAR_CFG_RTT_AT24_BASE_ADDR) */
#ifdef PAR_CFG_RTT_AT24_SIZE
    PAR_TEST_NVM_FSLOT_PRINT("at24_window_size=0x%08lX\n", (unsigned long)PAR_CFG_RTT_AT24_SIZE);
#endif /* defined(PAR_CFG_RTT_AT24_SIZE) */

    return par_test_nvm_fslot_status_ok(status) ? 0 : -1;
}

/**
 * @brief Print one fixed-slot address calculation.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Shell command status.
 */
static int par_test_nvm_fslot_cmd_slot(const int argc, char **argv)
{
    uint32_t slot = 0U;
    par_test_nvm_fslot_target_t target;

    if ((argc < 3) || (false == par_test_nvm_fslot_parse_u32(argv[2], &slot)))
    {
        PAR_TEST_NVM_FSLOT_PRINT("ERR invalid slot\n");
        return -1;
    }

    if (slot >= (uint32_t)PAR_PERSISTENT_COMPILE_COUNT)
    {
        PAR_TEST_NVM_FSLOT_PRINT("ERR slot out of range, max=%u\n", (unsigned)PAR_PERSISTENT_COMPILE_COUNT);
        return -1;
    }

    (void)par_test_nvm_fslot_find_slot_target(slot, &target);
    PAR_TEST_NVM_FSLOT_PRINT("slot=%lu offset=0x%08lX crc_offset=0x%08lX abs_crc_offset=0x%08lX\n",
                             (unsigned long)slot,
                             (unsigned long)par_test_nvm_fslot_slot_offset(slot),
                             (unsigned long)par_test_nvm_fslot_slot_crc_offset(slot),
                             (unsigned long)(PAR_CFG_RTT_AT24_BASE_ADDR + par_test_nvm_fslot_slot_crc_offset(slot)));
    if (true == target.found)
    {
        PAR_TEST_NVM_FSLOT_PRINT("par_num=%u id=%u name=%s\n",
                                 (unsigned)target.par_num,
                                 (unsigned)target.id,
                                 target.name);
    }
    else
    {
        PAR_TEST_NVM_FSLOT_PRINT("par_num=unknown\n");
    }

    return 0;
}

/**
 * @brief Corrupt the CRC byte of one fixed slot.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Shell command status.
 */
static int par_test_nvm_fslot_cmd_corrupt_slot_crc(const int argc, char **argv)
{
    uint32_t slot = 0U;
    uint8_t old_value = 0U;
    uint8_t new_value = 0U;
    uint32_t crc_offset;
    par_status_t status;

    if ((argc < 3) || (false == par_test_nvm_fslot_parse_u32(argv[2], &slot)))
    {
        PAR_TEST_NVM_FSLOT_PRINT("ERR usage: par_nvm_fslot corrupt_slot_crc <slot>\n");
        return -1;
    }

    if (slot >= (uint32_t)PAR_PERSISTENT_COMPILE_COUNT)
    {
        PAR_TEST_NVM_FSLOT_PRINT("ERR slot out of range, max=%u\n", (unsigned)PAR_PERSISTENT_COMPILE_COUNT);
        return -1;
    }

    crc_offset = par_test_nvm_fslot_slot_crc_offset(slot);
    status = par_test_nvm_fslot_flip_u8(crc_offset, 0x01U, &old_value, &new_value);
    PAR_TEST_NVM_FSLOT_PRINT("corrupt_slot_crc slot=%lu crc_offset=0x%08lX old=0x%02X new=0x%02X status=%s(0x%04X)\n",
                             (unsigned long)slot,
                             (unsigned long)crc_offset,
                             (unsigned)old_value,
                             (unsigned)new_value,
                             par_test_nvm_fslot_status_str(status),
                             (unsigned)status);
    PAR_TEST_NVM_FSLOT_PRINT("manual next step: reboot and check slot CRC recovery logs\n");
    return par_test_nvm_fslot_status_ok(status) ? 0 : -1;
}

/**
 * @brief MSH entry point for fixed-slot-with-size NVM manual tests.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 on success, otherwise -1.
 */
int par_test_nvm_fslot_exec(int argc, char **argv)
{
    if (argc < 2)
    {
        par_test_nvm_fslot_print_usage();
        return -1;
    }

    if (0 == strcmp(argv[1], "info"))
    {
        return par_test_nvm_fslot_cmd_info();
    }
    if (0 == strcmp(argv[1], "slot"))
    {
        return par_test_nvm_fslot_cmd_slot(argc, argv);
    }
    if (0 == strcmp(argv[1], "corrupt_slot_crc"))
    {
        return par_test_nvm_fslot_cmd_corrupt_slot_crc(argc, argv);
    }

    par_test_nvm_fslot_print_usage();
    return -1;
}
#if defined(RT_USING_FINSH)
/**
 * @brief MSH entry point for fixed-slot-with-size NVM manual tests.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 on success; otherwise a negative error code.
 */
static int par_test_nvm_fslot_msh(int argc, char **argv)
{
    par_test_bind_rt_console();
    return par_test_nvm_fslot_exec(argc, argv);
}
/**
 * @brief Option-completion IDs for par_nvm_fslot subcommands.
 */
typedef enum
{
    PAR_TEST_NVM_FSLOT_OPT_INFO = 1,       /**< Print fixed-slot layout information. */
    PAR_TEST_NVM_FSLOT_OPT_SLOT,           /**< Print one persistent slot. */
    PAR_TEST_NVM_FSLOT_OPT_CORRUPT_SLOT_CRC /**< Corrupt one slot CRC byte. */
} par_test_nvm_fslot_opt_id_t;

CMD_OPTIONS_NODE_START(par_test_nvm_fslot_msh)
CMD_OPTIONS_NODE(PAR_TEST_NVM_FSLOT_OPT_INFO, info, print fixed slot layout information)
CMD_OPTIONS_NODE(PAR_TEST_NVM_FSLOT_OPT_SLOT, slot, print one persistent slot)
CMD_OPTIONS_NODE(PAR_TEST_NVM_FSLOT_OPT_CORRUPT_SLOT_CRC, corrupt_slot_crc, corrupt one fixed slot CRC)
CMD_OPTIONS_NODE_END
MSH_CMD_EXPORT_ALIAS(par_test_nvm_fslot_msh, par_nvm_fslot, fixed-slot-with-size NVM manual tests, optenable);
#endif /* defined(RT_USING_FINSH) */

#endif /* (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE) */
#endif /* (1 == PAR_CFG_NVM_BACKEND_RTT_AT24CXX_EN) */
#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_SCALAR_EN) */
#endif /* defined(AUTOGEN_PM_TEST_NVM_FIXED_SLOT_WITH_SIZE) */
