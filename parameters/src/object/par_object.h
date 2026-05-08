/**
 * @file par_object.h
 * @brief Declare private fixed-capacity object-parameter helpers.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-04-28
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#ifndef _PAR_OBJECT_H_
#define _PAR_OBJECT_H_

#include <stdbool.h>
#include <stdint.h>

#include "par.h"

#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
/**
 * @brief Return whether one parameter type is object-backed.
 *
 * @param type Parameter type to classify.
 * @return true when the type uses the shared object pool.
 */
bool par_object_type_is_object(const par_type_list_t type);

/**
 * @brief Return whether one object default descriptor is valid.
 *
 * @param p_obj_cfg Pointer to object metadata.
 * @return true when default length, range, element-size and pointer are valid.
 */
bool par_object_default_cfg_is_valid(const par_object_cfg_t * const p_obj_cfg);

/**
 * @brief Validate an object payload length against object metadata.
 *
 * @param p_obj_cfg Pointer to object metadata.
 * @param len Payload length in bytes.
 * @return true when length is inside range and aligned to element size.
 */
bool par_object_len_is_valid(const par_object_cfg_t * const p_obj_cfg,
                             const uint16_t len);

/**
 * @brief Return RAM bytes used by the runtime object slot table.
 *
 * @param object_count Number of object slots from the storage layout.
 * @return Runtime slot-table byte size.
 */
uint32_t par_object_slot_table_bytes(const uint16_t object_count);

/**
 * @brief Initialize object slots and payload bytes from parameter defaults.
 *
 * @return Operation status.
 */
par_status_t par_object_init_defaults_from_table(void);

#if (1 == PAR_CFG_ENABLE_RESET_ALL_RAW)
/**
 * @brief Capture the current object pool and slot table as raw-reset defaults.
 */
void par_object_snapshot_default_mirror(void);

/**
 * @brief Restore the object pool and slot table from the raw-reset mirror.
 */
void par_object_restore_default_mirror(void);
#endif /* (1 == PAR_CFG_ENABLE_RESET_ALL_RAW) */

/**
 * @brief Return whether a source buffer overlaps the object payload pool.
 *
 * @param p_data Candidate source pointer.
 * @param len Candidate source length in bytes.
 * @return true when the buffer overlaps the live object pool.
 */
bool par_object_source_overlaps_pool(const uint8_t * const p_data,
                                     const uint16_t len);

/**
 * @brief Write one object payload into its live slot.
 *
 * @param par_num Parameter number.
 * @param p_data Source payload bytes, or NULL when len is zero.
 * @param len Source payload length in bytes.
 * @return Operation status.
 *
 * @note Caller is responsible for external access checks, callbacks and mutex.
 */
par_status_t par_object_write_payload(const par_num_t par_num,
                                      const uint8_t * const p_data,
                                      const uint16_t len);

/**
 * @brief Restore one object parameter to its table default payload.
 *
 * @param par_num Parameter number.
 * @return Operation status.
 *
 * @note Caller is responsible for external access checks and mutex.
 */
par_status_t par_object_write_default(const par_num_t par_num);

/**
 * @brief Export a read-only live object payload view.
 *
 * @param par_num Parameter number.
 * @param pp_data Output pointer to payload bytes.
 * @param p_len Output current payload length in bytes.
 * @param p_capacity Output payload capacity in bytes.
 * @return Operation status.
 */
par_status_t par_object_get_view(const par_num_t par_num,
                                 const uint8_t **pp_data,
                                 uint16_t * const p_len,
                                 uint16_t * const p_capacity);

/**
 * @brief Export a writable live object payload view.
 *
 * @param par_num Parameter number.
 * @param pp_data Output pointer to payload bytes.
 * @param p_capacity Output payload capacity in bytes.
 * @return Operation status.
 */
par_status_t par_object_get_mutable_view(const par_num_t par_num,
                                         uint8_t **pp_data,
                                         uint16_t * const p_capacity);

/**
 * @brief Commit a new live object payload length.
 *
 * @param par_num Parameter number.
 * @param len New payload length in bytes.
 * @param clear_tail Clear bytes after len up to capacity when true.
 * @return Operation status.
 */
par_status_t par_object_commit_len(const par_num_t par_num,
                                   const uint16_t len,
                                   const bool clear_tail);

/**
 * @brief Compare one live object payload with its table default.
 *
 * @param par_num Parameter number.
 * @param p_has_changed Output change flag.
 * @return Operation status.
 *
 * @note Caller is responsible for external access checks and mutex.
 */
par_status_t par_object_payload_changed_from_default(const par_num_t par_num,
                                                     bool * const p_has_changed);

#if (1 == PAR_CFG_NVM_EN)
/**
 * @brief Export a read-only object payload view for NVM serialization.
 *
 * @param par_num Parameter number.
 * @param pp_data Output pointer to live payload bytes.
 * @param p_len Output current payload length in bytes.
 * @param p_capacity Output configured payload capacity in bytes.
 * @return Operation status.
 */
par_status_t par_obj_nvm_export(const par_num_t par_num,
                                const uint8_t **pp_data,
                                uint16_t * const p_len,
                                uint16_t * const p_capacity);

/**
 * @brief Export a writable live object payload window for NVM restore.
 *
 * @param par_num Parameter number.
 * @param pp_data Output pointer to writable payload bytes.
 * @param p_capacity Output configured payload capacity in bytes.
 * @return Operation status.
 */
par_status_t par_obj_nvm_restore_window(const par_num_t par_num,
                                        uint8_t **pp_data,
                                        uint16_t * const p_capacity);

/**
 * @brief Commit a restored object payload length after bytes were validated.
 *
 * @param par_num Parameter number.
 * @param len Restored payload length in bytes.
 * @return Operation status.
 */
par_status_t par_obj_nvm_commit_restore(const par_num_t par_num,
                                        const uint16_t len);

/**
 * @brief Restore one object payload from a validated byte buffer.
 *
 * @param par_num Parameter number.
 * @param p_data Pointer to restored payload bytes.
 * @param len Restored payload length in bytes.
 * @return Operation status.
 */
par_status_t par_obj_nvm_restore(const par_num_t par_num,
                                 const uint8_t * const p_data,
                                 const uint16_t len);
#endif /* (1 == PAR_CFG_NVM_EN) */
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

#endif /* !defined(_PAR_OBJECT_H_) */
