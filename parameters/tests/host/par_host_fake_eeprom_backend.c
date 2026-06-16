/**
 * @file par_host_fake_eeprom_backend.c
 * @brief Provide a byte-addressable fake EEPROM backend for host CI tests.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#include "par_cfg.h"

#if (1 == PAR_CFG_NVM_EN) && !defined(PAR_HOST_BACKEND_FLASH_EE)

#include "par_host_fake_storage.h"
#include "par_store_backend.h"

/**
 * @brief Fake EEPROM image size used by host persistence tests.
 */
#define PAR_HOST_FAKE_EEPROM_SIZE (65536u)

/**
 * @brief Fake EEPROM page/write granularity used by host persistence tests.
 */
#define PAR_HOST_FAKE_EEPROM_PAGE_SIZE (1u)

/**
 * @brief Initialize the host fake EEPROM storage image.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_host_fake_eeprom_init(void)
{
    const par_host_fake_storage_cfg_t cfg = {
        .size = PAR_HOST_FAKE_EEPROM_SIZE,
        .erase_size = PAR_HOST_FAKE_EEPROM_PAGE_SIZE,
        .program_size = 1u,
        .erased_value = 0xFFu,
        .enforce_one_to_zero = false,
        .default_path = "par_host_eeprom.bin",
    };

    return par_host_fake_storage_init(&cfg);
}

/**
 * @brief Deinitialize the host fake EEPROM storage image.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_host_fake_eeprom_deinit(void)
{
    return par_host_fake_storage_deinit();
}

/**
 * @brief Report whether the fake EEPROM backend is initialized.
 * @param p_is_init Output initialization flag.
 */
static void par_host_fake_eeprom_is_init(bool * const p_is_init)
{
    if (NULL != p_is_init)
    {
        *p_is_init = par_host_fake_storage_is_init();
    }
}

/**
 * @brief Read from the fake EEPROM image.
 * @param addr Byte offset.
 * @param size Number of bytes to read.
 * @param p_buf Destination buffer.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_host_fake_eeprom_read(const uint32_t addr, const uint32_t size, uint8_t * const p_buf)
{
    return par_host_fake_storage_read(addr, size, p_buf);
}

/**
 * @brief Write to the fake EEPROM image.
 * @param addr Byte offset.
 * @param size Number of bytes to write.
 * @param p_buf Source buffer.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_host_fake_eeprom_write(const uint32_t addr, const uint32_t size, const uint8_t * const p_buf)
{
    return par_host_fake_storage_write(addr, size, p_buf);
}

/**
 * @brief Erase a fake EEPROM byte range to 0xFF.
 * @param addr Byte offset.
 * @param size Number of bytes to erase.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_host_fake_eeprom_erase(const uint32_t addr, const uint32_t size)
{
    return par_host_fake_storage_erase(addr, size);
}

/**
 * @brief Flush the fake EEPROM image to disk.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_host_fake_eeprom_sync(void)
{
    return par_host_fake_storage_sync();
}

/**
 * @brief Host fake EEPROM backend API table.
 */
static const par_store_backend_api_t g_par_host_fake_eeprom_api = {
    .init = par_host_fake_eeprom_init,
    .deinit = par_host_fake_eeprom_deinit,
    .is_init = par_host_fake_eeprom_is_init,
    .read = par_host_fake_eeprom_read,
    .write = par_host_fake_eeprom_write,
    .erase = par_host_fake_eeprom_erase,
    .sync = par_host_fake_eeprom_sync,
    .name = "host_fake_eeprom",
};

/**
 * @brief Bind the host fake EEPROM backend.
 * @return ePAR_OK because the fake backend has no external device binding step.
 */
par_status_t par_store_backend_bind(void)
{
    return ePAR_OK;
}

/**
 * @brief Return the host fake EEPROM backend API.
 * @return Pointer to the backend API table.
 */
const par_store_backend_api_t * par_store_backend_get_api(void)
{
    return &g_par_host_fake_eeprom_api;
}

#endif /* (1 == PAR_CFG_NVM_EN) && !defined(PAR_HOST_BACKEND_FLASH_EE) */
