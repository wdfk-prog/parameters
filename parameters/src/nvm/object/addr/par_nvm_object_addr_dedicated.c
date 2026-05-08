/**
 * @file par_nvm_object_addr_dedicated.c
 * @brief Resolve dedicated object persistence backend addresses.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-05-01
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include "par_nvm_object.h"

#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_OBJECT_EN) && \
    (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED)

#include "par_if.h"

/**
 * @brief Cached base address of the object persistence block.
 */
static uint32_t gu32_par_nvm_object_block_addr = 0U;

/**
 * @brief Validity flag for gu32_par_nvm_object_block_addr.
 */
static bool gb_par_nvm_object_block_addr_valid = false;

/**
 * @brief Invalidate the cached dedicated object block base address.
 */
void par_nvm_object_invalidate_block_addr_cache(void)
{
    gu32_par_nvm_object_block_addr = 0U;
    gb_par_nvm_object_block_addr_valid = false;
}

/**
 * @brief Return the scalar block end address for diagnostics or shared helpers.
 *
 * @param scalar_first_record_addr First scalar persisted-record address.
 * @param p_layout Active scalar record-layout adapter; optional when no scalar slot exists.
 * @param p_scalar_slot_to_par_num Scalar persistent-slot map; optional when no scalar slot exists.
 * @return First address after scalar records, or zero when scalar address resolution fails.
 */
uint32_t par_nvm_object_get_scalar_block_end_addr(const uint32_t scalar_first_record_addr,
                                                  const par_nvm_layout_api_t * const p_layout,
                                                  const par_num_t * const p_scalar_slot_to_par_num)
{
    uint32_t addr = scalar_first_record_addr;

    if (0U < PAR_PERSISTENT_COMPILE_COUNT)
    {
        PAR_ASSERT((NULL != p_layout) && (NULL != p_layout->record_size_from_par_num));
        PAR_ASSERT(NULL != p_scalar_slot_to_par_num);
        if ((NULL == p_layout) || (NULL == p_layout->record_size_from_par_num) ||
            (NULL == p_scalar_slot_to_par_num))
        {
            return 0U;
        }
    }

    for (uint16_t persist_idx = 0U; persist_idx < PAR_PERSISTENT_COMPILE_COUNT; persist_idx++)
    {
        addr += p_layout->record_size_from_par_num(p_scalar_slot_to_par_num[persist_idx]);
    }

    return addr;
}

/**
 * @brief Return the dedicated object backend block base address.
 *
 * @param scalar_first_record_addr Unused scalar first-record address.
 * @param p_layout Unused scalar record-layout adapter.
 * @param p_scalar_slot_to_par_num Unused scalar persistent-slot map.
 * @return Dedicated object persistence block base address.
 */
uint32_t par_nvm_object_get_block_addr(const uint32_t scalar_first_record_addr,
                                       const par_nvm_layout_api_t * const p_layout,
                                       const par_num_t * const p_scalar_slot_to_par_num)
{
    (void)scalar_first_record_addr;
    (void)p_layout;
    (void)p_scalar_slot_to_par_num;

    if (0U == par_nvm_object_get_count())
    {
        return 0U;
    }

    if (false == gb_par_nvm_object_block_addr_valid)
    {
        gu32_par_nvm_object_block_addr = PAR_CFG_NVM_OBJECT_DEDICATED_BASE_ADDR;
        gb_par_nvm_object_block_addr_valid = true;
    }

    return gu32_par_nvm_object_block_addr;
}

/**
 * @brief Return whether address zero is a valid object block base.
 *
 * @return true because a dedicated object partition may start at offset zero.
 */
bool par_nvm_object_block_addr_zero_is_valid(void)
{
    return true;
}

#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED) */
