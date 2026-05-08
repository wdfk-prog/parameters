/**
 * @file par_layout.h
 * @brief Declare parameter storage layout helpers.
 * @author wdfk-prog
 * @version 1.1
 * @date 2026-04-22
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-03-27 1.0     wdfk-prog     first version
 * 2026-04-22 1.1     wdfk-prog     add object layout counters and static object-pool map
 */
#ifndef _PAR_LAYOUT_H_
#define _PAR_LAYOUT_H_
/**
 * @brief Include dependencies.
 */
#include <stdint.h>

#include "par_cfg.h"
#include "par_def.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */
/**
 * @brief Aggregated storage counts for one active layout source.
 */
typedef struct
{
    uint16_t count8;       /**< Number of 8-bit scalar storage entries. */
    uint16_t count16;      /**< Number of 16-bit scalar storage entries. */
    uint16_t count32;      /**< Number of 32-bit scalar storage entries. */
    uint16_t count_obj;    /**< Number of object storage slots. */
    uint32_t obj_pool_bytes; /**< Total object-pool size in bytes. */
} par_layout_count_t;

#if (PAR_CFG_LAYOUT_SOURCE == PAR_CFG_LAYOUT_SCRIPT)
/**
 * @brief Prime modulus used by compile-time layout signature components.
 */
#define PAR_LAYOUT_SIGNATURE_MOD (65521u)
/**
 * @brief Stable type codes used by the layout signature.
 */
#define PAR_LAYOUT_SIGNATURE_TYPE_U8      (1u)
#define PAR_LAYOUT_SIGNATURE_TYPE_I8      (2u)
#define PAR_LAYOUT_SIGNATURE_TYPE_U16     (3u)
#define PAR_LAYOUT_SIGNATURE_TYPE_I16     (4u)
#define PAR_LAYOUT_SIGNATURE_TYPE_U32     (5u)
#define PAR_LAYOUT_SIGNATURE_TYPE_I32     (6u)
#define PAR_LAYOUT_SIGNATURE_TYPE_F32     (7u)
#define PAR_LAYOUT_SIGNATURE_TYPE_STR     (8u)
#define PAR_LAYOUT_SIGNATURE_TYPE_BYTES   (9u)
#define PAR_LAYOUT_SIGNATURE_TYPE_ARR_U8  (10u)
#define PAR_LAYOUT_SIGNATURE_TYPE_ARR_U16 (11u)
#define PAR_LAYOUT_SIGNATURE_TYPE_ARR_U32 (12u)
/**
 * @brief First signature contribution for one active X-Macro row.
 */
#define PAR_LAYOUT_SIGNATURE_TERM_A(enum_, id_, type_code_, obj_bytes_)      \
    (((((uint32_t)(enum_) + 1u) * ((uint32_t)(type_code_) + 1u) * 257u) +    \
      ((((uint32_t)(enum_) + 1u) * (((uint32_t)(id_) % 257u) + 1u)) * 17u) + \
      ((uint32_t)(id_) * 5u) + (((uint32_t)(obj_bytes_) + 1u) * 31u)) %      \
     PAR_LAYOUT_SIGNATURE_MOD)
/**
 * @brief Second signature contribution for one active X-Macro row.
 */
#define PAR_LAYOUT_SIGNATURE_TERM_B(enum_, id_, type_code_, obj_bytes_) \
    (((((uint32_t)(enum_) + 1u) * (((uint32_t)(id_) % 257u) + 1u)) +    \
      ((uint32_t)(id_) * 3u) + ((uint32_t)(type_code_) * 389u) +        \
      (((uint32_t)(obj_bytes_) + 1u) * 13u)) %                          \
     PAR_LAYOUT_SIGNATURE_MOD)
#define PAR_LAYOUT_SIGNATURE_A_SCALAR(type_code_, ...) \
    +PAR_LAYOUT_SIGNATURE_TERM_A(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_ID(__VA_ARGS__), type_code_, 0u)
#define PAR_LAYOUT_SIGNATURE_B_SCALAR(type_code_, ...) \
    +PAR_LAYOUT_SIGNATURE_TERM_B(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_ID(__VA_ARGS__), type_code_, 0u)
#define PAR_LAYOUT_SIGNATURE_A_OBJECT(type_code_, obj_bytes_, ...) \
    +PAR_LAYOUT_SIGNATURE_TERM_A(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_ID(__VA_ARGS__), type_code_, obj_bytes_)
#define PAR_LAYOUT_SIGNATURE_B_OBJECT(type_code_, obj_bytes_, ...) \
    +PAR_LAYOUT_SIGNATURE_TERM_B(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_ID(__VA_ARGS__), type_code_, obj_bytes_)
#define PAR_LAYOUT_SIGNATURE_ZERO(...) +0u
#define PAR_LAYOUT_SIGNATURE_A_STR(...) \
    PAR_LAYOUT_SIGNATURE_A_OBJECT(PAR_LAYOUT_SIGNATURE_TYPE_STR, (uint32_t)PAR_XARG_MAX(__VA_ARGS__), __VA_ARGS__)
#define PAR_LAYOUT_SIGNATURE_B_STR(...) \
    PAR_LAYOUT_SIGNATURE_B_OBJECT(PAR_LAYOUT_SIGNATURE_TYPE_STR, (uint32_t)PAR_XARG_MAX(__VA_ARGS__), __VA_ARGS__)
#define PAR_LAYOUT_SIGNATURE_A_BYTES(...) \
    PAR_LAYOUT_SIGNATURE_A_OBJECT(PAR_LAYOUT_SIGNATURE_TYPE_BYTES, (uint32_t)PAR_XARG_MAX(__VA_ARGS__), __VA_ARGS__)
#define PAR_LAYOUT_SIGNATURE_B_BYTES(...) \
    PAR_LAYOUT_SIGNATURE_B_OBJECT(PAR_LAYOUT_SIGNATURE_TYPE_BYTES, (uint32_t)PAR_XARG_MAX(__VA_ARGS__), __VA_ARGS__)
#define PAR_LAYOUT_SIGNATURE_A_ARR_U8(...) \
    PAR_LAYOUT_SIGNATURE_A_OBJECT(PAR_LAYOUT_SIGNATURE_TYPE_ARR_U8, (uint32_t)PAR_XARG_MAX(__VA_ARGS__), __VA_ARGS__)
#define PAR_LAYOUT_SIGNATURE_B_ARR_U8(...) \
    PAR_LAYOUT_SIGNATURE_B_OBJECT(PAR_LAYOUT_SIGNATURE_TYPE_ARR_U8, (uint32_t)PAR_XARG_MAX(__VA_ARGS__), __VA_ARGS__)
#define PAR_LAYOUT_SIGNATURE_A_ARR_U16(...)                                                           \
    PAR_LAYOUT_SIGNATURE_A_OBJECT(PAR_LAYOUT_SIGNATURE_TYPE_ARR_U16,                                  \
                                  ((uint32_t)PAR_XARG_MAX(__VA_ARGS__) * (uint32_t)sizeof(uint16_t)), \
                                  __VA_ARGS__)
#define PAR_LAYOUT_SIGNATURE_B_ARR_U16(...)                                                           \
    PAR_LAYOUT_SIGNATURE_B_OBJECT(PAR_LAYOUT_SIGNATURE_TYPE_ARR_U16,                                  \
                                  ((uint32_t)PAR_XARG_MAX(__VA_ARGS__) * (uint32_t)sizeof(uint16_t)), \
                                  __VA_ARGS__)
#define PAR_LAYOUT_SIGNATURE_A_ARR_U32(...)                                                           \
    PAR_LAYOUT_SIGNATURE_A_OBJECT(PAR_LAYOUT_SIGNATURE_TYPE_ARR_U32,                                  \
                                  ((uint32_t)PAR_XARG_MAX(__VA_ARGS__) * (uint32_t)sizeof(uint32_t)), \
                                  __VA_ARGS__)
#define PAR_LAYOUT_SIGNATURE_B_ARR_U32(...)                                                           \
    PAR_LAYOUT_SIGNATURE_B_OBJECT(PAR_LAYOUT_SIGNATURE_TYPE_ARR_U32,                                  \
                                  ((uint32_t)PAR_XARG_MAX(__VA_ARGS__) * (uint32_t)sizeof(uint32_t)), \
                                  __VA_ARGS__)
/**
 * @brief Layout signature independently expanded from the current X-Macro table.
 */
enum
{
    PAR_LAYOUT_COMPILE_SIGNATURE_A = (0u
#define PAR_ITEM_U8(...)                 PAR_LAYOUT_SIGNATURE_A_SCALAR(PAR_LAYOUT_SIGNATURE_TYPE_U8, __VA_ARGS__)
#define PAR_ITEM_U16(...)                PAR_LAYOUT_SIGNATURE_A_SCALAR(PAR_LAYOUT_SIGNATURE_TYPE_U16, __VA_ARGS__)
#define PAR_ITEM_U32(...)                PAR_LAYOUT_SIGNATURE_A_SCALAR(PAR_LAYOUT_SIGNATURE_TYPE_U32, __VA_ARGS__)
#define PAR_ITEM_I8(...)                 PAR_LAYOUT_SIGNATURE_A_SCALAR(PAR_LAYOUT_SIGNATURE_TYPE_I8, __VA_ARGS__)
#define PAR_ITEM_I16(...)                PAR_LAYOUT_SIGNATURE_A_SCALAR(PAR_LAYOUT_SIGNATURE_TYPE_I16, __VA_ARGS__)
#define PAR_ITEM_I32(...)                PAR_LAYOUT_SIGNATURE_A_SCALAR(PAR_LAYOUT_SIGNATURE_TYPE_I32, __VA_ARGS__)
#define PAR_ITEM_F32(...)                PAR_LAYOUT_SIGNATURE_A_SCALAR(PAR_LAYOUT_SIGNATURE_TYPE_F32, __VA_ARGS__)
#define PAR_OBJECT_ITEM_STR_HANDLER      PAR_LAYOUT_SIGNATURE_A_STR
#define PAR_OBJECT_ITEM_BYTES_HANDLER    PAR_LAYOUT_SIGNATURE_A_BYTES
#define PAR_OBJECT_ITEM_ARR_U8_HANDLER   PAR_LAYOUT_SIGNATURE_A_ARR_U8
#define PAR_OBJECT_ITEM_ARR_U16_HANDLER  PAR_LAYOUT_SIGNATURE_A_ARR_U16
#define PAR_OBJECT_ITEM_ARR_U32_HANDLER  PAR_LAYOUT_SIGNATURE_A_ARR_U32
#define PAR_OBJECT_ITEM_DISABLED_HANDLER PAR_LAYOUT_SIGNATURE_ZERO
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
                                      ) %
                                     PAR_LAYOUT_SIGNATURE_MOD,

    PAR_LAYOUT_COMPILE_SIGNATURE_B = (0u
#define PAR_ITEM_U8(...)                 PAR_LAYOUT_SIGNATURE_B_SCALAR(PAR_LAYOUT_SIGNATURE_TYPE_U8, __VA_ARGS__)
#define PAR_ITEM_U16(...)                PAR_LAYOUT_SIGNATURE_B_SCALAR(PAR_LAYOUT_SIGNATURE_TYPE_U16, __VA_ARGS__)
#define PAR_ITEM_U32(...)                PAR_LAYOUT_SIGNATURE_B_SCALAR(PAR_LAYOUT_SIGNATURE_TYPE_U32, __VA_ARGS__)
#define PAR_ITEM_I8(...)                 PAR_LAYOUT_SIGNATURE_B_SCALAR(PAR_LAYOUT_SIGNATURE_TYPE_I8, __VA_ARGS__)
#define PAR_ITEM_I16(...)                PAR_LAYOUT_SIGNATURE_B_SCALAR(PAR_LAYOUT_SIGNATURE_TYPE_I16, __VA_ARGS__)
#define PAR_ITEM_I32(...)                PAR_LAYOUT_SIGNATURE_B_SCALAR(PAR_LAYOUT_SIGNATURE_TYPE_I32, __VA_ARGS__)
#define PAR_ITEM_F32(...)                PAR_LAYOUT_SIGNATURE_B_SCALAR(PAR_LAYOUT_SIGNATURE_TYPE_F32, __VA_ARGS__)
#define PAR_OBJECT_ITEM_STR_HANDLER      PAR_LAYOUT_SIGNATURE_B_STR
#define PAR_OBJECT_ITEM_BYTES_HANDLER    PAR_LAYOUT_SIGNATURE_B_BYTES
#define PAR_OBJECT_ITEM_ARR_U8_HANDLER   PAR_LAYOUT_SIGNATURE_B_ARR_U8
#define PAR_OBJECT_ITEM_ARR_U16_HANDLER  PAR_LAYOUT_SIGNATURE_B_ARR_U16
#define PAR_OBJECT_ITEM_ARR_U32_HANDLER  PAR_LAYOUT_SIGNATURE_B_ARR_U32
#define PAR_OBJECT_ITEM_DISABLED_HANDLER PAR_LAYOUT_SIGNATURE_ZERO
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
                                      ) %
                                     PAR_LAYOUT_SIGNATURE_MOD
};
#define PAR_LAYOUT_COMPILE_SIGNATURE \
    (((uint32_t)PAR_LAYOUT_COMPILE_SIGNATURE_A << 16u) ^ (uint32_t)PAR_LAYOUT_COMPILE_SIGNATURE_B)
#endif /* (PAR_CFG_LAYOUT_SOURCE == PAR_CFG_LAYOUT_SCRIPT) */

#if (PAR_CFG_LAYOUT_SOURCE == PAR_CFG_LAYOUT_COMPILE_SCAN)
#define PAR_STORAGE_COUNT8         (PAR_LAYOUT_COMPILE_COUNT8)
#define PAR_STORAGE_COUNT16        (PAR_LAYOUT_COMPILE_COUNT16)
#define PAR_STORAGE_COUNT32        (PAR_LAYOUT_COMPILE_COUNT32)
#define PAR_STORAGE_COUNTOBJ       (PAR_LAYOUT_COMPILE_COUNTOBJ)
#define PAR_STORAGE_OBJ_POOL_BYTES (PAR_LAYOUT_COMPILE_OBJ_POOL_BYTES)
#elif (PAR_CFG_LAYOUT_SOURCE == PAR_CFG_LAYOUT_SCRIPT)
#include PAR_CFG_LAYOUT_STATIC_INCLUDE
#define PAR_STORAGE_COUNT8  (PAR_LAYOUT_STATIC_COUNT8)
#define PAR_STORAGE_COUNT16 (PAR_LAYOUT_STATIC_COUNT16)
#define PAR_STORAGE_COUNT32 (PAR_LAYOUT_STATIC_COUNT32)
#ifndef PAR_LAYOUT_STATIC_SIGNATURE
#error "PAR_LAYOUT_STATIC_SIGNATURE must be provided by static layout include!"
#endif /* !defined(PAR_LAYOUT_STATIC_SIGNATURE) */
PAR_STATIC_ASSERT(par_layout_static_signature_matches_table,
                  (PAR_LAYOUT_STATIC_SIGNATURE == PAR_LAYOUT_COMPILE_SIGNATURE));
PAR_STATIC_ASSERT(par_layout_static_count8_matches_table, (PAR_STORAGE_COUNT8 == PAR_LAYOUT_COMPILE_COUNT8));
PAR_STATIC_ASSERT(par_layout_static_count16_matches_table, (PAR_STORAGE_COUNT16 == PAR_LAYOUT_COMPILE_COUNT16));
PAR_STATIC_ASSERT(par_layout_static_count32_matches_table, (PAR_STORAGE_COUNT32 == PAR_LAYOUT_COMPILE_COUNT32));
#if (PAR_LAYOUT_COMPILE_COUNTOBJ > 0u)
#ifndef PAR_LAYOUT_STATIC_COUNTOBJ
#error "PAR_LAYOUT_STATIC_COUNTOBJ must be provided by static layout include when object rows are present!"
#endif /* !defined(PAR_LAYOUT_STATIC_COUNTOBJ) */
#ifndef PAR_LAYOUT_STATIC_OBJ_POOL_BYTES
#error "PAR_LAYOUT_STATIC_OBJ_POOL_BYTES must be provided by static layout include when object rows are present!"
#endif /* !defined(PAR_LAYOUT_STATIC_OBJ_POOL_BYTES) */
#define PAR_STORAGE_COUNTOBJ       (PAR_LAYOUT_STATIC_COUNTOBJ)
#define PAR_STORAGE_OBJ_POOL_BYTES (PAR_LAYOUT_STATIC_OBJ_POOL_BYTES)
PAR_STATIC_ASSERT(par_layout_static_count_obj_matches_table, (PAR_STORAGE_COUNTOBJ == PAR_LAYOUT_COMPILE_COUNTOBJ));
PAR_STATIC_ASSERT(par_layout_static_obj_pool_bytes_matches_table, (PAR_STORAGE_OBJ_POOL_BYTES == PAR_LAYOUT_COMPILE_OBJ_POOL_BYTES));
#else
#define PAR_STORAGE_COUNTOBJ       (0u)
#define PAR_STORAGE_OBJ_POOL_BYTES (0u)
#endif /* (PAR_LAYOUT_COMPILE_COUNTOBJ > 0u) */
#else
#error "Unsupported PAR_CFG_LAYOUT_SOURCE value!"
#endif /* (PAR_CFG_LAYOUT_SOURCE == PAR_CFG_LAYOUT_COMPILE_SCAN) */

#define PAR_STORAGE_NONZERO(count_) (((count_) > 0u) ? (count_) : 1u)
/**
 * @brief Return the active parameter offset table.
 * @return Pointer to the active offset table.
 */
const uint16_t *par_layout_get_offset_table(void);
/**
 * @brief Return the storage offset for one parameter.
 * @param par_num Parameter number.
 * @return Storage offset for one parameter.
 */
uint16_t par_layout_get_offset(const par_num_t par_num);
/**
 * @brief Return the shared object-pool byte offset for one parameter.
 * @param par_num Parameter number.
 * @return Object-pool byte offset for one parameter.
 */
uint32_t par_layout_get_obj_pool_offset(const par_num_t par_num);
/**
 * @brief Return the grouped storage counts.
 * @return Storage counts for scalar groups and the object pool.
 */
par_layout_count_t par_layout_get_count(void);
/**
 * @brief Initialize the storage layout metadata.
 */
void par_layout_init(void);

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* !defined(_PAR_LAYOUT_H_) */
