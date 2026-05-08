/**
 * @file par_nvm_object_addr_fixed.c
 * @brief Resolve fixed-address shared object persistence placement.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-05-01
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include "par_nvm_object.h"

#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_OBJECT_EN) && \
    (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) && \
    (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_FIXED)

#include "par_if.h"

/**
 * @brief Cached first address after the scalar NVM block.
 */
static uint32_t gu32_par_nvm_scalar_block_end_addr = 0U;

/**
 * @brief Validity flag for gu32_par_nvm_scalar_block_end_addr.
 */
static bool gb_par_nvm_scalar_block_end_addr_valid = false;

/**
 * @brief Cached base address of the object persistence block.
 */
static uint32_t gu32_par_nvm_object_block_addr = 0U;

/**
 * @brief Validity flag for gu32_par_nvm_object_block_addr.
 */
static bool gb_par_nvm_object_block_addr_valid = false;

/**
 * @brief Invalidate cached object and scalar-boundary addresses.
 */
void par_nvm_object_invalidate_block_addr_cache(void)
{
    gu32_par_nvm_scalar_block_end_addr = 0U;
    gb_par_nvm_scalar_block_end_addr_valid = false;
    gu32_par_nvm_object_block_addr = 0U;
    gb_par_nvm_object_block_addr_valid = false;
}

/**
 * @brief Return the first address after the scalar persisted-record block.
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
    if (false == gb_par_nvm_scalar_block_end_addr_valid)
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

        gu32_par_nvm_scalar_block_end_addr = addr;
        gb_par_nvm_scalar_block_end_addr_valid = true;
    }

    return gu32_par_nvm_scalar_block_end_addr;
}

/**
 * @brief Return the object persistence block base address.
 *
 * @param scalar_first_record_addr First scalar persisted-record address.
 * @param p_layout Active scalar record-layout adapter; optional when no scalar slot exists.
 * @param p_scalar_slot_to_par_num Scalar persistent-slot map; optional when no scalar slot exists.
 * @return Object persistence block base address, or zero on error.
 */
uint32_t par_nvm_object_get_block_addr(const uint32_t scalar_first_record_addr,
                                       const par_nvm_layout_api_t * const p_layout,
                                       const par_num_t * const p_scalar_slot_to_par_num)
{
    if (0U == par_nvm_object_get_count())
    {
        return 0U;
    }

    if (false == gb_par_nvm_object_block_addr_valid)
    {
        const uint32_t scalar_block_end_addr =
            par_nvm_object_get_scalar_block_end_addr(scalar_first_record_addr, p_layout, p_scalar_slot_to_par_num);
        const uint32_t object_block_size = par_nvm_object_get_block_size();
        const uint32_t object_last_offset = object_block_size - 1U;

        if (0U == scalar_block_end_addr)
        {
            return 0U;
        }
        if (PAR_CFG_NVM_OBJECT_FIXED_ADDR < scalar_block_end_addr)
        {
            PAR_ERR_PRINT("PAR_NVM: fixed object block overlaps scalar block, object=0x%08lX scalar_end=0x%08lX",
                          (unsigned long)PAR_CFG_NVM_OBJECT_FIXED_ADDR,
                          (unsigned long)scalar_block_end_addr);
            return 0U;
        }
        if (PAR_CFG_NVM_OBJECT_FIXED_ADDR > (UINT32_MAX - object_last_offset))
        {
            PAR_ERR_PRINT("PAR_NVM: fixed object block address overflow, object=0x%08lX size=%lu",
                          (unsigned long)PAR_CFG_NVM_OBJECT_FIXED_ADDR,
                          (unsigned long)object_block_size);
            return 0U;
        }
        if ((PAR_CFG_NVM_OBJECT_REGION_SIZE > 0U) &&
            (object_block_size > PAR_CFG_NVM_OBJECT_REGION_SIZE))
        {
            PAR_ERR_PRINT("PAR_NVM: object block exceeds fixed region, size=%lu region=%lu",
                          (unsigned long)object_block_size,
                          (unsigned long)PAR_CFG_NVM_OBJECT_REGION_SIZE);
            return 0U;
        }

        gu32_par_nvm_object_block_addr = PAR_CFG_NVM_OBJECT_FIXED_ADDR;
        gb_par_nvm_object_block_addr_valid = true;
    }

    return gu32_par_nvm_object_block_addr;
}

/**
 * @brief Return whether address zero is a valid object block base.
 *
 * @return false because shared scalar-backend placement uses zero as an error sentinel.
 */
bool par_nvm_object_block_addr_zero_is_valid(void)
{
    return false;
}

#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) && (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_FIXED) */
