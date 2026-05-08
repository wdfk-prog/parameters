/**
 * @file par_nvm_object.h
 * @brief Declare private NVM support for fixed-capacity object parameters.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-04-28
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#ifndef _PAR_NVM_OBJECT_H_
#define _PAR_NVM_OBJECT_H_

#include <stdint.h>
#include <stdbool.h>

#include "par.h"
#include "par_store_backend.h"
#include "par_nvm_layout.h"

#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
/**
 * @brief Invalidate cached object persistence addresses.
 *
 * @details Call this after scalar layout initialization or deinitialization.
 * The selected address adapter decides whether the scalar block end is used
 * for placement or overlap checks.
 */
void par_nvm_object_invalidate_block_addr_cache(void);

/**
 * @brief Validate active object storage capacity for the compiled object block.
 *
 * @param object_block_addr Object persistence block base address.
 * @return Operation status.
 */
par_status_t par_nvm_object_validate_storage_capacity(const uint32_t object_block_addr);

/**
 * @brief Initialize object persistence through the active object store.
 *
 * @param base_addr Object persistence block base address.
 * @return Operation status.
 */
par_status_t par_nvm_object_init_from_active_store(const uint32_t base_addr);

/**
 * @brief Persist one object parameter through the active object store.
 *
 * @param base_addr Object persistence block base address.
 * @param par_num Parameter number.
 * @param nvm_sync Request backend sync after write when true.
 * @return Operation status.
 */
par_status_t par_nvm_object_write_to_active_store(const uint32_t base_addr,
                                                  const par_num_t par_num,
                                                  const bool nvm_sync);

/**
 * @brief Persist one object parameter while the caller already holds its mutex.
 *
 * @param base_addr Object persistence block base address.
 * @param par_num Parameter number.
 * @param nvm_sync Request backend sync after write when true.
 * @return Operation status.
 */
par_status_t par_nvm_object_write_locked_to_active_store(const uint32_t base_addr,
                                                         const par_num_t par_num,
                                                         const bool nvm_sync);

/**
 * @brief Persist all object parameters through the active object store.
 *
 * @param base_addr Object persistence block base address.
 * @return Operation status.
 */
par_status_t par_nvm_object_write_all_to_active_store(const uint32_t base_addr);

/**
 * @brief Return the cached scalar NVM block end address.
 *
 * @param scalar_first_record_addr First scalar persistent-record address.
 * @param p_layout Active scalar NVM layout adapter.
 * @param p_scalar_slot_to_par_num Scalar persistent-slot map.
 * @return First address after all scalar persistent records, or zero on error.
 */
uint32_t par_nvm_object_get_scalar_block_end_addr(const uint32_t scalar_first_record_addr,
                                                  const par_nvm_layout_api_t * const p_layout,
                                                  const par_num_t * const p_scalar_slot_to_par_num);

/**
 * @brief Return the cached object persistence block base address.
 *
 * @details Store placement is selected by a compile-time address adapter.
 * Shared-store mode uses PAR_CFG_NVM_OBJECT_ADDR_MODE, while dedicated-store
 * mode uses the object backend address space. The core does not migrate
 * object blocks between addresses; use fixed or dedicated placement when the
 * object base must remain stable across scalar layout changes.
 *
 * @param scalar_first_record_addr First scalar persistent-record address.
 * @param p_layout Active scalar NVM layout adapter.
 * @param p_scalar_slot_to_par_num Scalar persistent-slot map.
 * @return Object persistence block base address. Zero is valid only for adapters that allow it.
 */
uint32_t par_nvm_object_get_block_addr(const uint32_t scalar_first_record_addr,
                                       const par_nvm_layout_api_t * const p_layout,
                                       const par_num_t * const p_scalar_slot_to_par_num);

/**
 * @brief Return whether address zero is valid for the selected object adapter.
 *
 * @return true when the selected adapter can intentionally resolve address zero.
 */
bool par_nvm_object_block_addr_zero_is_valid(void);

/**
 * @brief Return the number of persistent object parameters compiled in.
 * @return Persistent object count.
 */
uint16_t par_nvm_object_get_count(void);

/**
 * @brief Return serialized object persistence block size.
 *
 * @return Required object block size in bytes, or zero when no object
 * records are configured.
 */
uint32_t par_nvm_object_get_block_size(void);

/**
 * @brief Return one persistent object record address.
 *
 * @param base_addr Object persistence block base address.
 * @param par_num Parameter number.
 * @return Record address, or zero when the parameter has no object slot.
 */
uint32_t par_nvm_object_get_addr(const uint32_t base_addr, const par_num_t par_num);

/**
 * @brief Initialize object persistence block and restore object values.
 *
 * @param p_store Active storage backend API.
 * @param base_addr Object persistence block base address.
 * @return Operation status.
 */
par_status_t par_nvm_object_init(const par_store_backend_api_t * const p_store,
                                 const uint32_t base_addr);

/**
 * @brief Persist one object parameter to the object persistence block.
 *
 * @param p_store Active storage backend API.
 * @param base_addr Object persistence block base address.
 * @param par_num Parameter number.
 * @param nvm_sync Request backend sync after write when true.
 * @return Operation status.
 */
par_status_t par_nvm_object_write(const par_store_backend_api_t * const p_store,
                                  const uint32_t base_addr,
                                  const par_num_t par_num,
                                  const bool nvm_sync);

/**
 * @brief Persist one object parameter while the parameter mutex is already held.
 *
 * @param p_store Active storage backend API.
 * @param base_addr Object persistence block base address.
 * @param par_num Parameter number.
 * @param nvm_sync Request backend sync after write when true.
 * @return Operation status.
 *
 * @pre Caller must hold the parameter mutex for @p par_num.
 */
par_status_t par_nvm_object_write_locked(const par_store_backend_api_t * const p_store,
                                         const uint32_t base_addr,
                                         const par_num_t par_num,
                                         const bool nvm_sync);

/**
 * @brief Persist all persistent object parameters to the object persistence block.
 *
 * @param p_store Active storage backend API.
 * @param base_addr Object persistence block base address.
 * @return Operation status.
 */
par_status_t par_nvm_object_write_all(const par_store_backend_api_t * const p_store,
                                      const uint32_t base_addr);
#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

#endif /* !defined(_PAR_NVM_OBJECT_H_) */
