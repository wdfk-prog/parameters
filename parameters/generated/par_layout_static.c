/*
 * DO NOT EDIT.
 * Generated from parameters/schema/par_table.csv by parameters/tools/pargen.py.
 */
/**
 * @file par_layout_static.c
 * @brief Define generated static parameter layout tables.
 */

#include "par_layout_static.h"

const uint16_t g_par_layout_static_offset[ePAR_NUM_OF] = {
    [ePAR_TEST_U8_RW]       = (uint16_t)(PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_U8_RW      ),
    [ePAR_TEST_U8_RO]       = (uint16_t)(PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_U8_RO      ),
    [ePAR_TEST_U16_RW]      = (uint16_t)(PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_U16_RW     ),
    [ePAR_TEST_U32_RW]      = (uint16_t)(PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_U32_RW     ),
    [ePAR_TEST_I16_RW]      = (uint16_t)(PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_I16_RW     ),
    [ePAR_TEST_I32_RW]      = (uint16_t)(PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_I32_RW     ),
    [ePAR_TEST_F32_RW]      = (uint16_t)(PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_F32_RW     ),
    [ePAR_TEST_STR_RW]      = (uint16_t)(PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_STR_RW     ),
    [ePAR_TEST_BYTES_RW]    = (uint16_t)(PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_BYTES_RW   ),
    [ePAR_TEST_ARR_U8_RW]   = (uint16_t)(PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_ARR_U8_RW  ),
    [ePAR_TEST_ARR_U16_RW]  = (uint16_t)(PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_ARR_U16_RW ),
    [ePAR_TEST_ARR_U32_RW]  = (uint16_t)(PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_ARR_U32_RW ),
#if defined(AUTOGEN_PM_TEST_USING_AT24CXX)
    [ePAR_TEST_AT24_U16_RW] = (uint16_t)(PAR_LAYOUT_STATIC_OFFSET_ePAR_TEST_AT24_U16_RW),
#endif /* defined(AUTOGEN_PM_TEST_USING_AT24CXX) */
};

const uint32_t g_par_layout_static_object_pool_offset[ePAR_NUM_OF] = {
    0u, /* Keep the initializer valid when all object rows are compiled out. */
    [ePAR_TEST_STR_RW]     = (uint32_t)(PAR_LAYOUT_STATIC_OBJECT_POOL_OFFSET_ePAR_TEST_STR_RW    ),
    [ePAR_TEST_BYTES_RW]   = (uint32_t)(PAR_LAYOUT_STATIC_OBJECT_POOL_OFFSET_ePAR_TEST_BYTES_RW  ),
    [ePAR_TEST_ARR_U8_RW]  = (uint32_t)(PAR_LAYOUT_STATIC_OBJECT_POOL_OFFSET_ePAR_TEST_ARR_U8_RW ),
    [ePAR_TEST_ARR_U16_RW] = (uint32_t)(PAR_LAYOUT_STATIC_OBJECT_POOL_OFFSET_ePAR_TEST_ARR_U16_RW),
    [ePAR_TEST_ARR_U32_RW] = (uint32_t)(PAR_LAYOUT_STATIC_OBJECT_POOL_OFFSET_ePAR_TEST_ARR_U32_RW),
};
