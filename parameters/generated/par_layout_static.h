/*
 * DO NOT EDIT.
 * Generated from parameters/schema/par_table.csv by parameters/tools/pargen.py.
 */
/**
 * @file par_layout_static.h
 * @brief Declare generated static parameter layout tables.
 */

#ifndef _PAR_LAYOUT_STATIC_H_
#define _PAR_LAYOUT_STATIC_H_

#include <stdint.h>
#include "par_cfg.h"
#include "par_def.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/**
 * @brief Generated row-enable flags used by conditional layout expressions.
 */
#define PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_U8_RW      (1u)
#define PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_U8_RO      (1u)
#define PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_U16_RW     (1u)
#define PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_U32_RW     (1u)
#define PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_I16_RW     (1u)
#define PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_I32_RW     (1u)
#define PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_F32_RW     (1u)
#define PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_STR_RW     (1u)
#define PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_BYTES_RW   (1u)
#define PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_ARR_U8_RW  (1u)
#define PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_ARR_U16_RW (1u)
#define PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_ARR_U32_RW (1u)
#if defined(AUTOGEN_PM_TEST_USING_AT24CXX)
#define PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_AT24_U16_RW (1u)
#else
#define PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_AT24_U16_RW (0u)
#endif /* defined(AUTOGEN_PM_TEST_USING_AT24CXX) */

#define PAR_LAYOUT_STATIC_COUNT8         (2u)
#define PAR_LAYOUT_STATIC_COUNT16        (2u + (PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_AT24_U16_RW))
#define PAR_LAYOUT_STATIC_COUNT32        (3u)
#define PAR_LAYOUT_STATIC_COUNTOBJ       (5u)
#define PAR_LAYOUT_STATIC_OBJ_POOL_BYTES (30u)

/**
 * @brief Generated compiled enum-index helpers.
 */
#define PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_U8_RW       (0u)
#define PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_U8_RO       (PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_U8_RW + PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_U8_RW)
#define PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_U16_RW      (PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_U8_RO + PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_U8_RO)
#define PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_U32_RW      (PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_U16_RW + PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_U16_RW)
#define PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_I16_RW      (PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_U32_RW + PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_U32_RW)
#define PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_I32_RW      (PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_I16_RW + PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_I16_RW)
#define PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_F32_RW      (PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_I32_RW + PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_I32_RW)
#define PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_STR_RW      (PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_F32_RW + PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_F32_RW)
#define PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_BYTES_RW    (PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_STR_RW + PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_STR_RW)
#define PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_ARR_U8_RW   (PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_BYTES_RW + PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_BYTES_RW)
#define PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_ARR_U16_RW  (PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_ARR_U8_RW + PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_ARR_U8_RW)
#define PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_ARR_U32_RW  (PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_ARR_U16_RW + PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_ARR_U16_RW)
#define PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_AT24_U16_RW (PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_ARR_U32_RW + PAR_LAYOUT_ROW_ENABLED_ePAR_TEST_ARR_U32_RW)

/**
 * @brief Generated layout signature terms using compiled enum indexes.
 */
#define PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_U8_RW      ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_U8_RW) + 1u) * ((uint32_t)1u + 1u) * 257u) + ((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_U8_RW) + 1u) * (((uint32_t)0u % 257u) + 1u)) * 17u) + ((uint32_t)0u * 5u) + (((uint32_t)0u + 1u) * 31u)) % 65521u))
#define PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_U8_RO      ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_U8_RO) + 1u) * ((uint32_t)1u + 1u) * 257u) + ((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_U8_RO) + 1u) * (((uint32_t)1u % 257u) + 1u)) * 17u) + ((uint32_t)1u * 5u) + (((uint32_t)0u + 1u) * 31u)) % 65521u))
#define PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_U16_RW     ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_U16_RW) + 1u) * ((uint32_t)3u + 1u) * 257u) + ((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_U16_RW) + 1u) * (((uint32_t)2u % 257u) + 1u)) * 17u) + ((uint32_t)2u * 5u) + (((uint32_t)0u + 1u) * 31u)) % 65521u))
#define PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_U32_RW     ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_U32_RW) + 1u) * ((uint32_t)5u + 1u) * 257u) + ((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_U32_RW) + 1u) * (((uint32_t)3u % 257u) + 1u)) * 17u) + ((uint32_t)3u * 5u) + (((uint32_t)0u + 1u) * 31u)) % 65521u))
#define PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_I16_RW     ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_I16_RW) + 1u) * ((uint32_t)4u + 1u) * 257u) + ((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_I16_RW) + 1u) * (((uint32_t)4u % 257u) + 1u)) * 17u) + ((uint32_t)4u * 5u) + (((uint32_t)0u + 1u) * 31u)) % 65521u))
#define PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_I32_RW     ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_I32_RW) + 1u) * ((uint32_t)6u + 1u) * 257u) + ((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_I32_RW) + 1u) * (((uint32_t)5u % 257u) + 1u)) * 17u) + ((uint32_t)5u * 5u) + (((uint32_t)0u + 1u) * 31u)) % 65521u))
#define PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_F32_RW     ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_F32_RW) + 1u) * ((uint32_t)7u + 1u) * 257u) + ((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_F32_RW) + 1u) * (((uint32_t)6u % 257u) + 1u)) * 17u) + ((uint32_t)6u * 5u) + (((uint32_t)0u + 1u) * 31u)) % 65521u))
#define PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_STR_RW     ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_STR_RW) + 1u) * ((uint32_t)8u + 1u) * 257u) + ((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_STR_RW) + 1u) * (((uint32_t)7u % 257u) + 1u)) * 17u) + ((uint32_t)7u * 5u) + (((uint32_t)8u + 1u) * 31u)) % 65521u))
#define PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_BYTES_RW   ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_BYTES_RW) + 1u) * ((uint32_t)9u + 1u) * 257u) + ((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_BYTES_RW) + 1u) * (((uint32_t)8u % 257u) + 1u)) * 17u) + ((uint32_t)8u * 5u) + (((uint32_t)4u + 1u) * 31u)) % 65521u))
#define PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_ARR_U8_RW  ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_ARR_U8_RW) + 1u) * ((uint32_t)10u + 1u) * 257u) + ((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_ARR_U8_RW) + 1u) * (((uint32_t)9u % 257u) + 1u)) * 17u) + ((uint32_t)9u * 5u) + (((uint32_t)4u + 1u) * 31u)) % 65521u))
#define PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_ARR_U16_RW ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_ARR_U16_RW) + 1u) * ((uint32_t)11u + 1u) * 257u) + ((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_ARR_U16_RW) + 1u) * (((uint32_t)10u % 257u) + 1u)) * 17u) + ((uint32_t)10u * 5u) + (((uint32_t)6u + 1u) * 31u)) % 65521u))
#define PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_ARR_U32_RW ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_ARR_U32_RW) + 1u) * ((uint32_t)12u + 1u) * 257u) + ((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_ARR_U32_RW) + 1u) * (((uint32_t)12u % 257u) + 1u)) * 17u) + ((uint32_t)12u * 5u) + (((uint32_t)8u + 1u) * 31u)) % 65521u))
#if defined(AUTOGEN_PM_TEST_USING_AT24CXX)
#define PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_AT24_U16_RW ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_AT24_U16_RW) + 1u) * ((uint32_t)3u + 1u) * 257u) + ((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_AT24_U16_RW) + 1u) * (((uint32_t)477u % 257u) + 1u)) * 17u) + ((uint32_t)477u * 5u) + (((uint32_t)0u + 1u) * 31u)) % 65521u))
#else
#define PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_AT24_U16_RW (0u)
#endif /* defined(AUTOGEN_PM_TEST_USING_AT24CXX) */
#define PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_U8_RW      ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_U8_RW) + 1u) * (((uint32_t)0u % 257u) + 1u)) + ((uint32_t)0u * 3u) + ((uint32_t)1u * 389u) + (((uint32_t)0u + 1u) * 13u)) % 65521u))
#define PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_U8_RO      ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_U8_RO) + 1u) * (((uint32_t)1u % 257u) + 1u)) + ((uint32_t)1u * 3u) + ((uint32_t)1u * 389u) + (((uint32_t)0u + 1u) * 13u)) % 65521u))
#define PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_U16_RW     ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_U16_RW) + 1u) * (((uint32_t)2u % 257u) + 1u)) + ((uint32_t)2u * 3u) + ((uint32_t)3u * 389u) + (((uint32_t)0u + 1u) * 13u)) % 65521u))
#define PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_U32_RW     ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_U32_RW) + 1u) * (((uint32_t)3u % 257u) + 1u)) + ((uint32_t)3u * 3u) + ((uint32_t)5u * 389u) + (((uint32_t)0u + 1u) * 13u)) % 65521u))
#define PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_I16_RW     ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_I16_RW) + 1u) * (((uint32_t)4u % 257u) + 1u)) + ((uint32_t)4u * 3u) + ((uint32_t)4u * 389u) + (((uint32_t)0u + 1u) * 13u)) % 65521u))
#define PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_I32_RW     ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_I32_RW) + 1u) * (((uint32_t)5u % 257u) + 1u)) + ((uint32_t)5u * 3u) + ((uint32_t)6u * 389u) + (((uint32_t)0u + 1u) * 13u)) % 65521u))
#define PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_F32_RW     ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_F32_RW) + 1u) * (((uint32_t)6u % 257u) + 1u)) + ((uint32_t)6u * 3u) + ((uint32_t)7u * 389u) + (((uint32_t)0u + 1u) * 13u)) % 65521u))
#define PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_STR_RW     ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_STR_RW) + 1u) * (((uint32_t)7u % 257u) + 1u)) + ((uint32_t)7u * 3u) + ((uint32_t)8u * 389u) + (((uint32_t)8u + 1u) * 13u)) % 65521u))
#define PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_BYTES_RW   ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_BYTES_RW) + 1u) * (((uint32_t)8u % 257u) + 1u)) + ((uint32_t)8u * 3u) + ((uint32_t)9u * 389u) + (((uint32_t)4u + 1u) * 13u)) % 65521u))
#define PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_ARR_U8_RW  ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_ARR_U8_RW) + 1u) * (((uint32_t)9u % 257u) + 1u)) + ((uint32_t)9u * 3u) + ((uint32_t)10u * 389u) + (((uint32_t)4u + 1u) * 13u)) % 65521u))
#define PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_ARR_U16_RW ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_ARR_U16_RW) + 1u) * (((uint32_t)10u % 257u) + 1u)) + ((uint32_t)10u * 3u) + ((uint32_t)11u * 389u) + (((uint32_t)6u + 1u) * 13u)) % 65521u))
#define PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_ARR_U32_RW ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_ARR_U32_RW) + 1u) * (((uint32_t)12u % 257u) + 1u)) + ((uint32_t)12u * 3u) + ((uint32_t)12u * 389u) + (((uint32_t)8u + 1u) * 13u)) % 65521u))
#if defined(AUTOGEN_PM_TEST_USING_AT24CXX)
#define PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_AT24_U16_RW ((((((uint32_t)(PAR_LAYOUT_STATIC_INDEX_ePAR_TEST_AT24_U16_RW) + 1u) * (((uint32_t)477u % 257u) + 1u)) + ((uint32_t)477u * 3u) + ((uint32_t)3u * 389u) + (((uint32_t)0u + 1u) * 13u)) % 65521u))
#else
#define PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_AT24_U16_RW (0u)
#endif /* defined(AUTOGEN_PM_TEST_USING_AT24CXX) */

/**
 * @brief Generated layout signature used to detect stale static tables.
 */
#define PAR_LAYOUT_STATIC_SIGNATURE_A_CHUNK_0 ((0u + (PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_U8_RW) + (PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_U8_RO) + (PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_U16_RW) + (PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_U32_RW) + (PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_I16_RW) + (PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_I32_RW) + (PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_F32_RW) + (PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_STR_RW)) % 65521u)
#define PAR_LAYOUT_STATIC_SIGNATURE_A_CHUNK_1 ((0u + (PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_BYTES_RW) + (PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_ARR_U8_RW) + (PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_ARR_U16_RW) + (PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_ARR_U32_RW) + (PAR_LAYOUT_STATIC_SIGNATURE_A_TERM_ePAR_TEST_AT24_U16_RW)) % 65521u)
#define PAR_LAYOUT_STATIC_SIGNATURE_A ((0u + (PAR_LAYOUT_STATIC_SIGNATURE_A_CHUNK_0) + (PAR_LAYOUT_STATIC_SIGNATURE_A_CHUNK_1)) % 65521u)
#define PAR_LAYOUT_STATIC_SIGNATURE_B_CHUNK_0 ((0u + (PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_U8_RW) + (PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_U8_RO) + (PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_U16_RW) + (PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_U32_RW) + (PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_I16_RW) + (PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_I32_RW) + (PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_F32_RW) + (PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_STR_RW)) % 65521u)
#define PAR_LAYOUT_STATIC_SIGNATURE_B_CHUNK_1 ((0u + (PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_BYTES_RW) + (PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_ARR_U8_RW) + (PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_ARR_U16_RW) + (PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_ARR_U32_RW) + (PAR_LAYOUT_STATIC_SIGNATURE_B_TERM_ePAR_TEST_AT24_U16_RW)) % 65521u)
#define PAR_LAYOUT_STATIC_SIGNATURE_B ((0u + (PAR_LAYOUT_STATIC_SIGNATURE_B_CHUNK_0) + (PAR_LAYOUT_STATIC_SIGNATURE_B_CHUNK_1)) % 65521u)
#define PAR_LAYOUT_STATIC_SIGNATURE   (((uint32_t)PAR_LAYOUT_STATIC_SIGNATURE_A << 16u) ^ (uint32_t)PAR_LAYOUT_STATIC_SIGNATURE_B)

/**
 * @brief Generated static offset macros used for table freshness checks.
 */
#define PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_U8_RW       (0u)
#define PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_U8_RO       (1u)
#define PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_U16_RW      (0u)
#define PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_U32_RW      (0u)
#define PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_I16_RW      (1u)
#define PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_I32_RW      (1u)
#define PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_F32_RW      (2u)
#define PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_STR_RW             (0u)
#define PAR_LAYOUT_STATIC_OBJECT_POOL_OFFSET_ePAR_TEST_STR_RW (0u)
#define PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_BYTES_RW             (1u)
#define PAR_LAYOUT_STATIC_OBJECT_POOL_OFFSET_ePAR_TEST_BYTES_RW (8u)
#define PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_ARR_U8_RW             (2u)
#define PAR_LAYOUT_STATIC_OBJECT_POOL_OFFSET_ePAR_TEST_ARR_U8_RW (12u)
#define PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_ARR_U16_RW             (3u)
#define PAR_LAYOUT_STATIC_OBJECT_POOL_OFFSET_ePAR_TEST_ARR_U16_RW (16u)
#define PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_ARR_U32_RW             (4u)
#define PAR_LAYOUT_STATIC_OBJECT_POOL_OFFSET_ePAR_TEST_ARR_U32_RW (22u)
#if defined(AUTOGEN_PM_TEST_USING_AT24CXX)
#define PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_AT24_U16_RW (2u)
#endif /* defined(AUTOGEN_PM_TEST_USING_AT24CXX) */

/**
 * @brief Static scalar/object slot offset table indexed by par_num_t.
 */
extern const uint16_t g_par_layout_static_offset[ePAR_NUM_OF];
#define PAR_LAYOUT_STATIC_OFFSET_TABLE (g_par_layout_static_offset)

/**
 * @brief Static object-pool byte offset table indexed by par_num_t.
 */
extern const uint32_t g_par_layout_static_object_pool_offset[ePAR_NUM_OF];
#define PAR_LAYOUT_STATIC_OBJECT_POOL_OFFSET_TABLE (g_par_layout_static_object_pool_offset)

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* !defined(_PAR_LAYOUT_STATIC_H_) */
