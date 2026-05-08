/*
 * DO NOT EDIT.
 * Generated from parameters/schema/par_table.csv by parameters/tools/pargen.py.
 */
/**
 * @file par_generated_info.h
 * @brief Declare generated parameter-table summary metadata.
 */

#ifndef _PAR_GENERATED_INFO_H_
#define _PAR_GENERATED_INFO_H_

#include <stdint.h>
#include "par_layout_static.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/**
 * @brief Generated parameter-table summary.
 */
typedef struct
{
    uint16_t param_count;      /**< Total compiled parameter count. */
    uint16_t count8;           /**< Number of 8-bit storage entries. */
    uint16_t count16;          /**< Number of 16-bit storage entries. */
    uint16_t count32;          /**< Number of 32-bit storage entries. */
    uint16_t count_obj;        /**< Number of object storage entries. */
    uint32_t obj_pool_bytes;   /**< Total object-pool capacity in bytes. */
    uint32_t id_hash_bits;     /**< Static ID hash bit count. */
    uint32_t id_hash_size;     /**< Static ID hash bucket count. */
} par_generated_info_t;

/**
 * @brief Generated parameter-table summary instance.
 */
extern const par_generated_info_t g_par_generated_info;

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* !defined(_PAR_GENERATED_INFO_H_) */
