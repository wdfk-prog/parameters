/**
 * @file par_test_nvm_raw.c
 * @brief Provide layout-neutral manual MSH helpers for raw NVM backend validation.
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
#include "par_nvm.h"
#include "par_store_backend.h"

#if defined(AUTOGEN_PM_TEST_NVM_RAW_HELPER)
#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_SCALAR_EN)

/**
 * @brief Header signature byte offset used by the raw corruption helper.
 */
#define PAR_TEST_NVM_RAW_HEADER_SIGNATURE_OFFSET (0U)

/**
 * @brief Maximum number of bytes accepted by one raw dump command.
 */
#define PAR_TEST_NVM_RAW_DUMP_MAX_LEN (256U)

/**
 * @brief Hex dump line width.
 */
#define PAR_TEST_NVM_RAW_DUMP_LINE_SIZE (16U)

/**
 * @brief Print one line from the raw NVM MSH helper.
 */
#define PAR_TEST_NVM_RAW_PRINT(...) par_test_print(__VA_ARGS__)

/**
 * @brief Convert the configured scalar record layout to a diagnostic string.
 * @return Constant layout string.
 */
static const char *par_test_nvm_raw_layout_str(void)
{
#if (1 == PAR_CFG_NVM_SCALAR_EN)
#if (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE)
    return "fixed_slot_with_size";
#elif (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE)
    return "fixed_slot_no_size";
#elif (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_COMPACT_PAYLOAD)
    return "compact_payload";
#elif (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_PAYLOAD_ONLY)
    return "fixed_payload_only";
#elif (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY)
    return "grouped_payload_only";
#else
    return "unknown";
#endif /* (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE) */
#else
    return "not_enabled";
#endif /* (1 == PAR_CFG_NVM_SCALAR_EN) */
}

/**
 * @brief Convert one parameter status to a short diagnostic string.
 * @param status Parameter status value.
 * @return Constant status string.
 */
static const char *par_test_nvm_raw_status_str(const par_status_t status)
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
static bool par_test_nvm_raw_status_ok(const par_status_t status)
{
    return (((uint32_t)status & (uint32_t)ePAR_STATUS_ERROR_MASK) == 0U);
}

/**
 * @brief Parse one uint32 command argument.
 * @param text Input string, accepted in decimal or 0x-prefixed hexadecimal form.
 * @param[out] p_value Parsed value.
 * @return true on success.
 */
static bool par_test_nvm_raw_parse_u32(const char *text, uint32_t * const p_value)
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
 * @brief Parse one byte command argument.
 * @param text Input string.
 * @param[out] p_value Parsed byte value.
 * @return true on success.
 */
static bool par_test_nvm_raw_parse_u8(const char *text, uint8_t * const p_value)
{
    uint32_t value = 0U;

    if ((NULL == p_value) || (false == par_test_nvm_raw_parse_u32(text, &value)) || (value > UINT8_MAX))
    {
        return false;
    }

    *p_value = (uint8_t)value;
    return true;
}

/**
 * @brief Resolve and initialize the active raw storage backend.
 * @param[out] pp_store Active backend API.
 * @return Operation status.
 */
static par_status_t par_test_nvm_raw_get_store(const par_store_backend_api_t ** const pp_store)
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
 * @brief Write one byte to the raw NVM backend and verify readback.
 * @param offset Backend-relative byte offset inside the parameter NVM window.
 * @param value Byte value to write.
 * @param p_readback Optional readback byte destination.
 * @return Operation status.
 */
static par_status_t par_test_nvm_raw_write_u8(const uint32_t offset, const uint8_t value, uint8_t * const p_readback)
{
    const par_store_backend_api_t *store = NULL;
    uint8_t readback = 0U;
    par_status_t status;

    status = par_test_nvm_raw_get_store(&store);
    if (ePAR_OK != status)
    {
        return status;
    }

    status = store->write(offset, 1U, &value);
    if (ePAR_OK == status)
    {
        status = store->sync();
    }
    if (ePAR_OK == status)
    {
        status = store->read(offset, 1U, &readback);
    }
    if (ePAR_OK == status)
    {
        if (NULL != p_readback)
        {
            *p_readback = readback;
        }
        if (readback != value)
        {
            status = ePAR_ERROR_NVM;
        }
    }

    return status;
}

/**
 * @brief XOR one byte inside the raw NVM backend and verify readback.
 * @param offset Backend-relative byte offset inside the parameter NVM window.
 * @param mask XOR mask.
 * @param[out] p_old_value Original byte value.
 * @param[out] p_new_value New byte value.
 * @return Operation status.
 */
static par_status_t par_test_nvm_raw_flip_u8(const uint32_t offset,
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

    status = par_test_nvm_raw_get_store(&store);
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
static void par_test_nvm_raw_print_usage(void)
{
    PAR_TEST_NVM_RAW_PRINT("usage:\n");
    PAR_TEST_NVM_RAW_PRINT("  par_nvm_raw info\n");
    PAR_TEST_NVM_RAW_PRINT("  par_nvm_raw lut\n");
    PAR_TEST_NVM_RAW_PRINT("  par_nvm_raw dump <offset> <len>\n");
    PAR_TEST_NVM_RAW_PRINT("  par_nvm_raw poke <offset> <value>\n");
    PAR_TEST_NVM_RAW_PRINT("  par_nvm_raw flip <offset> <mask>\n");
    PAR_TEST_NVM_RAW_PRINT("  par_nvm_raw corrupt_header\n");
}

/**
 * @brief Print the compiled raw NVM backend summary.
 * @return Shell command status.
 */
static int par_test_nvm_raw_cmd_info(void)
{
    const par_store_backend_api_t *store = NULL;
    const par_status_t status = par_test_nvm_raw_get_store(&store);

    PAR_TEST_NVM_RAW_PRINT("backend=%s status=%s(0x%04X)\n",
                           ((NULL != store) && (NULL != store->name)) ? store->name : "unknown",
                           par_test_nvm_raw_status_str(status),
                           (unsigned)status);
    PAR_TEST_NVM_RAW_PRINT("scalar_nvm=%u\n", (unsigned)PAR_CFG_NVM_SCALAR_EN);
    PAR_TEST_NVM_RAW_PRINT("object_nvm=%u\n", (unsigned)PAR_CFG_NVM_OBJECT_EN);
#if (1 == PAR_CFG_NVM_OBJECT_EN)
    PAR_TEST_NVM_RAW_PRINT("object_store_mode=%s\n",
                           (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) ? "shared" : "dedicated");
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) */
    PAR_TEST_NVM_RAW_PRINT("scalar_record_layout=%s\n", par_test_nvm_raw_layout_str());
    PAR_TEST_NVM_RAW_PRINT("persistent_count=%u\n", (unsigned)PAR_PERSISTENT_COMPILE_COUNT);
#ifdef PAR_CFG_RTT_AT24_BASE_ADDR
    PAR_TEST_NVM_RAW_PRINT("at24_window_base=0x%08lX\n", (unsigned long)PAR_CFG_RTT_AT24_BASE_ADDR);
#endif /* defined(PAR_CFG_RTT_AT24_BASE_ADDR) */
#ifdef PAR_CFG_RTT_AT24_SIZE
    PAR_TEST_NVM_RAW_PRINT("at24_window_size=0x%08lX\n", (unsigned long)PAR_CFG_RTT_AT24_SIZE);
#endif /* defined(PAR_CFG_RTT_AT24_SIZE) */
#if (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN)
    PAR_TEST_NVM_RAW_PRINT("flash_ee_logical_size=0x%08lX\n", (unsigned long)PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE);
#endif /* (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN) */

    return par_test_nvm_raw_status_ok(status) ? 0 : -1;
}

/**
 * @brief Dump raw bytes from the NVM backend window.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Shell command status.
 */
static int par_test_nvm_raw_cmd_dump(const int argc, char **argv)
{
    const par_store_backend_api_t *store = NULL;
    uint32_t offset = 0U;
    uint32_t len = 0U;
    uint32_t dumped = 0U;
    par_status_t status;

    if ((argc < 4) || (false == par_test_nvm_raw_parse_u32(argv[2], &offset)) ||
        (false == par_test_nvm_raw_parse_u32(argv[3], &len)) || (0U == len) ||
        (len > PAR_TEST_NVM_RAW_DUMP_MAX_LEN) || (offset > (UINT32_MAX - (len - 1U))))
    {
        PAR_TEST_NVM_RAW_PRINT("ERR usage: par_nvm_raw dump <offset> <len>, len=1..%u\n",
                               (unsigned)PAR_TEST_NVM_RAW_DUMP_MAX_LEN);
        return -1;
    }

    status = par_test_nvm_raw_get_store(&store);
    if (ePAR_OK != status)
    {
        PAR_TEST_NVM_RAW_PRINT("ERR backend status=%s(0x%04X)\n", par_test_nvm_raw_status_str(status), (unsigned)status);
        return -1;
    }

    while (dumped < len)
    {
        uint8_t buf[PAR_TEST_NVM_RAW_DUMP_LINE_SIZE] = { 0U };
        uint32_t i;
        const uint32_t line_len = ((len - dumped) > PAR_TEST_NVM_RAW_DUMP_LINE_SIZE) ?
                                      PAR_TEST_NVM_RAW_DUMP_LINE_SIZE :
                                      (len - dumped);

        status = store->read(offset + dumped, line_len, buf);
        if (ePAR_OK != status)
        {
            PAR_TEST_NVM_RAW_PRINT("ERR read offset=0x%08lX status=%s(0x%04X)\n",
                                   (unsigned long)(offset + dumped),
                                   par_test_nvm_raw_status_str(status),
                                   (unsigned)status);
            return -1;
        }

        PAR_TEST_NVM_RAW_PRINT("0x%08lX:", (unsigned long)(offset + dumped));
        for (i = 0U; i < line_len; i++)
        {
            PAR_TEST_NVM_RAW_PRINT(" %02X", (unsigned)buf[i]);
        }
        PAR_TEST_NVM_RAW_PRINT("\n");

        dumped += line_len;
    }

    return 0;
}

/**
 * @brief Write one raw byte to the NVM backend window.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Shell command status.
 */
static int par_test_nvm_raw_cmd_poke(const int argc, char **argv)
{
    uint32_t offset = 0U;
    uint8_t value = 0U;
    uint8_t readback = 0U;
    par_status_t status;

    if ((argc < 4) || (false == par_test_nvm_raw_parse_u32(argv[2], &offset)) ||
        (false == par_test_nvm_raw_parse_u8(argv[3], &value)))
    {
        PAR_TEST_NVM_RAW_PRINT("ERR usage: par_nvm_raw poke <offset> <value>\n");
        return -1;
    }

    status = par_test_nvm_raw_write_u8(offset, value, &readback);
    PAR_TEST_NVM_RAW_PRINT("poke offset=0x%08lX value=0x%02X readback=0x%02X status=%s(0x%04X)\n",
                           (unsigned long)offset,
                           (unsigned)value,
                           (unsigned)readback,
                           par_test_nvm_raw_status_str(status),
                           (unsigned)status);
    return par_test_nvm_raw_status_ok(status) ? 0 : -1;
}

/**
 * @brief XOR one raw byte in the NVM backend window.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Shell command status.
 */
static int par_test_nvm_raw_cmd_flip(const int argc, char **argv)
{
    uint32_t offset = 0U;
    uint8_t mask = 0U;
    uint8_t old_value = 0U;
    uint8_t new_value = 0U;
    par_status_t status;

    if ((argc < 4) || (false == par_test_nvm_raw_parse_u32(argv[2], &offset)) ||
        (false == par_test_nvm_raw_parse_u8(argv[3], &mask)))
    {
        PAR_TEST_NVM_RAW_PRINT("ERR usage: par_nvm_raw flip <offset> <mask>\n");
        return -1;
    }

    status = par_test_nvm_raw_flip_u8(offset, mask, &old_value, &new_value);
    PAR_TEST_NVM_RAW_PRINT("flip offset=0x%08lX old=0x%02X mask=0x%02X new=0x%02X status=%s(0x%04X)\n",
                           (unsigned long)offset,
                           (unsigned)old_value,
                           (unsigned)mask,
                           (unsigned)new_value,
                           par_test_nvm_raw_status_str(status),
                           (unsigned)status);
    return par_test_nvm_raw_status_ok(status) ? 0 : -1;
}

/**
 * @brief Corrupt the first byte of the scalar NVM header signature.
 * @return Shell command status.
 */
static int par_test_nvm_raw_cmd_corrupt_header(void)
{
    uint8_t readback = 0U;
    const par_status_t status = par_test_nvm_raw_write_u8(PAR_TEST_NVM_RAW_HEADER_SIGNATURE_OFFSET, 0x00U, &readback);

    PAR_TEST_NVM_RAW_PRINT("corrupt_header offset=0x%08X value=0x00 readback=0x%02X status=%s(0x%04X)\n",
                           (unsigned)PAR_TEST_NVM_RAW_HEADER_SIGNATURE_OFFSET,
                           (unsigned)readback,
                           par_test_nvm_raw_status_str(status),
                           (unsigned)status);
    PAR_TEST_NVM_RAW_PRINT("manual next step: reboot and check header recovery logs\n");
    return par_test_nvm_raw_status_ok(status) ? 0 : -1;
}

/**
 * @brief Print the NVM LUT through the existing debug hook.
 * @return Shell command status.
 */
static int par_test_nvm_raw_cmd_lut(void)
{
    par_status_t status;

    if (false == par_is_init())
    {
        status = par_init();
        if (false == par_test_nvm_raw_status_ok(status))
        {
            PAR_TEST_NVM_RAW_PRINT("ERR par_init status=%s(0x%04X)\n", par_test_nvm_raw_status_str(status), (unsigned)status);
            return -1;
        }
    }

    status = par_nvm_print_nvm_lut();
    if (false == par_test_nvm_raw_status_ok(status))
    {
        PAR_TEST_NVM_RAW_PRINT("ERR lut status=%s(0x%04X), enable AUTOGEN_PM_USING_DEBUG if required\n",
                               par_test_nvm_raw_status_str(status),
                               (unsigned)status);
        return -1;
    }

    return 0;
}

/**
 * @brief MSH entry point for raw NVM manual tests.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 on success, otherwise -1.
 */
int par_test_nvm_raw_exec(int argc, char **argv)
{
    if (argc < 2)
    {
        par_test_nvm_raw_print_usage();
        return -1;
    }

    if (0 == strcmp(argv[1], "info"))
    {
        return par_test_nvm_raw_cmd_info();
    }
    if (0 == strcmp(argv[1], "lut"))
    {
        return par_test_nvm_raw_cmd_lut();
    }
    if (0 == strcmp(argv[1], "dump"))
    {
        return par_test_nvm_raw_cmd_dump(argc, argv);
    }
    if (0 == strcmp(argv[1], "poke"))
    {
        return par_test_nvm_raw_cmd_poke(argc, argv);
    }
    if (0 == strcmp(argv[1], "flip"))
    {
        return par_test_nvm_raw_cmd_flip(argc, argv);
    }
    if (0 == strcmp(argv[1], "corrupt_header"))
    {
        return par_test_nvm_raw_cmd_corrupt_header();
    }

    par_test_nvm_raw_print_usage();
    return -1;
}
#if defined(RT_USING_FINSH)
/**
 * @brief MSH entry point for raw NVM manual tests.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 on success; otherwise a negative error code.
 */
static int par_test_nvm_raw_msh(int argc, char **argv)
{
    par_test_bind_rt_console();
    return par_test_nvm_raw_exec(argc, argv);
}
/**
 * @brief Option-completion IDs for par_nvm_raw subcommands.
 */
typedef enum
{
    PAR_TEST_NVM_RAW_OPT_INFO = 1,    /**< Print raw NVM backend information. */
    PAR_TEST_NVM_RAW_OPT_LUT,         /**< Print the scalar NVM look-up table. */
    PAR_TEST_NVM_RAW_OPT_DUMP,        /**< Dump bytes from the raw NVM backend. */
    PAR_TEST_NVM_RAW_OPT_POKE,        /**< Write one raw byte. */
    PAR_TEST_NVM_RAW_OPT_FLIP,        /**< XOR one raw byte. */
    PAR_TEST_NVM_RAW_OPT_CORRUPT_HDR  /**< Corrupt the scalar NVM header signature. */
} par_test_nvm_raw_opt_id_t;

CMD_OPTIONS_NODE_START(par_test_nvm_raw_msh)
CMD_OPTIONS_NODE(PAR_TEST_NVM_RAW_OPT_INFO, info, print raw NVM backend information)
CMD_OPTIONS_NODE(PAR_TEST_NVM_RAW_OPT_LUT, lut, print scalar NVM lookup table)
CMD_OPTIONS_NODE(PAR_TEST_NVM_RAW_OPT_DUMP, dump, dump raw NVM bytes)
CMD_OPTIONS_NODE(PAR_TEST_NVM_RAW_OPT_POKE, poke, write one raw NVM byte)
CMD_OPTIONS_NODE(PAR_TEST_NVM_RAW_OPT_FLIP, flip, xor one raw NVM byte)
CMD_OPTIONS_NODE(PAR_TEST_NVM_RAW_OPT_CORRUPT_HDR, corrupt_header, corrupt scalar NVM header signature)
CMD_OPTIONS_NODE_END
MSH_CMD_EXPORT_ALIAS(par_test_nvm_raw_msh, par_nvm_raw, raw NVM backend manual tests, optenable);
#endif /* defined(RT_USING_FINSH) */

#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_SCALAR_EN) */
#endif /* defined(AUTOGEN_PM_TEST_NVM_RAW_HELPER) */
