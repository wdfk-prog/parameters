/**
 * @file par_host_flash_ee_native_port.c
 * @brief Bind Flash EE native hooks to the host fake flash image.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#include "par_cfg.h"

#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_NATIVE_EN)

#include "par_host_fake_storage.h"
#include "par_store_backend_flash_ee.h"

/**
 * @brief Host fake flash total region size in bytes.
 */
#define PAR_HOST_FLASH_EE_REGION_SIZE (32768u)

/**
 * @brief Host fake flash erase granularity in bytes.
 */
#define PAR_HOST_FLASH_EE_ERASE_SIZE  (4096u)

/**
 * @brief Initialize the host fake flash image for the Flash EE native port.
 * @return ePAR_OK on success, otherwise an error code.
 */
par_status_t par_store_flash_ee_native_port_init(void)
{
    const par_host_fake_storage_cfg_t cfg = {
        .size = PAR_HOST_FLASH_EE_REGION_SIZE,
        .erase_size = PAR_HOST_FLASH_EE_ERASE_SIZE,
        .program_size = PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE,
        .erased_value = 0xFFu,
        .enforce_one_to_zero = true,
        .default_path = "par_host_flash_ee.bin",
    };

    return par_host_fake_storage_init(&cfg);
}

/**
 * @brief Deinitialize the host fake flash image.
 * @return ePAR_OK on success, otherwise an error code.
 */
par_status_t par_store_flash_ee_native_port_deinit(void)
{
    return par_host_fake_storage_deinit();
}

/**
 * @brief Read bytes from the host fake flash image.
 * @param addr Flash-region byte offset.
 * @param size Number of bytes to read.
 * @param p_buf Destination buffer.
 * @return ePAR_OK on success, otherwise an error code.
 */
par_status_t par_store_flash_ee_native_port_read(uint32_t addr, uint32_t size, uint8_t *p_buf)
{
    return par_host_fake_storage_read(addr, size, p_buf);
}

/**
 * @brief Program bytes into the host fake flash image with NOR-like semantics.
 * @param addr Flash-region byte offset.
 * @param size Number of bytes to program.
 * @param p_buf Source buffer.
 * @return ePAR_OK on success, otherwise an error code.
 */
par_status_t par_store_flash_ee_native_port_program(uint32_t addr, uint32_t size, const uint8_t *p_buf)
{
    return par_host_fake_storage_program(addr, size, p_buf);
}

/**
 * @brief Erase a host fake flash range to the erased value.
 * @param addr Flash-region byte offset.
 * @param size Number of bytes to erase.
 * @return ePAR_OK on success, otherwise an error code.
 */
par_status_t par_store_flash_ee_native_port_erase(uint32_t addr, uint32_t size)
{
    return par_host_fake_storage_erase(addr, size);
}

/**
 * @brief Return the host fake flash region size.
 * @return Region size in bytes.
 */
uint32_t par_store_flash_ee_native_port_region_size(void)
{
    return PAR_HOST_FLASH_EE_REGION_SIZE;
}

/**
 * @brief Return the host fake flash erase granularity.
 * @return Erase size in bytes.
 */
uint32_t par_store_flash_ee_native_port_erase_size(void)
{
    return PAR_HOST_FLASH_EE_ERASE_SIZE;
}

/**
 * @brief Return the host fake flash program granularity.
 * @return Program size in bytes.
 */
uint32_t par_store_flash_ee_native_port_program_size(void)
{
    return PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE;
}

/**
 * @brief Return the host fake flash native-port name.
 * @return Constant backend name string.
 */
const char *par_store_flash_ee_native_port_name(void)
{
    return "host_fake_flash";
}

/**
 * @brief Reset the host Flash EE fake partition image to erased state.
 * @return ePAR_OK on success, otherwise an error code.
 */
par_status_t par_host_flash_ee_reset_image(void)
{
    par_status_t status = par_store_flash_ee_native_port_init();

    if (ePAR_OK != status)
    {
        return status;
    }

    status = par_host_fake_storage_reset_image();
    (void)par_store_flash_ee_native_port_deinit();
    return status;
}

/**
 * @brief Return the append-record capacity per host fake Flash EE bank.
 * @return Append-record capacity per bank.
 */
uint32_t par_host_flash_ee_bank_record_capacity(void)
{
    const uint32_t bank_size = PAR_HOST_FLASH_EE_REGION_SIZE / 2u;
    const uint32_t meta_offset = ((PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE + PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE - 1u) /
                                  PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE) * PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE;
    const uint32_t commit_offset = ((meta_offset + 12u + PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE - 1u) /
                                    PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE) * PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE;
    const uint32_t record_size = commit_offset + PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE;

    return (bank_size - 64u) / record_size;
}

#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_NATIVE_EN) */
