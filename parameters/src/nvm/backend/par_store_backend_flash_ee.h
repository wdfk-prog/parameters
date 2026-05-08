/**
 * @file par_store_backend_flash_ee.h
 * @brief Declare the generic flash-emulated EEPROM backend core.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-04-14
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @details This file declares the portable execution core used by the
 * flash-emulated EEPROM backend. The core does not depend on RT-Thread,
 * FAL, HAL, or any other platform SDK directly. A platform adapter binds the
 * core to one concrete physical flash implementation through the port API
 * defined in this header.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-04-14 1.0     wdfk-prog     first version
 */
#ifndef PAR_STORE_BACKEND_FLASH_EE_H
#define PAR_STORE_BACKEND_FLASH_EE_H

#include <stdbool.h>
#include <stdint.h>

#include "par.h"
#include "par_store_backend.h"

#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN)

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/**
 * @brief Diagnose the latest flash-emulated EEPROM backend event.
 *
 * @details The backend still returns the package-level @ref par_status_t codes
 * to preserve the existing persistence ABI. This diagnostic enumeration keeps a
 * more specific reason code for logging, debugging, and later extension.
 */
typedef enum
{
    ePAR_STORE_FLASH_EE_DIAG_NONE = 0,              /**< No diagnostic information is pending. */
    ePAR_STORE_FLASH_EE_DIAG_PORT_NOT_BOUND,        /**< No physical flash port was bound to the core. */
    ePAR_STORE_FLASH_EE_DIAG_PORT_INIT,             /**< Physical port initialization failed. */
    ePAR_STORE_FLASH_EE_DIAG_PORT_READ,             /**< Physical flash read failed. */
    ePAR_STORE_FLASH_EE_DIAG_PORT_PROGRAM,          /**< Physical flash program failed. */
    ePAR_STORE_FLASH_EE_DIAG_PORT_ERASE,            /**< Physical flash erase failed. */
    ePAR_STORE_FLASH_EE_DIAG_PORT_GEOMETRY,         /**< Physical flash geometry is invalid or unsupported. */
    ePAR_STORE_FLASH_EE_DIAG_CONFIG,                /**< Compile-time configuration is invalid. */
    ePAR_STORE_FLASH_EE_DIAG_BANK_ERASED,           /**< Flash bank is fully erased and contains no valid header. */
    ePAR_STORE_FLASH_EE_DIAG_HEADER_MAGIC,          /**< Flash bank header magic does not match. */
    ePAR_STORE_FLASH_EE_DIAG_HEADER_VERSION,        /**< Flash bank header version is not supported. */
    ePAR_STORE_FLASH_EE_DIAG_HEADER_STATE,          /**< Flash bank header state is not active. */
    ePAR_STORE_FLASH_EE_DIAG_HEADER_CRC,            /**< Flash bank header CRC is invalid. */
    ePAR_STORE_FLASH_EE_DIAG_HEADER_LAYOUT,         /**< Flash bank header geometry does not match the live configuration. */
    ePAR_STORE_FLASH_EE_DIAG_RECORD_MAGIC,          /**< Append record commit marker is closed but invalid. */
    ePAR_STORE_FLASH_EE_DIAG_RECORD_RANGE,          /**< Append record line index exceeds the logical address space. */
    ePAR_STORE_FLASH_EE_DIAG_RECORD_CRC,            /**< Append record payload or semantic metadata CRC is invalid. */
    ePAR_STORE_FLASH_EE_DIAG_RECORD_TAIL,           /**< One or more uncommitted append slots were skipped. */
    ePAR_STORE_FLASH_EE_DIAG_CACHE_WINDOW,          /**< Cache window alignment or bounds are invalid. */
    ePAR_STORE_FLASH_EE_DIAG_CACHE_DIRTY_WINDOW,    /**< A dirty cache window blocks switching to another window before sync. */
    ePAR_STORE_FLASH_EE_DIAG_CAPACITY,              /**< Active bank does not have enough capacity. */
    ePAR_STORE_FLASH_EE_DIAG_CHECKPOINT,            /**< Bank checkpoint or rollover failed. */
    ePAR_STORE_FLASH_EE_DIAG_NOT_INIT               /**< Backend was used before successful initialization. */
} par_store_flash_ee_diag_t;

/**
 * @brief Describe one concrete physical flash port.
 *
 * @details All offsets are relative to the beginning of the persistence region
 * owned by the backend adapter. The generic core splits that region into two
 * banks and never accesses flash outside the supplied region.
 *
 * @note The backend uses a data-first, commit-last append protocol. Only
 * fully committed records are visible during scan and recovery. The backend
 * still keeps only one dirty cache window at a time, but write and erase
 * operations may span multiple cache windows. When staging moves to a new
 * window, the current dirty window is synchronized first, and the final dirty
 * window is synchronized before a successful write or erase call returns.
 * Multi-window requests are therefore durable on success, but they are not
 * transactional across windows when a later step fails. Integrations should
 * size the cache so common parameter objects fit within one window, and should
 * avoid splitting one must-stay-consistent parameter group across multiple
 * independently committed windows.
 */
typedef struct
{
    /**
     * @brief Initialize the physical flash port.
     *
     * @param[in,out] p_ctx Mutable port context owned by the adapter.
     * @return ePAR_OK on success, otherwise a package error code.
     */
    par_status_t (*init)(void *p_ctx);

    /**
     * @brief Deinitialize the physical flash port.
     *
     * @param[in,out] p_ctx Mutable port context owned by the adapter.
     * @return ePAR_OK on success, otherwise a package error code.
     */
    par_status_t (*deinit)(void *p_ctx);

    /**
     * @brief Report whether the physical flash port is initialized.
     *
     * @param[in] p_ctx Immutable port context.
     * @param[out] p_is_init Receives the initialization state.
     * Invalid port implementations shall report false
     * rather than returning an error status.
     */
    void (*is_init)(const void *p_ctx, bool *p_is_init);

    /**
     * @brief Read raw bytes from the physical flash region.
     *
     * @param[in] p_ctx Immutable port context.
     * @param[in] addr Byte offset relative to the bound persistence region.
     * @param[in] size Number of bytes to read.
     * @param[out] p_buf Destination buffer.
     * @return ePAR_OK on success, otherwise a package error code.
     */
    par_status_t (*read)(const void *p_ctx, uint32_t addr, uint32_t size, uint8_t *p_buf);

    /**
     * @brief Program raw bytes into the physical flash region.
     *
     * @param[in] p_ctx Immutable port context.
     * @param[in] addr Byte offset relative to the bound persistence region.
     * @param[in] size Number of bytes to program.
     * @param[in] p_buf Source buffer.
     * @return ePAR_OK on success, otherwise a package error code.
     */
    par_status_t (*program)(const void *p_ctx, uint32_t addr, uint32_t size, const uint8_t *p_buf);

    /**
     * @brief Erase raw bytes inside the physical flash region.
     *
     * @param[in] p_ctx Immutable port context.
     * @param[in] addr Byte offset relative to the bound persistence region.
     * @param[in] size Number of bytes to erase.
     * @return ePAR_OK on success, otherwise a package error code.
     */
    par_status_t (*erase)(const void *p_ctx, uint32_t addr, uint32_t size);

    /**
     * @brief Return the total persistence-region size in bytes.
     *
     * @param[in] p_ctx Immutable port context.
     * @return Region size in bytes.
     */
    uint32_t (*get_region_size)(const void *p_ctx);

    /**
     * @brief Return the physical flash erase granularity in bytes.
     *
     * @param[in] p_ctx Immutable port context.
     * @return Erase granularity in bytes.
     */
    uint32_t (*get_erase_size)(const void *p_ctx);

    /**
     * @brief Return the physical flash program granularity in bytes.
     *
     * @param[in] p_ctx Immutable port context.
     * @return Program granularity in bytes.
     */
    uint32_t (*get_program_size)(const void *p_ctx);

    /**
     * @brief Return a short port name for diagnostics.
     *
     * @param[in] p_ctx Immutable port context.
     * @return Null-terminated name string, or NULL when unavailable.
     */
    const char *(*get_name)(const void *p_ctx);
} par_store_flash_ee_port_api_t;

#if (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_NATIVE_EN)
/**
 * @brief Initialize the user-supplied native flash port.
 *
 * @details The native adapter expects the application or BSP to provide strong
 * definitions for the required operational and geometry hooks declared in this
 * header. Missing required definitions should fail at link time rather than
 * falling back to runtime stub behavior. The package may still provide benign
 * defaults for optional helper hooks such as deinit or diagnostic naming.
 *
 * @return ePAR_OK on success, otherwise an implementation-defined package error.
 */
par_status_t par_store_flash_ee_native_port_init(void);

/**
 * @brief Deinitialize the user-supplied native flash port.
 *
 * @return ePAR_OK on success, otherwise an implementation-defined package error.
 */
par_status_t par_store_flash_ee_native_port_deinit(void);

/**
 * @brief Read raw bytes from the user-supplied native flash region.
 *
 * @param[in] addr Byte offset inside the bound persistence region.
 * @param[in] size Number of bytes to read.
 * @param[out] p_buf Destination buffer.
 * @return ePAR_OK on success, otherwise an implementation-defined package error.
 */
par_status_t par_store_flash_ee_native_port_read(uint32_t addr, uint32_t size, uint8_t *p_buf);

/**
 * @brief Program raw bytes into the user-supplied native flash region.
 *
 * @param[in] addr Byte offset inside the bound persistence region.
 * @param[in] size Number of bytes to program.
 * @param[in] p_buf Source buffer.
 * @return ePAR_OK on success, otherwise an implementation-defined package error.
 */
par_status_t par_store_flash_ee_native_port_program(uint32_t addr, uint32_t size, const uint8_t *p_buf);

/**
 * @brief Erase raw bytes inside the user-supplied native flash region.
 *
 * @param[in] addr Byte offset inside the bound persistence region.
 * @param[in] size Number of bytes to erase.
 * @return ePAR_OK on success, otherwise an implementation-defined package error.
 */
par_status_t par_store_flash_ee_native_port_erase(uint32_t addr, uint32_t size);

/**
 * @brief Return the total size of the user-supplied native flash region.
 *
 * @return Region size in bytes.
 */
uint32_t par_store_flash_ee_native_port_region_size(void);

/**
 * @brief Return the erase granularity of the user-supplied native flash region.
 *
 * @return Erase size in bytes.
 */
uint32_t par_store_flash_ee_native_port_erase_size(void);

/**
 * @brief Return the program granularity of the user-supplied native flash region.
 *
 * @return Program size in bytes.
 */
uint32_t par_store_flash_ee_native_port_program_size(void);

/**
 * @brief Return a short diagnostic name for the user-supplied native flash port.
 *
 * @return Null-terminated port name string.
 */
const char *par_store_flash_ee_native_port_name(void);
#endif /* (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_PORT_NATIVE_EN) */

/**
 * @brief Bind one physical flash port to the generic flash-emulated EEPROM core.
 *
 * @param[in] p_port_api Physical port API table.
 * @param[in,out] p_port_ctx Mutable physical port context.
 * @return ePAR_OK on success, otherwise a package error code.
 */
par_status_t par_store_backend_flash_ee_bind_port(const par_store_flash_ee_port_api_t *p_port_api, void *p_port_ctx);

/**
 * @brief Return the generic flash-emulated EEPROM backend API.
 *
 * @return Backend API pointer.
 */
const par_store_backend_api_t *par_store_backend_flash_ee_get_api(void);

/**
 * @brief Return the latest backend diagnostic code.
 *
 * @return Latest diagnostic code.
 */
par_store_flash_ee_diag_t par_store_backend_flash_ee_get_diag(void);

/**
 * @brief Return a human-readable string for one diagnostic code.
 *
 * @param[in] diag Diagnostic code.
 * @return Constant string representation.
 */
const char *par_store_backend_flash_ee_get_diag_str(par_store_flash_ee_diag_t diag);

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN) */

#endif /* !defined(PAR_STORE_BACKEND_FLASH_EE_H) */
