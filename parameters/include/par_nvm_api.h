/**
 * @file par_nvm_api.h
 * @brief Declare public parameter NVM APIs.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#ifndef _PAR_NVM_API_H_
#define _PAR_NVM_API_H_

#include "par_types.h"

/**
 * @addtogroup PARAMETERS_API
 * @{ <!-- BEGIN GROUP -->
 */

/**
 * @brief Parameter NVM storage API.
 */
#if (1 == PAR_CFG_NVM_EN)
/**
 * @brief Set one scalar parameter and persist it immediately when changed.
 * @details Object-backed rows do not use this scalar convenience API. Use
 * par_set_obj_n_save() for object payloads when object persistence is enabled.
 * @param par_num Parameter number.
 * @param p_val Pointer to the input scalar value.
 * @return Operation status. Returns ePAR_ERROR_TYPE for object rows.
 */
par_status_t par_set_scalar_n_save(const par_num_t par_num, const void *p_val);
#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
/**
 * @brief Set one object parameter and persist it immediately when changed.
 * @details The payload length is expressed in bytes. For STR parameters, len
 * excludes the trailing '\0' and p_data must not contain embedded NUL bytes.
 * For ARR_U16 and ARR_U32 parameters, len must be aligned to the element size
 * configured for the row.
 * @param par_num Parameter number.
 * @param p_data Pointer to the input payload bytes, or NULL when len is zero.
 * @param len Input payload length in bytes.
 * @return Operation status. Returns ePAR_ERROR_TYPE for scalar rows.
 */
par_status_t par_set_obj_n_save(const par_num_t par_num,
                                const uint8_t *p_data,
                                const uint16_t len);
#if (1 == PAR_CFG_ENABLE_ID)
/**
 * @brief Set one object parameter by external ID and persist it immediately when changed.
 * @param id External parameter ID.
 * @param p_data Pointer to the input payload bytes, or NULL when len is zero.
 * @param len Input payload length in bytes.
 * @return Operation status. Returns ePAR_ERROR_TYPE for scalar rows.
 */
par_status_t par_set_obj_n_save_by_id(const uint16_t id,
                                       const uint8_t *p_data,
                                       const uint16_t len);
#endif /* (1 == PAR_CFG_ENABLE_ID) */
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */
/**
 * @brief Persist all scalar and object persistent parameters.
 * @return Operation status.
 */
par_status_t par_save_all(void);
/**
 * @brief Persist one scalar or object parameter.
 * @param par_num Parameter number.
 * @return Operation status.
 */
par_status_t par_save(const par_num_t par_num);
#if (1 == PAR_CFG_ENABLE_ID)
/**
 * @brief Persist one scalar or object parameter by external parameter ID.
 * @param par_id External parameter ID.
 * @return Operation status.
 */
par_status_t par_save_by_id(const uint16_t par_id);
#endif /* (1 == PAR_CFG_ENABLE_ID) */
/**
 * @brief Rewrite the full managed parameter NVM area.
 * @return Operation status.
 */
par_status_t par_save_clean(void);
#endif /* (1 == PAR_CFG_NVM_EN) */


/** @} <!-- END GROUP --> */

#endif /* !defined(_PAR_NVM_API_H_) */
