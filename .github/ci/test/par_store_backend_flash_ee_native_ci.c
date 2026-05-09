/**
 * @file par_store_backend_flash_ee_native_ci.c
 * @brief Provide a RAM-backed native flash-ee port for CI compile coverage.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#include <string.h>

#include "par_cfg.h"
#include "par_store_backend_flash_ee.h"

#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN) && \
    (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_NATIVE_EN)

/**
 * @brief Erased byte value used by the CI RAM flash model.
 */
#define PAR_STORE_FLASH_EE_CI_ERASE_VALUE ((uint8_t)0xFFU)

/**
 * @brief CI backing-region multiplier for two banks plus append headroom.
 *
 * @details Each bank must hold the header, all logical lines, and at least
 * one extra append record. Four logical regions provide that capacity with
 * the CI geometry used by the package compile profile.
 */
#define PAR_STORE_FLASH_EE_CI_REGION_MULTIPLIER (4U)

/**
 * @brief RAM-backed flash region used only by CI package builds.
 */
static uint8_t g_par_store_flash_ee_ci_region[PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE *
                                              PAR_STORE_FLASH_EE_CI_REGION_MULTIPLIER];

/**
 * @brief Check whether one CI flash operation fits inside the backing region.
 * @param addr Byte address inside the CI flash region.
 * @param size Number of bytes requested.
 * @return true when the requested range is valid, otherwise false.
 */
static bool par_store_flash_ee_ci_range_is_valid(const uint32_t addr, const uint32_t size)
{
    const uint32_t region_size = (uint32_t)sizeof(g_par_store_flash_ee_ci_region);

    return (addr <= region_size) && (size <= (region_size - addr));
}

/**
 * @brief Initialize the CI native flash region.
 * @return ePAR_OK on success.
 */
par_status_t par_store_flash_ee_native_port_init(void)
{
    memset(g_par_store_flash_ee_ci_region,
           PAR_STORE_FLASH_EE_CI_ERASE_VALUE,
           sizeof(g_par_store_flash_ee_ci_region));
    return ePAR_OK;
}

/**
 * @brief Deinitialize the CI native flash region.
 * @return ePAR_OK on success.
 */
par_status_t par_store_flash_ee_native_port_deinit(void)
{
    return ePAR_OK;
}

/**
 * @brief Read bytes from the CI native flash region.
 * @param addr Byte address inside the CI flash region.
 * @param size Number of bytes to read.
 * @param p_buf Destination buffer.
 * @return ePAR_OK on success, otherwise ePAR_ERROR_PARAM.
 */
par_status_t par_store_flash_ee_native_port_read(uint32_t addr,
                                                uint32_t size,
                                                uint8_t *p_buf)
{
    if ((NULL == p_buf) || (false == par_store_flash_ee_ci_range_is_valid(addr, size)))
    {
        return ePAR_ERROR_PARAM;
    }

    memcpy(p_buf, &g_par_store_flash_ee_ci_region[addr], size);
    return ePAR_OK;
}

/**
 * @brief Program bytes into the CI native flash region.
 * @param addr Byte address inside the CI flash region.
 * @param size Number of bytes to program.
 * @param p_buf Source buffer.
 * @return ePAR_OK on success, otherwise ePAR_ERROR_PARAM.
 */
par_status_t par_store_flash_ee_native_port_program(uint32_t addr,
                                                   uint32_t size,
                                                   const uint8_t *p_buf)
{
    if ((NULL == p_buf) || (false == par_store_flash_ee_ci_range_is_valid(addr, size)))
    {
        return ePAR_ERROR_PARAM;
    }

    for (uint32_t i = 0U; i < size; i++)
    {
        g_par_store_flash_ee_ci_region[addr + i] &= p_buf[i];
    }

    return ePAR_OK;
}

/**
 * @brief Erase bytes inside the CI native flash region.
 * @param addr Byte address inside the CI flash region.
 * @param size Number of bytes to erase.
 * @return ePAR_OK on success, otherwise ePAR_ERROR_PARAM.
 */
par_status_t par_store_flash_ee_native_port_erase(uint32_t addr, uint32_t size)
{
    if (false == par_store_flash_ee_ci_range_is_valid(addr, size))
    {
        return ePAR_ERROR_PARAM;
    }

    memset(&g_par_store_flash_ee_ci_region[addr], PAR_STORE_FLASH_EE_CI_ERASE_VALUE, size);
    return ePAR_OK;
}

/**
 * @brief Return the CI native flash region size.
 * @return Region size in bytes.
 */
uint32_t par_store_flash_ee_native_port_region_size(void)
{
    return (uint32_t)sizeof(g_par_store_flash_ee_ci_region);
}

/**
 * @brief Return the CI native flash erase granularity.
 * @return Erase size in bytes.
 */
uint32_t par_store_flash_ee_native_port_erase_size(void)
{
    return PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE;
}

/**
 * @brief Return the CI native flash program granularity.
 * @return Program size in bytes.
 */
uint32_t par_store_flash_ee_native_port_program_size(void)
{
    return PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE;
}

/**
 * @brief Return the CI native flash port name.
 * @return Null-terminated diagnostic name.
 */
const char *par_store_flash_ee_native_port_name(void)
{
    return "ci-native";
}

#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN) && \
          (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_NATIVE_EN) */
