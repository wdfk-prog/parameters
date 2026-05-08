/**
 * @file par_storage_init.inc
 * @brief Generate parameter storage initialization code.
 * @author wdfk-prog
 * @version 1.1
 * @date 2026-04-23
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-03-27 1.0     wdfk-prog     first version
 * 2026-04-23 1.1     wdfk-prog     centralize object-row macro binding
 */
/**
 * @brief Private include fragment. Included only by par.c.
 * @details Expands compile-time default initializers for the grouped live
 * scalar storage object and preserves the internal U8/U16/U32 width-group
 * layout. Object rows do not participate in grouped scalar storage and are
 * therefore bound to a shared no-op handler here.
 */

#define PAR_STORAGE_INIT_NOP(...)
#define PAR_STORAGE_U8_FROM_U8(...)   (par_atomic_u8_t)((uint8_t)(PAR_XARG_DEF(__VA_ARGS__))),
#define PAR_STORAGE_U8_FROM_I8(...)   (par_atomic_u8_t)((uint8_t)(int8_t)(PAR_XARG_DEF(__VA_ARGS__))),
#define PAR_STORAGE_U16_FROM_U16(...) (par_atomic_u16_t)((uint16_t)(PAR_XARG_DEF(__VA_ARGS__))),
#define PAR_STORAGE_U16_FROM_I16(...) (par_atomic_u16_t)((uint16_t)(int16_t)(PAR_XARG_DEF(__VA_ARGS__))),
#define PAR_STORAGE_U32_FROM_U32(...) (par_atomic_u32_t)((uint32_t)(PAR_XARG_DEF(__VA_ARGS__))),
#define PAR_STORAGE_U32_FROM_I32(...) (par_atomic_u32_t)((uint32_t)(int32_t)(PAR_XARG_DEF(__VA_ARGS__))),
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
#define PAR_STORAGE_U32_FROM_F32(...) (par_atomic_u32_t)(0u),
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */
#define PAR_STORAGE_OBJECT_NOP(...)

/**
 * @brief Live grouped scalar storage initialized from par_table.def defaults.
 * @details The grouped arrays hold U8/I8, U16/I16, and U32/I32/F32
 * values in separate atomic-width backing stores. Object rows expand to
 * no-op initializers because their payloads are owned by par_object.c.
 */
static par_storage_groups_t gs_par_storage = {
    .u8 = {
#define PAR_ITEM_U8                      PAR_STORAGE_U8_FROM_U8
#define PAR_ITEM_U16                     PAR_STORAGE_INIT_NOP
#define PAR_ITEM_U32                     PAR_STORAGE_INIT_NOP
#define PAR_ITEM_I8                      PAR_STORAGE_U8_FROM_I8
#define PAR_ITEM_I16                     PAR_STORAGE_INIT_NOP
#define PAR_ITEM_I32                     PAR_STORAGE_INIT_NOP
#define PAR_ITEM_F32                     PAR_STORAGE_INIT_NOP
#define PAR_OBJECT_ITEM_ENABLED_HANDLER  PAR_STORAGE_OBJECT_NOP
#define PAR_OBJECT_ITEM_DISABLED_HANDLER PAR_STORAGE_OBJECT_NOP
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
    },

    .u16 = {
#define PAR_ITEM_U8                      PAR_STORAGE_INIT_NOP
#define PAR_ITEM_U16                     PAR_STORAGE_U16_FROM_U16
#define PAR_ITEM_U32                     PAR_STORAGE_INIT_NOP
#define PAR_ITEM_I8                      PAR_STORAGE_INIT_NOP
#define PAR_ITEM_I16                     PAR_STORAGE_U16_FROM_I16
#define PAR_ITEM_I32                     PAR_STORAGE_INIT_NOP
#define PAR_ITEM_F32                     PAR_STORAGE_INIT_NOP
#define PAR_OBJECT_ITEM_ENABLED_HANDLER  PAR_STORAGE_OBJECT_NOP
#define PAR_OBJECT_ITEM_DISABLED_HANDLER PAR_STORAGE_OBJECT_NOP
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
    },

    .u32 = {
#define PAR_ITEM_U8  PAR_STORAGE_INIT_NOP
#define PAR_ITEM_U16 PAR_STORAGE_INIT_NOP
#define PAR_ITEM_U32 PAR_STORAGE_U32_FROM_U32
#define PAR_ITEM_I8  PAR_STORAGE_INIT_NOP
#define PAR_ITEM_I16 PAR_STORAGE_INIT_NOP
#define PAR_ITEM_I32 PAR_STORAGE_U32_FROM_I32
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
#define PAR_ITEM_F32 PAR_STORAGE_U32_FROM_F32
#else
#define PAR_ITEM_F32 PAR_STORAGE_INIT_NOP
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */
#define PAR_OBJECT_ITEM_ENABLED_HANDLER  PAR_STORAGE_OBJECT_NOP
#define PAR_OBJECT_ITEM_DISABLED_HANDLER PAR_STORAGE_OBJECT_NOP
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
    }
};

#undef PAR_STORAGE_INIT_NOP
#undef PAR_STORAGE_U8_FROM_U8
#undef PAR_STORAGE_U8_FROM_I8
#undef PAR_STORAGE_U16_FROM_U16
#undef PAR_STORAGE_U16_FROM_I16
#undef PAR_STORAGE_U32_FROM_U32
#undef PAR_STORAGE_U32_FROM_I32
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
#undef PAR_STORAGE_U32_FROM_F32
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */
#undef PAR_STORAGE_OBJECT_NOP
