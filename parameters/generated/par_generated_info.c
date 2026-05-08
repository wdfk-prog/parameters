/*
 * DO NOT EDIT.
 * Generated from parameters/schema/par_table.csv by parameters/tools/pargen.py.
 */
/**
 * @file par_generated_info.c
 * @brief Define generated parameter-table summary metadata.
 */

#include "par_generated_info.h"
#include "par_id_map_static.h"

const par_generated_info_t g_par_generated_info = {
    .param_count = (uint16_t)ePAR_NUM_OF,
    .count8 = (uint16_t)PAR_LAYOUT_STATIC_COUNT8,
    .count16 = (uint16_t)PAR_LAYOUT_STATIC_COUNT16,
    .count32 = (uint16_t)PAR_LAYOUT_STATIC_COUNT32,
    .count_obj = (uint16_t)PAR_LAYOUT_STATIC_COUNTOBJ,
    .obj_pool_bytes = (uint32_t)PAR_LAYOUT_STATIC_OBJ_POOL_BYTES,
#if (1 == PAR_CFG_ENABLE_ID)
    .id_hash_bits = (uint32_t)PAR_ID_HASH_BITS,
    .id_hash_size = (uint32_t)PAR_ID_HASH_SIZE,
#else
    .id_hash_bits = 0u,
    .id_hash_size = 0u,
#endif /* (1 == PAR_CFG_ENABLE_ID) */
};
