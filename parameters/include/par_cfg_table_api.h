/**
 * @file par_cfg_table_api.h
 * @brief Declare parameter configuration table accessors.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#ifndef _PAR_CFG_TABLE_API_H_
#define _PAR_CFG_TABLE_API_H_

#include <stdint.h>

#include "par_fwd.h"

/**
 * @addtogroup PAR_CFG
 * @{ <!-- BEGIN GROUP -->
 */

/**
 * @brief Return the parameter configuration table.
 * @return Pointer to the first configuration table entry.
 */
const par_cfg_t *par_cfg_get_table(void);

/**
 * @brief Return one parameter configuration table entry.
 * @param par_num Parameter number.
 * @return Pointer to the selected configuration table entry.
 */
const par_cfg_t *par_cfg_get(const par_num_t par_num);

/**
 * @brief Return the number of configuration entries.
 * @return Configuration table size.
 */
uint32_t par_cfg_get_table_size(void);

/** @} <!-- END GROUP --> */

#endif /* !defined(_PAR_CFG_TABLE_API_H_) */
