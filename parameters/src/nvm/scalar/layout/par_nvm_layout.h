/**
 * @file par_nvm_layout.h
 * @brief Declare private persisted-record layout interfaces.
 * @author wdfk-prog
 * @version 1.2
 * @date 2026-04-13
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-04-06 1.0     wdfk-prog     first version
 * 2026-04-11 1.1     wdfk-prog     split layout structs by selected NVM layout
 * 2026-04-13 1.2     wdfk-prog     add layout-ops registration for NVM core
 */
#ifndef _PAR_NVM_LAYOUT_H_
#define _PAR_NVM_LAYOUT_H_

#include <stdbool.h>
#include <stdint.h>

#include "par.h"
#include "par_store_backend.h"

#if (1 == PAR_CFG_NVM_EN)

#define PAR_NVM_RECORD_ID_SIZE         ((uint32_t)sizeof(uint16_t))
#define PAR_NVM_RECORD_SIZE_FIELD_SIZE ((uint32_t)sizeof(uint8_t))
#define PAR_NVM_RECORD_CRC_SIZE        ((uint32_t)sizeof(uint8_t))
#define PAR_NVM_RECORD_DATA_SLOT_SIZE  ((uint8_t)sizeof(par_type_t))

/**
 * @brief Parameter NVM header object.
 *
 * @details This private header shape is shared between the common NVM flow and
 * the selected record-layout adapter so layout-specific compatibility policy
 * can be implemented below the top-level `par_nvm.c` logic.
 */
typedef struct
{
    uint32_t sign;      /**< Signature in host order. */
    uint16_t obj_nb;    /**< Stored data object number in host order. */
    uint32_t table_id;  /**< Stored parameter-table ID in platform-native order. */
    uint16_t crc;       /**< Header CRC-16 over serialized obj_nb and table_id. */
} par_nvm_head_obj_t;

/**
 * @brief Result of comparing the stored NVM image against the live schema.
 */
typedef enum
{
    ePAR_NVM_COMPAT_REBUILD = 0, /**< Stored image is incompatible and must be rebuilt. */
    ePAR_NVM_COMPAT_EXACT_MATCH, /**< Stored image matches the live schema exactly. */
    ePAR_NVM_COMPAT_PREFIX_APPEND /**< Stored prefix is compatible and new tail slots may be appended. */
} par_nvm_compat_result_t;

#if (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE)
/**
 * @brief Selected fixed-slot layout with explicit size descriptor.
 */
typedef struct
{
    uint16_t id;     /**< Parameter ID. */
    uint8_t size;    /**< Serialized payload-size descriptor. */
    uint8_t crc;     /**< CRC-8 over id, size, and payload bytes. */
    par_type_t data; /**< Fixed 4-byte payload slot. */
} par_nvm_layout_fixed_slot_with_size_record_t;

typedef struct
{
    uint16_t id;     /**< Parameter ID. */
    uint8_t size;    /**< Serialized payload-size descriptor. */
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
    uint16_t id;                                     /**< Parameter ID. */
    uint8_t crc;                                     /**< CRC-8 over id and payload bytes. */
    uint8_t payload[PAR_NVM_RECORD_DATA_SLOT_SIZE];  /**< Fixed 4-byte payload slot. */
} par_nvm_layout_fixed_slot_no_size_record_t;

typedef struct
{
    uint16_t id;     /**< Parameter ID. */
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
    uint16_t id;                                     /**< Parameter ID. */
    uint8_t size;                                    /**< Serialized payload-size descriptor. */
    uint8_t crc;                                     /**< CRC-8 over id, size, and payload bytes. */
    uint8_t payload[PAR_NVM_RECORD_DATA_SLOT_SIZE];  /**< Maximum payload storage. */
} par_nvm_layout_compact_payload_record_t;

typedef struct
{
    uint16_t id;     /**< Parameter ID. */
    uint8_t size;    /**< Serialized payload-size descriptor. */
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
    uint8_t crc;                                     /**< CRC-8 over payload bytes. */
    uint8_t payload[PAR_NVM_RECORD_DATA_SLOT_SIZE];  /**< Maximum payload storage. */
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
    uint8_t crc;                                     /**< CRC-8 over payload bytes. */
    uint8_t payload[PAR_NVM_RECORD_DATA_SLOT_SIZE];  /**< Maximum payload storage. */
} par_nvm_layout_grouped_payload_only_record_t;

typedef struct
{
    par_type_t data; /**< Canonical parameter value. */
} par_nvm_layout_grouped_payload_only_data_obj_t;

typedef par_nvm_layout_grouped_payload_only_data_obj_t par_nvm_data_obj_t;
#else
#error "Unsupported PAR_CFG_NVM_RECORD_LAYOUT selection."
#endif /* (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE) */

/**
 * @brief Selected persisted-record layout vtable.
 *
 * @details Each concrete layout owns its own object preparation, serialized
 * read/write path, compatibility decision, and optional readback-compare
 * policy. `par_nvm.c` binds this table once during initialization and then
 * operates only through these callbacks.
 */
typedef struct
{
    /**
     * @brief Return the serialized byte size of one persisted record.
     *
     * @param par_num Live parameter number that owns the persistent slot.
     * @return Serialized record size in bytes for the selected layout.
     */
    uint32_t (*record_size_from_par_num)(const par_num_t par_num);
    /**
     * @brief Translate one persistent slot index into its serialized storage address.
     *
     * @param first_data_obj_addr Absolute start address of the first persisted record.
     * @param persist_idx Compile-time persistent slot index.
     * @param p_persist_slot_to_par_num Compile-time slot-to-parameter mapping table.
     * @return Absolute address of the selected serialized record.
     */
    uint32_t (*addr_from_persist_idx)(const uint32_t first_data_obj_addr,
                                      const uint16_t persist_idx,
                                      const par_num_t * const p_persist_slot_to_par_num);
    /**
     * @brief Populate one canonical NVM data object from the live RAM value.
     *
     * @param par_num Live parameter number.
     * @param p_live_data Pointer to the live parameter value in canonical form.
     * @param p_obj Output canonical NVM object prepared for the selected layout.
     */
    void (*populate_data_obj)(const par_num_t par_num,
                              const par_type_t * const p_live_data,
                              par_nvm_data_obj_t * const p_obj);
    /**
     * @brief Read and validate one serialized record from the backend.
     *
     * @param p_store Active storage backend API.
     * @param addr Absolute record address inside the managed NVM image.
     * @param par_num Live parameter number associated with the slot.
     * @param p_obj Output canonical NVM object loaded from storage.
     * @return Operation status. Layout-level CRC and size validation are handled here.
     */
    par_status_t (*read)(const par_store_backend_api_t * const p_store,
                         const uint32_t addr,
                         const par_num_t par_num,
                         par_nvm_data_obj_t * const p_obj);
    /**
     * @brief Serialize and write one canonical NVM object to storage.
     *
     * @param p_store Active storage backend API.
     * @param addr Absolute record address inside the managed NVM image.
     * @param par_num Live parameter number associated with the slot.
     * @param p_obj Canonical NVM object to serialize and persist.
     * @return Operation status.
     */
    par_status_t (*write)(const par_store_backend_api_t * const p_store,
                          const uint32_t addr,
                          const par_num_t par_num,
                          const par_nvm_data_obj_t * const p_obj);
    /**
     * @brief Validate one already loaded canonical object against the live schema.
     *
     * @param par_num Live parameter number expected at this slot.
     * @param p_obj Canonical NVM object that was read from storage.
     * @param pp_reason Output short mismatch reason string for diagnostics.
     * @param p_stored_id Output stored ID value when the layout carries one.
     * @return `ePAR_OK` when the object matches the current live schema.
     */
    par_status_t (*validate_loaded_obj)(const par_num_t par_num,
                                        const par_nvm_data_obj_t * const p_obj,
                                        const char ** const pp_reason,
                                        uint16_t * const p_stored_id);
    /**
     * @brief Return the best stored-ID diagnostic value for an error path.
     *
     * @param par_num Live parameter number associated with the slot.
     * @param p_obj Canonical NVM object that was read from storage.
     * @return Stored ID for diagnostics, or a layout-defined fallback value.
     */
    uint16_t (*get_error_stored_id)(const par_num_t par_num,
                                    const par_nvm_data_obj_t * const p_obj);
    /**
     * @brief Decide whether the stored header remains layout-compatible.
     *
     * @param p_head_obj Validated NVM header loaded from storage.
     * @return Layout-specific compatibility decision for rebuild/exact/prefix-append.
     */
    par_nvm_compat_result_t (*check_compat)(const par_nvm_head_obj_t * const p_head_obj);
#if (1 == PAR_CFG_NVM_WRITE_VERIFY_EN)
    /**
     * @brief Compare an expected object against a read-back object.
     *
     * @param par_num Live parameter number associated with the slot.
     * @param p_expected Canonical object prepared from the live value before write.
     * @param p_actual Canonical object reloaded from storage after write.
     * @return True when the write-readback comparison passes for this layout.
     */
    bool (*data_obj_matches)(const par_num_t par_num,
                             const par_nvm_data_obj_t * const p_expected,
                             const par_nvm_data_obj_t * const p_actual);
#endif /* (1 == PAR_CFG_NVM_WRITE_VERIFY_EN) */
} par_nvm_layout_api_t;

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

/**
 * @brief Bind and return the concrete layout-ops table selected at compile time.
 *
 * @return Non-null pointer to the selected layout adapter.
 */
const par_nvm_layout_api_t *par_nvm_layout_init(void);

#endif /* (1 == PAR_CFG_NVM_EN) */

#endif /* !defined(_PAR_NVM_LAYOUT_H_) */
