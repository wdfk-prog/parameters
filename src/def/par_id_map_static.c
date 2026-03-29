/**
 * @file par_id_map_static.c
 * @brief Compile-time generated ID lookup map
 * @author wdfk-prog ()
 * @version 1.0
 * @date 2026-03-24
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author      Description
 * 2026-03-24 1.0     wdfk-prog   first version
 */
#include "def/par_id_map_static.h"

#if (1 == PAR_CFG_ENABLE_ID)

#define PAR_ID_MAP_ITEM(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_) \
    [PAR_HASH_ID_CONST(id_)] = { .id = (uint16_t)(id_), .par_num = (enum_), .used = 1u },

const par_id_map_entry_t g_par_id_map_static[PAR_ID_HASH_SIZE] = {
#define PAR_ITEM_U8  PAR_ID_MAP_ITEM
#define PAR_ITEM_U16 PAR_ID_MAP_ITEM
#define PAR_ITEM_U32 PAR_ID_MAP_ITEM
#define PAR_ITEM_I8  PAR_ID_MAP_ITEM
#define PAR_ITEM_I16 PAR_ID_MAP_ITEM
#define PAR_ITEM_I32 PAR_ID_MAP_ITEM
#define PAR_ITEM_F32 PAR_ID_MAP_ITEM
#include "../../par_table.def"
#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32
};

#undef PAR_ID_MAP_ITEM

#endif
