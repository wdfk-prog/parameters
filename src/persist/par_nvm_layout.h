/**
 * @file par_nvm_layout.h
 * @brief Declare private persisted-record layout interfaces.
 * @author wdfk-prog ()
 * @version 1.0
 * @date 2026-04-11
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author      Description
 * 2026-04-06 1.0     wdfk-prog   first version
 * 2026-04-11 1.1     wdfk-prog   split layout structs by selected NVM layout
 */
#ifndef _PAR_NVM_LAYOUT_H_
#define _PAR_NVM_LAYOUT_H_

#include <stdbool.h>
#include <stdint.h>

#include "par.h"
#include "persist/backend/par_store_backend.h"

#if (1 == PAR_CFG_NVM_EN)

#define PAR_NVM_RECORD_ID_SIZE         ((uint32_t)sizeof(uint16_t))
#define PAR_NVM_RECORD_SIZE_FIELD_SIZE ((uint32_t)sizeof(uint8_t))
#define PAR_NVM_RECORD_CRC_SIZE        ((uint32_t)sizeof(uint8_t))
#define PAR_NVM_RECORD_DATA_SLOT_SIZE  ((uint8_t)sizeof(par_type_t))

#if (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE)
/**
 * @brief Selected fixed-slot layout with explicit size descriptor.
 */
typedef struct
{
    uint16_t id;   /**< Parameter ID. */
    uint8_t size;  /**< Serialized payload-size descriptor. */
    uint8_t crc;   /**< CRC-8 over id, size, and payload bytes. */
    par_type_t data; /**< Fixed 4-byte payload slot. */
} par_nvm_layout_fixed_slot_with_size_record_t;

typedef struct
{
    uint16_t id;   /**< Parameter ID. */
    uint8_t size;  /**< Serialized payload-size descriptor. */
    par_type_t data; /**< Canonical parameter value. */
} par_nvm_layout_fixed_slot_with_size_data_obj_t;

typedef par_nvm_layout_fixed_slot_with_size_data_obj_t par_nvm_data_obj_t;
#elif (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE)
/**
 * @brief Selected fixed-slot layout without a size descriptor.
 *
 * @note The serialized image is 7 bytes: id[2] + crc[1] + payload[4].
 * Do not derive the persisted size from sizeof(this type) because some
 * compilers may append trailing padding to the in-memory view.
 */
typedef struct
{
    uint16_t id;     /**< Parameter ID. */
    uint8_t crc;     /**< CRC-8 over id and payload bytes. */
    uint8_t payload[PAR_NVM_RECORD_DATA_SLOT_SIZE]; /**< Fixed 4-byte payload slot. */
} par_nvm_layout_fixed_slot_no_size_record_t;

typedef struct
{
    uint16_t id; /**< Parameter ID. */
    par_type_t data; /**< Canonical parameter value. */
} par_nvm_layout_fixed_slot_no_size_data_obj_t;

typedef par_nvm_layout_fixed_slot_no_size_data_obj_t par_nvm_data_obj_t;
#elif (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_COMPACT_PAYLOAD)
/**
 * @brief Selected compact layout with id, size, crc, and variable payload bytes.
 *
 * @note The payload width is variable. Only the first @ref size bytes inside
 * @ref payload belong to the serialized record.
 */
typedef struct
{
    uint16_t id;     /**< Parameter ID. */
    uint8_t size;    /**< Serialized payload-size descriptor. */
    uint8_t crc;     /**< CRC-8 over id, size, and payload bytes. */
    uint8_t payload[PAR_NVM_RECORD_DATA_SLOT_SIZE]; /**< Maximum payload storage. */
} par_nvm_layout_compact_payload_record_t;

typedef struct
{
    uint16_t id;   /**< Parameter ID. */
    uint8_t size;  /**< Serialized payload-size descriptor. */
    par_type_t data; /**< Canonical parameter value. */
} par_nvm_layout_compact_payload_data_obj_t;

typedef par_nvm_layout_compact_payload_data_obj_t par_nvm_data_obj_t;
#elif (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_PAYLOAD_ONLY)
/**
 * @brief Selected fixed persistent-order payload-only layout.
 *
 * @note Only crc + active payload bytes are serialized. The persisted record
 * does not contain an ID field or a size descriptor.
 */
typedef struct
{
    uint8_t crc;     /**< CRC-8 over payload bytes. */
    uint8_t payload[PAR_NVM_RECORD_DATA_SLOT_SIZE]; /**< Maximum payload storage. */
} par_nvm_layout_fixed_payload_only_record_t;

typedef struct
{
    par_type_t data; /**< Canonical parameter value. */
} par_nvm_layout_fixed_payload_only_data_obj_t;

typedef par_nvm_layout_fixed_payload_only_data_obj_t par_nvm_data_obj_t;
#elif (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY)
/**
 * @brief Selected grouped payload-only layout.
 *
 * @note Only crc + active payload bytes are serialized. Persistent-slot order
 * is regrouped into 8-bit, 16-bit, and 32-bit payload bands.
 */
typedef struct
{
    uint8_t crc;     /**< CRC-8 over payload bytes. */
    uint8_t payload[PAR_NVM_RECORD_DATA_SLOT_SIZE]; /**< Maximum payload storage. */
} par_nvm_layout_grouped_payload_only_record_t;

typedef struct
{
    par_type_t data; /**< Canonical parameter value. */
} par_nvm_layout_grouped_payload_only_data_obj_t;

typedef par_nvm_layout_grouped_payload_only_data_obj_t par_nvm_data_obj_t;
#else
#error "Unsupported PAR_CFG_NVM_RECORD_LAYOUT selection."
#endif

uint8_t par_nvm_layout_payload_size_from_type(const par_type_list_t type);
uint8_t par_nvm_layout_payload_size_from_par_num(const par_num_t par_num);
void par_nvm_layout_pack_payload_bytes(const par_type_list_t type,
                                       const par_type_t * const p_data,
                                       uint8_t * const p_payload);
void par_nvm_layout_unpack_payload_bytes(const par_type_list_t type,
                                         const uint8_t * const p_payload,
                                         par_type_t * const p_data);

uint8_t par_nvm_layout_calc_crc(const uint8_t size_desc,
                                const uint8_t * const p_payload,
                                const uint8_t payload_size,
                                const bool include_size_desc);
uint8_t par_nvm_layout_calc_crc_with_id(const uint16_t id,
                                        const uint8_t size_desc,
                                        const uint8_t * const p_payload,
                                        const uint8_t payload_size,
                                        const bool include_size_desc);

uint32_t par_nvm_layout_record_size_from_par_num(const par_num_t par_num);
uint32_t par_nvm_layout_addr_from_persist_idx(const uint32_t first_data_obj_addr,
                                              const uint16_t persist_idx,
                                              const par_num_t * const p_persist_slot_to_par_num);

par_status_t par_nvm_layout_read(const par_store_backend_api_t * const p_store,
                                 const uint32_t addr,
                                 const par_num_t par_num,
                                 par_nvm_data_obj_t * const p_obj);
par_status_t par_nvm_layout_write(const par_store_backend_api_t * const p_store,
                                  const uint32_t addr,
                                  const par_num_t par_num,
                                  const par_nvm_data_obj_t * const p_obj);

#endif /* 1 == PAR_CFG_NVM_EN */

#endif /* _PAR_NVM_LAYOUT_H_ */
