/**
 * @file par_layout.h
 * @brief Declare parameter storage layout helpers.
 * @author wdfk-prog ()
 * @version 1.0
 * @date 2026-03-27
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author      Description
 * 2026-03-27 1.0     wdfk-prog   first version
 */

#ifndef _PAR_LAYOUT_H_
#define _PAR_LAYOUT_H_
/**
 * @brief Include dependencies.
 */
#include <stdint.h>

#include "par_cfg.h"
#include "def/par_def.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Compile-time definitions.
 */
typedef struct
{
    uint16_t count8;
    uint16_t count16;
    uint16_t count32;
} par_layout_count_t;

#if (PAR_CFG_LAYOUT_SOURCE == PAR_CFG_LAYOUT_COMPILE_SCAN)
#define PAR_STORAGE_COUNT8  (PAR_LAYOUT_COMPILE_COUNT8)
#define PAR_STORAGE_COUNT16 (PAR_LAYOUT_COMPILE_COUNT16)
#define PAR_STORAGE_COUNT32 (PAR_LAYOUT_COMPILE_COUNT32)
#elif (PAR_CFG_LAYOUT_SOURCE == PAR_CFG_LAYOUT_SCRIPT)
#include PAR_CFG_LAYOUT_STATIC_INCLUDE
#define PAR_STORAGE_COUNT8  (PAR_LAYOUT_STATIC_COUNT8)
#define PAR_STORAGE_COUNT16 (PAR_LAYOUT_STATIC_COUNT16)
#define PAR_STORAGE_COUNT32 (PAR_LAYOUT_STATIC_COUNT32)
#else
#error "Unsupported PAR_CFG_LAYOUT_SOURCE value!"
#endif

#define PAR_STORAGE_NONZERO(count_) (((count_) > 0u) ? (count_) : 1u)
/**
 * @brief Function declarations and definitions.
 */
/**
 * @brief Return the parameter offset table.
 * @return Pointer to the offset table.
 */
const uint16_t *par_layout_get_offset_table(void);
/**
 * @brief Return the storage offset for one parameter.
 * @param par_num Parameter number.
 * @return Storage offset for par_num.
 */
uint16_t par_layout_get_offset(const par_num_t par_num);
/**
 * @brief Return the grouped storage counts.
 * @return Grouped storage counts for 8-bit, 16-bit, and 32-bit data.
 */
par_layout_count_t par_layout_get_count(void);
/**
 * @brief Initialize the storage layout metadata.
 */
void par_layout_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _PAR_LAYOUT_H_ */
