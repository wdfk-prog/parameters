/**
 * @file par_nvm_layout_fixed_slot_no_size.c
 * @brief Implement the fixed-slot persisted-record layout without a size descriptor.
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

#if (1 == PAR_CFG_NVM_EN) && (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE)

#include <string.h>

#if (1 == PAR_CFG_NVM_BACKEND_FLASH_EN)
#error "PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE is not supported with the flash backend because it serializes to 7 bytes."
#endif

/**
 * @brief Serialized record size in bytes.
 *
 * @details This backend stores `id(2) + crc(1) + data(4)` as a 7-byte record.
 * A raw C struct is intentionally not used for on-storage I/O here because the
 * 7-byte wire format does not naturally match the default alignment rules of
 * ordinary structs on common targets.
 */
#define PAR_NVM_LAYOUT_RECORD_SIZE (PAR_NVM_RECORD_ID_SIZE + PAR_NVM_RECORD_CRC_SIZE + PAR_NVM_RECORD_DATA_SLOT_SIZE)

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
 * @brief Read one fixed-slot-without-size record.
 *
 * @details Byte-buffer deserialization is used on purpose instead of direct
 * struct I/O so the 7-byte on-storage format stays independent of compiler
 * padding, packing pragmas, and unaligned member-access code generation.
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
    uint8_t record_buf[PAR_NVM_LAYOUT_RECORD_SIZE] = { 0U };
    uint32_t payload_raw = 0U;
    uint8_t crc_stored = 0U;
    uint8_t crc_calc = 0U;

    (void)par_num;
    PAR_ASSERT((NULL != p_store) && (NULL != p_obj));
    memset(p_obj, 0, sizeof(*p_obj));

    if (ePAR_OK != p_store->read(addr, PAR_NVM_LAYOUT_RECORD_SIZE, record_buf))
    {
        return ePAR_ERROR_NVM;
    }

    memcpy(&p_obj->id, &record_buf[0], sizeof(p_obj->id));
    crc_stored = record_buf[PAR_NVM_RECORD_ID_SIZE];
    memcpy(&payload_raw, &record_buf[PAR_NVM_RECORD_ID_SIZE + PAR_NVM_RECORD_CRC_SIZE], sizeof(payload_raw));

    crc_calc = par_nvm_layout_calc_crc(p_obj->id, 0U, (const uint8_t * const)&payload_raw, PAR_NVM_RECORD_DATA_SLOT_SIZE, false);
    if (crc_calc != crc_stored)
    {
        return ePAR_ERROR_CRC;
    }

    memcpy(&p_obj->data, &payload_raw, sizeof(payload_raw));
    return ePAR_OK;
}

/**
 * @brief Write one fixed-slot-without-size record.
 *
 * @details Byte-buffer serialization is used on purpose instead of direct
 * struct I/O so the 7-byte on-storage format stays stable across compiler
 * padding and alignment choices.
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
    uint8_t record_buf[PAR_NVM_LAYOUT_RECORD_SIZE] = { 0U };
    uint32_t payload_raw = 0U;
    uint8_t crc = 0U;

    (void)par_num;
    PAR_ASSERT((NULL != p_store) && (NULL != p_obj));
    memcpy(&payload_raw, &p_obj->data, sizeof(payload_raw));
    crc = par_nvm_layout_calc_crc(p_obj->id, 0U, (const uint8_t * const)&payload_raw, PAR_NVM_RECORD_DATA_SLOT_SIZE, false);

    memcpy(&record_buf[0], &p_obj->id, sizeof(p_obj->id));
    record_buf[PAR_NVM_RECORD_ID_SIZE] = crc;
    memcpy(&record_buf[PAR_NVM_RECORD_ID_SIZE + PAR_NVM_RECORD_CRC_SIZE], &payload_raw, sizeof(payload_raw));

    return (ePAR_OK == p_store->write(addr, PAR_NVM_LAYOUT_RECORD_SIZE, record_buf)) ? ePAR_OK : ePAR_ERROR_NVM;
}

#endif /* fixed-slot-no-size */
