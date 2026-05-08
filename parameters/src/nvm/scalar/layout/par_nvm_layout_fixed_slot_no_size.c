/**
 * @file par_nvm_layout_fixed_slot_no_size.c
 * @brief Implement the fixed-slot persisted-record layout without a size descriptor.
 * @author wdfk-prog
 * @version 1.1
 * @date 2026-04-13
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-04-06 1.0     wdfk-prog     first version
 * 2026-04-13 1.1     wdfk-prog     add layout-ops adapter
 */
#include "par_nvm_layout.h"

#if (1 == PAR_CFG_NVM_EN) && (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE)

#include <string.h>

#include "par_nvm_table_id.h"

#define PAR_NVM_LAYOUT_RECORD_SIZE (PAR_NVM_RECORD_ID_SIZE + PAR_NVM_RECORD_CRC_SIZE + (uint32_t)PAR_NVM_RECORD_DATA_SLOT_SIZE)

PAR_STATIC_ASSERT(par_nvm_layout_fixed_no_size_payload_slot_is_4_bytes,
                  (sizeof(((par_nvm_layout_fixed_slot_no_size_record_t *)0)->payload) == 4u));

/**
 * @brief Return the serialized byte size of one persisted record for this layout.
 *
 * @param par_num Live parameter number associated with the slot.
 * @return Serialized record size in bytes.
 */
static uint32_t par_nvm_layout_fixed_slot_no_size_record_size_from_par_num(const par_num_t par_num)
{
    (void)par_num;
    return PAR_NVM_LAYOUT_RECORD_SIZE;
}

/**
 * @brief Translate one persistent slot index into its serialized record address.
 *
 * @param first_data_obj_addr Absolute address of the first persisted record.
 * @param persist_idx Compile-time persistent slot index.
 * @param p_persist_slot_to_par_num Compile-time slot-to-parameter mapping table.
 * @return Absolute address of the selected record.
 */
static uint32_t par_nvm_layout_fixed_slot_no_size_addr_from_persist_idx(const uint32_t first_data_obj_addr,
                                                                         const uint16_t persist_idx,
                                                                         const par_num_t * const p_persist_slot_to_par_num)
{
    (void)p_persist_slot_to_par_num;
    return (first_data_obj_addr + ((uint32_t)persist_idx * PAR_NVM_LAYOUT_RECORD_SIZE));
}

/**
 * @brief Populate one canonical NVM object from the live parameter value.
 *
 * @param par_num Live parameter number.
 * @param p_live_data Pointer to the live canonical parameter value.
 * @param p_obj Output canonical NVM object.
 */
static void par_nvm_layout_fixed_slot_no_size_populate_data_obj(const par_num_t par_num,
                                                                const par_type_t * const p_live_data,
                                                                par_nvm_data_obj_t * const p_obj)
{
    PAR_ASSERT((NULL != p_live_data) && (NULL != p_obj));

    memset(p_obj, 0, sizeof(*p_obj));
    p_obj->id = par_cfg_get_param_id_const(par_num);
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
static par_status_t par_nvm_layout_fixed_slot_no_size_read(const par_store_backend_api_t * const p_store,
                                                           const uint32_t addr,
                                                           const par_num_t par_num,
                                                           par_nvm_data_obj_t * const p_obj)
{
    uint8_t record_buf[PAR_NVM_LAYOUT_RECORD_SIZE] = { 0U };
    par_type_t payload_raw = { 0U };
    uint8_t crc_calc = 0U;

    (void)par_num;
    PAR_ASSERT((NULL != p_store) && (NULL != p_obj));
    memset(p_obj, 0, sizeof(*p_obj));

    if (ePAR_OK != p_store->read(addr, PAR_NVM_LAYOUT_RECORD_SIZE, record_buf))
    {
        return ePAR_ERROR_NVM;
    }

    memcpy(&p_obj->id, &record_buf[0], sizeof(p_obj->id));
    memcpy(&payload_raw, &record_buf[PAR_NVM_RECORD_ID_SIZE + PAR_NVM_RECORD_CRC_SIZE], sizeof(payload_raw));

    crc_calc = par_nvm_layout_calc_crc_with_id(p_obj->id,
                                               0U,
                                               (const uint8_t * const)&payload_raw,
                                               PAR_NVM_RECORD_DATA_SLOT_SIZE,
                                               false);
    if (crc_calc != record_buf[PAR_NVM_RECORD_ID_SIZE])
    {
        return ePAR_ERROR_CRC;
    }

    p_obj->data = payload_raw;
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
static par_status_t par_nvm_layout_fixed_slot_no_size_write(const par_store_backend_api_t * const p_store,
                                                            const uint32_t addr,
                                                            const par_num_t par_num,
                                                            const par_nvm_data_obj_t * const p_obj)
{
    uint8_t record_buf[PAR_NVM_LAYOUT_RECORD_SIZE] = { 0U };
    par_type_t payload_raw = p_obj->data;
    uint8_t crc = 0U;

    (void)par_num;
    PAR_ASSERT((NULL != p_store) && (NULL != p_obj));

    crc = par_nvm_layout_calc_crc_with_id(p_obj->id,
                                          0U,
                                          (const uint8_t * const)&payload_raw,
                                          PAR_NVM_RECORD_DATA_SLOT_SIZE,
                                          false);

    memcpy(&record_buf[0], &p_obj->id, sizeof(p_obj->id));
    record_buf[PAR_NVM_RECORD_ID_SIZE] = crc;
    memcpy(&record_buf[PAR_NVM_RECORD_ID_SIZE + PAR_NVM_RECORD_CRC_SIZE], &payload_raw, sizeof(payload_raw));

    return (ePAR_OK == p_store->write(addr, PAR_NVM_LAYOUT_RECORD_SIZE, record_buf)) ? ePAR_OK : ePAR_ERROR_NVM;
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
static par_status_t par_nvm_layout_fixed_slot_no_size_validate_loaded_obj(const par_num_t par_num,
                                                                          const par_nvm_data_obj_t * const p_obj,
                                                                          const char ** const pp_reason,
                                                                          uint16_t * const p_stored_id)
{
    const uint16_t expected_id = par_cfg_get_param_id_const(par_num);

    PAR_ASSERT((NULL != p_obj) && (NULL != pp_reason) && (NULL != p_stored_id));

    *pp_reason = NULL;
    *p_stored_id = p_obj->id;

    if (p_obj->id != expected_id)
    {
        *pp_reason = "id-mismatch";
        return ePAR_ERROR;
    }

    return ePAR_OK;
}

/**
 * @brief Return the stored-ID diagnostic value for an error path.
 *
 * @param par_num Live parameter number associated with the slot.
 * @param p_obj Canonical object loaded from NVM.
 * @return Stored ID or a layout-defined fallback value.
 */
static uint16_t par_nvm_layout_fixed_slot_no_size_get_error_stored_id(const par_num_t par_num,
                                                                       const par_nvm_data_obj_t * const p_obj)
{
    (void)par_num;
    return (NULL != p_obj) ? p_obj->id : 0U;
}

/**
 * @brief Decide whether the stored header remains compatible with this layout.
 *
 * @param p_head_obj Validated NVM header object.
 * @return Layout-specific compatibility decision.
 */
static par_nvm_compat_result_t par_nvm_layout_fixed_slot_no_size_check_compat(const par_nvm_head_obj_t * const p_head_obj)
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

    return (p_head_obj->obj_nb == (uint16_t)PAR_PERSISTENT_COMPILE_COUNT) ?
               ePAR_NVM_COMPAT_EXACT_MATCH :
               ePAR_NVM_COMPAT_PREFIX_APPEND;
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
static bool par_nvm_layout_fixed_slot_no_size_data_obj_matches(const par_num_t par_num,
                                                               const par_nvm_data_obj_t * const p_expected,
                                                               const par_nvm_data_obj_t * const p_actual)
{
    uint8_t expected_payload[PAR_NVM_RECORD_DATA_SLOT_SIZE] = { 0U };
    uint8_t actual_payload[PAR_NVM_RECORD_DATA_SLOT_SIZE] = { 0U };

    (void)par_num;
    PAR_ASSERT((NULL != p_expected) && (NULL != p_actual));

    if (p_expected->id != p_actual->id)
    {
        return false;
    }

    memcpy(expected_payload, &p_expected->data, sizeof(p_expected->data));
    memcpy(actual_payload, &p_actual->data, sizeof(p_actual->data));
    return (0 == memcmp(expected_payload, actual_payload, PAR_NVM_RECORD_DATA_SLOT_SIZE));
}
#endif /* (1 == PAR_CFG_NVM_WRITE_VERIFY_EN) */

/**
 * @brief Concrete layout adapter bound by the common NVM core.
 */
static const par_nvm_layout_api_t g_par_nvm_layout_api = {
    .record_size_from_par_num = par_nvm_layout_fixed_slot_no_size_record_size_from_par_num,
    .addr_from_persist_idx = par_nvm_layout_fixed_slot_no_size_addr_from_persist_idx,
    .populate_data_obj = par_nvm_layout_fixed_slot_no_size_populate_data_obj,
    .read = par_nvm_layout_fixed_slot_no_size_read,
    .write = par_nvm_layout_fixed_slot_no_size_write,
    .validate_loaded_obj = par_nvm_layout_fixed_slot_no_size_validate_loaded_obj,
    .get_error_stored_id = par_nvm_layout_fixed_slot_no_size_get_error_stored_id,
    .check_compat = par_nvm_layout_fixed_slot_no_size_check_compat,
#if (1 == PAR_CFG_NVM_WRITE_VERIFY_EN)
    .data_obj_matches = par_nvm_layout_fixed_slot_no_size_data_obj_matches,
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

#endif /* (1 == PAR_CFG_NVM_EN) && (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE) */
