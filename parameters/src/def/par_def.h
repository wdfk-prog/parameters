/**
 * @file par_def.h
 * @brief Declare parameter-definition types and compile-time enumerations.
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
 * 2026-04-22 1.1     wdfk-prog     add object arrays and compile-time metadata helpers
 */
/**
 * @addtogroup PAR_DEF
 * @{ <!-- BEGIN GROUP -->
 */

#ifndef _PAR_DEF_CORE_H_
#define _PAR_DEF_CORE_H_

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */
/**
 * @brief Include dependencies.
 */
#include <stdint.h>
/**
 * @brief Compile-time definitions.
 *
 * @note par_table.def uses 1/0 in the persistence column, so X-macro
 * token-pasting selectors do not depend on stdbool macro expansion.
 */
typedef struct par_cfg_s par_cfg_t;
/**
 * @brief Centralized argument extractors for par_table.def X-Macro rows.
 * @details Keep row-signature changes localized here. Consumers can use
 * variadic wrappers plus these selectors instead of repeating the full row
 * signature in every macro definition.
 */
#define PAR_XARG_ENUM(enum_, ...)                                                                                   enum_
#define PAR_XARG_ID(_enum, id_, ...)                                                                                id_
#define PAR_XARG_NAME(_enum, _id, name_, ...)                                                                       name_
#define PAR_XARG_MIN(_enum, _id, _name, min_, ...)                                                                  min_
#define PAR_XARG_MAX(_enum, _id, _name, _min, max_, ...)                                                            max_
#define PAR_XARG_DEF(_enum, _id, _name, _min, _max, def_, ...)                                                      def_
#define PAR_XARG_UNIT(_enum, _id, _name, _min, _max, _def, unit_, ...)                                              unit_
#define PAR_XARG_ACCESS(_enum, _id, _name, _min, _max, _def, _unit, access_, ...)                                   access_
#define PAR_XARG_READ_ROLES(_enum, _id, _name, _min, _max, _def, _unit, _access, read_roles_, ...)                  read_roles_
#define PAR_XARG_WRITE_ROLES(_enum, _id, _name, _min, _max, _def, _unit, _access, _read_roles, write_roles_, ...)   write_roles_
#define PAR_XARG_PERS(_enum, _id, _name, _min, _max, _def, _unit, _access, _read_roles, _write_roles, pers_, ...)   pers_
#define PAR_XARG_DESC(_enum, _id, _name, _min, _max, _def, _unit, _access, _read_roles, _write_roles, _pers, desc_) desc_
/**
 * @brief List of device parameters.
 * @note Must be started with 0! @note Enum expansion is intentionally configuration-independent: PAR_ITEM_F32 always maps to PAR_ITEM_ENUM. F32 enable/disable fail-fast is enforced in par_def.c.
 */
#define PAR_ITEM_ENUM(...) PAR_XARG_ENUM(__VA_ARGS__),
#define PAR_ITEM_NOP(...)
enum
{
#define PAR_ITEM_U8                      PAR_ITEM_ENUM
#define PAR_ITEM_U16                     PAR_ITEM_ENUM
#define PAR_ITEM_U32                     PAR_ITEM_ENUM
#define PAR_ITEM_I8                      PAR_ITEM_ENUM
#define PAR_ITEM_I16                     PAR_ITEM_ENUM
#define PAR_ITEM_I32                     PAR_ITEM_ENUM
#define PAR_ITEM_F32                     PAR_ITEM_ENUM
#define PAR_OBJECT_ITEM_ENABLED_HANDLER  PAR_ITEM_ENUM
#define PAR_OBJECT_ITEM_DISABLED_HANDLER PAR_ITEM_ENUM
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

    ePAR_NUM_OF
};
#undef PAR_ITEM_ENUM
typedef uint16_t par_num_t;

/**
 * @brief Sentinel used by par_cfg_t.persist_idx for non-persistent parameters.
 */
#define PAR_PERSIST_IDX_INVALID UINT16_MAX
/**
 * @brief Compile-time storage group counts derived from par_table.def.
 * @note These constants are used by layout and static storage allocation.
 */
#define PAR_ITEM_COUNT_ONE(...)  +1u
#define PAR_ITEM_COUNT_ZERO(...) +0u

enum
{
    PAR_LAYOUT_COMPILE_COUNT8 = 0u
#define PAR_ITEM_U8                      PAR_ITEM_COUNT_ONE
#define PAR_ITEM_U16                     PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_U32                     PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_I8                      PAR_ITEM_COUNT_ONE
#define PAR_ITEM_I16                     PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_I32                     PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_F32                     PAR_ITEM_COUNT_ZERO
#define PAR_OBJECT_ITEM_ENABLED_HANDLER  PAR_ITEM_COUNT_ZERO
#define PAR_OBJECT_ITEM_DISABLED_HANDLER PAR_ITEM_COUNT_ZERO
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
};

enum
{
    PAR_LAYOUT_COMPILE_COUNT16 = 0u
#define PAR_ITEM_U8                      PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_U16                     PAR_ITEM_COUNT_ONE
#define PAR_ITEM_U32                     PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_I8                      PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_I16                     PAR_ITEM_COUNT_ONE
#define PAR_ITEM_I32                     PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_F32                     PAR_ITEM_COUNT_ZERO
#define PAR_OBJECT_ITEM_ENABLED_HANDLER  PAR_ITEM_COUNT_ZERO
#define PAR_OBJECT_ITEM_DISABLED_HANDLER PAR_ITEM_COUNT_ZERO
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
};

enum
{
    PAR_LAYOUT_COMPILE_COUNT32 = 0u
#define PAR_ITEM_U8                      PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_U16                     PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_U32                     PAR_ITEM_COUNT_ONE
#define PAR_ITEM_I8                      PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_I16                     PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_I32                     PAR_ITEM_COUNT_ONE
#define PAR_ITEM_F32                     PAR_ITEM_COUNT_ONE
#define PAR_OBJECT_ITEM_ENABLED_HANDLER  PAR_ITEM_COUNT_ZERO
#define PAR_OBJECT_ITEM_DISABLED_HANDLER PAR_ITEM_COUNT_ZERO
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
};


enum
{
    PAR_LAYOUT_COMPILE_COUNT_OBJECT = 0u
#define PAR_ITEM_U8                      PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_U16                     PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_U32                     PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_I8                      PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_I16                     PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_I32                     PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_F32                     PAR_ITEM_COUNT_ZERO
#define PAR_OBJECT_ITEM_ENABLED_HANDLER  PAR_ITEM_COUNT_ONE
#define PAR_OBJECT_ITEM_DISABLED_HANDLER PAR_ITEM_COUNT_ZERO
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
};

/**
 * @brief Backward-compatible alias for compile-time object-slot count.
 */
#define PAR_LAYOUT_COMPILE_COUNTOBJ PAR_LAYOUT_COMPILE_COUNT_OBJECT

/**
 * @brief Compile-time total size of the shared object payload pool in bytes.
 */
#define PAR_ITEM_OBJECT_BYTES_ZERO(...)                                                                                             +0u
#define PAR_ITEM_OBJECT_BYTES_STR(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_)     +((uint32_t)(max_))
#define PAR_ITEM_OBJECT_BYTES_BYTES(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_)   +((uint32_t)(max_))
#define PAR_ITEM_OBJECT_BYTES_ARR_U8(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_)  +((uint32_t)(max_))
#define PAR_ITEM_OBJECT_BYTES_ARR_U16(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) +((uint32_t)(max_) * (uint32_t)sizeof(uint16_t))
#define PAR_ITEM_OBJECT_BYTES_ARR_U32(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) +((uint32_t)(max_) * (uint32_t)sizeof(uint32_t))

/**
 * @brief Compile-time object byte-capacity fit checks.
 * @details These checks are emitted at file scope because PAR_STATIC_ASSERT()
 * expands to a declaration on supported C89/C99 fallback paths and cannot be
 * placed inside an enum initializer list.
 */
#define PAR_OBJECT_BYTES_FIT_U16_ZERO(...)
#define PAR_OBJECT_BYTES_FIT_U16_STR(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    PAR_STATIC_ASSERT(enum_##_str_min_len_fits_u16, ((uint32_t)(min_) <= (uint32_t)(UINT16_MAX - 1U)));                            \
    PAR_STATIC_ASSERT(enum_##_str_max_len_leaves_nul_room, ((uint32_t)(max_) <= (uint32_t)(UINT16_MAX - 1U)))
#define PAR_OBJECT_BYTES_FIT_U16_BYTES(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    PAR_STATIC_ASSERT(enum_##_obj_min_len_fits_u16, ((uint32_t)(min_) <= (uint32_t)UINT16_MAX));                                     \
    PAR_STATIC_ASSERT(enum_##_obj_max_len_fits_u16, ((uint32_t)(max_) <= (uint32_t)UINT16_MAX))
#define PAR_OBJECT_BYTES_FIT_U16_ARR_U8(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    PAR_STATIC_ASSERT(enum_##_obj_min_len_fits_u16, ((uint32_t)(min_) <= (uint32_t)UINT16_MAX));                                      \
    PAR_STATIC_ASSERT(enum_##_obj_max_len_fits_u16, ((uint32_t)(max_) <= (uint32_t)UINT16_MAX))
#define PAR_OBJECT_BYTES_FIT_U16_ARR_U16(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    PAR_STATIC_ASSERT(enum_##_obj_min_len_fits_u16, (((uint32_t)(min_) * (uint32_t)sizeof(uint16_t)) <= (uint32_t)UINT16_MAX));        \
    PAR_STATIC_ASSERT(enum_##_obj_max_len_fits_u16, (((uint32_t)(max_) * (uint32_t)sizeof(uint16_t)) <= (uint32_t)UINT16_MAX))
#define PAR_OBJECT_BYTES_FIT_U16_ARR_U32(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    PAR_STATIC_ASSERT(enum_##_obj_min_len_fits_u16, (((uint32_t)(min_) * (uint32_t)sizeof(uint32_t)) <= (uint32_t)UINT16_MAX));        \
    PAR_STATIC_ASSERT(enum_##_obj_max_len_fits_u16, (((uint32_t)(max_) * (uint32_t)sizeof(uint32_t)) <= (uint32_t)UINT16_MAX))

#define PAR_ITEM_U8                      PAR_OBJECT_BYTES_FIT_U16_ZERO
#define PAR_ITEM_U16                     PAR_OBJECT_BYTES_FIT_U16_ZERO
#define PAR_ITEM_U32                     PAR_OBJECT_BYTES_FIT_U16_ZERO
#define PAR_ITEM_I8                      PAR_OBJECT_BYTES_FIT_U16_ZERO
#define PAR_ITEM_I16                     PAR_OBJECT_BYTES_FIT_U16_ZERO
#define PAR_ITEM_I32                     PAR_OBJECT_BYTES_FIT_U16_ZERO
#define PAR_ITEM_F32                     PAR_OBJECT_BYTES_FIT_U16_ZERO
#define PAR_OBJECT_ITEM_STR_HANDLER      PAR_OBJECT_BYTES_FIT_U16_STR
#define PAR_OBJECT_ITEM_BYTES_HANDLER    PAR_OBJECT_BYTES_FIT_U16_BYTES
#define PAR_OBJECT_ITEM_ARR_U8_HANDLER   PAR_OBJECT_BYTES_FIT_U16_ARR_U8
#define PAR_OBJECT_ITEM_ARR_U16_HANDLER  PAR_OBJECT_BYTES_FIT_U16_ARR_U16
#define PAR_OBJECT_ITEM_ARR_U32_HANDLER  PAR_OBJECT_BYTES_FIT_U16_ARR_U32
#define PAR_OBJECT_ITEM_DISABLED_HANDLER PAR_OBJECT_BYTES_FIT_U16_ZERO
#include "par_object_item_bind_typed.inc"
#include "par_table.def"
#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32
#include "par_object_item_unbind.inc"
#undef PAR_OBJECT_ITEM_STR_HANDLER
#undef PAR_OBJECT_ITEM_BYTES_HANDLER
#undef PAR_OBJECT_ITEM_ARR_U8_HANDLER
#undef PAR_OBJECT_ITEM_ARR_U16_HANDLER
#undef PAR_OBJECT_ITEM_ARR_U32_HANDLER
#undef PAR_OBJECT_ITEM_DISABLED_HANDLER

/**
 * @brief Marker confirming object byte-capacity checks were expanded.
 */
enum
{
    PAR_LAYOUT_COMPILE_OBJ_BYTES_FIT_U16 = 1u
};

enum
{
    PAR_LAYOUT_COMPILE_OBJ_POOL_BYTES = 0u
#define PAR_ITEM_U8                      PAR_ITEM_OBJECT_BYTES_ZERO
#define PAR_ITEM_U16                     PAR_ITEM_OBJECT_BYTES_ZERO
#define PAR_ITEM_U32                     PAR_ITEM_OBJECT_BYTES_ZERO
#define PAR_ITEM_I8                      PAR_ITEM_OBJECT_BYTES_ZERO
#define PAR_ITEM_I16                     PAR_ITEM_OBJECT_BYTES_ZERO
#define PAR_ITEM_I32                     PAR_ITEM_OBJECT_BYTES_ZERO
#define PAR_ITEM_F32                     PAR_ITEM_OBJECT_BYTES_ZERO
#define PAR_OBJECT_ITEM_STR_HANDLER      PAR_ITEM_OBJECT_BYTES_STR
#define PAR_OBJECT_ITEM_BYTES_HANDLER    PAR_ITEM_OBJECT_BYTES_BYTES
#define PAR_OBJECT_ITEM_ARR_U8_HANDLER   PAR_ITEM_OBJECT_BYTES_ARR_U8
#define PAR_OBJECT_ITEM_ARR_U16_HANDLER  PAR_ITEM_OBJECT_BYTES_ARR_U16
#define PAR_OBJECT_ITEM_ARR_U32_HANDLER  PAR_ITEM_OBJECT_BYTES_ARR_U32
#define PAR_OBJECT_ITEM_DISABLED_HANDLER PAR_ITEM_OBJECT_BYTES_ZERO
#include "par_object_item_bind_typed.inc"
#include "par_table.def"
#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32
#include "par_object_item_unbind.inc"
#undef PAR_OBJECT_ITEM_STR_HANDLER
#undef PAR_OBJECT_ITEM_BYTES_HANDLER
#undef PAR_OBJECT_ITEM_ARR_U8_HANDLER
#undef PAR_OBJECT_ITEM_ARR_U16_HANDLER
#undef PAR_OBJECT_ITEM_ARR_U32_HANDLER
#undef PAR_OBJECT_ITEM_DISABLED_HANDLER
};

#undef PAR_ITEM_OBJECT_BYTES_ZERO
#undef PAR_ITEM_OBJECT_BYTES_STR
#undef PAR_ITEM_OBJECT_BYTES_BYTES
#undef PAR_ITEM_OBJECT_BYTES_ARR_U8
#undef PAR_ITEM_OBJECT_BYTES_ARR_U16
#undef PAR_ITEM_OBJECT_BYTES_ARR_U32
#undef PAR_OBJECT_BYTES_FIT_U16_ZERO
#undef PAR_OBJECT_BYTES_FIT_U16_STR
#undef PAR_OBJECT_BYTES_FIT_U16_BYTES
#undef PAR_OBJECT_BYTES_FIT_U16_ARR_U8
#undef PAR_OBJECT_BYTES_FIT_U16_ARR_U16
#undef PAR_OBJECT_BYTES_FIT_U16_ARR_U32

enum
{
    PAR_LAYOUT_COMPILE_COUNT_SUM = (PAR_LAYOUT_COMPILE_COUNT8 + PAR_LAYOUT_COMPILE_COUNT16 + PAR_LAYOUT_COMPILE_COUNT32 + PAR_LAYOUT_COMPILE_COUNT_OBJECT)
};

/**
 * @brief Compile-time scalar persistent-slot enumeration derived from par_table.def.
 * @details Only scalar entries flagged with pers_ == 1 contribute a slot. The
 * resulting PAR_PERSIST_IDX_<enum_> constants are dense and ordered exactly as
 * the scalar persistent subset in the source table.
 */
#define PAR_PERSIST_ENUM_SELECT(enum_, pers_)   PAR_PERSIST_ENUM_SELECT_I(enum_, pers_)
#define PAR_PERSIST_ENUM_SELECT_I(enum_, pers_) PAR_PERSIST_ENUM_SELECT_##pers_(enum_)
#define PAR_PERSIST_ENUM_SELECT_1(enum_)        PAR_PERSIST_IDX_##enum_,
#define PAR_PERSIST_ENUM_SELECT_0(enum_)
#define PAR_ITEM_PERSIST_ENUM(...) PAR_PERSIST_ENUM_SELECT(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_PERS(__VA_ARGS__))
#define PAR_ITEM_NOP(...)
enum
{
#define PAR_ITEM_U8                      PAR_ITEM_PERSIST_ENUM
#define PAR_ITEM_U16                     PAR_ITEM_PERSIST_ENUM
#define PAR_ITEM_U32                     PAR_ITEM_PERSIST_ENUM
#define PAR_ITEM_I8                      PAR_ITEM_PERSIST_ENUM
#define PAR_ITEM_I16                     PAR_ITEM_PERSIST_ENUM
#define PAR_ITEM_I32                     PAR_ITEM_PERSIST_ENUM
#define PAR_ITEM_F32                     PAR_ITEM_PERSIST_ENUM
#define PAR_OBJECT_ITEM_ENABLED_HANDLER  PAR_ITEM_NOP
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

    PAR_PERSISTENT_COMPILE_COUNT,
    PAR_PERSIST_SLOT_MAP_CAPACITY = (PAR_PERSISTENT_COMPILE_COUNT > 0U) ? PAR_PERSISTENT_COMPILE_COUNT : 1U
};
#undef PAR_ITEM_PERSIST_ENUM
#undef PAR_PERSIST_ENUM_SELECT
#undef PAR_PERSIST_ENUM_SELECT_I
#undef PAR_PERSIST_ENUM_SELECT_1
#undef PAR_PERSIST_ENUM_SELECT_0

#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
/**
 * @brief Compile-time object persistent-slot enumeration derived from par_table.def.
 * @details Only object entries flagged with pers_ == 1 contribute a slot. The
 * resulting PAR_OBJECT_PERSIST_IDX_<enum_> constants are dense and ordered
 * exactly as the persistent object subset in the source table.
 */
#define PAR_OBJECT_PERSIST_ENUM_SELECT(enum_, pers_)   PAR_OBJECT_PERSIST_ENUM_SELECT_I(enum_, pers_)
#define PAR_OBJECT_PERSIST_ENUM_SELECT_I(enum_, pers_) PAR_OBJECT_PERSIST_ENUM_SELECT_##pers_(enum_)
#define PAR_OBJECT_PERSIST_ENUM_SELECT_1(enum_)        PAR_OBJECT_PERSIST_IDX_##enum_,
#define PAR_OBJECT_PERSIST_ENUM_SELECT_0(enum_)
#define PAR_ITEM_OBJECT_PERSIST_ENUM(...) PAR_OBJECT_PERSIST_ENUM_SELECT(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_PERS(__VA_ARGS__))
#define PAR_ITEM_NOP(...)
enum
{
#define PAR_ITEM_U8                      PAR_ITEM_NOP
#define PAR_ITEM_U16                     PAR_ITEM_NOP
#define PAR_ITEM_U32                     PAR_ITEM_NOP
#define PAR_ITEM_I8                      PAR_ITEM_NOP
#define PAR_ITEM_I16                     PAR_ITEM_NOP
#define PAR_ITEM_I32                     PAR_ITEM_NOP
#define PAR_ITEM_F32                     PAR_ITEM_NOP
#define PAR_OBJECT_ITEM_ENABLED_HANDLER  PAR_ITEM_OBJECT_PERSIST_ENUM
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

    PAR_OBJECT_PERSISTENT_COMPILE_COUNT,
    PAR_OBJECT_PERSIST_SLOT_MAP_CAPACITY = (PAR_OBJECT_PERSISTENT_COMPILE_COUNT > 0U) ? PAR_OBJECT_PERSISTENT_COMPILE_COUNT : 1U
};
#undef PAR_ITEM_OBJECT_PERSIST_ENUM
#undef PAR_OBJECT_PERSIST_ENUM_SELECT
#undef PAR_OBJECT_PERSIST_ENUM_SELECT_I
#undef PAR_OBJECT_PERSIST_ENUM_SELECT_1
#undef PAR_OBJECT_PERSIST_ENUM_SELECT_0
#else
/**
 * @brief Object persistent-slot counters when object support is disabled.
 */
enum
{
    PAR_OBJECT_PERSISTENT_COMPILE_COUNT = 0U,
    PAR_OBJECT_PERSIST_SLOT_MAP_CAPACITY = 1U
};
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

#undef PAR_ITEM_COUNT_ONE
#undef PAR_ITEM_COUNT_ZERO
/**
 * @brief Function declarations.
 */
const par_cfg_t *par_cfg_get_table(void);
const par_cfg_t *par_cfg_get(const par_num_t par_num);
/**
 * @brief Return the compile-time parameter ID for one entry.
 *
 * @details This accessor is configuration-independent and remains available
 * even when PAR_CFG_ENABLE_ID disables the runtime metadata field inside
 * par_cfg_t.
 *
 * @param par_num Parameter number.
 * @return Parameter ID from par_table.def.
 */
uint16_t par_cfg_get_param_id_const(const par_num_t par_num);
/**
 * @brief Return the number of configuration entries.
 * @return Configuration table size.
 */
uint32_t par_cfg_get_table_size(void);


#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */
/**
 * @} <!-- END GROUP -->
 */

#endif /* !defined(_PAR_DEF_CORE_H_) */
