/**
 * @file par_store_backend_rtt_at24cxx.c
 * @brief Bind the generic parameter-storage backend API to RT-Thread AT24CXX.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-03-29
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-03-29 1.0     wdfk-prog    first version
 */
#include "par_cfg.h"
#include "par_store_backend.h"

#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_RTT_AT24CXX_EN)

#include "at24cxx.h"

/**
 * @brief Erase pattern used by EEPROM-backed storage.
 *
 * @details AT24CXX devices do not provide a native erase command. The backend
 * therefore emulates erase by writing the erased-state pattern into the target
 * window.
 */
#define PAR_STORE_RTT_AT24_ERASE_VALUE (0xFFu)

/**
 * @brief Absolute end offset of the parameter-owned EEPROM window.
 *
 * @details The end offset is exclusive.
 */
#define PAR_STORE_RTT_AT24_WINDOW_END ((uint32_t)PAR_CFG_RTT_AT24_BASE_ADDR + (uint32_t)PAR_CFG_RTT_AT24_SIZE)

/**
 * @brief Compile-time validation for the configured EEPROM window.
 *
 * @details
 * - The parameter-owned window must fit in the AT24CXX memory space.
 * - Zero-sized window is invalid.
 * - Overflow is rejected by checking the exclusive end address.
 */
RT_STATIC_ASSERT(par_rtt_at24_window_size_nonzero, ((uint32_t)(PAR_CFG_RTT_AT24_SIZE) > 0u));
RT_STATIC_ASSERT(par_rtt_at24_window_base_in_range, ((uint32_t)(PAR_CFG_RTT_AT24_BASE_ADDR) < (uint32_t)(AT24CXX_MAX_MEM_ADDRESS)));
RT_STATIC_ASSERT(par_rtt_at24_window_end_in_range, (PAR_STORE_RTT_AT24_WINDOW_END <= (uint32_t)(AT24CXX_MAX_MEM_ADDRESS)));
RT_STATIC_ASSERT(par_rtt_at24_window_no_overflow, (PAR_STORE_RTT_AT24_WINDOW_END >= (uint32_t)(PAR_CFG_RTT_AT24_BASE_ADDR)));
/**
 * @brief Convert a backend-relative offset to an EEPROM absolute offset.
 */
#define PAR_STORE_RTT_AT24_ABS_ADDR(addr) ((uint32_t)PAR_CFG_RTT_AT24_BASE_ADDR + (uint32_t)(addr))

/**
 * @brief Backing AT24CXX device handle.
 */
static at24cxx_device_t gp_at24_dev = RT_NULL;

/**
 * @brief Check whether a relative access range stays inside the parameter-owned
 * EEPROM window.
 * @param addr Start offset relative to the parameter storage window.
 * @param size Access length in bytes.
 * @return RT_TRUE if the range is valid; RT_FALSE otherwise.
 * @note This helper validates zero-length access, integer overflow on
 * `addr + size`, and upper-bound violations against `PAR_CFG_RTT_AT24_SIZE`.
 */
static rt_bool_t par_store_rtt_at24_is_range_valid(uint32_t addr, uint32_t size)
{
    uint32_t end = 0u;

    if (0u == size)
    {
        return RT_FALSE;
    }

    if (addr >= (uint32_t)PAR_CFG_RTT_AT24_SIZE)
    {
        return RT_FALSE;
    }

    end = addr + size;

    if (end < addr)
    {
        return RT_FALSE; /* overflow */
    }

    if (end > (uint32_t)PAR_CFG_RTT_AT24_SIZE)
    {
        return RT_FALSE;
    }

    return RT_TRUE;
}

/**
 * @brief Check whether the backend has an opened AT24CXX device.
 *
 * @param[out] p_is_init Destination initialization flag.
 */
static void par_store_rtt_at24_is_init(bool * const p_is_init)
{
    if (NULL == p_is_init)
    {
        return;
    }

    *p_is_init = (RT_NULL != gp_at24_dev);
}

/**
 * @brief Initialize the RT-Thread AT24CXX backend.
 *
 * @details The backend opens the configured I2C-backed EEPROM device and runs
 * the package-provided probe routine once before exposing the storage API.
 *
 * @return Operation status.
 */
static par_status_t par_store_rtt_at24_init(void)
{
    if (RT_NULL != gp_at24_dev)
    {
        return ePAR_OK;
    }

    gp_at24_dev = at24cxx_init(PAR_CFG_RTT_AT24_I2C_BUS_NAME, (uint8_t)PAR_CFG_RTT_AT24_ADDR_INPUT);
    if (RT_NULL == gp_at24_dev)
    {
        return ePAR_ERROR_INIT;
    }

    if (RT_EOK != at24cxx_check(gp_at24_dev))
    {
        at24cxx_deinit(gp_at24_dev);
        gp_at24_dev = RT_NULL;
        return ePAR_ERROR_INIT;
    }

    return ePAR_OK;
}

/**
 * @brief Deinitialize the RT-Thread AT24CXX backend.
 *
 * @return Operation status.
 */
static par_status_t par_store_rtt_at24_deinit(void)
{
    if (RT_NULL != gp_at24_dev)
    {
        at24cxx_deinit(gp_at24_dev);
        gp_at24_dev = RT_NULL;
    }

    return ePAR_OK;
}

/**
 * @brief Read raw bytes from the parameter-owned EEPROM window.
 *
 * @param addr Backend-relative start address.
 * @param size Number of bytes to read.
 * @param p_buf Destination buffer.
 * @return Operation status.
 */
static par_status_t par_store_rtt_at24_read(const uint32_t addr, const uint32_t size, uint8_t * const p_buf)
{
    uint32_t abs_addr = 0u;
    uint32_t remaining = size;
    uint8_t *p_dst = p_buf;

    if ((RT_NULL == gp_at24_dev) || (NULL == p_buf))
    {
        return ePAR_ERROR_PARAM;
    }

    if (false == par_store_rtt_at24_is_range_valid(addr, size))
    {
        return ePAR_ERROR_NVM;
    }

    abs_addr = PAR_STORE_RTT_AT24_ABS_ADDR(addr);

    while (remaining > 0u)
    {
        uint16_t xfer = (remaining > (uint32_t)AT24CXX_PAGE_BYTE) ? (uint16_t)AT24CXX_PAGE_BYTE : (uint16_t)remaining;

        if (RT_EOK != at24cxx_page_read(gp_at24_dev, abs_addr, p_dst, xfer))
        {
            return ePAR_ERROR_NVM;
        }

        abs_addr += (uint32_t)xfer;
        p_dst += xfer;
        remaining -= (uint32_t)xfer;
    }

    return ePAR_OK;
}

/**
 * @brief Write raw bytes into the parameter-owned EEPROM window.
 *
 * @param addr Backend-relative start address.
 * @param size Number of bytes to write.
 * @param p_buf Source buffer.
 * @return Operation status.
 */
static par_status_t par_store_rtt_at24_write(const uint32_t addr, const uint32_t size, const uint8_t * const p_buf)
{
    uint32_t abs_addr = 0u;
    uint32_t remaining = size;
    const uint8_t *p_src = p_buf;

    if ((RT_NULL == gp_at24_dev) || (NULL == p_buf))
    {
        return ePAR_ERROR_PARAM;
    }

    if (false == par_store_rtt_at24_is_range_valid(addr, size))
    {
        return ePAR_ERROR_NVM;
    }

    abs_addr = PAR_STORE_RTT_AT24_ABS_ADDR(addr);

    while (remaining > 0u)
    {
        uint16_t xfer = (remaining > (uint32_t)AT24CXX_PAGE_BYTE) ? (uint16_t)AT24CXX_PAGE_BYTE : (uint16_t)remaining;
        /* The AT24CXX write API accepts a non-const buffer pointer, but the
         * backend only passes source bytes for transmission and does not expect the
         * driver to modify them. Cast away constness locally to match the driver
         * signature and avoid an unnecessary temporary copy.
         */
        if (RT_EOK != at24cxx_page_write(gp_at24_dev, abs_addr, (uint8_t *)p_src, xfer))
        {
            return ePAR_ERROR_NVM;
        }

        abs_addr += (uint32_t)xfer;
        p_src += xfer;
        remaining -= (uint32_t)xfer;
    }

    return ePAR_OK;
}

/**
 * @brief Emulate erase over the parameter-owned EEPROM window.
 *
 * @details EEPROM has no native erase operation. The backend therefore fills
 * the requested range with the erased-state pattern 0xFF.
 *
 * @param addr Backend-relative start address.
 * @param size Number of bytes to erase.
 * @return Operation status.
 */
static par_status_t par_store_rtt_at24_erase(const uint32_t addr, const uint32_t size)
{
    uint32_t abs_addr = 0u;
    uint32_t remaining = size;
    uint8_t erase_buf[PAR_STORE_RTT_AT24_ERASE_CHUNK];

    if (RT_NULL == gp_at24_dev)
    {
        return ePAR_ERROR_PARAM;
    }

    if (false == par_store_rtt_at24_is_range_valid(addr, size))
    {
        return ePAR_ERROR_NVM;
    }

    abs_addr = PAR_STORE_RTT_AT24_ABS_ADDR(addr);

    (void)rt_memset(erase_buf, (int)PAR_STORE_RTT_AT24_ERASE_VALUE, sizeof(erase_buf));

    while (remaining > 0u)
    {
        uint16_t xfer = (remaining > (uint32_t)PAR_STORE_RTT_AT24_ERASE_CHUNK) ? (uint16_t)PAR_STORE_RTT_AT24_ERASE_CHUNK : (uint16_t)remaining;

        if (RT_EOK != at24cxx_page_write(gp_at24_dev, abs_addr, erase_buf, xfer))
        {
            return ePAR_ERROR_NVM;
        }

        abs_addr += (uint32_t)xfer;
        remaining -= (uint32_t)xfer;
    }

    return ePAR_OK;
}

/**
 * @brief Flush pending backend data.
 *
 * @details The RT-Thread AT24CXX package performs synchronous page writes, so
 * no extra flush action is required here.
 *
 * @return Operation status.
 */
static par_status_t par_store_rtt_at24_sync(void)
{
    return ePAR_OK;
}

/**
 * @brief Prepare the RT-Thread AT24CXX backend before API resolution.
 *
 * @details This backend has no extra bind step, so the function is a no-op.
 *
 * @return ePAR_OK.
 */
par_status_t par_store_backend_bind(void)
{
    return ePAR_OK;
}

/**
 * @brief Concrete RT-Thread AT24CXX backend API instance.
 */
static const par_store_backend_api_t g_par_store_backend_rtt_at24cxx = {
    .init = par_store_rtt_at24_init,
    .deinit = par_store_rtt_at24_deinit,
    .is_init = par_store_rtt_at24_is_init,
    .read = par_store_rtt_at24_read,
    .write = par_store_rtt_at24_write,
    .erase = par_store_rtt_at24_erase,
    .sync = par_store_rtt_at24_sync,
    .name = "rtt_at24cxx",
};

/**
 * @brief Return the active RT-Thread AT24CXX backend API.
 *
 * @return Backend API instance.
 */
const par_store_backend_api_t *par_store_backend_get_api(void)
{
    return &g_par_store_backend_rtt_at24cxx;
}

#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_RTT_AT24CXX_EN) */
