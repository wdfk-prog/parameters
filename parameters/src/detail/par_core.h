/**
 * @file par_core.h
 * @brief Declare private helpers shared by scalar and object API units.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-05-01
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#ifndef _PAR_CORE_H_
#define _PAR_CORE_H_

#include <stdbool.h>

#include "par.h"

/**
 * @brief Resolve one parameter metadata entry without requiring module init.
 *
 * @param par_num Parameter number.
 * @param p_arg Optional argument pointer to validate.
 * @param require_arg True when @p p_arg must not be NULL.
 * @param pp_cfg Optional output pointer to parameter metadata.
 * @return Status of operation.
 */
par_status_t par_core_resolve_metadata(const par_num_t par_num,
                                       const void * const p_arg,
                                       const bool require_arg,
                                       const par_cfg_t ** const pp_cfg);

/**
 * @brief Resolve one parameter metadata entry and require module init.
 *
 * @param par_num Parameter number.
 * @param p_arg Optional argument pointer to validate.
 * @param require_arg True when @p p_arg must not be NULL.
 * @param pp_cfg Optional output pointer to parameter metadata.
 * @return Status of operation.
 */
par_status_t par_core_resolve_runtime(const par_num_t par_num,
                                      const void * const p_arg,
                                      const bool require_arg,
                                      const par_cfg_t ** const pp_cfg);

#if (1 == PAR_CFG_ENABLE_ACCESS)
/**
 * @brief Return whether the access mask contains read capability.
 *
 * @param access Parameter access mask.
 * @return true when the parameter may be read externally.
 */
bool par_core_access_has_read(const par_access_t access);

/**
 * @brief Return whether the access mask grants external write capability.
 *
 * @param access Parameter access mask.
 * @return true when the parameter may be written externally.
 */
bool par_core_access_has_write(const par_access_t access);
#endif /* (1 == PAR_CFG_ENABLE_ACCESS) */


#if (1 == PAR_CFG_ENABLE_TYPE_F32)
/**
 * @brief Compare two scalar F32 values by raw bit pattern.
 *
 * @param lhs Left-hand scalar float value.
 * @param rhs Right-hand scalar float value.
 * @return true if raw 32-bit representations are equal.
 */
bool par_core_scalar_f32_bits_equal(const float32_t lhs, const float32_t rhs);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */

#if (1 == PAR_CFG_ENABLE_TYPE_F32)
/**
 * @brief Patch scalar F32 defaults into the live scalar storage.
 */
void par_core_scalar_patch_f32_defaults_from_table(void);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */

#if (1 == PAR_CFG_ENABLE_RESET_ALL_RAW)
/**
 * @brief Capture scalar defaults for the raw reset path.
 */
void par_core_scalar_snapshot_default_mirror(void);
#endif /* (1 == PAR_CFG_ENABLE_RESET_ALL_RAW) */

#if (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION)
/**
 * @brief Run the registered scalar validation callback, when present.
 *
 * @param par_num Parameter number.
 * @param val Candidate scalar value.
 * @return true when no callback is registered or the callback accepts the value.
 */
bool par_core_scalar_validation_accepts(const par_num_t par_num, const par_type_t val);
#endif /* (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION) */

#if (1 == PAR_CFG_ENABLE_CHANGE_CALLBACK)
/**
 * @brief Notify the registered scalar on-change callback when the value changed.
 *
 * @param par_num Parameter number.
 * @param new_val New scalar value.
 * @param old_val Previous scalar value.
 * @param p_value_changed Optional precomputed changed-state pointer.
 * @note Called from the checked scalar setter path after the live value has
 * been written, while the caller still owns the parameter lock.
 */
void par_core_notify_scalar_change_if_changed(const par_num_t par_num,
                                              const par_type_t new_val,
                                              const par_type_t old_val,
                                              const bool * const p_value_changed);
#endif /* (1 == PAR_CFG_ENABLE_CHANGE_CALLBACK) */

#if (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
/**
 * @brief Run the registered object validation callback, when present.
 *
 * @param par_num Parameter number.
 * @param p_data Pointer to candidate object payload bytes.
 * @param len Candidate object payload length in bytes.
 * @return true when no callback is registered or the callback accepts the payload.
 */
bool par_core_object_validation_accepts(const par_num_t par_num,
                                        const uint8_t * const p_data,
                                        const uint16_t len);

/**
 * @brief Register one object validation callback in the shared callback table.
 *
 * @param par_num Parameter number.
 * @param validation Object validation callback function pointer.
 */
void par_core_register_obj_validation(const par_num_t par_num,
                                      const pf_par_obj_validation_t validation);
#endif /* (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

#endif /* !defined(_PAR_CORE_H_) */
