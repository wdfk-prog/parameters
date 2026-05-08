/**
 * @file par_layout.c
 * @brief Implement parameter storage layout helpers.
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
 * 2026-04-22 1.1     wdfk-prog     add object array types
 */
/**
 * @addtogroup PAR_LAYOUT
 * @{ <!-- BEGIN GROUP -->
 */
/**
 * @brief Include dependencies.
 */
#include "par_layout.h"
#include "par.h"
/**
 * @brief Compile-time definitions.
 */
PAR_STATIC_ASSERT(par_layout_max_par_num_fit_u16, (ePAR_NUM_OF <= PAR_UINT16_MAX));
PAR_STATIC_ASSERT(par_layout_compile_count_sum_match, ((uint32_t)PAR_LAYOUT_COMPILE_COUNT_SUM == (uint32_t)ePAR_NUM_OF));

static const par_layout_count_t gs_layout_count = {
    .count8 = (uint16_t)PAR_STORAGE_COUNT8,
    .count16 = (uint16_t)PAR_STORAGE_COUNT16,
    .count32 = (uint16_t)PAR_STORAGE_COUNT32,
    .count_obj = (uint16_t)PAR_STORAGE_COUNTOBJ,
    .obj_pool_bytes = (uint32_t)PAR_STORAGE_OBJ_POOL_BYTES,
};
/**
 * @brief Runtime-generated offset table.
 * @note Compile-scan mode fills the runtime tables with one linear pass.
 * @note Script layout mode consumes the generated static tables directly and
 * requires object-specific static fields only when object rows are compiled in.
 */
static uint16_t gs_runtime_offset[ePAR_NUM_OF] = { 0u };
/**
 * @brief Runtime object-pool offsets indexed by parameter number.
 * @details For STR/BYTES/ARR_* parameters this array stores the start byte offset of
 * the parameter payload inside the shared object pool. Scalar parameters keep a
 * zero value here because they do not consume the object pool.
 */
static uint32_t gs_runtime_obj_pool_offset[ePAR_NUM_OF] = { 0u };
static const uint16_t *gsp_active_offset = gs_runtime_offset;
static const uint32_t *gsp_active_obj_pool_offset = gs_runtime_obj_pool_offset;

#if (PAR_CFG_LAYOUT_SOURCE == PAR_CFG_LAYOUT_SCRIPT)
#ifndef PAR_LAYOUT_STATIC_OFFSET_TABLE
#error "PAR_LAYOUT_STATIC_OFFSET_TABLE must be provided by static layout include!"
#endif /* !defined(PAR_LAYOUT_STATIC_OFFSET_TABLE) */
#if (PAR_LAYOUT_COMPILE_COUNTOBJ > 0u)
#ifndef PAR_LAYOUT_STATIC_OBJECT_POOL_OFFSET_TABLE
#error "PAR_LAYOUT_STATIC_OBJECT_POOL_OFFSET_TABLE must be provided when object rows are present!"
#endif /* !defined(PAR_LAYOUT_STATIC_OBJECT_POOL_OFFSET_TABLE) */
#endif /* (PAR_LAYOUT_COMPILE_COUNTOBJ > 0u) */
#endif /* (PAR_CFG_LAYOUT_SOURCE == PAR_CFG_LAYOUT_SCRIPT) */
/**
 * @brief Function declarations and definitions.
 */
/**
 * @brief Initialize active parameter layout source.
 *
 * @note COMPILE_SCAN mode fills runtime offset table.
 * SCRIPT mode directly consumes static script table.
 */
void par_layout_init(void)
{
#if (PAR_CFG_LAYOUT_SOURCE == PAR_CFG_LAYOUT_COMPILE_SCAN)
    par_layout_count_t scan_count = { 0u, 0u, 0u, 0u, 0u };
    gsp_active_offset = gs_runtime_offset;
    gsp_active_obj_pool_offset = gs_runtime_obj_pool_offset;

    for (uint32_t par_it = 0u; par_it < (uint32_t)ePAR_NUM_OF; par_it++)
    {
        const par_cfg_t * const p_cfg = par_cfg_get((par_num_t)par_it);
        switch (p_cfg->type)
        {
        case ePAR_TYPE_U8:
        case ePAR_TYPE_I8:
            gs_runtime_offset[par_it] = scan_count.count8;
            scan_count.count8++;
            break;

        case ePAR_TYPE_U16:
        case ePAR_TYPE_I16:
            gs_runtime_offset[par_it] = scan_count.count16;
            scan_count.count16++;
            break;

        case ePAR_TYPE_U32:
        case ePAR_TYPE_I32:
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
        case ePAR_TYPE_F32:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */
            gs_runtime_offset[par_it] = scan_count.count32;
            scan_count.count32++;
            break;

#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
        case ePAR_TYPE_STR:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
        case ePAR_TYPE_BYTES:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
        case ePAR_TYPE_ARR_U8:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
        case ePAR_TYPE_ARR_U16:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
        case ePAR_TYPE_ARR_U32:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */
            gs_runtime_offset[par_it] = scan_count.count_obj;
            gs_runtime_obj_pool_offset[par_it] = scan_count.obj_pool_bytes;
            scan_count.count_obj++;
            scan_count.obj_pool_bytes += (uint32_t)p_cfg->value_cfg.object.range.max_len;
            break;
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

        case ePAR_TYPE_NUM_OF:
        default:
            PAR_ERR_PRINT("PAR: layout encountered unsupported type at par_num=%u", (unsigned)par_it);
            PAR_ASSERT(0);
            return;
        }
    }

    if ((scan_count.count8 != gs_layout_count.count8) || (scan_count.count16 != gs_layout_count.count16) || (scan_count.count32 != gs_layout_count.count32) || (scan_count.count_obj != gs_layout_count.count_obj) || (scan_count.obj_pool_bytes != gs_layout_count.obj_pool_bytes))
    {
        PAR_ERR_PRINT("PAR: layout count mismatch, scan=(%u,%u,%u,%u,%lu) cfg=(%u,%u,%u,%u,%lu)",
                      (unsigned)scan_count.count8,
                      (unsigned)scan_count.count16,
                      (unsigned)scan_count.count32,
                      (unsigned)scan_count.count_obj,
                      (unsigned long)scan_count.obj_pool_bytes,
                      (unsigned)gs_layout_count.count8,
                      (unsigned)gs_layout_count.count16,
                      (unsigned)gs_layout_count.count32,
                      (unsigned)gs_layout_count.count_obj,
                      (unsigned long)gs_layout_count.obj_pool_bytes);
        PAR_ASSERT(0);
        return;
    }
#else
    gsp_active_offset = PAR_LAYOUT_STATIC_OFFSET_TABLE;
#if (PAR_STORAGE_COUNTOBJ > 0u)
    gsp_active_obj_pool_offset = PAR_LAYOUT_STATIC_OBJECT_POOL_OFFSET_TABLE;
#else
    gsp_active_obj_pool_offset = gs_runtime_obj_pool_offset;
#endif /* (PAR_STORAGE_COUNTOBJ > 0u) */
    PAR_DBG_PRINT("PAR: layout initialized from generated static tables");
    return;
#endif /* (PAR_CFG_LAYOUT_SOURCE == PAR_CFG_LAYOUT_COMPILE_SCAN) */
}
/**
 * @brief Get active offset table pointer.
 *
 * @return Pointer to active offset table.
 */
const uint16_t *par_layout_get_offset_table(void)
{
    return gsp_active_offset;
}
/**
 * @brief Get offset by parameter number.
 *
 * @param par_num Parameter number (enumeration).
 * @return Offset inside type group storage.
 */
uint16_t par_layout_get_offset(const par_num_t par_num)
{
    return gsp_active_offset[par_num];
}

/**
 * @brief Return the object-pool byte offset for one object parameter.
 * @param par_num Parameter number.
 * @return Byte offset inside the shared object payload pool.
 */
uint32_t par_layout_get_obj_pool_offset(const par_num_t par_num)
{
    return gsp_active_obj_pool_offset[par_num];
}
/**
 * @brief Return the active scalar-group and object-pool counts.
 *
 * @return Active layout counters for scalar groups and object storage.
 */
par_layout_count_t par_layout_get_count(void)
{
    return gs_layout_count;
}
/**
 * @} <!-- END GROUP -->
 */
