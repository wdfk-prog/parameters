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
 * 2026-01-29 V3.0.1  Ziga Miklosic first version
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
 * @brief Initial value for CRC-16/CCITT-FALSE accumulation.
 */
#define PAR_IF_CRC16_INIT ((uint16_t)0xFFFFU)

/**
 * @brief Initial value for CRC-8 accumulation.
 */
#define PAR_IF_CRC8_INIT ((uint8_t)0x00U)
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
 * @brief Accumulate CRC-16 for serialized NVM header content.
 *
 * @details The portable core provides a weak software default in
 * src/port/par_if.c. The integrator may replace it with a strong platform
 * implementation, for example one backed by a hardware CRC engine.
 *
 * @param crc Current CRC-16 accumulator value.
 * @param p_data Pointer to serialized bytes.
 * @param size Number of bytes to process.
 * @return Updated CRC-16 value.
 */
uint16_t par_if_crc16_accumulate(uint16_t crc, const uint8_t * const p_data, const uint32_t size);
/**
 * @brief Accumulate CRC-8 for one serialized NVM data record.
 *
 * @details The portable core provides a weak software default in
 * src/port/par_if.c. The integrator may replace it with a strong platform
 * implementation, for example one backed by a hardware CRC engine.
 *
 * @param crc Current CRC-8 accumulator value.
 * @param p_data Pointer to serialized bytes.
 * @param size Number of bytes to process.
 * @return Updated CRC-8 value.
 */
uint8_t par_if_crc8_accumulate(uint8_t crc, const uint8_t * const p_data, const uint32_t size);

#endif /* !defined(_PAR_IF_H_) */
