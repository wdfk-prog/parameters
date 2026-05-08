/**
 * @file par_core_api.h
 * @brief Declare public parameter core lifecycle and maintenance APIs.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#ifndef _PAR_CORE_API_H_
#define _PAR_CORE_API_H_

#include "par_types.h"

/**
 * @addtogroup PARAMETERS_API
 * @{ <!-- BEGIN GROUP -->
 */

/**
 * @brief Initialize the parameter module.
 * @return Operation status.
 */
par_status_t par_init(void);
/**
 * @brief Deinitialize the parameter module.
 * @return Operation status.
 */
par_status_t par_deinit(void);
/**
 * @brief Report whether the parameter module is initialized.
 * @return True when the module is initialized; otherwise false.
 */
bool par_is_init(void);
/**
 * @brief Acquire the parameter lock for one parameter path.
 * @param par_num Parameter number.
 * @return Operation status.
 */
par_status_t par_acquire_mutex(const par_num_t par_num);
/**
 * @brief Release the parameter lock for one parameter path.
 * @param par_num Parameter number.
 */
void par_release_mutex(const par_num_t par_num);
/**
 * @brief Restore one parameter to its configured default value.
 * @param par_num Parameter number.
 * @return Operation status.
 * @note Default restore is an internal recovery/maintenance path and does not
 * enforce external write access or role-policy checks. It also bypasses runtime
 * validation callbacks and on-change callbacks. Range limiting still follows the
 * typed fast setter behavior for scalar rows.
 */
par_status_t par_set_to_default(const par_num_t par_num);
/**
 * @brief Restore all parameters to their configured default values.
 * @return Operation status.
 * @note Bulk default restore is an internal recovery/maintenance path and does
 * not enforce external write access or role-policy checks. When raw reset is
 * disabled, this API iterates through par_set_to_default().
 */
par_status_t par_set_all_to_default(void);
#if (1 == PAR_CFG_ENABLE_RESET_ALL_RAW)
/**
 * @brief Restore all parameters through the raw grouped-storage path.
 * @return Operation status.
 * @note Raw default restore bypasses normal setter hooks, external write access,
 * role-policy checks, validation callbacks, on-change callbacks, and setter-side
 * range behavior.
 */
par_status_t par_reset_all_to_default_raw(void);
#endif /* (1 == PAR_CFG_ENABLE_RESET_ALL_RAW) */

/**
 * @brief Report whether one parameter differs from its default value.
 * @param par_num Parameter number.
 * @param p_has_changed Pointer to the changed-state output flag.
 * @return Operation status.
 */
par_status_t par_has_changed(const par_num_t par_num, bool * const p_has_changed);

/** @} <!-- END GROUP --> */

#endif /* !defined(_PAR_CORE_API_H_) */
