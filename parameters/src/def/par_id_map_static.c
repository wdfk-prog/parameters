/**
 * @file par_id_map_static.c
 * @brief Compile-time generated ID lookup map.
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
#include "par_id_map_static.h"

#if (1 == PAR_CFG_ENABLE_ID)

/**
 * @brief Emit one static ID hash-map entry from a parameter table row.
 * @param ... Parameter table row tuple from par_table.def.
 */
#define PAR_ID_MAP_ITEM(...) \
    [PAR_HASH_ID_CONST(PAR_XARG_ID(__VA_ARGS__))] = { .id = (uint16_t)(PAR_XARG_ID(__VA_ARGS__)), .par_num = (PAR_XARG_ENUM(__VA_ARGS__)), .used = 1u },

/**
 * @brief Static ID lookup table indexed by PAR_HASH_ID_CONST().
 */
const par_id_map_entry_t g_par_id_map_static[PAR_ID_HASH_SIZE] = {
#define PAR_ITEM_NOP(...)
#define PAR_ITEM_U8                      PAR_ID_MAP_ITEM
#define PAR_ITEM_U16                     PAR_ID_MAP_ITEM
#define PAR_ITEM_U32                     PAR_ID_MAP_ITEM
#define PAR_ITEM_I8                      PAR_ID_MAP_ITEM
#define PAR_ITEM_I16                     PAR_ID_MAP_ITEM
#define PAR_ITEM_I32                     PAR_ID_MAP_ITEM
#define PAR_ITEM_F32                     PAR_ID_MAP_ITEM
#define PAR_OBJECT_ITEM_ENABLED_HANDLER  PAR_ID_MAP_ITEM
#define PAR_OBJECT_ITEM_DISABLED_HANDLER PAR_ITEM_NOP
#include "par_object_item_bind.inc"
#include "par_table.def"
#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32
#include "par_object_item_unbind.inc"
#undef PAR_OBJECT_ITEM_ENABLED_HANDLER
#undef PAR_OBJECT_ITEM_DISABLED_HANDLER
#undef PAR_ITEM_NOP
};

#undef PAR_ID_MAP_ITEM

#endif /* (1 == PAR_CFG_ENABLE_ID) */
