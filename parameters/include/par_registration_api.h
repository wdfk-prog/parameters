/**
 * @file par_registration_api.h
 * @brief Declare public registration, port hook, and debug APIs.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#ifndef _PAR_REGISTRATION_API_H_
#define _PAR_REGISTRATION_API_H_

#include "par_types.h"

/**
 * @addtogroup PARAMETERS_API
 * @{ <!-- BEGIN GROUP -->
 */

/**
 * @brief Registration API.
 */
#if (1 == PAR_CFG_ENABLE_CHANGE_CALLBACK)
/**
 * @brief Register a change callback for one parameter.
 * @param par_num Parameter number.
 * @param cb Change callback function pointer.
 * @note Registered callback is not used by startup default initialization,
 * raw restore paths, fast setters, or bitwise fast setters.
 * @note Register callbacks during single-threaded setup or under application
 * synchronization. Runtime registration is not serialized against callback
 * dispatch by the parameter module.
 */
void par_register_on_change_cb(const par_num_t par_num, const pf_par_on_change_cb_t cb);
#endif /* (1 == PAR_CFG_ENABLE_CHANGE_CALLBACK) */
#if (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION)
/**
 * @brief Register a validation callback for one scalar parameter.
 * @param par_num Parameter number.
 * @param validation Validation callback function pointer.
 */
void par_register_validation(const par_num_t par_num, const pf_par_validation_t validation);
#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
/**
 * @brief Register an object validation callback for one object parameter.
 * @param par_num Parameter number.
 * @param validation Validation callback function pointer.
 */
void par_register_obj_validation(const par_num_t par_num, const pf_par_obj_validation_t validation);
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */
#endif /* (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION) */
#if (1 == PAR_CFG_ENABLE_DESC) && (1 == PAR_CFG_ENABLE_DESC_CHECK)
/**
 * @brief Validate a description string in the port layer.
 * @param p_desc Pointer to the description string.
 * @return True when the description is valid; otherwise false.
 */
PAR_PORT_WEAK bool par_port_is_desc_valid(const char * const p_desc);
#endif /* (1 == PAR_CFG_ENABLE_DESC) && (1 == PAR_CFG_ENABLE_DESC_CHECK) */

#if defined(AUTOGEN_PM_USING_MSH_TOOL) && defined(RT_USING_FINSH)
/**
 * @brief Return the shell group name for a parameter.
 * @param par_num Parameter number.
 * @return Group name string, or RT_NULL when no group label is used.
 */
PAR_PORT_WEAK const char *par_port_get_shell_group(const par_num_t par_num);
#endif /* defined(AUTOGEN_PM_USING_MSH_TOOL) && defined(RT_USING_FINSH) */

#if (PAR_CFG_DEBUG_EN)
/**
 * @brief Return a debug string for one parameter status code.
 * @param status Parameter status code.
 * @return Status string.
 */
const char *par_get_status_str(const par_status_t status);
#endif /* (PAR_CFG_DEBUG_EN) */

/** @} <!-- END GROUP --> */

#endif /* !defined(_PAR_REGISTRATION_API_H_) */
