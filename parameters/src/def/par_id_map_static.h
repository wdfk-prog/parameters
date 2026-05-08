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
/* Internal-only ID hash geometry shared by compile-time table checks,
 * compile-time static ID-map generation, and optional runtime diagnostic
 * scans. These helpers are not a stable public extension API.
 */
#ifndef PAR_ID_HASH_GOLDEN_RATIO_32
#define PAR_ID_HASH_GOLDEN_RATIO_32 (0x61C88647u)
#endif /* !defined(PAR_ID_HASH_GOLDEN_RATIO_32) */

#ifndef PAR_ID_HASH_MIN_BUCKETS
#define PAR_ID_HASH_MIN_BUCKETS ((uint32_t)(2u * (uint32_t)ePAR_NUM_OF))
#endif /* !defined(PAR_ID_HASH_MIN_BUCKETS) */

#ifndef PAR_ID_HASH_BITS_FROM_MIN_BUCKETS
/**
 * @brief Get the minimum hash-bit count for a required bucket count.
 * @param min_buckets_ Minimum required bucket count.
 * @return Number of hash index bits.
 */
#define PAR_ID_HASH_BITS_FROM_MIN_BUCKETS(min_buckets_)                       \
    (((min_buckets_) <= (1u << 1)) ? 1u : ((min_buckets_) <= (1u << 2)) ? 2u  \
                                      : ((min_buckets_) <= (1u << 3))   ? 3u  \
                                      : ((min_buckets_) <= (1u << 4))   ? 4u  \
                                      : ((min_buckets_) <= (1u << 5))   ? 5u  \
                                      : ((min_buckets_) <= (1u << 6))   ? 6u  \
                                      : ((min_buckets_) <= (1u << 7))   ? 7u  \
                                      : ((min_buckets_) <= (1u << 8))   ? 8u  \
                                      : ((min_buckets_) <= (1u << 9))   ? 9u  \
                                      : ((min_buckets_) <= (1u << 10))  ? 10u \
                                      : ((min_buckets_) <= (1u << 11))  ? 11u \
                                      : ((min_buckets_) <= (1u << 12))  ? 12u \
                                      : ((min_buckets_) <= (1u << 13))  ? 13u \
                                      : ((min_buckets_) <= (1u << 14))  ? 14u \
                                      : ((min_buckets_) <= (1u << 15))  ? 15u \
                                      : ((min_buckets_) <= (1u << 16))  ? 16u \
                                      : ((min_buckets_) <= (1u << 17))  ? 17u \
                                                                        : 18u)
#endif /* !defined(PAR_ID_HASH_BITS_FROM_MIN_BUCKETS) */

#ifndef PAR_ID_HASH_BITS
#define PAR_ID_HASH_BITS PAR_ID_HASH_BITS_FROM_MIN_BUCKETS(PAR_ID_HASH_MIN_BUCKETS)
#endif /* !defined(PAR_ID_HASH_BITS) */

#ifndef PAR_ID_HASH_SIZE
#define PAR_ID_HASH_SIZE (1u << PAR_ID_HASH_BITS)
#endif /* !defined(PAR_ID_HASH_SIZE) */

#ifndef PAR_HASH_ID_CONST
/**
 * @brief Compute the compile-time bucket index for an external parameter ID.
 * @param id_ External parameter ID value.
 * @return Hash bucket index in the static ID map.
 */
#define PAR_HASH_ID_CONST(id_)                            \
    ((((uint32_t)(id_)) * PAR_ID_HASH_GOLDEN_RATIO_32) >> \
     (32u - PAR_ID_HASH_BITS))
#endif /* !defined(PAR_HASH_ID_CONST) */

PAR_STATIC_ASSERT(par_id_hash_size_valid, (PAR_ID_HASH_SIZE >= PAR_ID_HASH_MIN_BUCKETS));
PAR_STATIC_ASSERT(par_id_hash_bits_valid, ((PAR_ID_HASH_BITS > 0u) && (PAR_ID_HASH_BITS < 32u)));
/**
 * @brief Static parameter-ID hash-map entry.
 */
typedef struct
{
    uint16_t id;       /**< External parameter ID. */
    par_num_t par_num; /**< Internal parameter number. */
    uint8_t used;      /**< Non-zero when this hash bucket is occupied. */
} par_id_map_entry_t;

/**
 * @brief Static parameter-ID hash map generated from par_table.def.
 */
extern const par_id_map_entry_t g_par_id_map_static[PAR_ID_HASH_SIZE];
#endif /* (1 == PAR_CFG_ENABLE_ID) */

#endif /* !defined(_PAR_ID_MAP_STATIC_H_) */
