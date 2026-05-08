/**
 * @file par_def.c
 * @brief Build parameter-definition tables and derived metadata.
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
 * 2026-04-22 1.1     wdfk-prog     add object arrays and value_cfg union metadata
 */
/**
 * @addtogroup PAR_CFG
 * @{ <!-- BEGIN GROUP -->
 *
 * @brief Configuration for device parameters.
 *
 * User shall put code inside inside code block start with.
 * "USER_CODE_BEGIN" and with end of "USER_CODE_END".
 */
/**
 * @brief Include dependencies.
 */
#include "par.h"
#include "par_def.h"
#include "par_id_map_static.h"
/**
 * @brief Compile-time definitions.
 */
/**
 * @brief Shared compile-time range checks for integer parameter items.
 */
#if (1 == PAR_CFG_ENABLE_RANGE)
#define PAR_CHECK_INT_COMMON(enum_, min_, max_, def_)          \
    PAR_STATIC_ASSERT(enum_##_min_lt_max, ((min_) < (max_)));  \
    PAR_STATIC_ASSERT(enum_##_def_ge_min, ((def_) >= (min_))); \
    PAR_STATIC_ASSERT(enum_##_def_le_max, ((def_) <= (max_)))
#else
#define PAR_CHECK_INT_COMMON(enum_, min_, max_, def_)
#endif /* (1 == PAR_CFG_ENABLE_RANGE) */

#if (1 == PAR_CFG_ENABLE_ACCESS)
#define PAR_CHECK_ACCESS_COMMON(enum_, access_)                                                                            \
    PAR_STATIC_ASSERT(enum_##_access_mask_valid, (((uint32_t)(access_) & (uint32_t)(~((uint32_t)ePAR_ACCESS_RW))) == 0U)); \
    PAR_STATIC_ASSERT(enum_##_access_not_none, ((uint32_t)(access_) != (uint32_t)ePAR_ACCESS_NONE));                       \
    PAR_STATIC_ASSERT(enum_##_access_not_write_only, (((uint32_t)(access_) & (uint32_t)ePAR_ACCESS_WRITE) == 0U) ||        \
                                                         (((uint32_t)(access_) & (uint32_t)ePAR_ACCESS_READ) != 0U))
#else
#define PAR_CHECK_ACCESS_COMMON(enum_, access_)
#endif /* (1 == PAR_CFG_ENABLE_ACCESS) */

#if (1 == PAR_CFG_NVM_SCALAR_EN)
#define PAR_CHECK_SCALAR_PERSISTENCE(enum_, pers_)
#else
#define PAR_CHECK_SCALAR_PERSISTENCE(enum_, pers_) \
    PAR_STATIC_ASSERT(enum_##_scalar_persistence_requires_PAR_CFG_NVM_SCALAR_EN, (!(pers_)))
#endif /* (1 == PAR_CFG_NVM_SCALAR_EN) */

#define PAR_CHECK_SCALAR_COMMON(enum_, min_, max_, def_, access_, pers_) \
    PAR_CHECK_INT_COMMON(enum_, min_, max_, def_);                       \
    PAR_CHECK_SCALAR_PERSISTENCE(enum_, pers_);                          \
    PAR_CHECK_ACCESS_COMMON(enum_, access_)

#define PAR_CHECK_U8(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_)  PAR_CHECK_SCALAR_COMMON(enum_, min_, max_, def_, access_, pers_)
#define PAR_CHECK_U16(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) PAR_CHECK_SCALAR_COMMON(enum_, min_, max_, def_, access_, pers_)
#define PAR_CHECK_U32(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) PAR_CHECK_SCALAR_COMMON(enum_, min_, max_, def_, access_, pers_)
#define PAR_CHECK_I8(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_)  PAR_CHECK_SCALAR_COMMON(enum_, min_, max_, def_, access_, pers_)
#define PAR_CHECK_I16(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) PAR_CHECK_SCALAR_COMMON(enum_, min_, max_, def_, access_, pers_)
#define PAR_CHECK_I32(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) PAR_CHECK_SCALAR_COMMON(enum_, min_, max_, def_, access_, pers_)

#if (1 == PAR_CFG_ENABLE_TYPE_F32)
#define PAR_CHECK_F32(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    PAR_CHECK_SCALAR_PERSISTENCE(enum_, pers_);                                                                     \
    PAR_CHECK_ACCESS_COMMON(enum_, access_)
#else
#define PAR_CHECK_F32(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    PAR_STATIC_ASSERT(enum_##_f32_type_is_disabled__remove_PAR_ITEM_F32, 0)
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */

#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
#if (1 == PAR_CFG_NVM_OBJECT_EN)
#define PAR_CHECK_OBJECT_PERSISTENCE(enum_, pers_)
#else
#define PAR_CHECK_OBJECT_PERSISTENCE(enum_, pers_) \
    PAR_STATIC_ASSERT(enum_##_obj_persistence_requires_PAR_CFG_NVM_OBJECT_EN, (!(pers_)))
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) */
#define PAR_CHECK_OBJECT_COMMON(enum_, min_len_, max_len_, pers_, access_)                     \
    PAR_STATIC_ASSERT(enum_##_obj_min_le_max, ((uint16_t)(min_len_) <= (uint16_t)(max_len_))); \
    PAR_CHECK_OBJECT_PERSISTENCE(enum_, pers_);                                                \
    PAR_CHECK_ACCESS_COMMON(enum_, access_)
#else
#define PAR_CHECK_OBJECT_COMMON(enum_, min_len_, max_len_, pers_, access_) \
    PAR_CHECK_ACCESS_COMMON(enum_, access_)
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

#if (1 == PAR_CFG_ENABLE_TYPE_STR)
#define PAR_CHECK_STR(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    PAR_CHECK_OBJECT_COMMON(enum_, (min_), (max_), (pers_), access_)
#else
#define PAR_CHECK_STR(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    PAR_STATIC_ASSERT(enum_##_str_type_is_disabled__remove_PAR_ITEM_STR, 0)
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */

#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
#define PAR_CHECK_BYTES(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    PAR_CHECK_OBJECT_COMMON(enum_, (min_), (max_), (pers_), access_)
#else
#define PAR_CHECK_BYTES(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    PAR_STATIC_ASSERT(enum_##_bytes_type_is_disabled__remove_PAR_ITEM_BYTES, 0)
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */

#define PAR_CHECK_ARRAY_COMMON(enum_, min_count_, max_count_, pers_, elem_size_, access_)                   \
    PAR_STATIC_ASSERT(enum_##_arr_fixed_size_required, ((uint16_t)(min_count_) == (uint16_t)(max_count_))); \
    PAR_CHECK_OBJECT_COMMON(enum_,                                                                          \
                            ((uint32_t)(min_count_) * (uint32_t)(elem_size_)),                              \
                            ((uint32_t)(max_count_) * (uint32_t)(elem_size_)),                              \
                            (pers_),                                                                        \
                            access_)

#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
#define PAR_CHECK_ARR_U8(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    PAR_CHECK_ARRAY_COMMON(enum_, (min_), (max_), (pers_), 1U, access_)
#else
#define PAR_CHECK_ARR_U8(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    PAR_STATIC_ASSERT(enum_##_arr_u8_type_is_disabled__remove_PAR_ITEM_ARR_U8, 0)
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */

#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
#define PAR_CHECK_ARR_U16(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    PAR_CHECK_ARRAY_COMMON(enum_, (min_), (max_), (pers_), sizeof(uint16_t), access_)
#else
#define PAR_CHECK_ARR_U16(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    PAR_STATIC_ASSERT(enum_##_arr_u16_type_is_disabled__remove_PAR_ITEM_ARR_U16, 0)
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */

#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
#define PAR_CHECK_ARR_U32(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    PAR_CHECK_ARRAY_COMMON(enum_, (min_), (max_), (pers_), sizeof(uint32_t), access_)
#else
#define PAR_CHECK_ARR_U32(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    PAR_STATIC_ASSERT(enum_##_arr_u32_type_is_disabled__remove_PAR_ITEM_ARR_U32, 0)
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */

#define PAR_ITEM_U8      PAR_CHECK_U8
#define PAR_ITEM_U16     PAR_CHECK_U16
#define PAR_ITEM_U32     PAR_CHECK_U32
#define PAR_ITEM_I8      PAR_CHECK_I8
#define PAR_ITEM_I16     PAR_CHECK_I16
#define PAR_ITEM_I32     PAR_CHECK_I32
#define PAR_ITEM_F32     PAR_CHECK_F32
#define PAR_ITEM_STR     PAR_CHECK_STR
#define PAR_ITEM_BYTES   PAR_CHECK_BYTES
#define PAR_ITEM_ARR_U8  PAR_CHECK_ARR_U8
#define PAR_ITEM_ARR_U16 PAR_CHECK_ARR_U16
#define PAR_ITEM_ARR_U32 PAR_CHECK_ARR_U32

#include "par_table.def"

#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32
#undef PAR_ITEM_STR
#undef PAR_ITEM_BYTES
#undef PAR_ITEM_ARR_U8
#undef PAR_ITEM_ARR_U16
#undef PAR_ITEM_ARR_U32

#undef PAR_CHECK_U8
#undef PAR_CHECK_U16
#undef PAR_CHECK_U32
#undef PAR_CHECK_I8
#undef PAR_CHECK_I16
#undef PAR_CHECK_I32
#undef PAR_CHECK_F32
#undef PAR_CHECK_STR
#undef PAR_CHECK_BYTES
#undef PAR_CHECK_ARR_U8
#undef PAR_CHECK_ARR_U16
#undef PAR_CHECK_ARR_U32
#undef PAR_CHECK_ARRAY_COMMON
#undef PAR_CHECK_OBJECT_COMMON
#undef PAR_CHECK_OBJECT_PERSISTENCE
#undef PAR_CHECK_SCALAR_COMMON
#undef PAR_CHECK_SCALAR_PERSISTENCE
#undef PAR_CHECK_ACCESS_COMMON
#undef PAR_CHECK_INT_COMMON

#if (1 == PAR_CFG_ENABLE_ID)
/**
 * @brief Compile-time check A: duplicated parameter IDs in par_table.def.
 *
 * @note Duplicate ID values trigger duplicated "case" labels.
 */
#define PAR_CHECK_ID_DUPLICATE_CASE(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    case ((uint32_t)(id_)):                                                                                                       \
        break;
static void par_compile_check_duplicate_ids(void)
{
    switch (0u)
    {
#define PAR_ITEM_U8                      PAR_CHECK_ID_DUPLICATE_CASE
#define PAR_ITEM_U16                     PAR_CHECK_ID_DUPLICATE_CASE
#define PAR_ITEM_U32                     PAR_CHECK_ID_DUPLICATE_CASE
#define PAR_ITEM_I8                      PAR_CHECK_ID_DUPLICATE_CASE
#define PAR_ITEM_I16                     PAR_CHECK_ID_DUPLICATE_CASE
#define PAR_ITEM_I32                     PAR_CHECK_ID_DUPLICATE_CASE
#define PAR_ITEM_F32                     PAR_CHECK_ID_DUPLICATE_CASE
#define PAR_OBJECT_ITEM_ENABLED_HANDLER  PAR_CHECK_ID_DUPLICATE_CASE
#define PAR_OBJECT_ITEM_DISABLED_HANDLER PAR_CHECK_ID_DUPLICATE_CASE
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
    default:
        break;
    }
}

/**
 * @brief Compile-time check B: external ID hash-bucket collisions in par_table.def.
 * @details The runtime ID map is a strict one-entry-per-bucket structure and does not.
 * implement probing or chaining.
 * Therefore two different external IDs are still invalid when.
 * PAR_HASH_ID_CONST(id_a) == PAR_HASH_ID_CONST(id_b).
 * This check intentionally fails the build early by generating duplicated.
 * "case" labels for colliding bucket indices.
 */
#define PAR_CHECK_ID_BUCKET_CASE(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    case PAR_HASH_ID_CONST(id_):                                                                                               \
        break;
static void par_compile_check_hash_bucket_collision(void)
{
    switch (0u)
    {
#define PAR_ITEM_U8                      PAR_CHECK_ID_BUCKET_CASE
#define PAR_ITEM_U16                     PAR_CHECK_ID_BUCKET_CASE
#define PAR_ITEM_U32                     PAR_CHECK_ID_BUCKET_CASE
#define PAR_ITEM_I8                      PAR_CHECK_ID_BUCKET_CASE
#define PAR_ITEM_I16                     PAR_CHECK_ID_BUCKET_CASE
#define PAR_ITEM_I32                     PAR_CHECK_ID_BUCKET_CASE
#define PAR_ITEM_F32                     PAR_CHECK_ID_BUCKET_CASE
#define PAR_OBJECT_ITEM_ENABLED_HANDLER  PAR_CHECK_ID_BUCKET_CASE
#define PAR_OBJECT_ITEM_DISABLED_HANDLER PAR_CHECK_ID_BUCKET_CASE
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
    default:
        break;
    }
}

/**
 * @brief Keep compile-check helper functions "used" to avoid unused-function warnings.
 */
PAR_STATIC_ASSERT(par_compile_check_duplicate_ids_ref, (sizeof(&par_compile_check_duplicate_ids) > 0u));
PAR_STATIC_ASSERT(par_compile_check_hash_bucket_collision_ref, (sizeof(&par_compile_check_hash_bucket_collision) > 0u));

#undef PAR_CHECK_ID_DUPLICATE_CASE
#undef PAR_CHECK_ID_BUCKET_CASE
#endif /* (1 == PAR_CFG_ENABLE_ID) */
/**
 * @brief Module-scope variables.
 */
/**
 * Parameters definitions.
 *
 * @brief
 *
 * Each defined parameter has following properties:
 *
 * i) Parameter ID: Unique parameter identification number. ID shall not be duplicated.
 * ii) Name: Parameter name. Max. length of 32 chars.
 * iii) Min: Parameter minimum value. Min value must be less than max value.
 * iv) Max: Parameter maximum value. Max value must be more than min value.
 * v) Def: Parameter default value. Default value must lie between interval: [min, max].
 * vi) Unit: In case parameter shows physical value. Max. length of 32 chars.
 * vii) Data type: Parameter data type. Supported types: uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t and float32_t.
 * viii) Access: Access type visible from external device such as PC. Either ReadWrite or ReadOnly.
 * ix) Persistence: Tells if parameter value is being written into NVM.
 *
 *
 * @note User shall fill up wanted parameter definitions!
 */
/**
 * @brief X-Macro table initializers for each parameter value type.
 * @details Signature:
 * (enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_).
 */
#if (1 == PAR_CFG_ENABLE_ID)
#define PAR_INIT_ID(id_) .id = (uint16_t)(id_),
#else
#define PAR_INIT_ID(id_)
#endif /* (1 == PAR_CFG_ENABLE_ID) */

#if (1 == PAR_CFG_ENABLE_NAME)
#define PAR_INIT_NAME(name_) .name = (name_),
#else
#define PAR_INIT_NAME(name_)
#endif /* (1 == PAR_CFG_ENABLE_NAME) */

#if (1 == PAR_CFG_ENABLE_RANGE)
#define PAR_INIT_RANGE_U8(min_, max_)  .value_cfg.scalar.range.min.u8 = (uint8_t)(min_), .value_cfg.scalar.range.max.u8 = (uint8_t)(max_),
#define PAR_INIT_RANGE_U16(min_, max_) .value_cfg.scalar.range.min.u16 = (uint16_t)(min_), .value_cfg.scalar.range.max.u16 = (uint16_t)(max_),
#define PAR_INIT_RANGE_U32(min_, max_) .value_cfg.scalar.range.min.u32 = (uint32_t)(min_), .value_cfg.scalar.range.max.u32 = (uint32_t)(max_),
#define PAR_INIT_RANGE_I8(min_, max_)  .value_cfg.scalar.range.min.i8 = (int8_t)(min_), .value_cfg.scalar.range.max.i8 = (int8_t)(max_),
#define PAR_INIT_RANGE_I16(min_, max_) .value_cfg.scalar.range.min.i16 = (int16_t)(min_), .value_cfg.scalar.range.max.i16 = (int16_t)(max_),
#define PAR_INIT_RANGE_I32(min_, max_) .value_cfg.scalar.range.min.i32 = (int32_t)(min_), .value_cfg.scalar.range.max.i32 = (int32_t)(max_),
#define PAR_INIT_RANGE_F32(min_, max_) .value_cfg.scalar.range.min.f32 = (float32_t)(min_), .value_cfg.scalar.range.max.f32 = (float32_t)(max_),
#else
#define PAR_INIT_RANGE_U8(min_, max_)
#define PAR_INIT_RANGE_U16(min_, max_)
#define PAR_INIT_RANGE_U32(min_, max_)
#define PAR_INIT_RANGE_I8(min_, max_)
#define PAR_INIT_RANGE_I16(min_, max_)
#define PAR_INIT_RANGE_I32(min_, max_)
#define PAR_INIT_RANGE_F32(min_, max_)
#endif /* (1 == PAR_CFG_ENABLE_RANGE) */

#define PAR_INIT_SCALAR_DEF(field_, def_) .value_cfg.scalar.def.field_ = (def_),
#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
#define PAR_INIT_OBJECT(elem_size_, min_len_, max_len_, def_) \
    .value_cfg.object = { .elem_size = (uint16_t)(elem_size_), .range = { .min_len = (uint16_t)(min_len_), .max_len = (uint16_t)(max_len_) }, .def = (def_) },
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

#if (1 == PAR_CFG_ENABLE_UNIT)
#define PAR_INIT_UNIT(unit_) .unit = (unit_),
#else
#define PAR_INIT_UNIT(unit_)
#endif /* (1 == PAR_CFG_ENABLE_UNIT) */

#if (1 == PAR_CFG_ENABLE_ACCESS)
#define PAR_INIT_ACCESS(access_) .access = (access_),
#else
#define PAR_INIT_ACCESS(access_)
#endif /* (1 == PAR_CFG_ENABLE_ACCESS) */

#if (1 == PAR_CFG_ENABLE_ROLE_POLICY)
#define PAR_INIT_READ_ROLES(read_roles_)   .read_roles = (read_roles_),
#define PAR_INIT_WRITE_ROLES(write_roles_) .write_roles = (write_roles_),
#else
#define PAR_INIT_READ_ROLES(read_roles_)
#define PAR_INIT_WRITE_ROLES(write_roles_)
#endif /* (1 == PAR_CFG_ENABLE_ROLE_POLICY) */

#if (1 == PAR_CFG_NVM_EN)
/**
 * @brief Translate the X-Macro persistence column into a stored persist slot index.
 *
 * @details The pers_ argument in par_table.def is written as 1/0. The two-step
 * helper first lets pers_ expand normally, then token-pastes the result into either:
 * - PAR_PERSIST_IDX_VALUE_1(enum_) for persistent entries
 * - PAR_PERSIST_IDX_VALUE_0(enum_) for non-persistent entries
 *
 * The _1 branch returns the dense compile-time slot constant
 * PAR_PERSIST_IDX_<enum_>.
 * The _0 branch returns PAR_PERSIST_IDX_INVALID, because non-persistent
 * parameters do not own any slot in the managed NVM image.
 *
 * Two macro layers are required here because macro arguments are not expanded
 * before token pasting with ##. GCC documents this prescan rule explicitly.
 */
#define PAR_PERSIST_IDX_VALUE(enum_, pers_)        PAR_PERSIST_IDX_VALUE_I(enum_, pers_)
#define PAR_PERSIST_IDX_VALUE_I(enum_, pers_)      PAR_PERSIST_IDX_VALUE_##pers_(enum_)
#define PAR_PERSIST_IDX_VALUE_1(enum_)             PAR_PERSIST_IDX_##enum_
#define PAR_PERSIST_IDX_VALUE_0(enum_)             PAR_PERSIST_IDX_INVALID
#define PAR_OBJECT_PERSIST_IDX_VALUE(enum_, pers_) PAR_OBJECT_PERSIST_IDX_VALUE_I(enum_, pers_)
#define PAR_OBJECT_PERSIST_IDX_VALUE_I(enum_, pers_) \
    PAR_OBJECT_PERSIST_IDX_VALUE_##pers_(enum_)
#define PAR_OBJECT_PERSIST_IDX_VALUE_1(enum_) PAR_OBJECT_PERSIST_IDX_##enum_
#define PAR_OBJECT_PERSIST_IDX_VALUE_0(enum_) PAR_PERSIST_IDX_INVALID
#define PAR_INIT_PERSIST(enum_, pers_)        .persistent = (pers_), .persist_idx = PAR_PERSIST_IDX_VALUE(enum_, pers_),
#define PAR_INIT_OBJECT_PERSIST(enum_, pers_) .persistent = (pers_), .persist_idx = PAR_OBJECT_PERSIST_IDX_VALUE(enum_, pers_),
#else
#define PAR_INIT_PERSIST(enum_, pers_)
#define PAR_INIT_OBJECT_PERSIST(enum_, pers_)
#endif /* (1 == PAR_CFG_NVM_EN) */

#if (1 == PAR_CFG_ENABLE_DESC)
#define PAR_INIT_DESC(desc_) .desc = (desc_),
#else
#define PAR_INIT_DESC(desc_)
#endif /* (1 == PAR_CFG_ENABLE_DESC) */

#define PAR_INIT_SCALAR_ITEM(enum_, id_, name_, type_, range_init_, def_field_, def_value_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    [enum_] = {                                                                                                                                      \
        PAR_INIT_ID(id_)                                                                                                                             \
            PAR_INIT_NAME(name_)                                                                                                                     \
                range_init_                                                                                                                          \
                    PAR_INIT_SCALAR_DEF(def_field_, def_value_)                                                                                      \
                        PAR_INIT_UNIT(unit_)                                                                                                         \
                            .type = (type_),                                                                                                         \
        PAR_INIT_ACCESS(access_)                                                                                                                     \
            PAR_INIT_READ_ROLES(read_roles_)                                                                                                         \
                PAR_INIT_WRITE_ROLES(write_roles_)                                                                                                   \
                    PAR_INIT_PERSIST(enum_, pers_)                                                                                                   \
                        PAR_INIT_DESC(desc_)                                                                                                         \
    },

#define PAR_INIT_U8(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_)  PAR_INIT_SCALAR_ITEM(enum_, id_, name_, ePAR_TYPE_U8, PAR_INIT_RANGE_U8(min_, max_), u8, (uint8_t)(def_), unit_, access_, read_roles_, write_roles_, pers_, desc_)
#define PAR_INIT_U16(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) PAR_INIT_SCALAR_ITEM(enum_, id_, name_, ePAR_TYPE_U16, PAR_INIT_RANGE_U16(min_, max_), u16, (uint16_t)(def_), unit_, access_, read_roles_, write_roles_, pers_, desc_)
#define PAR_INIT_U32(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) PAR_INIT_SCALAR_ITEM(enum_, id_, name_, ePAR_TYPE_U32, PAR_INIT_RANGE_U32(min_, max_), u32, (uint32_t)(def_), unit_, access_, read_roles_, write_roles_, pers_, desc_)
#define PAR_INIT_I8(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_)  PAR_INIT_SCALAR_ITEM(enum_, id_, name_, ePAR_TYPE_I8, PAR_INIT_RANGE_I8(min_, max_), i8, (int8_t)(def_), unit_, access_, read_roles_, write_roles_, pers_, desc_)
#define PAR_INIT_I16(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) PAR_INIT_SCALAR_ITEM(enum_, id_, name_, ePAR_TYPE_I16, PAR_INIT_RANGE_I16(min_, max_), i16, (int16_t)(def_), unit_, access_, read_roles_, write_roles_, pers_, desc_)
#define PAR_INIT_I32(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) PAR_INIT_SCALAR_ITEM(enum_, id_, name_, ePAR_TYPE_I32, PAR_INIT_RANGE_I32(min_, max_), i32, (int32_t)(def_), unit_, access_, read_roles_, write_roles_, pers_, desc_)
#define PAR_INIT_F32(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) PAR_INIT_SCALAR_ITEM(enum_, id_, name_, ePAR_TYPE_F32, PAR_INIT_RANGE_F32(min_, max_), f32, (float32_t)(def_), unit_, access_, read_roles_, write_roles_, pers_, desc_)

#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
#define PAR_INIT_OBJECT_ITEM(enum_, id_, name_, type_, elem_size_, min_len_, max_len_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    [enum_] = {                                                                                                                                       \
        PAR_INIT_ID(id_)                                                                                                                              \
            PAR_INIT_NAME(name_)                                                                                                                      \
                PAR_INIT_OBJECT((elem_size_), (min_len_), (max_len_), (def_))                                                                         \
                    PAR_INIT_UNIT(unit_)                                                                                                              \
                        .type = (type_),                                                                                                              \
        PAR_INIT_ACCESS(access_)                                                                                                                      \
            PAR_INIT_READ_ROLES(read_roles_)                                                                                                          \
                PAR_INIT_WRITE_ROLES(write_roles_)                                                                                                    \
                    PAR_INIT_OBJECT_PERSIST(enum_, pers_)                                                                                             \
                        PAR_INIT_DESC(desc_)                                                                                                          \
    },
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

#define PAR_INIT_STR(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_)     PAR_INIT_OBJECT_ITEM(enum_, id_, name_, ePAR_TYPE_STR, 1U, (min_), (max_), (def_), unit_, access_, read_roles_, write_roles_, pers_, desc_)
#define PAR_INIT_BYTES(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_)   PAR_INIT_OBJECT_ITEM(enum_, id_, name_, ePAR_TYPE_BYTES, 1U, (min_), (max_), (def_), unit_, access_, read_roles_, write_roles_, pers_, desc_)
#define PAR_INIT_ARR_U8(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_)  PAR_INIT_OBJECT_ITEM(enum_, id_, name_, ePAR_TYPE_ARR_U8, 1U, ((uint32_t)(min_) * 1U), ((uint32_t)(max_) * 1U), (def_), unit_, access_, read_roles_, write_roles_, pers_, desc_)
#define PAR_INIT_ARR_U16(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) PAR_INIT_OBJECT_ITEM(enum_, id_, name_, ePAR_TYPE_ARR_U16, sizeof(uint16_t), ((uint32_t)(min_) * (uint32_t)sizeof(uint16_t)), ((uint32_t)(max_) * (uint32_t)sizeof(uint16_t)), (def_), unit_, access_, read_roles_, write_roles_, pers_, desc_)
#define PAR_INIT_ARR_U32(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) PAR_INIT_OBJECT_ITEM(enum_, id_, name_, ePAR_TYPE_ARR_U32, sizeof(uint32_t), ((uint32_t)(min_) * (uint32_t)sizeof(uint32_t)), ((uint32_t)(max_) * (uint32_t)sizeof(uint32_t)), (def_), unit_, access_, read_roles_, write_roles_, pers_, desc_)
#define PAR_ITEM_NOP(...)

#define PAR_ITEM_U8                      PAR_INIT_U8
#define PAR_ITEM_U16                     PAR_INIT_U16
#define PAR_ITEM_U32                     PAR_INIT_U32
#define PAR_ITEM_I8                      PAR_INIT_I8
#define PAR_ITEM_I16                     PAR_INIT_I16
#define PAR_ITEM_I32                     PAR_INIT_I32
#define PAR_ITEM_F32                     PAR_INIT_F32
#define PAR_OBJECT_ITEM_STR_HANDLER      PAR_INIT_STR
#define PAR_OBJECT_ITEM_BYTES_HANDLER    PAR_INIT_BYTES
#define PAR_OBJECT_ITEM_ARR_U8_HANDLER   PAR_INIT_ARR_U8
#define PAR_OBJECT_ITEM_ARR_U16_HANDLER  PAR_INIT_ARR_U16
#define PAR_OBJECT_ITEM_ARR_U32_HANDLER  PAR_INIT_ARR_U32
#define PAR_OBJECT_ITEM_DISABLED_HANDLER PAR_ITEM_NOP
#include "par_object_item_bind_typed.inc"

/**
 * @brief Compile-time parameter metadata table generated from par_table.def.
 */
static const par_cfg_t g_par_table[ePAR_NUM_OF] = {
#include "par_table.def"
};

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
#undef PAR_ITEM_NOP
#undef PAR_INIT_STR
#undef PAR_INIT_BYTES
#undef PAR_INIT_ARR_U8
#undef PAR_INIT_ARR_U16
#undef PAR_INIT_ARR_U32

#undef PAR_INIT_U8
#undef PAR_INIT_U16
#undef PAR_INIT_U32
#undef PAR_INIT_I8
#undef PAR_INIT_I16
#undef PAR_INIT_I32
#undef PAR_INIT_F32
#undef PAR_INIT_SCALAR_ITEM
#undef PAR_INIT_SCALAR_DEF
#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
#undef PAR_INIT_OBJECT_ITEM
#undef PAR_INIT_OBJECT
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */
#undef PAR_INIT_ID
#undef PAR_INIT_NAME
#undef PAR_INIT_RANGE_U8
#undef PAR_INIT_RANGE_U16
#undef PAR_INIT_RANGE_U32
#undef PAR_INIT_RANGE_I8
#undef PAR_INIT_RANGE_I16
#undef PAR_INIT_RANGE_I32
#undef PAR_INIT_RANGE_F32
#undef PAR_INIT_UNIT
#undef PAR_INIT_ACCESS
#undef PAR_INIT_READ_ROLES
#undef PAR_INIT_WRITE_ROLES
#undef PAR_INIT_PERSIST
#undef PAR_INIT_OBJECT_PERSIST
#undef PAR_INIT_DESC
#if (1 == PAR_CFG_NVM_EN)
#undef PAR_PERSIST_IDX_VALUE
#undef PAR_PERSIST_IDX_VALUE_I
#undef PAR_PERSIST_IDX_VALUE_1
#undef PAR_PERSIST_IDX_VALUE_0
#undef PAR_OBJECT_PERSIST_IDX_VALUE
#undef PAR_OBJECT_PERSIST_IDX_VALUE_I
#undef PAR_OBJECT_PERSIST_IDX_VALUE_1
#undef PAR_OBJECT_PERSIST_IDX_VALUE_0
#endif /* (1 == PAR_CFG_NVM_EN) */

/**
 * @brief Configuration-independent compile-time parameter-ID table.
 */
#define PAR_ITEM_ID_VALUE(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) [enum_] = (uint16_t)(id_),
static const uint16_t g_par_id_table[ePAR_NUM_OF] = {
#define PAR_ITEM_NOP(...)
#define PAR_ITEM_U8                      PAR_ITEM_ID_VALUE
#define PAR_ITEM_U16                     PAR_ITEM_ID_VALUE
#define PAR_ITEM_U32                     PAR_ITEM_ID_VALUE
#define PAR_ITEM_I8                      PAR_ITEM_ID_VALUE
#define PAR_ITEM_I16                     PAR_ITEM_ID_VALUE
#define PAR_ITEM_I32                     PAR_ITEM_ID_VALUE
#define PAR_ITEM_F32                     PAR_ITEM_ID_VALUE
#define PAR_OBJECT_ITEM_ENABLED_HANDLER  PAR_ITEM_ID_VALUE
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
#undef PAR_ITEM_ID_VALUE

/**
 * @brief Table size in bytes.
 */
static const uint32_t gu32_par_table_size = sizeof(g_par_table);

/**
 * @brief Compile-time derived number of parameters flagged persistent.
 */
const uint16_t g_par_persistent_count = (uint16_t)PAR_PERSISTENT_COMPILE_COUNT;
/**
 * @brief Function declarations and definitions.
 */
/**
 * @brief Get full Device Parameter configuration table.
 *
 * @return pointer to configuration table.
 */
const par_cfg_t *par_cfg_get_table(void)
{
    return g_par_table;
}
/**
 * @brief Get single Device Parameter configuration.
 *
 * @return pointer to parameter config.
 */
const par_cfg_t *par_cfg_get(const par_num_t par_num)
{
    return &g_par_table[par_num];
}

/**
 * @brief Return the compile-time parameter ID for one entry.
 *
 * @param par_num Parameter number.
 * @return Parameter ID from the generated parameter table.
 */
uint16_t par_cfg_get_param_id_const(const par_num_t par_num)
{
    PAR_ASSERT(par_num < ePAR_NUM_OF);
    return g_par_id_table[par_num];
}
/**
 * @brief Get configuration table size in bytes.
 *
 * @return Size of table in bytes.
 */
uint32_t par_cfg_get_table_size(void)
{
    return gu32_par_table_size;
}
/**
 * @} <!-- END GROUP -->
 */
