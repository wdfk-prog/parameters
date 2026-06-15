/**
 * @file par_test_nvm_object.c
 * @brief Provide manual MSH helpers for object NVM validation.
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
#include "par_nvm.h"
#include "par_nvm_layout.h"
#include "par_nvm_object.h"
#include "par_nvm_object_store.h"
#include "par_store_backend.h"

#if defined(AUTOGEN_PM_TEST_NVM_OBJECT_HELPER)

#define PAR_TEST_NVM_OBJ_BUILD_ENABLED \
    ((1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_SCALAR_EN) && \
     (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED))

#if PAR_TEST_NVM_OBJ_BUILD_ENABLED

/**
 * @brief First scalar persistent-record address after the scalar NVM header.
 */
#define PAR_TEST_NVM_OBJ_SCALAR_FIRST_RECORD_OFFSET (0x0CU)

/**
 * @brief Serialized object block header size.
 */
#define PAR_TEST_NVM_OBJ_HEADER_SIZE (18U)

/**
 * @brief Serialized object record metadata header size.
 */
#define PAR_TEST_NVM_OBJ_RECORD_HEAD_SIZE (12U)

/**
 * @brief Signature byte offset inside the object persistence block header.
 */
#define PAR_TEST_NVM_OBJ_HEADER_SIGNATURE_OFFSET (0U)

/**
 * @brief Record CRC field byte offset inside one object record header.
 */
#define PAR_TEST_NVM_OBJ_RECORD_CRC_OFFSET (10U)

/**
 * @brief Offset of the payload bytes after one object record header.
 */
#define PAR_TEST_NVM_OBJ_RECORD_PAYLOAD_OFFSET (PAR_TEST_NVM_OBJ_RECORD_HEAD_SIZE)

/**
 * @brief Maximum number of bytes accepted by one object dump command.
 */
#define PAR_TEST_NVM_OBJ_DUMP_MAX_LEN (256U)

/**
 * @brief Hex dump line width.
 */
#define PAR_TEST_NVM_OBJ_DUMP_LINE_SIZE (16U)

/**
 * @brief Maximum payload bytes accepted by one set_hex command.
 */
#define PAR_TEST_NVM_OBJ_SET_HEX_MAX_LEN (64U)

/**
 * @brief Print one line from the object NVM MSH helper.
 */
#define PAR_TEST_NVM_OBJ_PRINT(...) par_test_print(__VA_ARGS__)

/**
 * @brief Compile-time assertion helper for host and RT-Thread builds.
 */
#define PAR_TEST_NVM_OBJ_STATIC_ASSERT(name, condition) typedef char name[(condition) ? 1 : -1]

PAR_TEST_NVM_OBJ_STATIC_ASSERT(par_test_nvm_obj_scalar_header_size_matches_core,
                               (PAR_TEST_NVM_OBJ_SCALAR_FIRST_RECORD_OFFSET ==
                                (sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t))));
PAR_TEST_NVM_OBJ_STATIC_ASSERT(par_test_nvm_obj_header_size_matches_object_core,
                               (PAR_TEST_NVM_OBJ_HEADER_SIZE ==
                                (sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t) +
                                 sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint16_t))));
PAR_TEST_NVM_OBJ_STATIC_ASSERT(par_test_nvm_obj_record_head_size_matches_object_core,
                               (PAR_TEST_NVM_OBJ_RECORD_HEAD_SIZE ==
                                (sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t) +
                                 sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t))));

#define PAR_TEST_NVM_OBJ_SCALAR_SLOT_ENTRY_SELECT(enum_, pers_)   PAR_TEST_NVM_OBJ_SCALAR_SLOT_ENTRY_SELECT_I(enum_, pers_)
#define PAR_TEST_NVM_OBJ_SCALAR_SLOT_ENTRY_SELECT_I(enum_, pers_) PAR_TEST_NVM_OBJ_SCALAR_SLOT_ENTRY_SELECT_##pers_(enum_)
#define PAR_TEST_NVM_OBJ_SCALAR_SLOT_ENTRY_SELECT_true(enum_)     [PAR_PERSIST_IDX_##enum_] = enum_,
#define PAR_TEST_NVM_OBJ_SCALAR_SLOT_ENTRY_SELECT_false(enum_)
#define PAR_TEST_NVM_OBJ_SCALAR_SLOT_ENTRY_SELECT_1(enum_) [PAR_PERSIST_IDX_##enum_] = enum_,
#define PAR_TEST_NVM_OBJ_SCALAR_SLOT_ENTRY_SELECT_0(enum_)
#define PAR_TEST_NVM_OBJ_SCALAR_SLOT_ITEM(...) \
    PAR_TEST_NVM_OBJ_SCALAR_SLOT_ENTRY_SELECT(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_PERS(__VA_ARGS__))
#define PAR_TEST_NVM_OBJ_NOP(...)
/**
 * @brief Compile-time scalar persistent-slot map used to resolve shared object block placement.
 */
static const par_num_t g_par_test_nvm_obj_scalar_slot_to_par_num[PAR_PERSIST_SLOT_MAP_CAPACITY] = {
#define PAR_ITEM_U8      PAR_TEST_NVM_OBJ_SCALAR_SLOT_ITEM
#define PAR_ITEM_U16     PAR_TEST_NVM_OBJ_SCALAR_SLOT_ITEM
#define PAR_ITEM_U32     PAR_TEST_NVM_OBJ_SCALAR_SLOT_ITEM
#define PAR_ITEM_I8      PAR_TEST_NVM_OBJ_SCALAR_SLOT_ITEM
#define PAR_ITEM_I16     PAR_TEST_NVM_OBJ_SCALAR_SLOT_ITEM
#define PAR_ITEM_I32     PAR_TEST_NVM_OBJ_SCALAR_SLOT_ITEM
#define PAR_ITEM_F32     PAR_TEST_NVM_OBJ_SCALAR_SLOT_ITEM
#define PAR_ITEM_STR     PAR_TEST_NVM_OBJ_NOP
#define PAR_ITEM_BYTES   PAR_TEST_NVM_OBJ_NOP
#define PAR_ITEM_ARR_U8  PAR_TEST_NVM_OBJ_NOP
#define PAR_ITEM_ARR_U16 PAR_TEST_NVM_OBJ_NOP
#define PAR_ITEM_ARR_U32 PAR_TEST_NVM_OBJ_NOP
#include "par_table.def"
#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32
#undef PAR_ITEM_STR
#undef PAR_ITEM_BYTES
#undef PAR_ITEM_ARR_U8
#undef PAR_ITEM_ARR_U16
#undef PAR_ITEM_ARR_U32
};
#undef PAR_TEST_NVM_OBJ_SCALAR_SLOT_ITEM
#undef PAR_TEST_NVM_OBJ_NOP
#undef PAR_TEST_NVM_OBJ_SCALAR_SLOT_ENTRY_SELECT
#undef PAR_TEST_NVM_OBJ_SCALAR_SLOT_ENTRY_SELECT_I
#undef PAR_TEST_NVM_OBJ_SCALAR_SLOT_ENTRY_SELECT_true
#undef PAR_TEST_NVM_OBJ_SCALAR_SLOT_ENTRY_SELECT_false
#undef PAR_TEST_NVM_OBJ_SCALAR_SLOT_ENTRY_SELECT_1
#undef PAR_TEST_NVM_OBJ_SCALAR_SLOT_ENTRY_SELECT_0

/**
 * @brief Describes one persistent object slot resolved from the live parameter table.
 */
typedef struct
{
    bool found;          /**< True when a live object parameter owns the slot. */
    par_num_t par_num;   /**< Live parameter number. */
    uint16_t id;         /**< External parameter ID. */
    par_type_list_t type; /**< Parameter type. */
    uint16_t elem_size;  /**< Object element size in bytes. */
    uint16_t min_len;    /**< Minimum payload length in bytes. */
    uint16_t capacity;   /**< Maximum payload length in bytes. */
    const char *name;    /**< Optional parameter name. */
} par_test_nvm_obj_target_t;

/**
 * @brief Convert one parameter status to a short diagnostic string.
 * @param status Parameter status value.
 * @return Constant status string.
 */
static const char *par_test_nvm_obj_status_str(const par_status_t status)
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
static bool par_test_nvm_obj_status_ok(const par_status_t status)
{
    return (((uint32_t)status & (uint32_t)ePAR_STATUS_ERROR_MASK) == 0U);
}

/**
 * @brief Convert one object parameter type to a diagnostic string.
 * @param type Parameter type.
 * @return Constant type string.
 */
static const char *par_test_nvm_obj_type_str(const par_type_list_t type)
{
    switch (type)
    {
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
    case ePAR_TYPE_STR:
        return "STR";
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
    case ePAR_TYPE_BYTES:
        return "BYTES";
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
    case ePAR_TYPE_ARR_U8:
        return "ARR_U8";
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
    case ePAR_TYPE_ARR_U16:
        return "ARR_U16";
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
    case ePAR_TYPE_ARR_U32:
        return "ARR_U32";
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */
    default:
        return "UNKNOWN";
    }
}

/**
 * @brief Return true when a parameter type uses object storage.
 * @param type Parameter type.
 * @return true for object parameter types.
 */
static bool par_test_nvm_obj_type_is_object(const par_type_list_t type)
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

/**
 * @brief Parse one uint32 command argument.
 * @param text Input string, accepted in decimal or 0x-prefixed hexadecimal form.
 * @param[out] p_value Parsed value.
 * @return true on success.
 */
static bool par_test_nvm_obj_parse_u32(const char *text, uint32_t * const p_value)
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
static bool par_test_nvm_obj_parse_u8(const char *text, uint8_t * const p_value)
{
    uint32_t value = 0U;

    if ((NULL == p_value) || (false == par_test_nvm_obj_parse_u32(text, &value)) || (value > UINT8_MAX))
    {
        return false;
    }

    *p_value = (uint8_t)value;
    return true;
}

/**
 * @brief Resolve the live parameter entry that owns one persistent object slot.
 * @param slot Persistent object slot index.
 * @param[out] p_target Resolved live parameter details.
 * @return true when an object parameter entry was found.
 */
static bool par_test_nvm_obj_find_slot_target(const uint32_t slot, par_test_nvm_obj_target_t * const p_target)
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
    p_target->type = ePAR_TYPE_U8;
    p_target->elem_size = 0U;
    p_target->min_len = 0U;
    p_target->capacity = 0U;
    p_target->name = "";

    for (par_num = 0U; par_num < table_size; par_num++)
    {
        const par_cfg_t *cfg = par_cfg_get((par_num_t)par_num);
        if ((NULL != cfg) && (true == par_test_nvm_obj_type_is_object(cfg->type)) &&
            (true == cfg->persistent) && (cfg->persist_idx == (uint16_t)slot))
        {
            p_target->found = true;
            p_target->par_num = (par_num_t)par_num;
#if (1 == PAR_CFG_ENABLE_ID)
            p_target->id = cfg->id;
#else
            p_target->id = 0U;
#endif /* (1 == PAR_CFG_ENABLE_ID) */
            p_target->type = cfg->type;
            p_target->elem_size = cfg->value_cfg.object.elem_size;
            p_target->min_len = cfg->value_cfg.object.range.min_len;
            p_target->capacity = cfg->value_cfg.object.range.max_len;
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
 * @brief Resolve one persistent object record start offset inside the object block.
 * @param slot Persistent object slot index.
 * @return Object-block-relative record start offset, or zero when slot is invalid.
 */
static uint32_t par_test_nvm_obj_record_offset_from_slot(const uint32_t slot)
{
    uint32_t offset = PAR_TEST_NVM_OBJ_HEADER_SIZE;
    uint32_t idx;

    for (idx = 0U; idx < slot; idx++)
    {
        par_test_nvm_obj_target_t target;

        if (false == par_test_nvm_obj_find_slot_target(idx, &target))
        {
            return 0U;
        }

        offset += PAR_TEST_NVM_OBJ_RECORD_HEAD_SIZE + (uint32_t)target.capacity;
    }

    return offset;
}

/**
 * @brief Initialize the object module when needed and resolve the active object backend.
 * @param[out] pp_store Active object storage backend API.
 * @return Operation status.
 */
static par_status_t par_test_nvm_obj_get_store(const par_store_backend_api_t ** const pp_store)
{
    par_status_t status;
    const par_store_backend_api_t *store = NULL;

    if (NULL == pp_store)
    {
        return ePAR_ERROR_PARAM;
    }

    *pp_store = NULL;
    if (false == par_is_init())
    {
        status = par_init();
        if (false == par_test_nvm_obj_status_ok(status))
        {
            return status;
        }
    }

    store = par_nvm_object_store_get_api();
    if ((NULL == store) || (NULL == store->read) || (NULL == store->write) || (NULL == store->sync))
    {
        return ePAR_ERROR_INIT;
    }

    *pp_store = store;
    return ePAR_OK;
}

/**
 * @brief Resolve the object persistence block base address in the active backend.
 * @param[out] p_base_addr Object block base address.
 * @return Operation status.
 */
static par_status_t par_test_nvm_obj_get_block_base(uint32_t * const p_base_addr)
{
    const par_nvm_layout_api_t *layout = NULL;
    uint32_t base_addr;

    if (NULL == p_base_addr)
    {
        return ePAR_ERROR_PARAM;
    }

    layout = par_nvm_layout_init();
    if ((0U < PAR_PERSISTENT_COMPILE_COUNT) && ((NULL == layout) || (NULL == layout->record_size_from_par_num)))
    {
        return ePAR_ERROR_INIT;
    }

    base_addr = par_nvm_object_get_block_addr(PAR_TEST_NVM_OBJ_SCALAR_FIRST_RECORD_OFFSET,
                                              layout,
                                              g_par_test_nvm_obj_scalar_slot_to_par_num);
    if ((0U == base_addr) && (false == par_nvm_object_block_addr_zero_is_valid()))
    {
        return ePAR_ERROR;
    }

    *p_base_addr = base_addr;
    return ePAR_OK;
}

/**
 * @brief Resolve the active object backend and object block base address.
 * @param[out] pp_store Active object storage backend API.
 * @param[out] p_base_addr Object block base address.
 * @return Operation status.
 */
static par_status_t par_test_nvm_obj_get_context(const par_store_backend_api_t ** const pp_store,
                                                 uint32_t * const p_base_addr)
{
    par_status_t status = par_test_nvm_obj_get_store(pp_store);

    if (ePAR_OK == status)
    {
        status = par_test_nvm_obj_get_block_base(p_base_addr);
    }

    return status;
}

/**
 * @brief Write one byte to the object persistence block and verify readback.
 * @param block_offset Object-block-relative byte offset.
 * @param value Byte value to write.
 * @param p_readback Optional readback byte destination.
 * @return Operation status.
 */
static par_status_t par_test_nvm_obj_write_u8(const uint32_t block_offset, const uint8_t value, uint8_t * const p_readback)
{
    const par_store_backend_api_t *store = NULL;
    uint32_t base_addr = 0U;
    uint8_t readback = 0U;
    par_status_t status;

    status = par_test_nvm_obj_get_context(&store, &base_addr);
    if (ePAR_OK != status)
    {
        return status;
    }

    status = store->write(base_addr + block_offset, 1U, &value);
    if (ePAR_OK == status)
    {
        status = store->sync();
    }
    if (ePAR_OK == status)
    {
        status = store->read(base_addr + block_offset, 1U, &readback);
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
 * @brief XOR one byte in the object persistence block and verify readback.
 * @param block_offset Object-block-relative byte offset.
 * @param mask XOR mask.
 * @param[out] p_old_value Original byte value.
 * @param[out] p_new_value New byte value.
 * @return Operation status.
 */
static par_status_t par_test_nvm_obj_flip_u8(const uint32_t block_offset,
                                             const uint8_t mask,
                                             uint8_t * const p_old_value,
                                             uint8_t * const p_new_value)
{
    const par_store_backend_api_t *store = NULL;
    uint32_t base_addr = 0U;
    uint8_t old_value = 0U;
    uint8_t new_value = 0U;
    par_status_t status;

    if ((NULL == p_old_value) || (NULL == p_new_value))
    {
        return ePAR_ERROR_PARAM;
    }

    status = par_test_nvm_obj_get_context(&store, &base_addr);
    if (ePAR_OK != status)
    {
        return status;
    }

    status = store->read(base_addr + block_offset, 1U, &old_value);
    if (ePAR_OK != status)
    {
        return status;
    }

    new_value = (uint8_t)(old_value ^ mask);
    status = store->write(base_addr + block_offset, 1U, &new_value);
    if (ePAR_OK == status)
    {
        status = store->sync();
    }
    if (ePAR_OK == status)
    {
        uint8_t readback = 0U;
        status = store->read(base_addr + block_offset, 1U, &readback);
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
static void par_test_nvm_obj_print_usage(void)
{
    PAR_TEST_NVM_OBJ_PRINT("usage:\n");
    PAR_TEST_NVM_OBJ_PRINT("  par_nvm_obj info\n");
    PAR_TEST_NVM_OBJ_PRINT("  par_nvm_obj list\n");
    PAR_TEST_NVM_OBJ_PRINT("  par_nvm_obj slot <slot>\n");
    PAR_TEST_NVM_OBJ_PRINT("  par_nvm_obj dump <block_offset> <len>\n");
    PAR_TEST_NVM_OBJ_PRINT("  par_nvm_obj poke <block_offset> <value>\n");
    PAR_TEST_NVM_OBJ_PRINT("  par_nvm_obj flip <block_offset> <mask>\n");
    PAR_TEST_NVM_OBJ_PRINT("  par_nvm_obj corrupt_header\n");
    PAR_TEST_NVM_OBJ_PRINT("  par_nvm_obj corrupt_record_crc <slot>\n");
    PAR_TEST_NVM_OBJ_PRINT("  par_nvm_obj corrupt_payload <slot> <payload_offset> [mask]\n");
    PAR_TEST_NVM_OBJ_PRINT("  par_nvm_obj set_ascii <id> <text>\n");
    PAR_TEST_NVM_OBJ_PRINT("  par_nvm_obj set_hex <id> <byte0> [byte1 ...]\n");
}

/**
 * @brief Print the compiled object NVM summary.
 * @return Shell command status.
 */
static int par_test_nvm_obj_cmd_info(void)
{
    const par_store_backend_api_t *store = NULL;
    uint32_t base_addr = 0U;
    const par_status_t status = par_test_nvm_obj_get_context(&store, &base_addr);

    PAR_TEST_NVM_OBJ_PRINT("object_nvm=1\n");
    PAR_TEST_NVM_OBJ_PRINT("backend=%s status=%s(0x%04X)\n",
                           ((NULL != store) && (NULL != store->name)) ? store->name : "unknown",
                           par_test_nvm_obj_status_str(status),
                           (unsigned)status);
    PAR_TEST_NVM_OBJ_PRINT("store_mode=%s\n",
                           (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) ? "shared" : "dedicated");
#if (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED)
    PAR_TEST_NVM_OBJ_PRINT("addr_mode=%s\n",
                           (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_AFTER_SCALAR) ? "after_scalar" : "fixed");
#endif /* (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) */
    PAR_TEST_NVM_OBJ_PRINT("object_count=%u\n", (unsigned)par_nvm_object_get_count());
    PAR_TEST_NVM_OBJ_PRINT("object_block_base=0x%08lX\n", (unsigned long)base_addr);
    PAR_TEST_NVM_OBJ_PRINT("object_block_size=0x%08lX\n", (unsigned long)par_nvm_object_get_block_size());
    PAR_TEST_NVM_OBJ_PRINT("header_size=0x%02X\n", (unsigned)PAR_TEST_NVM_OBJ_HEADER_SIZE);
    PAR_TEST_NVM_OBJ_PRINT("record_head_size=0x%02X\n", (unsigned)PAR_TEST_NVM_OBJ_RECORD_HEAD_SIZE);
    PAR_TEST_NVM_OBJ_PRINT("record_crc_offset=%u\n", (unsigned)PAR_TEST_NVM_OBJ_RECORD_CRC_OFFSET);
#ifdef PAR_CFG_RTT_AT24_BASE_ADDR
    PAR_TEST_NVM_OBJ_PRINT("at24_window_base=0x%08lX\n", (unsigned long)PAR_CFG_RTT_AT24_BASE_ADDR);
    PAR_TEST_NVM_OBJ_PRINT("at24_object_abs_base=0x%08lX\n", (unsigned long)(PAR_CFG_RTT_AT24_BASE_ADDR + base_addr));
#endif /* defined(PAR_CFG_RTT_AT24_BASE_ADDR) */

    return par_test_nvm_obj_status_ok(status) ? 0 : -1;
}

/**
 * @brief Print one object slot description.
 * @param slot Persistent object slot index.
 * @return Shell command status.
 */
static int par_test_nvm_obj_print_slot(const uint32_t slot)
{
    uint32_t base_addr = 0U;
    par_test_nvm_obj_target_t target;
    const uint32_t object_count = (uint32_t)par_nvm_object_get_count();
    par_status_t status;
    uint32_t record_offset;

    if (slot >= object_count)
    {
        PAR_TEST_NVM_OBJ_PRINT("ERR object slot out of range, max=%u\n", (unsigned)object_count);
        return -1;
    }

    if (false == par_test_nvm_obj_find_slot_target(slot, &target))
    {
        PAR_TEST_NVM_OBJ_PRINT("ERR object slot not found\n");
        return -1;
    }

    status = par_test_nvm_obj_get_block_base(&base_addr);
    if (ePAR_OK != status)
    {
        PAR_TEST_NVM_OBJ_PRINT("ERR object block base status=%s(0x%04X)\n",
                               par_test_nvm_obj_status_str(status),
                               (unsigned)status);
        return -1;
    }

    record_offset = par_test_nvm_obj_record_offset_from_slot(slot);
    PAR_TEST_NVM_OBJ_PRINT("slot=%lu record_offset=0x%08lX backend_addr=0x%08lX crc_offset=0x%08lX payload_offset=0x%08lX\n",
                           (unsigned long)slot,
                           (unsigned long)record_offset,
                           (unsigned long)(base_addr + record_offset),
                           (unsigned long)(record_offset + PAR_TEST_NVM_OBJ_RECORD_CRC_OFFSET),
                           (unsigned long)(record_offset + PAR_TEST_NVM_OBJ_RECORD_PAYLOAD_OFFSET));
#ifdef PAR_CFG_RTT_AT24_BASE_ADDR
    PAR_TEST_NVM_OBJ_PRINT(
        "abs_crc_offset=0x%08lX abs_payload_offset=0x%08lX\n",
        (unsigned long)(PAR_CFG_RTT_AT24_BASE_ADDR + base_addr + record_offset + PAR_TEST_NVM_OBJ_RECORD_CRC_OFFSET),
        (unsigned long)(PAR_CFG_RTT_AT24_BASE_ADDR + base_addr + record_offset + PAR_TEST_NVM_OBJ_RECORD_PAYLOAD_OFFSET));
#endif /* defined(PAR_CFG_RTT_AT24_BASE_ADDR) */
    PAR_TEST_NVM_OBJ_PRINT("par_num=%u id=%u type=%s elem_size=%u min_len=%u capacity=%u name=%s\n",
                           (unsigned)target.par_num,
                           (unsigned)target.id,
                           par_test_nvm_obj_type_str(target.type),
                           (unsigned)target.elem_size,
                           (unsigned)target.min_len,
                           (unsigned)target.capacity,
                           target.name);

    return 0;
}

/**
 * @brief Print one object slot description from command arguments.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Shell command status.
 */
static int par_test_nvm_obj_cmd_slot(const int argc, char **argv)
{
    uint32_t slot = 0U;

    if ((argc < 3) || (false == par_test_nvm_obj_parse_u32(argv[2], &slot)))
    {
        PAR_TEST_NVM_OBJ_PRINT("ERR invalid slot\n");
        return -1;
    }

    return par_test_nvm_obj_print_slot(slot);
}

/**
 * @brief Print all persistent object slots.
 * @return Shell command status.
 */
static int par_test_nvm_obj_cmd_list(void)
{
    const uint32_t object_count = (uint32_t)par_nvm_object_get_count();

    for (uint32_t slot = 0U; slot < object_count; slot++)
    {
        if (0 != par_test_nvm_obj_print_slot(slot))
        {
            return -1;
        }
    }

    if (0U == object_count)
    {
        PAR_TEST_NVM_OBJ_PRINT("no persistent object slots configured\n");
    }

    return 0;
}

/**
 * @brief Dump raw bytes from the object persistence block.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Shell command status.
 */
static int par_test_nvm_obj_cmd_dump(const int argc, char **argv)
{
    const par_store_backend_api_t *store = NULL;
    uint32_t base_addr = 0U;
    uint32_t offset = 0U;
    uint32_t len = 0U;
    uint32_t dumped = 0U;
    par_status_t status;

    if ((argc < 4) || (false == par_test_nvm_obj_parse_u32(argv[2], &offset)) ||
        (false == par_test_nvm_obj_parse_u32(argv[3], &len)) || (0U == len) ||
        (len > PAR_TEST_NVM_OBJ_DUMP_MAX_LEN) || (offset > (UINT32_MAX - (len - 1U))))
    {
        PAR_TEST_NVM_OBJ_PRINT("ERR usage: par_nvm_obj dump <block_offset> <len>, len=1..%u\n",
                               (unsigned)PAR_TEST_NVM_OBJ_DUMP_MAX_LEN);
        return -1;
    }

    status = par_test_nvm_obj_get_context(&store, &base_addr);
    if (ePAR_OK != status)
    {
        PAR_TEST_NVM_OBJ_PRINT("ERR backend status=%s(0x%04X)\n", par_test_nvm_obj_status_str(status), (unsigned)status);
        return -1;
    }

    while (dumped < len)
    {
        uint8_t buf[PAR_TEST_NVM_OBJ_DUMP_LINE_SIZE] = { 0U };
        const uint32_t line_len = ((len - dumped) > PAR_TEST_NVM_OBJ_DUMP_LINE_SIZE) ?
                                      PAR_TEST_NVM_OBJ_DUMP_LINE_SIZE :
                                      (len - dumped);

        status = store->read(base_addr + offset + dumped, line_len, buf);
        if (ePAR_OK != status)
        {
            PAR_TEST_NVM_OBJ_PRINT("ERR read block_offset=0x%08lX backend_addr=0x%08lX status=%s(0x%04X)\n",
                                   (unsigned long)(offset + dumped),
                                   (unsigned long)(base_addr + offset + dumped),
                                   par_test_nvm_obj_status_str(status),
                                   (unsigned)status);
            return -1;
        }

        PAR_TEST_NVM_OBJ_PRINT("0x%08lX:", (unsigned long)(offset + dumped));
        for (uint32_t i = 0U; i < line_len; i++)
        {
            PAR_TEST_NVM_OBJ_PRINT(" %02X", (unsigned)buf[i]);
        }
        PAR_TEST_NVM_OBJ_PRINT("\n");
        dumped += line_len;
    }

    return 0;
}

/**
 * @brief Write one byte to the object persistence block.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Shell command status.
 */
static int par_test_nvm_obj_cmd_poke(const int argc, char **argv)
{
    uint32_t offset = 0U;
    uint8_t value = 0U;
    uint8_t readback = 0U;
    par_status_t status;

    if ((argc < 4) || (false == par_test_nvm_obj_parse_u32(argv[2], &offset)) ||
        (false == par_test_nvm_obj_parse_u8(argv[3], &value)))
    {
        PAR_TEST_NVM_OBJ_PRINT("ERR usage: par_nvm_obj poke <block_offset> <value>\n");
        return -1;
    }

    status = par_test_nvm_obj_write_u8(offset, value, &readback);
    PAR_TEST_NVM_OBJ_PRINT("poke block_offset=0x%08lX value=0x%02X readback=0x%02X status=%s(0x%04X)\n",
                           (unsigned long)offset,
                           (unsigned)value,
                           (unsigned)readback,
                           par_test_nvm_obj_status_str(status),
                           (unsigned)status);
    return par_test_nvm_obj_status_ok(status) ? 0 : -1;
}

/**
 * @brief XOR one byte inside the object persistence block.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Shell command status.
 */
static int par_test_nvm_obj_cmd_flip(const int argc, char **argv)
{
    uint32_t offset = 0U;
    uint8_t mask = 0U;
    uint8_t old_value = 0U;
    uint8_t new_value = 0U;
    par_status_t status;

    if ((argc < 4) || (false == par_test_nvm_obj_parse_u32(argv[2], &offset)) ||
        (false == par_test_nvm_obj_parse_u8(argv[3], &mask)))
    {
        PAR_TEST_NVM_OBJ_PRINT("ERR usage: par_nvm_obj flip <block_offset> <mask>\n");
        return -1;
    }

    status = par_test_nvm_obj_flip_u8(offset, mask, &old_value, &new_value);
    PAR_TEST_NVM_OBJ_PRINT("flip block_offset=0x%08lX old=0x%02X mask=0x%02X new=0x%02X status=%s(0x%04X)\n",
                           (unsigned long)offset,
                           (unsigned)old_value,
                           (unsigned)mask,
                           (unsigned)new_value,
                           par_test_nvm_obj_status_str(status),
                           (unsigned)status);
    return par_test_nvm_obj_status_ok(status) ? 0 : -1;
}

/**
 * @brief Corrupt the first byte of the object header signature.
 * @return Shell command status.
 */
static int par_test_nvm_obj_cmd_corrupt_header(void)
{
    uint8_t readback = 0U;
    const par_status_t status = par_test_nvm_obj_write_u8(PAR_TEST_NVM_OBJ_HEADER_SIGNATURE_OFFSET, 0x00U, &readback);

    PAR_TEST_NVM_OBJ_PRINT("corrupt_header block_offset=0x%08X value=0x00 readback=0x%02X status=%s(0x%04X)\n",
                           (unsigned)PAR_TEST_NVM_OBJ_HEADER_SIGNATURE_OFFSET,
                           (unsigned)readback,
                           par_test_nvm_obj_status_str(status),
                           (unsigned)status);
    PAR_TEST_NVM_OBJ_PRINT("manual next step: reboot and check object header recovery logs\n");
    return par_test_nvm_obj_status_ok(status) ? 0 : -1;
}

/**
 * @brief Corrupt the CRC field of one object record.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Shell command status.
 */
static int par_test_nvm_obj_cmd_corrupt_record_crc(const int argc, char **argv)
{
    uint32_t slot = 0U;
    uint8_t old_value = 0U;
    uint8_t new_value = 0U;
    uint32_t crc_offset;
    par_test_nvm_obj_target_t target;
    par_status_t status;

    if ((argc < 3) || (false == par_test_nvm_obj_parse_u32(argv[2], &slot)))
    {
        PAR_TEST_NVM_OBJ_PRINT("ERR usage: par_nvm_obj corrupt_record_crc <slot>\n");
        return -1;
    }
    if ((slot >= (uint32_t)par_nvm_object_get_count()) ||
        (false == par_test_nvm_obj_find_slot_target(slot, &target)))
    {
        PAR_TEST_NVM_OBJ_PRINT("ERR object slot out of range, max=%u\n", (unsigned)par_nvm_object_get_count());
        return -1;
    }

    crc_offset = par_test_nvm_obj_record_offset_from_slot(slot) + PAR_TEST_NVM_OBJ_RECORD_CRC_OFFSET;
    status = par_test_nvm_obj_flip_u8(crc_offset, 0x01U, &old_value, &new_value);
    PAR_TEST_NVM_OBJ_PRINT("corrupt_record_crc slot=%lu block_offset=0x%08lX old=0x%02X new=0x%02X status=%s(0x%04X)\n",
                           (unsigned long)slot,
                           (unsigned long)crc_offset,
                           (unsigned)old_value,
                           (unsigned)new_value,
                           par_test_nvm_obj_status_str(status),
                           (unsigned)status);
    PAR_TEST_NVM_OBJ_PRINT("manual next step: reboot and check object record CRC recovery logs\n");
    return par_test_nvm_obj_status_ok(status) ? 0 : -1;
}

/**
 * @brief Corrupt one payload byte of an object record.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Shell command status.
 */
static int par_test_nvm_obj_cmd_corrupt_payload(const int argc, char **argv)
{
    uint32_t slot = 0U;
    uint32_t payload_offset = 0U;
    uint8_t mask = 0x01U;
    uint8_t old_value = 0U;
    uint8_t new_value = 0U;
    uint32_t block_offset;
    par_test_nvm_obj_target_t target;
    par_status_t status;

    if ((argc < 4) || (false == par_test_nvm_obj_parse_u32(argv[2], &slot)) ||
        (false == par_test_nvm_obj_parse_u32(argv[3], &payload_offset)) ||
        ((argc >= 5) && (false == par_test_nvm_obj_parse_u8(argv[4], &mask))))
    {
        PAR_TEST_NVM_OBJ_PRINT("ERR usage: par_nvm_obj corrupt_payload <slot> <payload_offset> [mask]\n");
        return -1;
    }
    if ((slot >= (uint32_t)par_nvm_object_get_count()) ||
        (false == par_test_nvm_obj_find_slot_target(slot, &target)) ||
        (payload_offset >= (uint32_t)target.capacity))
    {
        PAR_TEST_NVM_OBJ_PRINT("ERR object slot or payload offset out of range\n");
        return -1;
    }

    block_offset = par_test_nvm_obj_record_offset_from_slot(slot) + PAR_TEST_NVM_OBJ_RECORD_PAYLOAD_OFFSET + payload_offset;
    status = par_test_nvm_obj_flip_u8(block_offset, mask, &old_value, &new_value);
    PAR_TEST_NVM_OBJ_PRINT(
        "corrupt_payload slot=%lu payload_offset=0x%08lX block_offset=0x%08lX old=0x%02X new=0x%02X status=%s(0x%04X)\n",
        (unsigned long)slot,
        (unsigned long)payload_offset,
        (unsigned long)block_offset,
        (unsigned)old_value,
        (unsigned)new_value,
        par_test_nvm_obj_status_str(status),
        (unsigned)status);
    PAR_TEST_NVM_OBJ_PRINT("manual next step: reboot and check object record CRC recovery logs\n");
    return par_test_nvm_obj_status_ok(status) ? 0 : -1;
}

/**
 * @brief Set and immediately persist one object payload from an ASCII string.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Shell command status.
 */
static int par_test_nvm_obj_cmd_set_ascii(const int argc, char **argv)
{
    uint32_t id = 0U;
    par_status_t status;

    if ((argc < 4) || (false == par_test_nvm_obj_parse_u32(argv[2], &id)) || (id > UINT16_MAX) ||
        (strlen(argv[3]) > UINT16_MAX))
    {
        PAR_TEST_NVM_OBJ_PRINT("ERR usage: par_nvm_obj set_ascii <id> <text>\n");
        return -1;
    }

    status = par_set_obj_n_save_by_id((uint16_t)id, (const uint8_t *)argv[3], (uint16_t)strlen(argv[3]));
    PAR_TEST_NVM_OBJ_PRINT("set_ascii id=%lu len=%u status=%s(0x%04X)\n",
                           (unsigned long)id,
                           (unsigned)strlen(argv[3]),
                           par_test_nvm_obj_status_str(status),
                           (unsigned)status);
    return par_test_nvm_obj_status_ok(status) ? 0 : -1;
}

/**
 * @brief Set and immediately persist one object payload from raw byte arguments.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Shell command status.
 */
static int par_test_nvm_obj_cmd_set_hex(const int argc, char **argv)
{
    uint32_t id = 0U;
    uint8_t payload[PAR_TEST_NVM_OBJ_SET_HEX_MAX_LEN] = { 0U };
    uint16_t len;
    par_status_t status;

    if ((argc < 4) || (false == par_test_nvm_obj_parse_u32(argv[2], &id)) || (id > UINT16_MAX) ||
        ((argc - 3) > (int)PAR_TEST_NVM_OBJ_SET_HEX_MAX_LEN))
    {
        PAR_TEST_NVM_OBJ_PRINT("ERR usage: par_nvm_obj set_hex <id> <byte0> [byte1 ...], max=%u\n",
                               (unsigned)PAR_TEST_NVM_OBJ_SET_HEX_MAX_LEN);
        return -1;
    }

    len = (uint16_t)(argc - 3);
    for (uint16_t i = 0U; i < len; i++)
    {
        if (false == par_test_nvm_obj_parse_u8(argv[3 + i], &payload[i]))
        {
            PAR_TEST_NVM_OBJ_PRINT("ERR invalid byte at index=%u\n", (unsigned)i);
            return -1;
        }
    }

    status = par_set_obj_n_save_by_id((uint16_t)id, payload, len);
    PAR_TEST_NVM_OBJ_PRINT("set_hex id=%lu len=%u status=%s(0x%04X)\n",
                           (unsigned long)id,
                           (unsigned)len,
                           par_test_nvm_obj_status_str(status),
                           (unsigned)status);
    return par_test_nvm_obj_status_ok(status) ? 0 : -1;
}

/**
 * @brief MSH entry point for object NVM manual tests.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 on success, otherwise -1.
 */
int par_test_nvm_obj_exec(int argc, char **argv)
{
    if (argc < 2)
    {
        par_test_nvm_obj_print_usage();
        return -1;
    }

    if (0 == strcmp(argv[1], "info"))
    {
        return par_test_nvm_obj_cmd_info();
    }
    if (0 == strcmp(argv[1], "list"))
    {
        return par_test_nvm_obj_cmd_list();
    }
    if (0 == strcmp(argv[1], "slot"))
    {
        return par_test_nvm_obj_cmd_slot(argc, argv);
    }
    if (0 == strcmp(argv[1], "dump"))
    {
        return par_test_nvm_obj_cmd_dump(argc, argv);
    }
    if (0 == strcmp(argv[1], "poke"))
    {
        return par_test_nvm_obj_cmd_poke(argc, argv);
    }
    if (0 == strcmp(argv[1], "flip"))
    {
        return par_test_nvm_obj_cmd_flip(argc, argv);
    }
    if (0 == strcmp(argv[1], "corrupt_header"))
    {
        return par_test_nvm_obj_cmd_corrupt_header();
    }
    if (0 == strcmp(argv[1], "corrupt_record_crc"))
    {
        return par_test_nvm_obj_cmd_corrupt_record_crc(argc, argv);
    }
    if (0 == strcmp(argv[1], "corrupt_payload"))
    {
        return par_test_nvm_obj_cmd_corrupt_payload(argc, argv);
    }
    if (0 == strcmp(argv[1], "set_ascii"))
    {
        return par_test_nvm_obj_cmd_set_ascii(argc, argv);
    }
    if (0 == strcmp(argv[1], "set_hex"))
    {
        return par_test_nvm_obj_cmd_set_hex(argc, argv);
    }

    par_test_nvm_obj_print_usage();
    return -1;
}
#if defined(RT_USING_FINSH)
/**
 * @brief MSH entry point for object NVM manual tests.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 on success; otherwise a negative error code.
 */
static int par_test_nvm_obj_msh(int argc, char **argv)
{
    par_test_bind_rt_console();
    return par_test_nvm_obj_exec(argc, argv);
}
/**
 * @brief Option-completion IDs for par_nvm_obj subcommands.
 */
typedef enum
{
    PAR_TEST_NVM_OBJ_OPT_INFO = 1,       /**< Print object storage information. */
    PAR_TEST_NVM_OBJ_OPT_LIST,           /**< List object records. */
    PAR_TEST_NVM_OBJ_OPT_SLOT,           /**< Print one object record. */
    PAR_TEST_NVM_OBJ_OPT_DUMP,           /**< Dump object storage bytes. */
    PAR_TEST_NVM_OBJ_OPT_POKE,           /**< Write one object storage byte. */
    PAR_TEST_NVM_OBJ_OPT_FLIP,           /**< XOR one object storage byte. */
    PAR_TEST_NVM_OBJ_OPT_CORRUPT_HEADER, /**< Corrupt the object block header. */
    PAR_TEST_NVM_OBJ_OPT_CORRUPT_RECORD_CRC, /**< Corrupt one object record CRC. */
    PAR_TEST_NVM_OBJ_OPT_CORRUPT_PAYLOAD,    /**< Corrupt one object payload byte. */
    PAR_TEST_NVM_OBJ_OPT_SET_ASCII,      /**< Set one string object payload. */
    PAR_TEST_NVM_OBJ_OPT_SET_HEX         /**< Set one bytes object payload. */
} par_test_nvm_obj_opt_id_t;

CMD_OPTIONS_NODE_START(par_test_nvm_obj_msh)
CMD_OPTIONS_NODE(PAR_TEST_NVM_OBJ_OPT_INFO, info, print object storage information)
CMD_OPTIONS_NODE(PAR_TEST_NVM_OBJ_OPT_LIST, list, list object records)
CMD_OPTIONS_NODE(PAR_TEST_NVM_OBJ_OPT_SLOT, slot, print one object record)
CMD_OPTIONS_NODE(PAR_TEST_NVM_OBJ_OPT_DUMP, dump, dump object storage bytes)
CMD_OPTIONS_NODE(PAR_TEST_NVM_OBJ_OPT_POKE, poke, write one object storage byte)
CMD_OPTIONS_NODE(PAR_TEST_NVM_OBJ_OPT_FLIP, flip, xor one object storage byte)
CMD_OPTIONS_NODE(PAR_TEST_NVM_OBJ_OPT_CORRUPT_HEADER, corrupt_header, corrupt object block header)
CMD_OPTIONS_NODE(PAR_TEST_NVM_OBJ_OPT_CORRUPT_RECORD_CRC, corrupt_record_crc, corrupt one object record CRC)
CMD_OPTIONS_NODE(PAR_TEST_NVM_OBJ_OPT_CORRUPT_PAYLOAD, corrupt_payload, corrupt one object payload byte)
CMD_OPTIONS_NODE(PAR_TEST_NVM_OBJ_OPT_SET_ASCII, set_ascii, set string object payload)
CMD_OPTIONS_NODE(PAR_TEST_NVM_OBJ_OPT_SET_HEX, set_hex, set bytes object payload)
CMD_OPTIONS_NODE_END
MSH_CMD_EXPORT_ALIAS(par_test_nvm_obj_msh, par_nvm_obj, object NVM manual tests, optenable);
#endif /* defined(RT_USING_FINSH) */

#endif /* PAR_TEST_NVM_OBJ_BUILD_ENABLED */
#undef PAR_TEST_NVM_OBJ_BUILD_ENABLED
#endif /* defined(AUTOGEN_PM_TEST_NVM_OBJECT_HELPER) */
