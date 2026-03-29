/**
 * @file par_if.h
 * @brief Declare the parameter interface layer.
 * @author Ziga Miklosic
 * @version V3.0.1
 * @date 2026-01-29
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-01-29 V3.0.1  Ziga Miklosic
 */

/**
 * @addtogroup PAR_IF
 * @{ <!-- BEGIN GROUP -->
 */

#ifndef _PAR_IF_H_
#define _PAR_IF_H_
/**
 * @brief Include dependencies.
 */
#include <stdint.h>
#include "par.h"
/**
 * @brief Compile-time definitions.
 */
/**
 * @brief Function declarations.
 */
/**
 * @brief Initialize the interface layer.
 * @return Operation status.
 */
par_status_t par_if_init(void);
/**
 * @brief Deinitialize the interface layer.
 * @return Operation status.
 */
par_status_t par_if_deinit(void);
/**
 * @brief Acquire the parameter mutex for one parameter path.
 * @param par_num Parameter number.
 * @return Operation status.
 */
par_status_t par_if_aquire_mutex(const par_num_t par_num);
/**
 * @brief Release the parameter mutex for one parameter path.
 * @param par_num Parameter number.
 */
void par_if_release_mutex(const par_num_t par_num);
/**
 * @brief Calculate the table hash for parameter metadata.
 * @param p_data Pointer to the input bytes.
 * @param size Number of bytes in p_data.
 * @param p_hash Pointer to the output hash buffer.
 */
void par_if_calc_hash(const uint8_t * const p_data, const uint32_t size, uint8_t * const p_hash);

#endif /* _PAR_IF_H_ */
