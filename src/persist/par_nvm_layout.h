/**
 * @file par_nvm_layout.h
 * @brief Declare private persisted-record layout interfaces.
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

#if (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE)
#define PAR_NVM_LAYOUT_OBJ_HAS_SIZE_FIELD (0)
#else
#define PAR_NVM_LAYOUT_OBJ_HAS_SIZE_FIELD (1)
#endif

/**
 * @brief Canonical payload view used by the common NVM flow.
 *
 * @details This type is intentionally not the same thing as every serialized
 * on-storage record. It represents only the logical payload that the common
 * load/save flow needs: identifier, optional layout-selected size descriptor,
 * and parameter bytes. Layouts that place CRC bytes inside the serialized
 * record may use a private local record type in their `.c` file.
 */
typedef struct
{
    uint16_t id; /**< Parameter ID. */
#if (1 == PAR_NVM_LAYOUT_OBJ_HAS_SIZE_FIELD)
    uint8_t size; /**< Layout-selected payload size descriptor. */
#endif
    par_type_t data; /**< Parameter value bytes in native target order. */
} par_nvm_data_obj_t;

uint8_t par_nvm_layout_calc_crc(const uint16_t id,
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
