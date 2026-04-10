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
 * @brief Serialized fixed-slot record with explicit size descriptor.
 *
 * @details This is a private on-storage record view used only by the
 * fixed-slot-with-size backend. It is kept separate from
 * `par_nvm_data_obj_t` because the serialized record also contains the CRC
 * byte between the size field and the data payload.
 */
typedef struct
{
    uint16_t id;   /**< Parameter ID. */
    uint8_t size;  /**< Serialized payload-size descriptor. */
    uint8_t crc;   /**< CRC-8 over id, size, and payload bytes. */
    par_type_t data; /**< Fixed 4-byte payload slot. */
} par_nvm_layout_record_t;

#define PAR_NVM_LAYOUT_RECORD_SIZE ((uint32_t)sizeof(par_nvm_layout_record_t))

PAR_STATIC_ASSERT(par_nvm_layout_fixed_with_size_record_is_8_bytes, (sizeof(par_nvm_layout_record_t) == 8u));

/**
 * @brief Get serialized record size for one live parameter.
 *
 * @param par_num Live parameter number.
 * @return Fixed serialized record size in bytes.
 */
uint32_t par_nvm_layout_record_size_from_par_num(const par_num_t par_num)
{
    (void)par_num;
    return PAR_NVM_LAYOUT_RECORD_SIZE;
}

/**
 * @brief Resolve record address from persistent slot index.
 *
 * @param first_data_obj_addr Start address of the first data record.
 * @param persist_idx Persistent slot index.
 * @param p_persist_slot_to_par_num Unused compile-time slot map.
 * @return Start address of the serialized record.
 */
uint32_t par_nvm_layout_addr_from_persist_idx(const uint32_t first_data_obj_addr,
                                              const uint16_t persist_idx,
                                              const par_num_t * const p_persist_slot_to_par_num)
{
    (void)p_persist_slot_to_par_num;
    return (first_data_obj_addr + ((uint32_t)persist_idx * PAR_NVM_LAYOUT_RECORD_SIZE));
}

/**
 * @brief Read one fixed-slot-with-size record.
 *
 * @param p_store Mounted storage backend.
 * @param addr Serialized record address.
 * @param par_num Unused live parameter number.
 * @param p_obj Output canonical payload view.
 * @return Operation status.
 */
par_status_t par_nvm_layout_read(const par_store_backend_api_t * const p_store,
                                 const uint32_t addr,
                                 const par_num_t par_num,
                                 par_nvm_data_obj_t * const p_obj)
{
    par_nvm_layout_record_t record = { 0U };
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

    crc_calc = par_nvm_layout_calc_crc(record.id, record.size, (const uint8_t * const)&record.data, PAR_NVM_RECORD_DATA_SLOT_SIZE, true);
    if (crc_calc != record.crc)
    {
        return ePAR_ERROR_CRC;
    }

    p_obj->id = record.id;
    p_obj->size = PAR_NVM_RECORD_DATA_SLOT_SIZE;
    p_obj->data = record.data;
    return ePAR_OK;
}

/**
 * @brief Write one fixed-slot-with-size record.
 *
 * @param p_store Mounted storage backend.
 * @param addr Serialized record address.
 * @param par_num Unused live parameter number.
 * @param p_obj Input canonical payload view.
 * @return Operation status.
 */
par_status_t par_nvm_layout_write(const par_store_backend_api_t * const p_store,
                                  const uint32_t addr,
                                  const par_num_t par_num,
                                  const par_nvm_data_obj_t * const p_obj)
{
    par_nvm_layout_record_t record = { 0U };

    (void)par_num;
    PAR_ASSERT((NULL != p_store) && (NULL != p_obj));

    record.id = p_obj->id;
    record.size = PAR_NVM_RECORD_DATA_SLOT_SIZE;
    record.data = p_obj->data;
    record.crc = par_nvm_layout_calc_crc(record.id, record.size, (const uint8_t * const)&record.data, PAR_NVM_RECORD_DATA_SLOT_SIZE, true);

    return (ePAR_OK == p_store->write(addr, PAR_NVM_LAYOUT_RECORD_SIZE, (const uint8_t *)&record)) ? ePAR_OK : ePAR_ERROR_NVM;
}

#endif /* fixed-slot-with-size */
