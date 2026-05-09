/**
 * @file par_backend_adapter_ci_stubs.c
 * @brief Provide weak FAL and AT24CXX symbols for CI package compile smoke.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#include <stdint.h>
#include <string.h>

#include "fal.h"
#include "fal_cfg.h"
#include "at24cxx.h"

#ifndef AUTOGEN_PM_CI_WEAK
#define AUTOGEN_PM_CI_WEAK __attribute__((weak))
#endif /* !defined(AUTOGEN_PM_CI_WEAK) */

/** @brief Backing size for weak FAL and AT24CXX storage stubs. */
#define AUTOGEN_PM_CI_BACKEND_SIZE (4096U)

/** @brief Weak FAL/AT24CXX byte storage used only by CI package builds. */
static uint8_t g_autogen_pm_ci_backend[AUTOGEN_PM_CI_BACKEND_SIZE];
/**
 * @brief Initialize the CI FAL flash device.
 * @return 0 on success.
 */
static int autogen_pm_ci_fal_flash_init(void)
{
    memset(g_autogen_pm_ci_backend, 0xFF, sizeof(g_autogen_pm_ci_backend));
    return 0;
}

/**
 * @brief Read from the CI FAL flash device.
 * @param offset Byte offset inside the flash device.
 * @param buf Destination buffer.
 * @param size Number of bytes to read.
 * @return Number of bytes read on success, otherwise -1.
 */
static int autogen_pm_ci_fal_flash_read(long offset, uint8_t *buf, size_t size)
{
    if ((NULL == buf) || (offset < 0) || (((size_t)offset) > sizeof(g_autogen_pm_ci_backend)) ||
        (size > (sizeof(g_autogen_pm_ci_backend) - (size_t)offset)))
    {
        return -1;
    }

    memcpy(buf, &g_autogen_pm_ci_backend[offset], size);
    return (int)size;
}

/**
 * @brief Write to the CI FAL flash device.
 * @param offset Byte offset inside the flash device.
 * @param buf Source buffer.
 * @param size Number of bytes to write.
 * @return Number of bytes written on success, otherwise -1.
 */
static int autogen_pm_ci_fal_flash_write(long offset, const uint8_t *buf, size_t size)
{
    if ((NULL == buf) || (offset < 0) || (((size_t)offset) > sizeof(g_autogen_pm_ci_backend)) ||
        (size > (sizeof(g_autogen_pm_ci_backend) - (size_t)offset)))
    {
        return -1;
    }

    memcpy(&g_autogen_pm_ci_backend[offset], buf, size);
    return (int)size;
}

/**
 * @brief Erase the CI FAL flash device.
 * @param offset Byte offset inside the flash device.
 * @param size Number of bytes to erase.
 * @return Number of bytes erased on success, otherwise -1.
 */
static int autogen_pm_ci_fal_flash_erase(long offset, size_t size)
{
    if ((offset < 0) || (((size_t)offset) > sizeof(g_autogen_pm_ci_backend)) ||
        (size > (sizeof(g_autogen_pm_ci_backend) - (size_t)offset)))
    {
        return -1;
    }

    memset(&g_autogen_pm_ci_backend[offset], 0xFF, size);
    return (int)size;
}

/** @brief Weak FAL flash device descriptor used only by CI package builds. */
#ifdef FAL_DEV_NAME_MAX
const struct fal_flash_dev g_autogen_pm_ci_fal_flash = {
    .name = AUTOGEN_PM_CI_FAL_FLASH_NAME,
    .addr = 0,
    .len = AUTOGEN_PM_CI_FAL_FLASH_SIZE,
    .blk_size = AUTOGEN_PM_CI_FAL_BLOCK_SIZE,
    .ops = {
        .init = autogen_pm_ci_fal_flash_init,
        .read = autogen_pm_ci_fal_flash_read,
        .write = autogen_pm_ci_fal_flash_write,
        .erase = autogen_pm_ci_fal_flash_erase,
    },
    .write_gran = 8,
};
#else
const struct fal_flash_dev g_autogen_pm_ci_fal_flash = {
    .len = AUTOGEN_PM_CI_BACKEND_SIZE,
    .blk_size = 64,
};
#endif /* FAL_DEV_NAME_MAX */
/** @brief Weak FAL partition descriptor used only by CI package builds. */
static const struct fal_partition g_autogen_pm_ci_fal_partition = {
    .name = "autogen_pm",
    .flash_name = AUTOGEN_PM_CI_FAL_FLASH_NAME,
    .offset = 0,
    .len = AUTOGEN_PM_CI_BACKEND_SIZE,
};

/** @brief Concrete AT24CXX device used only by CI package builds. */
struct autogen_pm_ci_at24_device
{
    uint8_t mem[AT24CXX_MAX_MEM_ADDRESS]; /**< EEPROM contents. */
};

/** @brief Singleton AT24CXX device used only by CI package builds. */
static struct autogen_pm_ci_at24_device g_autogen_pm_ci_at24_device;

/**
 * @brief Resolve the CI FAL partition by name.
 * @param name Requested partition name.
 * @return Partition handle when the name matches, otherwise NULL.
 */
AUTOGEN_PM_CI_WEAK const struct fal_partition *fal_partition_find(const char *name)
{
    if ((NULL != name) && (0 == strcmp(name, g_autogen_pm_ci_fal_partition.name)))
    {
        return &g_autogen_pm_ci_fal_partition;
    }

    return NULL;
}

/**
 * @brief Resolve the CI FAL flash device by name.
 * @param name Requested flash device name.
 * @return Flash device handle when the name matches, otherwise NULL.
 */
AUTOGEN_PM_CI_WEAK const struct fal_flash_dev *fal_flash_device_find(const char *name)
{
    if ((NULL != name) && (0 == strcmp(name, g_autogen_pm_ci_fal_partition.flash_name)))
    {
        return &g_autogen_pm_ci_fal_flash;
    }

    return NULL;
}

/**
 * @brief Read bytes from the weak CI FAL partition.
 * @param part Partition handle.
 * @param addr Byte offset inside the partition.
 * @param buf Destination buffer.
 * @param size Number of bytes to read.
 * @return Number of bytes read on success, otherwise -1.
 */
AUTOGEN_PM_CI_WEAK int fal_partition_read(const struct fal_partition *part,
                                          rt_uint32_t addr,
                                          rt_uint8_t *buf,
                                          rt_size_t size)
{
    if ((NULL == part) || (NULL == buf) ||
        ((addr > sizeof(g_autogen_pm_ci_backend)) || (size > (sizeof(g_autogen_pm_ci_backend) - addr))))
    {
        return -1;
    }

    memcpy(buf, &g_autogen_pm_ci_backend[addr], size);
    return (int)size;
}

/**
 * @brief Write bytes into the weak CI FAL partition.
 * @param part Partition handle.
 * @param addr Byte offset inside the partition.
 * @param buf Source buffer.
 * @param size Number of bytes to write.
 * @return Number of bytes written on success, otherwise -1.
 */
AUTOGEN_PM_CI_WEAK int fal_partition_write(const struct fal_partition *part,
                                           rt_uint32_t addr,
                                           const rt_uint8_t *buf,
                                           rt_size_t size)
{
    if ((NULL == part) || (NULL == buf) ||
        ((addr > sizeof(g_autogen_pm_ci_backend)) || (size > (sizeof(g_autogen_pm_ci_backend) - addr))))
    {
        return -1;
    }

    memcpy(&g_autogen_pm_ci_backend[addr], buf, size);
    return (int)size;
}

/**
 * @brief Erase bytes inside the weak CI FAL partition.
 * @param part Partition handle.
 * @param addr Byte offset inside the partition.
 * @param size Number of bytes to erase.
 * @return Number of bytes erased on success, otherwise -1.
 */
AUTOGEN_PM_CI_WEAK int fal_partition_erase(const struct fal_partition *part,
                                           rt_uint32_t addr,
                                           rt_size_t size)
{
    if ((NULL == part) ||
        ((addr > sizeof(g_autogen_pm_ci_backend)) || (size > (sizeof(g_autogen_pm_ci_backend) - addr))))
    {
        return -1;
    }

    memset(&g_autogen_pm_ci_backend[addr], 0xFF, size);
    return (int)size;
}

/**
 * @brief Initialize the weak CI AT24CXX device.
 * @param i2c_bus_name I2C bus name.
 * @param addr_input Hardware address input bits.
 * @return Device handle.
 */
AUTOGEN_PM_CI_WEAK at24cxx_device_t at24cxx_init(const char *i2c_bus_name, uint8_t addr_input)
{
    (void)i2c_bus_name;
    (void)addr_input;
    memset(&g_autogen_pm_ci_at24_device, 0xFF, sizeof(g_autogen_pm_ci_at24_device));
    return &g_autogen_pm_ci_at24_device;
}

/**
 * @brief Deinitialize the weak CI AT24CXX device.
 * @param dev Device handle.
 */
AUTOGEN_PM_CI_WEAK void at24cxx_deinit(at24cxx_device_t dev)
{
    (void)dev;
}

/**
 * @brief Probe the weak CI AT24CXX device.
 * @param dev Device handle.
 * @return RT_EOK when the handle is valid, otherwise -1.
 */
AUTOGEN_PM_CI_WEAK int at24cxx_check(at24cxx_device_t dev)
{
    return (RT_NULL != dev) ? RT_EOK : -1;
}

/**
 * @brief Read one page fragment from the weak CI AT24CXX device.
 * @param dev Device handle.
 * @param addr Absolute EEPROM address.
 * @param buf Destination buffer.
 * @param size Number of bytes to read.
 * @return RT_EOK on success, otherwise -1.
 */
AUTOGEN_PM_CI_WEAK int at24cxx_page_read(at24cxx_device_t dev, uint32_t addr, uint8_t *buf, uint16_t size)
{
    if ((RT_NULL == dev) || (NULL == buf) ||
        ((addr > AT24CXX_MAX_MEM_ADDRESS) || (size > (AT24CXX_MAX_MEM_ADDRESS - addr))))
    {
        return -1;
    }

    memcpy(buf, &dev->mem[addr], size);
    return RT_EOK;
}

/**
 * @brief Write one page fragment into the weak CI AT24CXX device.
 * @param dev Device handle.
 * @param addr Absolute EEPROM address.
 * @param buf Source buffer.
 * @param size Number of bytes to write.
 * @return RT_EOK on success, otherwise -1.
 */
AUTOGEN_PM_CI_WEAK int at24cxx_page_write(at24cxx_device_t dev, uint32_t addr, const uint8_t *buf, uint16_t size)
{
    if ((RT_NULL == dev) || (NULL == buf) ||
        ((addr > AT24CXX_MAX_MEM_ADDRESS) || (size > (AT24CXX_MAX_MEM_ADDRESS - addr))))
    {
        return -1;
    }

    memcpy(&dev->mem[addr], buf, size);
    return RT_EOK;
}
