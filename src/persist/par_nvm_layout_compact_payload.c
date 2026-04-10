/**
 * @file par_nvm_layout_compact_payload.c
 * @brief Implement the compact persisted-record layout with natural payload width.
 * @author wdfk-prog ()
 * @version 1.0
 * @date 2026-04-06
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @details
 * This compact layout stores only the natural payload width of each parameter
 * type (1/2/4 bytes). For that reason, serialization and deserialization are
 * implemented through explicit type-matched local objects instead of by taking
 * the first N bytes from a wider 32-bit temporary.
 *
 * Copying partial bytes from a uint32_t object is endianness-sensitive.
 * On little-endian targets the first bytes in memory happen to contain the
 * narrow U8/U16 payload, but on big-endian targets those first bytes belong
 * to the high-order part of the 32-bit object representation. That would make
 * narrow-value storage and CRC calculation depend on target endianness in the
 * wrong way.
 *
 * To keep the native-endian storage model self-consistent on both little-
 * endian and big-endian MCUs, this implementation packs and unpacks
 * U8/I8, U16/I16, and U32/I32/F32 through explicit same-width typed objects
 * and copies exactly that full object representation.
 *
 * @par Change Log:
 * Date       Version Author      Description
 * 2026-04-06 1.0     wdfk-prog   first version
 */
#include "persist/par_nvm_layout.h"

#if (1 == PAR_CFG_NVM_EN) && (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_COMPACT_PAYLOAD)

#include <string.h>

#define PAR_NVM_LAYOUT_RECORD_OVERHEAD (PAR_NVM_RECORD_ID_SIZE + PAR_NVM_RECORD_SIZE_FIELD_SIZE + PAR_NVM_RECORD_CRC_SIZE)
#define PAR_NVM_LAYOUT_RECORD_MAX_SIZE (PAR_NVM_LAYOUT_RECORD_OVERHEAD + PAR_NVM_RECORD_DATA_SLOT_SIZE)

/**
 * @brief Resolve natural payload size from parameter type.
 *
 * @param type Parameter type.
 * @return Natural payload width in bytes.
 */
static uint8_t par_nvm_layout_payload_size_from_type(const par_type_list_t type)
{
    switch (type)
    {
    case ePAR_TYPE_U8:
    case ePAR_TYPE_I8:
        return 1U;

    case ePAR_TYPE_U16:
    case ePAR_TYPE_I16:
        return 2U;

    case ePAR_TYPE_U32:
    case ePAR_TYPE_I32:
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
    case ePAR_TYPE_F32:
#endif
        return 4U;

    case ePAR_TYPE_NUM_OF:
    default:
        PAR_ASSERT(0);
        return PAR_NVM_RECORD_DATA_SLOT_SIZE;
    }
}

/**
 * @brief Serialize one parameter value to native-endian payload bytes.
 *
 * @details Narrow integer types are serialized through explicit typed locals
 * so the compact layout does not depend on the byte placement of a 32-bit
 * temporary carrier object.
 *
 * @param type Parameter type.
 * @param p_data Canonical parameter value.
 * @param p_payload Output payload buffer.
 */
static void par_nvm_layout_pack_payload_bytes(const par_type_list_t type,
                                              const par_type_t * const p_data,
                                              uint8_t * const p_payload)
{
    PAR_ASSERT((NULL != p_data) && (NULL != p_payload));

    switch (type)
    {
    case ePAR_TYPE_U8:
    {
        const uint8_t value = (uint8_t)(*p_data);
        memcpy(p_payload, &value, sizeof(value));
        break;
    }

    case ePAR_TYPE_I8:
    {
        const int8_t value = (int8_t)(*p_data);
        memcpy(p_payload, &value, sizeof(value));
        break;
    }

    case ePAR_TYPE_U16:
    {
        const uint16_t value = (uint16_t)(*p_data);
        memcpy(p_payload, &value, sizeof(value));
        break;
    }

    case ePAR_TYPE_I16:
    {
        const int16_t value = (int16_t)(*p_data);
        memcpy(p_payload, &value, sizeof(value));
        break;
    }

    case ePAR_TYPE_U32:
    case ePAR_TYPE_I32:
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
    case ePAR_TYPE_F32:
#endif
        memcpy(p_payload, p_data, PAR_NVM_RECORD_DATA_SLOT_SIZE);
        break;

    case ePAR_TYPE_NUM_OF:
    default:
        PAR_ASSERT(0);
        break;
    }
}

/**
 * @brief Deserialize native-endian payload bytes into the canonical value carrier.
 *
 * @details Narrow integer types are reconstructed through explicit typed locals
 * so the compact layout does not depend on partial writes into a 32-bit
 * carrier object's storage representation.
 *
 * @param type Parameter type.
 * @param p_payload Input payload buffer.
 * @param p_data Output canonical parameter value.
 */
static void par_nvm_layout_unpack_payload_bytes(const par_type_list_t type,
                                                const uint8_t * const p_payload,
                                                par_type_t * const p_data)
{
    PAR_ASSERT((NULL != p_payload) && (NULL != p_data));

    switch (type)
    {
    case ePAR_TYPE_U8:
    {
        uint8_t value = 0U;
        memcpy(&value, p_payload, sizeof(value));
        *p_data = (par_type_t)value;
        break;
    }

    case ePAR_TYPE_I8:
    {
        int8_t value = 0;
        memcpy(&value, p_payload, sizeof(value));
        *p_data = (par_type_t)value;
        break;
    }

    case ePAR_TYPE_U16:
    {
        uint16_t value = 0U;
        memcpy(&value, p_payload, sizeof(value));
        *p_data = (par_type_t)value;
        break;
    }

    case ePAR_TYPE_I16:
    {
        int16_t value = 0;
        memcpy(&value, p_payload, sizeof(value));
        *p_data = (par_type_t)value;
        break;
    }

    case ePAR_TYPE_U32:
    case ePAR_TYPE_I32:
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
    case ePAR_TYPE_F32:
#endif
        memcpy(p_data, p_payload, PAR_NVM_RECORD_DATA_SLOT_SIZE);
        break;

    case ePAR_TYPE_NUM_OF:
    default:
        PAR_ASSERT(0);
        break;
    }
}

/**
 * @brief Resolve serialized record size from payload width.
 *
 * @param payload_size Payload width in bytes.
 * @return Serialized compact-record size in bytes.
 */
static uint32_t par_nvm_layout_record_size_from_payload_size(const uint8_t payload_size)
{
    return (PAR_NVM_LAYOUT_RECORD_OVERHEAD + (uint32_t)payload_size);
}

/**
 * @brief Get serialized record size for one live parameter.
 *
 * @param par_num Live parameter number.
 * @return Serialized compact-record size in bytes.
 */
uint32_t par_nvm_layout_record_size_from_par_num(const par_num_t par_num)
{
    const par_cfg_t * const p_cfg = par_get_config(par_num);

    PAR_ASSERT(NULL != p_cfg);
    return par_nvm_layout_record_size_from_payload_size(par_nvm_layout_payload_size_from_type(p_cfg->type));
}

/**
 * @brief Resolve record address from persistent slot index.
 *
 * @param first_data_obj_addr Start address of the first data record.
 * @param persist_idx Persistent slot index.
 * @param p_persist_slot_to_par_num Compile-time persistent slot map.
 * @return Start address of the serialized record.
 */
uint32_t par_nvm_layout_addr_from_persist_idx(const uint32_t first_data_obj_addr,
                                              const uint16_t persist_idx,
                                              const par_num_t * const p_persist_slot_to_par_num)
{
    uint32_t addr = first_data_obj_addr;

    PAR_ASSERT(NULL != p_persist_slot_to_par_num);

    for (uint16_t it = 0U; it < persist_idx; it++)
    {
        addr += par_nvm_layout_record_size_from_par_num(p_persist_slot_to_par_num[it]);
    }

    return addr;
}

/**
 * @brief Read one compact-payload record.
 *
 * @param p_store Mounted storage backend.
 * @param addr Serialized record address.
 * @param par_num Live parameter number.
 * @param p_obj Output canonical payload view.
 * @return Operation status.
 */
par_status_t par_nvm_layout_read(const par_store_backend_api_t * const p_store,
                                 const uint32_t addr,
                                 const par_num_t par_num,
                                 par_nvm_data_obj_t * const p_obj)
{
    uint8_t record_buf[PAR_NVM_LAYOUT_RECORD_MAX_SIZE] = { 0U };
    uint8_t size_desc = 0U;
    uint8_t crc_stored = 0U;
    uint8_t crc_calc = 0U;
    const uint8_t * const p_payload = &record_buf[PAR_NVM_RECORD_ID_SIZE + PAR_NVM_RECORD_SIZE_FIELD_SIZE + PAR_NVM_RECORD_CRC_SIZE];
    const par_cfg_t * const p_cfg = par_get_config(par_num);
    const uint8_t expected_payload_size = par_nvm_layout_payload_size_from_type(p_cfg->type);
    const uint32_t record_size = par_nvm_layout_record_size_from_payload_size(expected_payload_size);

    PAR_ASSERT(NULL != p_cfg);

    PAR_ASSERT((NULL != p_store) && (NULL != p_obj));
    memset(p_obj, 0, sizeof(*p_obj));

    if (ePAR_OK != p_store->read(addr, record_size, record_buf))
    {
        return ePAR_ERROR_NVM;
    }

    memcpy(&p_obj->id, &record_buf[0], sizeof(p_obj->id));
    size_desc = record_buf[PAR_NVM_RECORD_ID_SIZE];
    crc_stored = record_buf[PAR_NVM_RECORD_ID_SIZE + PAR_NVM_RECORD_SIZE_FIELD_SIZE];

    if (size_desc != expected_payload_size)
    {
        return ePAR_ERROR;
    }

    crc_calc = par_nvm_layout_calc_crc(p_obj->id, size_desc, p_payload, size_desc, true);
    if (crc_calc != crc_stored)
    {
        return ePAR_ERROR_CRC;
    }

    p_obj->size = size_desc;
    par_nvm_layout_unpack_payload_bytes(p_cfg->type, p_payload, &p_obj->data);
    return ePAR_OK;
}

/**
 * @brief Write one compact-payload record.
 *
 * @param p_store Mounted storage backend.
 * @param addr Serialized record address.
 * @param par_num Live parameter number.
 * @param p_obj Input canonical payload view.
 * @return Operation status.
 */
par_status_t par_nvm_layout_write(const par_store_backend_api_t * const p_store,
                                  const uint32_t addr,
                                  const par_num_t par_num,
                                  const par_nvm_data_obj_t * const p_obj)
{
    uint8_t record_buf[PAR_NVM_LAYOUT_RECORD_MAX_SIZE] = { 0U };
    uint8_t * const p_payload = &record_buf[PAR_NVM_RECORD_ID_SIZE + PAR_NVM_RECORD_SIZE_FIELD_SIZE + PAR_NVM_RECORD_CRC_SIZE];
    uint8_t crc = 0U;
    const par_cfg_t * const p_cfg = par_get_config(par_num);
    const uint8_t payload_size = par_nvm_layout_payload_size_from_type(p_cfg->type);
    const uint32_t record_size = par_nvm_layout_record_size_from_payload_size(payload_size);

    PAR_ASSERT((NULL != p_store) && (NULL != p_obj));
    PAR_ASSERT(NULL != p_cfg);

    par_nvm_layout_pack_payload_bytes(p_cfg->type, &p_obj->data, p_payload);
    crc = par_nvm_layout_calc_crc(p_obj->id, payload_size, p_payload, payload_size, true);

    memcpy(&record_buf[0], &p_obj->id, sizeof(p_obj->id));
    record_buf[PAR_NVM_RECORD_ID_SIZE] = payload_size;
    record_buf[PAR_NVM_RECORD_ID_SIZE + PAR_NVM_RECORD_SIZE_FIELD_SIZE] = crc;

    return (ePAR_OK == p_store->write(addr, record_size, record_buf)) ? ePAR_OK : ePAR_ERROR_NVM;
}

#endif /* compact-payload */
