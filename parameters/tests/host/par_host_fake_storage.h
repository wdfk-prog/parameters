/**
 * @file par_host_fake_storage.h
 * @brief Declare host-backed persistent storage used by CI simulators.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#ifndef PAR_HOST_FAKE_STORAGE_H
#define PAR_HOST_FAKE_STORAGE_H

#include <stdbool.h>
#include <stdint.h>
#include <setjmp.h>

#include "par.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/**
 * @brief Host fake-storage operation class used by failpoint injection.
 */
typedef enum
{
    ePAR_HOST_FAKE_STORAGE_OP_NONE = 0, /**< No operation selected. */
    ePAR_HOST_FAKE_STORAGE_OP_WRITE,    /**< Byte-addressable write operation. */
    ePAR_HOST_FAKE_STORAGE_OP_PROGRAM,  /**< Flash program operation. */
    ePAR_HOST_FAKE_STORAGE_OP_ERASE     /**< Flash erase operation. */
} par_host_fake_storage_op_t;

/**
 * @brief Host fake-storage medium behavior.
 */
typedef struct
{
    uint32_t size;          /**< Total storage image size in bytes. */
    uint32_t erase_size;    /**< Erase granularity in bytes. */
    uint32_t program_size;  /**< Program granularity in bytes. */
    uint8_t erased_value;   /**< Erased byte value. */
    bool enforce_one_to_zero; /**< true to reject flash-invalid 0-to-1 programming. */
    const char *default_path; /**< Default image path when PAR_HOST_NVM_IMAGE is unset. */
} par_host_fake_storage_cfg_t;

/**
 * @brief Configure and load the fake persistent storage image.
 * @param cfg Medium behavior configuration.
 * @return ePAR_OK on success, otherwise an error code.
 */
par_status_t par_host_fake_storage_init(const par_host_fake_storage_cfg_t *cfg);

/**
 * @brief Flush and release the fake persistent storage image.
 * @return ePAR_OK on success, otherwise an error code.
 */
par_status_t par_host_fake_storage_deinit(void);

/**
 * @brief Return whether the fake storage image is initialized.
 * @return true after successful init, otherwise false.
 */
bool par_host_fake_storage_is_init(void);

/**
 * @brief Reset the storage image to the configured erased value and flush it.
 * @return ePAR_OK on success, otherwise an error code.
 */
par_status_t par_host_fake_storage_reset_image(void);

/**
 * @brief Read raw bytes from the fake storage image.
 * @param addr Byte offset.
 * @param size Number of bytes to read.
 * @param p_buf Destination buffer.
 * @return ePAR_OK on success, otherwise an error code.
 */
par_status_t par_host_fake_storage_read(uint32_t addr, uint32_t size, uint8_t *p_buf);

/**
 * @brief Write raw bytes with EEPROM semantics.
 * @param addr Byte offset.
 * @param size Number of bytes to write.
 * @param p_buf Source buffer.
 * @return ePAR_OK on success, otherwise an error code.
 */
par_status_t par_host_fake_storage_write(uint32_t addr, uint32_t size, const uint8_t *p_buf);

/**
 * @brief Program raw bytes with flash semantics.
 * @param addr Byte offset.
 * @param size Number of bytes to program.
 * @param p_buf Source buffer.
 * @return ePAR_OK on success, otherwise an error code.
 */
par_status_t par_host_fake_storage_program(uint32_t addr, uint32_t size, const uint8_t *p_buf);

/**
 * @brief Erase raw bytes in the fake storage image.
 * @param addr Byte offset.
 * @param size Number of bytes to erase.
 * @return ePAR_OK on success, otherwise an error code.
 */
par_status_t par_host_fake_storage_erase(uint32_t addr, uint32_t size);

/**
 * @brief Flush the fake storage image to its backing file.
 * @return ePAR_OK on success, otherwise an error code.
 */
par_status_t par_host_fake_storage_sync(void);

/**
 * @brief Configure one deterministic operation failpoint.
 * @param op Operation type to intercept.
 * @param hit_index One-based operation index to intercept.
 * @param prefix_bytes Number of bytes to apply before returning failure.
 */
void par_host_fake_storage_set_failpoint(par_host_fake_storage_op_t op, uint32_t hit_index, uint32_t prefix_bytes);

/**
 * @brief Clear the active failpoint and operation counters.
 */
void par_host_fake_storage_clear_failpoint(void);

/**
 * @brief Configure a jump target used to emulate an immediate power cut.
 * @param p_jump Jump target installed by the active test case, or NULL to disable it.
 */
void par_host_fake_storage_set_powercut_jump(jmp_buf *p_jump);

/**
 * @brief Disable the immediate power-cut jump target.
 */
void par_host_fake_storage_clear_powercut_jump(void);

/**
 * @brief Return the configured fake storage size.
 * @return Storage size in bytes.
 */
uint32_t par_host_fake_storage_size(void);

/**
 * @brief Return the configured fake storage erase granularity.
 * @return Erase size in bytes.
 */
uint32_t par_host_fake_storage_erase_size(void);

/**
 * @brief Return the configured fake storage program granularity.
 * @return Program size in bytes.
 */
uint32_t par_host_fake_storage_program_size(void);

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* !defined(PAR_HOST_FAKE_STORAGE_H) */
