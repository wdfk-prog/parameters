/**
 * @file par_def.c
 * @brief Build parameter-definition tables and derived metadata.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-03-27
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-03-27 1.0     wdfk-prog    first version
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
#include "def/par_def.h"
#include "par.h"
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

/**
 * @brief Compile-time checks for each parameter value type.
 * @details Signature:
 * (enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_).
 */
#define PAR_CHECK_U8(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_)  PAR_CHECK_INT_COMMON(enum_, min_, max_, def_)
#define PAR_CHECK_U16(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) PAR_CHECK_INT_COMMON(enum_, min_, max_, def_)
#define PAR_CHECK_U32(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) PAR_CHECK_INT_COMMON(enum_, min_, max_, def_)
#define PAR_CHECK_I8(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_)  PAR_CHECK_INT_COMMON(enum_, min_, max_, def_)
#define PAR_CHECK_I16(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) PAR_CHECK_INT_COMMON(enum_, min_, max_, def_)
#define PAR_CHECK_I32(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) PAR_CHECK_INT_COMMON(enum_, min_, max_, def_)
#else
#define PAR_CHECK_U8(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_)
#define PAR_CHECK_U16(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_)
#define PAR_CHECK_U32(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_)
#define PAR_CHECK_I8(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_)
#define PAR_CHECK_I16(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_)
#define PAR_CHECK_I32(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_)
#endif
/**
 * @brief NOTE: F32 range checks are runtime-only.
 * @details Some embedded/legacy GCC toolchains do not reliably treat float comparisons.
 * in static assertions as integer constant expressions, and may emit.
 * "variably modified '_static_assert_...' at file scope".
 */
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
#define PAR_CHECK_F32(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_)
#else
#define PAR_CHECK_F32(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) PAR_STATIC_ASSERT(enum_##_f32_type_is_disabled__remove_PAR_ITEM_F32, 0)
#endif

/**
 * @brief Dispatch map for compile-time checks.
 */
#define PAR_ITEM_U8  PAR_CHECK_U8
#define PAR_ITEM_U16 PAR_CHECK_U16
#define PAR_ITEM_U32 PAR_CHECK_U32
#define PAR_ITEM_I8  PAR_CHECK_I8
#define PAR_ITEM_I16 PAR_CHECK_I16
#define PAR_ITEM_I32 PAR_CHECK_I32
#define PAR_ITEM_F32 PAR_CHECK_F32

#include "../../par_table.def"

#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32

#undef PAR_CHECK_U8
#undef PAR_CHECK_U16
#undef PAR_CHECK_U32
#undef PAR_CHECK_I8
#undef PAR_CHECK_I16
#undef PAR_CHECK_I32
#undef PAR_CHECK_F32
#if (1 == PAR_CFG_ENABLE_RANGE)
#undef PAR_CHECK_INT_COMMON
#endif

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
#define PAR_ITEM_U8  PAR_CHECK_ID_DUPLICATE_CASE
#define PAR_ITEM_U16 PAR_CHECK_ID_DUPLICATE_CASE
#define PAR_ITEM_U32 PAR_CHECK_ID_DUPLICATE_CASE
#define PAR_ITEM_I8  PAR_CHECK_ID_DUPLICATE_CASE
#define PAR_ITEM_I16 PAR_CHECK_ID_DUPLICATE_CASE
#define PAR_ITEM_I32 PAR_CHECK_ID_DUPLICATE_CASE
#define PAR_ITEM_F32 PAR_CHECK_ID_DUPLICATE_CASE
#include "../../par_table.def"
#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32
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
#define PAR_ITEM_U8  PAR_CHECK_ID_BUCKET_CASE
#define PAR_ITEM_U16 PAR_CHECK_ID_BUCKET_CASE
#define PAR_ITEM_U32 PAR_CHECK_ID_BUCKET_CASE
#define PAR_ITEM_I8  PAR_CHECK_ID_BUCKET_CASE
#define PAR_ITEM_I16 PAR_CHECK_ID_BUCKET_CASE
#define PAR_ITEM_I32 PAR_CHECK_ID_BUCKET_CASE
#define PAR_ITEM_F32 PAR_CHECK_ID_BUCKET_CASE
#include "../../par_table.def"
#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32
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
#endif
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
#endif

#if (1 == PAR_CFG_ENABLE_NAME)
#define PAR_INIT_NAME(name_) .name = (name_),
#else
#define PAR_INIT_NAME(name_)
#endif

#if (1 == PAR_CFG_ENABLE_RANGE)
#define PAR_INIT_RANGE_U8(min_, max_)  .range.min.u8 = (uint8_t)(min_), .range.max.u8 = (uint8_t)(max_),
#define PAR_INIT_RANGE_U16(min_, max_) .range.min.u16 = (uint16_t)(min_), .range.max.u16 = (uint16_t)(max_),
#define PAR_INIT_RANGE_U32(min_, max_) .range.min.u32 = (uint32_t)(min_), .range.max.u32 = (uint32_t)(max_),
#define PAR_INIT_RANGE_I8(min_, max_)  .range.min.i8 = (int8_t)(min_), .range.max.i8 = (int8_t)(max_),
#define PAR_INIT_RANGE_I16(min_, max_) .range.min.i16 = (int16_t)(min_), .range.max.i16 = (int16_t)(max_),
#define PAR_INIT_RANGE_I32(min_, max_) .range.min.i32 = (int32_t)(min_), .range.max.i32 = (int32_t)(max_),
#define PAR_INIT_RANGE_F32(min_, max_) .range.min.f32 = (float32_t)(min_), .range.max.f32 = (float32_t)(max_),
#else
#define PAR_INIT_RANGE_U8(min_, max_)
#define PAR_INIT_RANGE_U16(min_, max_)
#define PAR_INIT_RANGE_U32(min_, max_)
#define PAR_INIT_RANGE_I8(min_, max_)
#define PAR_INIT_RANGE_I16(min_, max_)
#define PAR_INIT_RANGE_I32(min_, max_)
#define PAR_INIT_RANGE_F32(min_, max_)
#endif

#if (1 == PAR_CFG_ENABLE_UNIT)
#define PAR_INIT_UNIT(unit_) .unit = (unit_),
#else
#define PAR_INIT_UNIT(unit_)
#endif

#if (1 == PAR_CFG_ENABLE_ACCESS)
#define PAR_INIT_ACCESS(access_) .access = (access_),
#else
#define PAR_INIT_ACCESS(access_)
#endif

#if (1 == PAR_CFG_ENABLE_ROLE_POLICY)
#define PAR_INIT_READ_ROLES(read_roles_)   .read_roles = (read_roles_),
#define PAR_INIT_WRITE_ROLES(write_roles_) .write_roles = (write_roles_),
#else
#define PAR_INIT_READ_ROLES(read_roles_)
#define PAR_INIT_WRITE_ROLES(write_roles_)
#endif

#if (1 == PAR_CFG_NVM_EN)
/**
 * @brief Translate the X-Macro persistence column into a stored persist slot index.
 *
 * @details The pers_ argument in par_table.def is written as true/false. Because
 * <stdbool.h> expands those tokens to 1/0, the two-step helper first lets pers_
 * expand normally, then token-pastes the result into either:
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
#define PAR_PERSIST_IDX_VALUE(enum_, pers_)   PAR_PERSIST_IDX_VALUE_I(enum_, pers_)
#define PAR_PERSIST_IDX_VALUE_I(enum_, pers_) PAR_PERSIST_IDX_VALUE_##pers_(enum_)
#define PAR_PERSIST_IDX_VALUE_1(enum_)        PAR_PERSIST_IDX_##enum_
#define PAR_PERSIST_IDX_VALUE_0(enum_)        PAR_PERSIST_IDX_INVALID
#define PAR_INIT_PERSIST(enum_, pers_)        .persistent = (pers_), .persist_idx = PAR_PERSIST_IDX_VALUE(enum_, pers_),
#else
#define PAR_INIT_PERSIST(enum_, pers_)
#endif

#if (1 == PAR_CFG_ENABLE_DESC)
#define PAR_INIT_DESC(desc_) .desc = (desc_),
#else
#define PAR_INIT_DESC(desc_)
#endif

#define PAR_INIT_U8(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    [enum_] = {                                                                                                   \
        PAR_INIT_ID(id_)                                                                                          \
            PAR_INIT_NAME(name_)                                                                                  \
                PAR_INIT_RANGE_U8(min_, max_)                                                                     \
                    .def.u8 = (uint8_t)(def_),                                                                    \
        PAR_INIT_UNIT(unit_)                                                                                      \
            .type = ePAR_TYPE_U8,                                                                                 \
        PAR_INIT_ACCESS(access_)                                                                                  \
            PAR_INIT_READ_ROLES(read_roles_)                                                                      \
                PAR_INIT_WRITE_ROLES(write_roles_)                                                                \
                    PAR_INIT_PERSIST(enum_, pers_)                                                                \
                        PAR_INIT_DESC(desc_)                                                                      \
    },

#define PAR_INIT_U16(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    [enum_] = {                                                                                                    \
        PAR_INIT_ID(id_)                                                                                           \
            PAR_INIT_NAME(name_)                                                                                   \
                PAR_INIT_RANGE_U16(min_, max_)                                                                     \
                    .def.u16 = (uint16_t)(def_),                                                                   \
        PAR_INIT_UNIT(unit_)                                                                                       \
            .type = ePAR_TYPE_U16,                                                                                 \
        PAR_INIT_ACCESS(access_)                                                                                   \
            PAR_INIT_READ_ROLES(read_roles_)                                                                       \
                PAR_INIT_WRITE_ROLES(write_roles_)                                                                 \
                    PAR_INIT_PERSIST(enum_, pers_)                                                                 \
                        PAR_INIT_DESC(desc_)                                                                       \
    },

#define PAR_INIT_U32(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    [enum_] = {                                                                                                    \
        PAR_INIT_ID(id_)                                                                                           \
            PAR_INIT_NAME(name_)                                                                                   \
                PAR_INIT_RANGE_U32(min_, max_)                                                                     \
                    .def.u32 = (uint32_t)(def_),                                                                   \
        PAR_INIT_UNIT(unit_)                                                                                       \
            .type = ePAR_TYPE_U32,                                                                                 \
        PAR_INIT_ACCESS(access_)                                                                                   \
            PAR_INIT_READ_ROLES(read_roles_)                                                                       \
                PAR_INIT_WRITE_ROLES(write_roles_)                                                                 \
                    PAR_INIT_PERSIST(enum_, pers_)                                                                 \
                        PAR_INIT_DESC(desc_)                                                                       \
    },

#define PAR_INIT_I8(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    [enum_] = {                                                                                                   \
        PAR_INIT_ID(id_)                                                                                          \
            PAR_INIT_NAME(name_)                                                                                  \
                PAR_INIT_RANGE_I8(min_, max_)                                                                     \
                    .def.i8 = (int8_t)(def_),                                                                     \
        PAR_INIT_UNIT(unit_)                                                                                      \
            .type = ePAR_TYPE_I8,                                                                                 \
        PAR_INIT_ACCESS(access_)                                                                                  \
            PAR_INIT_READ_ROLES(read_roles_)                                                                      \
                PAR_INIT_WRITE_ROLES(write_roles_)                                                                \
                    PAR_INIT_PERSIST(enum_, pers_)                                                                \
                        PAR_INIT_DESC(desc_)                                                                      \
    },

#define PAR_INIT_I16(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    [enum_] = {                                                                                                    \
        PAR_INIT_ID(id_)                                                                                           \
            PAR_INIT_NAME(name_)                                                                                   \
                PAR_INIT_RANGE_I16(min_, max_)                                                                     \
                    .def.i16 = (int16_t)(def_),                                                                    \
        PAR_INIT_UNIT(unit_)                                                                                       \
            .type = ePAR_TYPE_I16,                                                                                 \
        PAR_INIT_ACCESS(access_)                                                                                   \
            PAR_INIT_READ_ROLES(read_roles_)                                                                       \
                PAR_INIT_WRITE_ROLES(write_roles_)                                                                 \
                    PAR_INIT_PERSIST(enum_, pers_)                                                                 \
                        PAR_INIT_DESC(desc_)                                                                       \
    },

#define PAR_INIT_I32(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    [enum_] = {                                                                                                    \
        PAR_INIT_ID(id_)                                                                                           \
            PAR_INIT_NAME(name_)                                                                                   \
                PAR_INIT_RANGE_I32(min_, max_)                                                                     \
                    .def.i32 = (int32_t)(def_),                                                                    \
        PAR_INIT_UNIT(unit_)                                                                                       \
            .type = ePAR_TYPE_I32,                                                                                 \
        PAR_INIT_ACCESS(access_)                                                                                   \
            PAR_INIT_READ_ROLES(read_roles_)                                                                       \
                PAR_INIT_WRITE_ROLES(write_roles_)                                                                 \
                    PAR_INIT_PERSIST(enum_, pers_)                                                                 \
                        PAR_INIT_DESC(desc_)                                                                       \
    },

#define PAR_INIT_F32(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) \
    [enum_] = {                                                                                                    \
        PAR_INIT_ID(id_)                                                                                           \
            PAR_INIT_NAME(name_)                                                                                   \
                PAR_INIT_RANGE_F32(min_, max_)                                                                     \
                    .def.f32 = (float32_t)(def_),                                                                  \
        PAR_INIT_UNIT(unit_)                                                                                       \
            .type = ePAR_TYPE_F32,                                                                                 \
        PAR_INIT_ACCESS(access_)                                                                                   \
            PAR_INIT_READ_ROLES(read_roles_)                                                                       \
                PAR_INIT_WRITE_ROLES(write_roles_)                                                                 \
                    PAR_INIT_PERSIST(enum_, pers_)                                                                 \
                        PAR_INIT_DESC(desc_)                                                                       \
    },

#define PAR_ITEM_U8  PAR_INIT_U8
#define PAR_ITEM_U16 PAR_INIT_U16
#define PAR_ITEM_U32 PAR_INIT_U32
#define PAR_ITEM_I8  PAR_INIT_I8
#define PAR_ITEM_I16 PAR_INIT_I16
#define PAR_ITEM_I32 PAR_INIT_I32
#define PAR_ITEM_F32 PAR_INIT_F32

static const par_cfg_t g_par_table[ePAR_NUM_OF] = {
#include "../../par_table.def"
};

#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32

#undef PAR_INIT_U8
#undef PAR_INIT_U16
#undef PAR_INIT_U32
#undef PAR_INIT_I8
#undef PAR_INIT_I16
#undef PAR_INIT_I32
#undef PAR_INIT_F32
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
#undef PAR_INIT_PERSIST
#undef PAR_INIT_DESC
#if (1 == PAR_CFG_NVM_EN)
#undef PAR_PERSIST_IDX_VALUE
#undef PAR_PERSIST_IDX_VALUE_I
#undef PAR_PERSIST_IDX_VALUE_1
#undef PAR_PERSIST_IDX_VALUE_0
#endif

/**
 * @brief Configuration-independent compile-time parameter-ID table.
 */
#define PAR_ITEM_ID_VALUE(enum_, id_, name_, min_, max_, def_, unit_, access_, read_roles_, write_roles_, pers_, desc_) [enum_] = (uint16_t)(id_),
static const uint16_t g_par_id_table[ePAR_NUM_OF] = {
#define PAR_ITEM_U8  PAR_ITEM_ID_VALUE
#define PAR_ITEM_U16 PAR_ITEM_ID_VALUE
#define PAR_ITEM_U32 PAR_ITEM_ID_VALUE
#define PAR_ITEM_I8  PAR_ITEM_ID_VALUE
#define PAR_ITEM_I16 PAR_ITEM_ID_VALUE
#define PAR_ITEM_I32 PAR_ITEM_ID_VALUE
#define PAR_ITEM_F32 PAR_ITEM_ID_VALUE
#include "../../par_table.def"
#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32
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
