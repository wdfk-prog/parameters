/**
 * @file par_nvm_layout_fixed_slot_with_size.c
 * @brief Implement the fixed-slot persisted-record layout with a size descriptor.
 * @author wdfk-prog ()
 * @version 1.0
 * @date 2026-04-06
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author      Description
 * 2026-04-06 1.0     wdfk-prog   first version
 */
#include "persist/par_nvm_layout.h"

#if (1 == PAR_CFG_NVM_EN) && (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE)

#include <string.h>

/**
 * @brief Serialized size of one fixed-slot record with an explicit size field.
 */
#define PAR_NVM_LAYOUT_RECORD_SIZE ((uint32_t)sizeof(par_nvm_layout_fixed_slot_with_size_record_t))

PAR_STATIC_ASSERT(par_nvm_layout_fixed_with_size_record_is_8_bytes,
                  (sizeof(par_nvm_layout_fixed_slot_with_size_record_t) == 8u));

/**
 * @brief Get serialized record size for one persistent parameter.
 *
 * @param par_num Live parameter number.
 * @return Serialized record size in bytes.
 */
uint32_t par_nvm_layout_record_size_from_par_num(const par_num_t par_num)
{
    (void)par_num;
    return PAR_NVM_LAYOUT_RECORD_SIZE;
}

/**
 * @brief Resolve record address for the fixed-slot-with-size layout.
 *
 * @param first_data_obj_addr Start address of the first persisted object.
 * @param persist_idx Compile-time persistent slot index.
 * @param p_persist_slot_to_par_num Persistent-slot to live-parameter mapping.
 * @return Absolute NVM address of the selected record.
 */
uint32_t par_nvm_layout_addr_from_persist_idx(const uint32_t first_data_obj_addr,
                                              const uint16_t persist_idx,
                                              const par_num_t * const p_persist_slot_to_par_num)
{
    (void)p_persist_slot_to_par_num;
    return (first_data_obj_addr + ((uint32_t)persist_idx * PAR_NVM_LAYOUT_RECORD_SIZE));
}

/**
 * @brief Read one fixed-slot-with-size record from NVM.
 *
 * @param p_store Storage backend API.
 * @param addr Record start address.
 * @param par_num Live parameter number.
 * @param p_obj Output canonical object.
 * @return Operation status.
 */
par_status_t par_nvm_layout_read(const par_store_backend_api_t * const p_store,
                                 const uint32_t addr,
                                 const par_num_t par_num,
                                 par_nvm_data_obj_t * const p_obj)
{
    par_nvm_layout_fixed_slot_with_size_record_t record = { 0U };
    uint8_t crc_calc = 0U;

    (void)par_num;
    PAR_ASSERT((NULL != p_store) && (NULL != p_obj));
    memset(p_obj, 0, sizeof(*p_obj));

    if (ePAR_OK != p_store->read(addr, PAR_NVM_LAYOUT_RECORD_SIZE, (uint8_t *)&record))
    {
        return ePAR_ERROR_NVM;
    }

    if (record.size != PAR_NVM_RECORD_DATA_SLOT_SIZE)
    {
        return ePAR_ERROR;
    }

    crc_calc = par_nvm_layout_calc_crc_with_id(record.id,
                                               record.size,
                                               (const uint8_t * const)&record.data,
                                               PAR_NVM_RECORD_DATA_SLOT_SIZE,
                                               true);
    if (crc_calc != record.crc)
    {
        return ePAR_ERROR_CRC;
    }

    p_obj->id = record.id;
    p_obj->size = record.size;
    p_obj->data = record.data;
    return ePAR_OK;
}

/**
 * @brief Write one fixed-slot-with-size record to NVM.
 *
 * @param p_store Storage backend API.
 * @param addr Record start address.
 * @param par_num Live parameter number.
 * @param p_obj Canonical object to serialize.
 * @return Operation status.
 */
par_status_t par_nvm_layout_write(const par_store_backend_api_t * const p_store,
                                  const uint32_t addr,
                                  const par_num_t par_num,
                                  const par_nvm_data_obj_t * const p_obj)
{
    par_nvm_layout_fixed_slot_with_size_record_t record = { 0U };

    (void)par_num;
    PAR_ASSERT((NULL != p_store) && (NULL != p_obj));

    record.id = p_obj->id;
    record.size = PAR_NVM_RECORD_DATA_SLOT_SIZE;
    record.data = p_obj->data;
    record.crc = par_nvm_layout_calc_crc_with_id(record.id,
                                                 record.size,
                                                 (const uint8_t * const)&record.data,
                                                 PAR_NVM_RECORD_DATA_SLOT_SIZE,
                                                 true);

    return (ePAR_OK == p_store->write(addr, PAR_NVM_LAYOUT_RECORD_SIZE, (const uint8_t *)&record)) ? ePAR_OK : ePAR_ERROR_NVM;
}

#endif /* fixed-slot-with-size */
