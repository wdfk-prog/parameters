/**
 * @file par_layout.c
 * @brief Implement parameter storage layout helpers.
 * @author wdfk-prog ()
 * @version 1.0
 * @date 2026-03-27
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author      Description
 * 2026-03-27 1.0     wdfk-prog   first version
 */

/**
 * @addtogroup PAR_LAYOUT
 * @{ <!-- BEGIN GROUP -->
 */
/**
 * @brief Include dependencies.
 */
#include "layout/par_layout.h"
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
};
/**
 * @brief Runtime-generated offset table.
 * @note Offsets are generated with a plain runtime scan loop instead of macro. expansion to keep the mapping logic readable and easy to debug.
 * @note When PAR_CFG_LAYOUT_SOURCE is PAR_CFG_LAYOUT_SCRIPT, runtime fill is. compile-time disabled and PAR_LAYOUT_STATIC_OFFSET_TABLE is consumed. directly, avoiding extra maintenance complexity.
 */
static uint16_t gs_runtime_offset[ePAR_NUM_OF] = { 0u };
static const uint16_t *gsp_active_offset = gs_runtime_offset;

#if (PAR_CFG_LAYOUT_SOURCE == PAR_CFG_LAYOUT_SCRIPT)
#ifndef PAR_LAYOUT_STATIC_OFFSET_TABLE
#error "PAR_LAYOUT_STATIC_OFFSET_TABLE must be provided by static layout include!"
#endif
#endif
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
    par_layout_count_t scan_count = { 0u, 0u, 0u };
    gsp_active_offset = gs_runtime_offset;

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
#endif
            gs_runtime_offset[par_it] = scan_count.count32;
            scan_count.count32++;
            break;

        case ePAR_TYPE_NUM_OF:
        default:
            PAR_ERR_PRINT("PAR: layout encountered unsupported type at par_num=%u", (unsigned)par_it);
            PAR_ASSERT(0);
            return;
        }
    }

    if ((scan_count.count8 != gs_layout_count.count8) || (scan_count.count16 != gs_layout_count.count16) || (scan_count.count32 != gs_layout_count.count32))
    {
        PAR_ERR_PRINT("PAR: layout count mismatch, scan=(%u,%u,%u) cfg=(%u,%u,%u)",
                      (unsigned)scan_count.count8,
                      (unsigned)scan_count.count16,
                      (unsigned)scan_count.count32,
                      (unsigned)gs_layout_count.count8,
                      (unsigned)gs_layout_count.count16,
                      (unsigned)gs_layout_count.count32);
        PAR_ASSERT(0);
        return;
    }
#else
    /* Script layout mode: consume provided static layout directly with no runtime validation. */
    gsp_active_offset = PAR_LAYOUT_STATIC_OFFSET_TABLE;
    PAR_DBG_PRINT("PAR: layout initialized from generated static table");
    return;
#endif
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
 * @brief Get per-width layout counts.
 *
 * @return Number of 8/16/32-bit parameters.
 */
par_layout_count_t par_layout_get_count(void)
{
    return gs_layout_count;
}
/**
 * @} <!-- END GROUP -->
 */
