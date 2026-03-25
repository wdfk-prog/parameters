// Copyright (c) 2026 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      par_cfg.c
*@brief     Configuration for device parameters
*@author    wdfk-prog
*@email     1425075683@qq.com
*@date      29.01.2026
*@version   V3.0.1
*/
////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup PAR_CFG
* @{ <!-- BEGIN GROUP -->
*
* 	Configuration for device parameters
*
* 	User shall put code inside inside code block start with
* 	"USER_CODE_BEGIN" and with end of "USER_CODE_END".
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include "par_def.h"
#include "par.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////
/**
 * Shared compile-time range checks for integer parameter items.
 */
#if ( 1 == PAR_CFG_ENABLE_RANGE )
#define PAR_CHECK_INT_COMMON(enum_, min_, max_, def_)                      \
    PAR_STATIC_ASSERT(enum_##_min_lt_max, ((min_) < (max_)));              \
    PAR_STATIC_ASSERT(enum_##_def_ge_min, ((def_) >= (min_)));             \
    PAR_STATIC_ASSERT(enum_##_def_le_max, ((def_) <= (max_)))

/**
 * Compile-time checks for each parameter value type.
 *
 * Signature:
 *   (enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)
 */
#define PAR_CHECK_U8(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)   PAR_CHECK_INT_COMMON(enum_, min_, max_, def_)
#define PAR_CHECK_U16(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)  PAR_CHECK_INT_COMMON(enum_, min_, max_, def_)
#define PAR_CHECK_U32(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)  PAR_CHECK_INT_COMMON(enum_, min_, max_, def_)
#define PAR_CHECK_I8(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)   PAR_CHECK_INT_COMMON(enum_, min_, max_, def_)
#define PAR_CHECK_I16(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)  PAR_CHECK_INT_COMMON(enum_, min_, max_, def_)
#define PAR_CHECK_I32(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)  PAR_CHECK_INT_COMMON(enum_, min_, max_, def_)
#else
#define PAR_CHECK_U8(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)
#define PAR_CHECK_U16(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)
#define PAR_CHECK_U32(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)
#define PAR_CHECK_I8(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)
#define PAR_CHECK_I16(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)
#define PAR_CHECK_I32(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)
#endif
/*
 * NOTE: F32 range checks are runtime-only.
 *
 * Some embedded/legacy GCC toolchains do not reliably treat float comparisons
 * in static assertions as integer constant expressions, and may emit
 * "variably modified '_static_assert_...' at file scope".
 */
#if ( 1 == PAR_CFG_ENABLE_TYPE_F32 )
    #define PAR_CHECK_F32(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)
#else
    #define PAR_CHECK_F32(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_) PAR_STATIC_ASSERT(enum_##_f32_type_is_disabled__remove_PAR_ITEM_F32, 0)
#endif

/**
 * Dispatch map for compile-time checks.
 */
#define PAR_ITEM_U8   PAR_CHECK_U8
#define PAR_ITEM_U16  PAR_CHECK_U16
#define PAR_ITEM_U32  PAR_CHECK_U32
#define PAR_ITEM_I8   PAR_CHECK_I8
#define PAR_ITEM_I16  PAR_CHECK_I16
#define PAR_ITEM_I32  PAR_CHECK_I32
#define PAR_ITEM_F32  PAR_CHECK_F32

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
#if ( 1 == PAR_CFG_ENABLE_RANGE )
#undef PAR_CHECK_INT_COMMON
#endif

#if ( 1 == PAR_CFG_ENABLE_ID )
/**
 * Compile-time check A: duplicated parameter IDs in par_table.def.
 *
 * @note Duplicate ID values trigger duplicated "case" labels.
 */
#define PAR_CHECK_ID_DUPLICATE_CASE(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)  case ((uint32_t)(id_)): break;
static void par_compile_check_duplicate_ids(void)
{
    switch (0u)
    {
        #define PAR_ITEM_U8   PAR_CHECK_ID_DUPLICATE_CASE
        #define PAR_ITEM_U16  PAR_CHECK_ID_DUPLICATE_CASE
        #define PAR_ITEM_U32  PAR_CHECK_ID_DUPLICATE_CASE
        #define PAR_ITEM_I8   PAR_CHECK_ID_DUPLICATE_CASE
        #define PAR_ITEM_I16  PAR_CHECK_ID_DUPLICATE_CASE
        #define PAR_ITEM_I32  PAR_CHECK_ID_DUPLICATE_CASE
        #define PAR_ITEM_F32  PAR_CHECK_ID_DUPLICATE_CASE
        #include "../../par_table.def"
        #undef PAR_ITEM_U8
        #undef PAR_ITEM_U16
        #undef PAR_ITEM_U32
        #undef PAR_ITEM_I8
        #undef PAR_ITEM_I16
        #undef PAR_ITEM_I32
        #undef PAR_ITEM_F32
        default: break;
    }
}

/**
 * Compile-time check B: external ID hash-bucket collisions in par_table.def.
 *
 * The runtime ID map is a strict one-entry-per-bucket structure and does not
 * implement probing or chaining.
 *
 * Therefore two different external IDs are still invalid when
 * PAR_HASH_ID_CONST(id_a) == PAR_HASH_ID_CONST(id_b).
 *
 * This check intentionally fails the build early by generating duplicated
 * "case" labels for colliding bucket indices.
 */
#define PAR_CHECK_ID_BUCKET_CASE(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)     case PAR_HASH_ID_CONST(id_): break;
static void par_compile_check_hash_bucket_collision(void)
{
    switch (0u)
    {
        #define PAR_ITEM_U8   PAR_CHECK_ID_BUCKET_CASE
        #define PAR_ITEM_U16  PAR_CHECK_ID_BUCKET_CASE
        #define PAR_ITEM_U32  PAR_CHECK_ID_BUCKET_CASE
        #define PAR_ITEM_I8   PAR_CHECK_ID_BUCKET_CASE
        #define PAR_ITEM_I16  PAR_CHECK_ID_BUCKET_CASE
        #define PAR_ITEM_I32  PAR_CHECK_ID_BUCKET_CASE
        #define PAR_ITEM_F32  PAR_CHECK_ID_BUCKET_CASE
        #include "../../par_table.def"
        #undef PAR_ITEM_U8
        #undef PAR_ITEM_U16
        #undef PAR_ITEM_U32
        #undef PAR_ITEM_I8
        #undef PAR_ITEM_I16
        #undef PAR_ITEM_I32
        #undef PAR_ITEM_F32
        default: break;
    }
}

/*
 * Keep compile-check helper functions "used" to avoid unused-function warnings.
 */
PAR_STATIC_ASSERT(par_compile_check_duplicate_ids_ref, (sizeof(&par_compile_check_duplicate_ids) > 0u));
PAR_STATIC_ASSERT(par_compile_check_hash_bucket_collision_ref, (sizeof(&par_compile_check_hash_bucket_collision) > 0u));

#undef PAR_CHECK_ID_DUPLICATE_CASE
#undef PAR_CHECK_ID_BUCKET_CASE
#endif

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

/**
 *	Parameters definitions
 *
 *	@brief
 *
 *	Each defined parameter has following properties:
 *
 *		i) 		Parameter ID: 	Unique parameter identification number. ID shall not be duplicated.
 *		ii) 	Name:			Parameter name. Max. length of 32 chars.
 *		iii)	Min:			Parameter minimum value. Min value must be less than max value.
 *		iv)		Max:			Parameter maximum value. Max value must be more than min value.
 *		v)		Def:			Parameter default value. Default value must lie between interval: [min, max]
 *		vi)		Unit:			In case parameter shows physical value. Max. length of 32 chars.
 *		vii)	Data type:		Parameter data type. Supported types: uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t and float32_t
 *		viii)	Access:			Access type visible from external device such as PC. Either ReadWrite or ReadOnly.
 *		ix)		Persistence:	Tells if parameter value is being written into NVM.
 *
 *
 *	@note	User shall fill up wanted parameter definitions!
 */
/**
 * X-Macro table initializers for each parameter value type.
 *
 * Signature:
 *   (enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)
 */
#if ( 1 == PAR_CFG_ENABLE_ID )
    #define PAR_INIT_ID(id_)                        .id = (uint16_t)(id_),
#else
    #define PAR_INIT_ID(id_)
#endif

#if ( 1 == PAR_CFG_ENABLE_NAME )
    #define PAR_INIT_NAME(name_)                    .name = (name_),
#else
    #define PAR_INIT_NAME(name_)
#endif

#if ( 1 == PAR_CFG_ENABLE_RANGE )
    #define PAR_INIT_RANGE_U8(min_, max_)           .range.min.u8  = (uint8_t)(min_),   .range.max.u8 = (uint8_t)(max_),
    #define PAR_INIT_RANGE_U16(min_, max_)          .range.min.u16 = (uint16_t)(min_),  .range.max.u16 = (uint16_t)(max_),
    #define PAR_INIT_RANGE_U32(min_, max_)          .range.min.u32 = (uint32_t)(min_),  .range.max.u32 = (uint32_t)(max_),
    #define PAR_INIT_RANGE_I8(min_, max_)           .range.min.i8  = (int8_t)(min_),    .range.max.i8 = (int8_t)(max_),
    #define PAR_INIT_RANGE_I16(min_, max_)          .range.min.i16 = (int16_t)(min_),   .range.max.i16 = (int16_t)(max_),
    #define PAR_INIT_RANGE_I32(min_, max_)          .range.min.i32 = (int32_t)(min_),   .range.max.i32 = (int32_t)(max_),
    #define PAR_INIT_RANGE_F32(min_, max_)          .range.min.f32 = (float32_t)(min_), .range.max.f32 = (float32_t)(max_),
#else
    #define PAR_INIT_RANGE_U8(min_, max_)
    #define PAR_INIT_RANGE_U16(min_, max_)
    #define PAR_INIT_RANGE_U32(min_, max_)
    #define PAR_INIT_RANGE_I8(min_, max_)
    #define PAR_INIT_RANGE_I16(min_, max_)
    #define PAR_INIT_RANGE_I32(min_, max_)
    #define PAR_INIT_RANGE_F32(min_, max_)
#endif

#if ( 1 == PAR_CFG_ENABLE_UNIT )
    #define PAR_INIT_UNIT(unit_)                    .unit = (unit_),
#else
    #define PAR_INIT_UNIT(unit_)
#endif

#if ( 1 == PAR_CFG_ENABLE_ACCESS )
    #define PAR_INIT_ACCESS(access_)                .access = (access_),
#else
    #define PAR_INIT_ACCESS(access_)
#endif

#if ( 1 == PAR_CFG_ENABLE_PERSIST )
    #define PAR_INIT_PERSIST(pers_)                 .persistent = (pers_),
#else
    #define PAR_INIT_PERSIST(pers_)
#endif

#if ( 1 == PAR_CFG_ENABLE_DESC )
    #define PAR_INIT_DESC(desc_)                    .desc = (desc_),
#else
    #define PAR_INIT_DESC(desc_)
#endif

#define PAR_INIT_U8(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)      \
    [enum_] = {                                                                               \
        PAR_INIT_ID(id_)                                                                      \
        PAR_INIT_NAME(name_)                                                                  \
        PAR_INIT_RANGE_U8(min_, max_)                                                         \
        .def.u8 = (uint8_t)(def_),                                                            \
        PAR_INIT_UNIT(unit_)                                                                  \
        .type = ePAR_TYPE_U8,                                                                 \
        PAR_INIT_ACCESS(access_)                                                              \
        PAR_INIT_PERSIST(pers_)                                                               \
        PAR_INIT_DESC(desc_)                                                                  \
    },

#define PAR_INIT_U16(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)     \
    [enum_] = {                                                                               \
        PAR_INIT_ID(id_)                                                                      \
        PAR_INIT_NAME(name_)                                                                  \
        PAR_INIT_RANGE_U16(min_, max_)                                                        \
        .def.u16 = (uint16_t)(def_),                                                          \
        PAR_INIT_UNIT(unit_)                                                                  \
        .type = ePAR_TYPE_U16,                                                                \
        PAR_INIT_ACCESS(access_)                                                              \
        PAR_INIT_PERSIST(pers_)                                                               \
        PAR_INIT_DESC(desc_)                                                                  \
    },

#define PAR_INIT_U32(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)     \
    [enum_] = {                                                                               \
        PAR_INIT_ID(id_)                                                                      \
        PAR_INIT_NAME(name_)                                                                  \
        PAR_INIT_RANGE_U32(min_, max_)                                                        \
        .def.u32 = (uint32_t)(def_),                                                          \
        PAR_INIT_UNIT(unit_)                                                                  \
        .type = ePAR_TYPE_U32,                                                                \
        PAR_INIT_ACCESS(access_)                                                              \
        PAR_INIT_PERSIST(pers_)                                                               \
        PAR_INIT_DESC(desc_)                                                                  \
    },

#define PAR_INIT_I8(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)      \
    [enum_] = {                                                                               \
        PAR_INIT_ID(id_)                                                                      \
        PAR_INIT_NAME(name_)                                                                  \
        PAR_INIT_RANGE_I8(min_, max_)                                                         \
        .def.i8 = (int8_t)(def_),                                                             \
        PAR_INIT_UNIT(unit_)                                                                  \
        .type = ePAR_TYPE_I8,                                                                 \
        PAR_INIT_ACCESS(access_)                                                              \
        PAR_INIT_PERSIST(pers_)                                                               \
        PAR_INIT_DESC(desc_)                                                                  \
    },

#define PAR_INIT_I16(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)     \
    [enum_] = {                                                                               \
        PAR_INIT_ID(id_)                                                                      \
        PAR_INIT_NAME(name_)                                                                  \
        PAR_INIT_RANGE_I16(min_, max_)                                                        \
        .def.i16 = (int16_t)(def_),                                                           \
        PAR_INIT_UNIT(unit_)                                                                  \
        .type = ePAR_TYPE_I16,                                                                \
        PAR_INIT_ACCESS(access_)                                                              \
        PAR_INIT_PERSIST(pers_)                                                               \
        PAR_INIT_DESC(desc_)                                                                  \
    },

#define PAR_INIT_I32(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)     \
    [enum_] = {                                                                               \
        PAR_INIT_ID(id_)                                                                      \
        PAR_INIT_NAME(name_)                                                                  \
        PAR_INIT_RANGE_I32(min_, max_)                                                        \
        .def.i32 = (int32_t)(def_),                                                           \
        PAR_INIT_UNIT(unit_)                                                                  \
        .type = ePAR_TYPE_I32,                                                                \
        PAR_INIT_ACCESS(access_)                                                              \
        PAR_INIT_PERSIST(pers_)                                                               \
        PAR_INIT_DESC(desc_)                                                                  \
    },

#define PAR_INIT_F32(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)     \
    [enum_] = {                                                                               \
        PAR_INIT_ID(id_)                                                                      \
        PAR_INIT_NAME(name_)                                                                  \
        PAR_INIT_RANGE_F32(min_, max_)                                                        \
        .def.f32 = (float32_t)(def_),                                                         \
        PAR_INIT_UNIT(unit_)                                                                  \
        .type = ePAR_TYPE_F32,                                                                \
        PAR_INIT_ACCESS(access_)                                                              \
        PAR_INIT_PERSIST(pers_)                                                               \
        PAR_INIT_DESC(desc_)                                                                  \
    },

/**
 * Dispatch map for table initialization.
 */
#define PAR_ITEM_U8   PAR_INIT_U8
#define PAR_ITEM_U16  PAR_INIT_U16
#define PAR_ITEM_U32  PAR_INIT_U32
#define PAR_ITEM_I8   PAR_INIT_I8
#define PAR_ITEM_I16  PAR_INIT_I16
#define PAR_ITEM_I32  PAR_INIT_I32
#define PAR_ITEM_F32  PAR_INIT_F32

static const par_cfg_t g_par_table[ePAR_NUM_OF] =
{
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

/**
 * 	Table size in bytes
 */
static const uint32_t gu32_par_table_size = sizeof( g_par_table );

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*		Get full Device Parameter configuration table
*
* @return		pointer to configuration table
*/
////////////////////////////////////////////////////////////////////////////////
const par_cfg_t * par_cfg_get_table(void)
{
	return g_par_table;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Get single Device Parameter configuration
*
* @return       pointer to parameter config
*/
////////////////////////////////////////////////////////////////////////////////
const par_cfg_t * par_cfg_get(const par_num_t par_num)
{
    return &g_par_table[par_num];
}

////////////////////////////////////////////////////////////////////////////////
/**
*		Get configuration table size in bytes
*
* @return	gu32_par_table_size	- Size of table in bytes
*/
////////////////////////////////////////////////////////////////////////////////
uint32_t par_cfg_get_table_size(void)
{
	return gu32_par_table_size;
}

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
