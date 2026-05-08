/**
 * @file par_nvm_scalar.c
 * @brief Implement scalar NVM layout helper routines.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-05-01
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include "par_nvm_layout.h"

#if (1 == PAR_CFG_NVM_EN)

#include <stddef.h>
#include <string.h>

#include "par_cfg.h"
#include "par_if.h"

/**
 * @brief Resolve natural payload size from parameter type.
 *
 * @param type Parameter type.
 * @return Natural payload width in bytes.
 */
uint8_t par_nvm_layout_payload_size_from_type(const par_type_list_t type)
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
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */
        return 4U;

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
        PAR_ASSERT(0);
        return PAR_NVM_RECORD_DATA_SLOT_SIZE;

    case ePAR_TYPE_NUM_OF:
    default:
        PAR_ASSERT(0);
        return PAR_NVM_RECORD_DATA_SLOT_SIZE;
    }
}

/**
 * @brief Resolve one persistent slot payload width.
 *
 * @param par_num Live parameter number.
 * @return Natural payload width in bytes.
 */
uint8_t par_nvm_layout_payload_size_from_par_num(const par_num_t par_num)
{
    const par_cfg_t * const p_cfg = par_get_config(par_num);

    PAR_ASSERT(NULL != p_cfg);
    return par_nvm_layout_payload_size_from_type(p_cfg->type);
}

/**
 * @brief Serialize one parameter value to native-endian payload bytes.
 *
 * @param type Parameter type.
 * @param p_data Canonical parameter value.
 * @param p_payload Output payload buffer.
 */
void par_nvm_layout_pack_payload_bytes(const par_type_list_t type,
                                       const par_type_t * const p_data,
                                       uint8_t * const p_payload)
{
    PAR_ASSERT((NULL != p_data) && (NULL != p_payload));

    switch (type)
    {
    case ePAR_TYPE_U8:
    {
        const uint8_t value = p_data->u8;
        memcpy(p_payload, &value, sizeof(value));
        break;
    }

    case ePAR_TYPE_I8:
    {
        const int8_t value = p_data->i8;
        memcpy(p_payload, &value, sizeof(value));
        break;
    }

    case ePAR_TYPE_U16:
    {
        const uint16_t value = p_data->u16;
        memcpy(p_payload, &value, sizeof(value));
        break;
    }

    case ePAR_TYPE_I16:
    {
        const int16_t value = p_data->i16;
        memcpy(p_payload, &value, sizeof(value));
        break;
    }

    case ePAR_TYPE_U32:
    {
        const uint32_t value = p_data->u32;
        memcpy(p_payload, &value, sizeof(value));
        break;
    }

    case ePAR_TYPE_I32:
    {
        const int32_t value = p_data->i32;
        memcpy(p_payload, &value, sizeof(value));
        break;
    }

#if (1 == PAR_CFG_ENABLE_TYPE_F32)
    case ePAR_TYPE_F32:
    {
        const float32_t value = p_data->f32;
        memcpy(p_payload, &value, sizeof(value));
        break;
    }
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */

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
        PAR_ASSERT(0);
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
 * @param type Parameter type.
 * @param p_payload Input payload buffer.
 * @param p_data Output canonical parameter value.
 */
void par_nvm_layout_unpack_payload_bytes(const par_type_list_t type,
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
        p_data->u8 = value;
        break;
    }

    case ePAR_TYPE_I8:
    {
        int8_t value = 0;
        memcpy(&value, p_payload, sizeof(value));
        p_data->i8 = value;
        break;
    }

    case ePAR_TYPE_U16:
    {
        uint16_t value = 0U;
        memcpy(&value, p_payload, sizeof(value));
        p_data->u16 = value;
        break;
    }

    case ePAR_TYPE_I16:
    {
        int16_t value = 0;
        memcpy(&value, p_payload, sizeof(value));
        p_data->i16 = value;
        break;
    }

    case ePAR_TYPE_U32:
    {
        uint32_t value = 0U;
        memcpy(&value, p_payload, sizeof(value));
        p_data->u32 = value;
        break;
    }

    case ePAR_TYPE_I32:
    {
        int32_t value = 0;
        memcpy(&value, p_payload, sizeof(value));
        p_data->i32 = value;
        break;
    }

#if (1 == PAR_CFG_ENABLE_TYPE_F32)
    case ePAR_TYPE_F32:
    {
        float32_t value = 0.0f;
        memcpy(&value, p_payload, sizeof(value));
        p_data->f32 = value;
        break;
    }
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */

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
        PAR_ASSERT(0);
        break;

    case ePAR_TYPE_NUM_OF:
    default:
        PAR_ASSERT(0);
        break;
    }
}

/**
 * @brief Calculate per-record CRC-8 over serialized record bytes without an ID field.
 *
 * @param size_desc Serialized size descriptor.
 * @param p_payload Pointer to payload bytes.
 * @param payload_size Number of payload bytes.
 * @param include_size_desc True when the layout includes a size field.
 * @return Calculated CRC-8 value.
 */
uint8_t par_nvm_layout_calc_crc(const uint8_t size_desc,
                                const uint8_t * const p_payload,
                                const uint8_t payload_size,
                                const bool include_size_desc)
{
    uint8_t crc = PAR_IF_CRC8_INIT;

    PAR_ASSERT(NULL != p_payload);

    if (include_size_desc)
    {
        crc = par_if_crc8_accumulate(crc, (const uint8_t * const)&size_desc, (uint32_t)sizeof(size_desc));
    }
    crc = par_if_crc8_accumulate(crc, p_payload, (uint32_t)payload_size);

    return crc;
}

/**
 * @brief Calculate per-record CRC-8 over serialized record bytes with an ID field.
 *
 * @param id Parameter ID.
 * @param size_desc Serialized size descriptor.
 * @param p_payload Pointer to payload bytes.
 * @param payload_size Number of payload bytes.
 * @param include_size_desc True when the layout includes a size field.
 * @return Calculated CRC-8 value.
 */
uint8_t par_nvm_layout_calc_crc_with_id(const uint16_t id,
                                        const uint8_t size_desc,
                                        const uint8_t * const p_payload,
                                        const uint8_t payload_size,
                                        const bool include_size_desc)
{
    uint8_t crc = PAR_IF_CRC8_INIT;

    PAR_ASSERT(NULL != p_payload);

    crc = par_if_crc8_accumulate(crc, (const uint8_t * const)&id, (uint32_t)sizeof(id));
    if (include_size_desc)
    {
        crc = par_if_crc8_accumulate(crc, (const uint8_t * const)&size_desc, (uint32_t)sizeof(size_desc));
    }
    crc = par_if_crc8_accumulate(crc, p_payload, (uint32_t)payload_size);

    return crc;
}

#endif /* (1 == PAR_CFG_NVM_EN) */
