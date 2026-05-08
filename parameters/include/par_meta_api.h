/**
 * @file par_meta_api.h
 * @brief Declare public parameter metadata APIs.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#ifndef _PAR_META_API_H_
#define _PAR_META_API_H_

#include "par_types.h"

/**
 * @addtogroup PARAMETERS_API
 * @{ <!-- BEGIN GROUP -->
 */

/**
 * @brief Return one parameter configuration entry without requiring module init.
 * @param par_num Parameter number.
 * @return Pointer to the parameter configuration entry, or NULL when invalid.
 */
const par_cfg_t *par_get_config(const par_num_t par_num);
#if (1 == PAR_CFG_ENABLE_NAME)
/**
 * @brief Return the configured display name for one parameter.
 * @param par_num Parameter number.
 * @return Name string, or NULL when invalid.
 */
const char *par_get_name(const par_num_t par_num);
#endif /* (1 == PAR_CFG_ENABLE_NAME) */
#if (1 == PAR_CFG_ENABLE_RANGE)
/**
 * @brief Return the configured range for one parameter.
 * @param par_num Parameter number.
 * @return Configured parameter range.
 */
par_range_t par_get_range(const par_num_t par_num);
#endif /* (1 == PAR_CFG_ENABLE_RANGE) */
#if (1 == PAR_CFG_ENABLE_UNIT)
/**
 * @brief Return the configured engineering unit for one parameter.
 * @param par_num Parameter number.
 * @return Unit string, or NULL when invalid.
 */
const char *par_get_unit(const par_num_t par_num);
#endif /* (1 == PAR_CFG_ENABLE_UNIT) */
#if (1 == PAR_CFG_ENABLE_DESC)
/**
 * @brief Return the configured description for one parameter.
 * @param par_num Parameter number.
 * @return Description string, or NULL when invalid.
 */
const char *par_get_desc(const par_num_t par_num);
#endif /* (1 == PAR_CFG_ENABLE_DESC) */
/**
 * @brief Return the configured data type for one parameter.
 * @param par_num Parameter number.
 * @return Parameter data type.
 */
par_type_list_t par_get_type(const par_num_t par_num);
#if (1 == PAR_CFG_ENABLE_ACCESS)
/**
 * @brief Return the configured external read/write capability mask for one parameter.
 * @param par_num Parameter number.
 * @return Configured external capability mask.
 */
par_access_t par_get_access(const par_num_t par_num);
#endif /* (1 == PAR_CFG_ENABLE_ACCESS) */
#if (1 == PAR_CFG_ENABLE_ROLE_POLICY)
/**
 * @brief Return the configured external read-role mask for one parameter.
 * @param par_num Parameter number.
 * @return Configured read-role mask.
 */
par_role_t par_get_read_roles(const par_num_t par_num);
/**
 * @brief Return the configured external write-role mask for one parameter.
 * @param par_num Parameter number.
 * @return Configured write-role mask.
 */
par_role_t par_get_write_roles(const par_num_t par_num);
/**
 * @brief Test whether a caller role mask may read one parameter.
 * @param par_num Parameter number.
 * @param roles Caller role mask.
 * @return True when read is allowed; otherwise false.
 */
bool par_can_read(const par_num_t par_num, const par_role_t roles);
/**
 * @brief Test whether a caller role mask may write one parameter.
 * @param par_num Parameter number.
 * @param roles Caller role mask.
 * @return True when write is allowed; otherwise false.
 */
bool par_can_write(const par_num_t par_num, const par_role_t roles);
#endif /* (1 == PAR_CFG_ENABLE_ROLE_POLICY) */
#if (1 == PAR_CFG_NVM_EN)
/**
 * @brief Report whether one parameter is marked persistent.
 * @param par_num Parameter number.
 * @return True when the parameter is persistent; otherwise false.
 */
bool par_is_persistent(const par_num_t par_num);
#endif /* (1 == PAR_CFG_NVM_EN) */
#if (1 == PAR_CFG_ENABLE_ID)
/**
 * @brief Resolve an external parameter ID to an internal parameter number.
 * @param id External parameter ID.
 * @param p_par_num Pointer to the resolved parameter number.
 * @return Operation status.
 */
par_status_t par_get_num_by_id(const uint16_t id, par_num_t * const p_par_num);
/**
 * @brief Resolve an internal parameter number to an external parameter ID.
 * @param par_num Parameter number.
 * @param p_id Pointer to the resolved external parameter ID.
 * @return Operation status.
 */
par_status_t par_get_id_by_num(const par_num_t par_num, uint16_t * const p_id);
#endif /* (1 == PAR_CFG_ENABLE_ID) */


/** @} <!-- END GROUP --> */

#endif /* !defined(_PAR_META_API_H_) */
