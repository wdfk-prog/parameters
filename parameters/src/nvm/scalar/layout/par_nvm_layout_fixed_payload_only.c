/**
 * @file par_nvm_layout_fixed_payload_only.c
 * @brief Implement the fixed persistent-order payload-only NVM layout.
 * @author wdfk-prog
 * @version 1.3
 * @date 2026-04-13
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-04-10 1.0     wdfk-prog     first version
 * 2026-04-11 1.1     wdfk-prog     restore layout comments and split layout structs
 * 2026-04-13 1.2     wdfk-prog     add layout-ops adapter
 * 2026-04-13 1.3     wdfk-prog     auto-generate compile-time fixed-payload-only address LUT
 */
#include "par_nvm_layout.h"

#if (1 == PAR_CFG_NVM_EN) && (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_PAYLOAD_ONLY)

#include <stddef.h>
#include <string.h>

#include "par_nvm_table_id.h"

#define PAR_NVM_LAYOUT_RECORD_OVERHEAD ((uint32_t)PAR_NVM_RECORD_CRC_SIZE)
#define PAR_NVM_LAYOUT_RECORD_MAX_SIZE (PAR_NVM_LAYOUT_RECORD_OVERHEAD + PAR_NVM_RECORD_DATA_SLOT_SIZE)

PAR_STATIC_ASSERT(par_nvm_layout_fixed_payload_only_record_payload_slot_is_4_bytes,
                  (sizeof(((par_nvm_layout_fixed_payload_only_record_t *)0)->payload) == 4u));

/**
 * @brief Return the serialized record size for one payload-only record width.
 *
 * @param payload_size Natural payload width in bytes.
 * @return Serialized record size in bytes.
 */
static uint32_t par_nvm_layout_fixed_payload_only_record_size_from_payload_size(const uint8_t payload_size)
{
    return (PAR_NVM_LAYOUT_RECORD_OVERHEAD + (uint32_t)payload_size);
}

/**
 * @brief Return the serialized byte size of one persisted record for this layout.
 *
 * @param par_num Live parameter number associated with the slot.
 * @return Serialized record size in bytes.
 */
static uint32_t par_nvm_layout_fixed_payload_only_record_size_from_par_num(const par_num_t par_num)
{
    return par_nvm_layout_fixed_payload_only_record_size_from_payload_size(par_nvm_layout_payload_size_from_par_num(par_num));
}

/**
 * @brief Compile-time packed offset map for the fixed-payload-only layout.
 *
 * @details The generated struct keeps one anchor byte so the type remains valid
 * even when no parameters are persistent. Every persistent record then expands
 * into one tightly packed `uint8_t[]` member sized from `par_table.def`, and
 * `offsetof()` yields the exact persistent prefix sum without handwritten
 * offsets.
 */
#define PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_OFFSET_MAP_BASE_SIZE                  1U
#define PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_RECORD_SIZE_BYTES(payload_size_)      (PAR_NVM_LAYOUT_RECORD_OVERHEAD + (uint32_t)(payload_size_))
#define PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_PERSIST_SELECT(enum_, pers_, emit_)   PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_PERSIST_SELECT_I(enum_, pers_, emit_)
#define PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_PERSIST_SELECT_I(enum_, pers_, emit_) PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_PERSIST_SELECT_##pers_(enum_, emit_)
#define PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_PERSIST_SELECT_1(enum_, emit_)        emit_(enum_)
#define PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_PERSIST_SELECT_0(enum_, emit_)
#define PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_MEMBER_1(enum_) uint8_t slot_##enum_[PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_RECORD_SIZE_BYTES(1U)];
#define PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_MEMBER_2(enum_) uint8_t slot_##enum_[PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_RECORD_SIZE_BYTES(2U)];
#define PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_MEMBER_4(enum_) uint8_t slot_##enum_[PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_RECORD_SIZE_BYTES(4U)];
#define PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_OBJECT_NOP(...)
typedef struct
{
    uint8_t base__[PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_OFFSET_MAP_BASE_SIZE];
#define PAR_ITEM_U8(...)                 PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_PERSIST_SELECT(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_PERS(__VA_ARGS__), PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_MEMBER_1)
#define PAR_ITEM_U16(...)                PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_PERSIST_SELECT(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_PERS(__VA_ARGS__), PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_MEMBER_2)
#define PAR_ITEM_U32(...)                PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_PERSIST_SELECT(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_PERS(__VA_ARGS__), PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_MEMBER_4)
#define PAR_ITEM_I8(...)                 PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_PERSIST_SELECT(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_PERS(__VA_ARGS__), PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_MEMBER_1)
#define PAR_ITEM_I16(...)                PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_PERSIST_SELECT(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_PERS(__VA_ARGS__), PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_MEMBER_2)
#define PAR_ITEM_I32(...)                PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_PERSIST_SELECT(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_PERS(__VA_ARGS__), PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_MEMBER_4)
#define PAR_ITEM_F32(...)                PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_PERSIST_SELECT(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_PERS(__VA_ARGS__), PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_MEMBER_4)
#define PAR_OBJECT_ITEM_ENABLED_HANDLER  PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_OBJECT_NOP
#define PAR_OBJECT_ITEM_DISABLED_HANDLER PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_OBJECT_NOP
#include "par_object_item_bind.inc"
#include "par_table.def"
#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32
#include "par_object_item_unbind.inc"
#undef PAR_OBJECT_ITEM_ENABLED_HANDLER
#undef PAR_OBJECT_ITEM_DISABLED_HANDLER
} par_nvm_layout_fixed_payload_only_offset_map_t;
#define PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_OFFSET_OF(enum_)                  ((uint32_t)offsetof(par_nvm_layout_fixed_payload_only_offset_map_t, slot_##enum_) - PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_OFFSET_MAP_BASE_SIZE)
#define PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_ADDR_ENTRY_SELECT(enum_, pers_)   PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_ADDR_ENTRY_SELECT_I(enum_, pers_)
#define PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_ADDR_ENTRY_SELECT_I(enum_, pers_) PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_ADDR_ENTRY_SELECT_##pers_(enum_)
#define PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_ADDR_ENTRY_SELECT_1(enum_)        [PAR_PERSIST_IDX_##enum_] = PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_OFFSET_OF(enum_),
#define PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_ADDR_ENTRY_SELECT_0(enum_)
/**
 * @brief Compile-time fixed-payload-only offset lookup by persistent slot.
 */
static const uint32_t g_par_nvm_layout_fixed_payload_only_addr_lut[PAR_PERSIST_SLOT_MAP_CAPACITY] = {
#define PAR_ITEM_NOP(...)
#define PAR_ITEM_U8(...)                 PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_ADDR_ENTRY_SELECT(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_PERS(__VA_ARGS__))
#define PAR_ITEM_U16(...)                PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_ADDR_ENTRY_SELECT(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_PERS(__VA_ARGS__))
#define PAR_ITEM_U32(...)                PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_ADDR_ENTRY_SELECT(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_PERS(__VA_ARGS__))
#define PAR_ITEM_I8(...)                 PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_ADDR_ENTRY_SELECT(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_PERS(__VA_ARGS__))
#define PAR_ITEM_I16(...)                PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_ADDR_ENTRY_SELECT(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_PERS(__VA_ARGS__))
#define PAR_ITEM_I32(...)                PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_ADDR_ENTRY_SELECT(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_PERS(__VA_ARGS__))
#define PAR_ITEM_F32(...)                PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_ADDR_ENTRY_SELECT(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_PERS(__VA_ARGS__))
#define PAR_OBJECT_ITEM_ENABLED_HANDLER  PAR_ITEM_NOP
#define PAR_OBJECT_ITEM_DISABLED_HANDLER PAR_ITEM_NOP
#include "par_object_item_bind.inc"
#include "par_table.def"
#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32
#include "par_object_item_unbind.inc"
#undef PAR_OBJECT_ITEM_ENABLED_HANDLER
#undef PAR_OBJECT_ITEM_DISABLED_HANDLER
#undef PAR_ITEM_NOP
};
#undef PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_ADDR_ENTRY_SELECT
#undef PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_ADDR_ENTRY_SELECT_I
#undef PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_ADDR_ENTRY_SELECT_1
#undef PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_ADDR_ENTRY_SELECT_0
#undef PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_OFFSET_OF
#undef PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_MEMBER_1
#undef PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_MEMBER_2
#undef PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_MEMBER_4
#undef PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_OBJECT_NOP
#undef PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_PERSIST_SELECT
#undef PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_PERSIST_SELECT_I
#undef PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_PERSIST_SELECT_1
#undef PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_PERSIST_SELECT_0
#undef PAR_NVM_LAYOUT_FIXED_PAYLOAD_ONLY_RECORD_SIZE_BYTES

/**
 * @brief Translate one persistent slot index into its serialized record address.
 *
 * @param first_data_obj_addr Absolute address of the first persisted record.
 * @param persist_idx Compile-time persistent slot index.
 * @param p_persist_slot_to_par_num Compile-time slot-to-parameter mapping table.
 * @return Absolute address of the selected record.
 */
static uint32_t par_nvm_layout_fixed_payload_only_addr_from_persist_idx(const uint32_t first_data_obj_addr,
                                                                        const uint16_t persist_idx,
                                                                        const par_num_t * const p_persist_slot_to_par_num)
{
    PAR_ASSERT(NULL != p_persist_slot_to_par_num);
    PAR_ASSERT(persist_idx < PAR_PERSISTENT_COMPILE_COUNT);
    (void)p_persist_slot_to_par_num;

    return (first_data_obj_addr + g_par_nvm_layout_fixed_payload_only_addr_lut[persist_idx]);
}

/**
 * @brief Populate one canonical NVM object from the live parameter value.
 *
 * @param par_num Live parameter number.
 * @param p_live_data Pointer to the live canonical parameter value.
 * @param p_obj Output canonical NVM object.
 */
static void par_nvm_layout_fixed_payload_only_populate_data_obj(const par_num_t par_num,
                                                                const par_type_t * const p_live_data,
                                                                par_nvm_data_obj_t * const p_obj)
{
    (void)par_num;
    PAR_ASSERT((NULL != p_live_data) && (NULL != p_obj));

    memset(p_obj, 0, sizeof(*p_obj));
    p_obj->data = *p_live_data;
}

/**
 * @brief Read and validate one serialized record from storage.
 *
 * @param p_store Active storage backend API.
 * @param addr Absolute record address inside the managed NVM image.
 * @param par_num Live parameter number associated with the slot.
 * @param p_obj Output canonical NVM object.
 * @return Operation status.
 */
static par_status_t par_nvm_layout_fixed_payload_only_read(const par_store_backend_api_t * const p_store,
                                                           const uint32_t addr,
                                                           const par_num_t par_num,
                                                           par_nvm_data_obj_t * const p_obj)
{
    uint8_t record_buf[PAR_NVM_LAYOUT_RECORD_MAX_SIZE] = { 0U };
    const par_cfg_t * const p_cfg = par_get_config(par_num);
    const uint8_t payload_size = par_nvm_layout_payload_size_from_par_num(par_num);
    const uint32_t record_size = par_nvm_layout_fixed_payload_only_record_size_from_payload_size(payload_size);
    const uint8_t * const p_payload = &record_buf[PAR_NVM_RECORD_CRC_SIZE];
    uint8_t crc_calc = 0U;

    PAR_ASSERT((NULL != p_store) && (NULL != p_obj));
    PAR_ASSERT(NULL != p_cfg);
    memset(p_obj, 0, sizeof(*p_obj));

    if (ePAR_OK != p_store->read(addr, record_size, record_buf))
    {
        return ePAR_ERROR_NVM;
    }

    crc_calc = par_nvm_layout_calc_crc(0U, p_payload, payload_size, false);
    if (crc_calc != record_buf[0])
    {
        return ePAR_ERROR_CRC;
    }

    par_nvm_layout_unpack_payload_bytes(p_cfg->type, p_payload, &p_obj->data);
    return ePAR_OK;
}

/**
 * @brief Serialize and write one canonical NVM object to storage.
 *
 * @param p_store Active storage backend API.
 * @param addr Absolute record address inside the managed NVM image.
 * @param par_num Live parameter number associated with the slot.
 * @param p_obj Canonical NVM object to serialize.
 * @return Operation status.
 */
static par_status_t par_nvm_layout_fixed_payload_only_write(const par_store_backend_api_t * const p_store,
                                                            const uint32_t addr,
                                                            const par_num_t par_num,
                                                            const par_nvm_data_obj_t * const p_obj)
{
    uint8_t record_buf[PAR_NVM_LAYOUT_RECORD_MAX_SIZE] = { 0U };
    const par_cfg_t * const p_cfg = par_get_config(par_num);
    const uint8_t payload_size = par_nvm_layout_payload_size_from_par_num(par_num);
    const uint32_t record_size = par_nvm_layout_fixed_payload_only_record_size_from_payload_size(payload_size);
    uint8_t * const p_payload = &record_buf[PAR_NVM_RECORD_CRC_SIZE];

    PAR_ASSERT((NULL != p_store) && (NULL != p_obj));
    PAR_ASSERT(NULL != p_cfg);

    par_nvm_layout_pack_payload_bytes(p_cfg->type, &p_obj->data, p_payload);
    record_buf[0] = par_nvm_layout_calc_crc(0U, p_payload, payload_size, false);

    return (ePAR_OK == p_store->write(addr, record_size, record_buf)) ? ePAR_OK : ePAR_ERROR_NVM;
}

/**
 * @brief Validate one loaded canonical object against the current live schema.
 *
 * @param par_num Live parameter number expected at this slot.
 * @param p_obj Canonical object loaded from NVM.
 * @param pp_reason Output short mismatch reason for diagnostics.
 * @param p_stored_id Output stored ID value when the layout carries one.
 * @return Operation status.
 */
static par_status_t par_nvm_layout_fixed_payload_only_validate_loaded_obj(const par_num_t par_num,
                                                                          const par_nvm_data_obj_t * const p_obj,
                                                                          const char ** const pp_reason,
                                                                          uint16_t * const p_stored_id)
{
    PAR_ASSERT((NULL != p_obj) && (NULL != pp_reason) && (NULL != p_stored_id));

    (void)p_obj;
    *pp_reason = NULL;
    *p_stored_id = par_cfg_get_param_id_const(par_num);
    return ePAR_OK;
}

/**
 * @brief Return the stored-ID diagnostic value for an error path.
 *
 * @param par_num Live parameter number associated with the slot.
 * @param p_obj Canonical object loaded from NVM.
 * @return Stored ID or a layout-defined fallback value.
 */
static uint16_t par_nvm_layout_fixed_payload_only_get_error_stored_id(const par_num_t par_num,
                                                                      const par_nvm_data_obj_t * const p_obj)
{
    (void)p_obj;
    return par_cfg_get_param_id_const(par_num);
}

/**
 * @brief Decide whether the stored header remains compatible with this layout.
 *
 * @param p_head_obj Validated NVM header object.
 * @return Layout-specific compatibility decision.
 */
static par_nvm_compat_result_t par_nvm_layout_fixed_payload_only_check_compat(const par_nvm_head_obj_t * const p_head_obj)
{
    PAR_ASSERT(NULL != p_head_obj);

    if (p_head_obj->obj_nb > (uint16_t)PAR_PERSISTENT_COMPILE_COUNT)
    {
        return ePAR_NVM_COMPAT_REBUILD;
    }

    if (p_head_obj->table_id != par_nvm_table_id_calc_for_count(p_head_obj->obj_nb))
    {
        return ePAR_NVM_COMPAT_REBUILD;
    }

    return (p_head_obj->obj_nb == (uint16_t)PAR_PERSISTENT_COMPILE_COUNT) ? ePAR_NVM_COMPAT_EXACT_MATCH : ePAR_NVM_COMPAT_PREFIX_APPEND;
}

#if (1 == PAR_CFG_NVM_WRITE_VERIFY_EN)
/**
 * @brief Compare an expected object against a post-write read-back object.
 *
 * @param par_num Live parameter number associated with the slot.
 * @param p_expected Expected canonical NVM object.
 * @param p_actual Canonical object reloaded from storage.
 * @return True when both objects match under this layout policy.
 */
static bool par_nvm_layout_fixed_payload_only_data_obj_matches(const par_num_t par_num,
                                                               const par_nvm_data_obj_t * const p_expected,
                                                               const par_nvm_data_obj_t * const p_actual)
{
    const par_cfg_t * const p_cfg = par_get_config(par_num);
    const uint8_t payload_size = par_nvm_layout_payload_size_from_par_num(par_num);
    uint8_t expected_payload[PAR_NVM_RECORD_DATA_SLOT_SIZE] = { 0U };
    uint8_t actual_payload[PAR_NVM_RECORD_DATA_SLOT_SIZE] = { 0U };

    PAR_ASSERT((NULL != p_cfg) && (NULL != p_expected) && (NULL != p_actual));

    par_nvm_layout_pack_payload_bytes(p_cfg->type, &p_expected->data, expected_payload);
    par_nvm_layout_pack_payload_bytes(p_cfg->type, &p_actual->data, actual_payload);
    return (0 == memcmp(expected_payload, actual_payload, payload_size));
}
#endif /* (1 == PAR_CFG_NVM_WRITE_VERIFY_EN) */

/**
 * @brief Concrete layout adapter bound by the common NVM core.
 */
static const par_nvm_layout_api_t g_par_nvm_layout_api = {
    .record_size_from_par_num = par_nvm_layout_fixed_payload_only_record_size_from_par_num,
    .addr_from_persist_idx = par_nvm_layout_fixed_payload_only_addr_from_persist_idx,
    .populate_data_obj = par_nvm_layout_fixed_payload_only_populate_data_obj,
    .read = par_nvm_layout_fixed_payload_only_read,
    .write = par_nvm_layout_fixed_payload_only_write,
    .validate_loaded_obj = par_nvm_layout_fixed_payload_only_validate_loaded_obj,
    .get_error_stored_id = par_nvm_layout_fixed_payload_only_get_error_stored_id,
    .check_compat = par_nvm_layout_fixed_payload_only_check_compat,
#if (1 == PAR_CFG_NVM_WRITE_VERIFY_EN)
    .data_obj_matches = par_nvm_layout_fixed_payload_only_data_obj_matches,
#endif /* (1 == PAR_CFG_NVM_WRITE_VERIFY_EN) */
};

/**
 * @brief Return the concrete layout adapter selected for this build.
 *
 * @return Non-null pointer to this layout adapter.
 */
const par_nvm_layout_api_t *par_nvm_layout_init(void)
{
    return &g_par_nvm_layout_api;
}

#endif /* (1 == PAR_CFG_NVM_EN) && (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_PAYLOAD_ONLY) */
