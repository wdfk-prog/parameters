/**
 * @file par_nvm_layout_grouped_payload_only.c
 * @brief Implement the grouped payload-only NVM layout.
 * @author wdfk-prog ()
 * @version 1.0
 * @date 2026-04-11
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author      Description
 * 2026-04-10 1.0     wdfk-prog   first version
 * 2026-04-11 1.1     wdfk-prog   restore layout comments and split layout structs
 */
#include "persist/par_nvm_layout.h"

#if (1 == PAR_CFG_NVM_EN) && (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY)

#include <string.h>

#if (1 == PAR_CFG_NVM_BACKEND_FLASH_EN)
#error "Payload-only NVM layouts are not supported with the flash backend because records are variable width and not guaranteed to stay 8-byte aligned."
#endif

/**
 * @brief Serialized overhead of one grouped payload-only record.
 */
#define PAR_NVM_LAYOUT_RECORD_OVERHEAD ((uint32_t)PAR_NVM_RECORD_CRC_SIZE)

/**
 * @brief Maximum serialized size of one grouped payload-only record.
 */
#define PAR_NVM_LAYOUT_RECORD_MAX_SIZE (PAR_NVM_LAYOUT_RECORD_OVERHEAD + PAR_NVM_RECORD_DATA_SLOT_SIZE)

PAR_STATIC_ASSERT(par_nvm_layout_grouped_payload_only_record_payload_slot_is_4_bytes,
                  (sizeof(((par_nvm_layout_grouped_payload_only_record_t *)0)->payload) == 4u));

/**
 * @brief Resolve serialized record size from the active payload width.
 *
 * @param payload_size Active payload width in bytes.
 * @return Serialized record size in bytes.
 */
static uint32_t par_nvm_layout_record_size_from_payload_size(const uint8_t payload_size)
{
    return (PAR_NVM_LAYOUT_RECORD_OVERHEAD + (uint32_t)payload_size);
}

/**
 * @brief Get serialized record size for one persistent parameter.
 *
 * @param par_num Live parameter number.
 * @return Serialized record size in bytes.
 */
uint32_t par_nvm_layout_record_size_from_par_num(const par_num_t par_num)
{
    return par_nvm_layout_record_size_from_payload_size(par_nvm_layout_payload_size_from_par_num(par_num));
}

/**
 * @brief Resolve record address for the grouped payload-only layout.
 *
 * @details Persistent records are regrouped into 8-bit, 16-bit, and 32-bit
 * payload bands. Address resolution therefore sums complete preceding groups
 * and then adds the same-group prefix before the target record.
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
    uint32_t total_size_8 = 0U;
    uint32_t total_size_16 = 0U;
    uint32_t prefix_same_group = 0U;
    uint8_t target_payload_size = 0U;

    PAR_ASSERT(NULL != p_persist_slot_to_par_num);
    PAR_ASSERT(persist_idx < PAR_PERSISTENT_COMPILE_COUNT);
    target_payload_size = par_nvm_layout_payload_size_from_par_num(p_persist_slot_to_par_num[persist_idx]);

    for (uint16_t it = 0U; it < PAR_PERSISTENT_COMPILE_COUNT; it++)
    {
        const uint8_t payload_size = par_nvm_layout_payload_size_from_par_num(p_persist_slot_to_par_num[it]);
        const uint32_t record_size = par_nvm_layout_record_size_from_payload_size(payload_size);

        switch (payload_size)
        {
        case 1U:
            total_size_8 += record_size;
            break;

        case 2U:
            total_size_16 += record_size;
            break;

        case 4U:
            break;

        default:
            PAR_ASSERT(0);
            break;
        }

        if ((it < persist_idx) && (payload_size == target_payload_size))
        {
            prefix_same_group += record_size;
        }
    }

    switch (target_payload_size)
    {
    case 1U:
        return (first_data_obj_addr + prefix_same_group);

    case 2U:
        return (first_data_obj_addr + total_size_8 + prefix_same_group);

    case 4U:
        return (first_data_obj_addr + total_size_8 + total_size_16 + prefix_same_group);

    default:
        PAR_ASSERT(0);
        return first_data_obj_addr;
    }
}

/**
 * @brief Read one grouped payload-only record from NVM.
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
    uint8_t record_buf[PAR_NVM_LAYOUT_RECORD_MAX_SIZE] = { 0U };
    const par_cfg_t * const p_cfg = par_get_config(par_num);
    const uint8_t payload_size = par_nvm_layout_payload_size_from_par_num(par_num);
    const uint32_t record_size = par_nvm_layout_record_size_from_payload_size(payload_size);
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
 * @brief Write one grouped payload-only record to NVM.
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
    uint8_t record_buf[PAR_NVM_LAYOUT_RECORD_MAX_SIZE] = { 0U };
    const par_cfg_t * const p_cfg = par_get_config(par_num);
    const uint8_t payload_size = par_nvm_layout_payload_size_from_par_num(par_num);
    const uint32_t record_size = par_nvm_layout_record_size_from_payload_size(payload_size);
    uint8_t * const p_payload = &record_buf[PAR_NVM_RECORD_CRC_SIZE];

    PAR_ASSERT((NULL != p_store) && (NULL != p_obj));
    PAR_ASSERT(NULL != p_cfg);

    par_nvm_layout_pack_payload_bytes(p_cfg->type, &p_obj->data, p_payload);
    record_buf[0] = par_nvm_layout_calc_crc(0U, p_payload, payload_size, false);

    return (ePAR_OK == p_store->write(addr, record_size, record_buf)) ? ePAR_OK : ePAR_ERROR_NVM;
}

#endif /* grouped-payload-only */
