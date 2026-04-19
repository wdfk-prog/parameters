/**
 * @file par_id_map_static.h
 * @brief Declare the compile-time generated static ID lookup map.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-03-24
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-03-24 1.0     wdfk-prog    first version
 */
#ifndef _PAR_ID_MAP_STATIC_H_
#define _PAR_ID_MAP_STATIC_H_

#include "par.h"

#if (1 == PAR_CFG_ENABLE_ID)
typedef struct
{
    uint16_t id;
    par_num_t par_num;
    uint8_t used;
} par_id_map_entry_t;

extern const par_id_map_entry_t g_par_id_map_static[PAR_ID_HASH_SIZE];
#endif

#endif /* _PAR_ID_MAP_STATIC_H_ */
