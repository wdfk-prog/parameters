/**
 * @file par_store_backend.h
 * @brief Declare the abstract parameter-storage backend interface.
 * @author wdfk-prog ()
 * @version 1.0
 * @date 2026-03-29
 * 
 * @copyright Copyright (c) 2026  
 * 
 * @details par_nvm.c uses this interface instead of depending directly on a
 * concrete NVM repository layout. Integrators may provide any storage backend
 * that supports the required byte-addressable operations.
 * @par Change Log:
 * Date       Version Author      Description
 * 2026-03-29 1.0     wdfk-prog   first version
 */
#ifndef _PAR_STORE_BACKEND_H_
#define _PAR_STORE_BACKEND_H_

#include <stdint.h>
#include <stdbool.h>

#include "par.h"

#if (1 == PAR_CFG_NVM_EN)
/**
 * @brief Abstract parameter-storage backend contract.
 *
 * @details All offsets are relative to the storage region reserved for the
 * parameter image. The backend owns any region, partition, or device-specific
 * context needed to execute the operation.
 */
typedef struct
{
    /** @brief Initialize the storage backend. */
    par_status_t (*init)(void);
    /** @brief Deinitialize the storage backend. */
    par_status_t (*deinit)(void);
    /** @brief Query whether the storage backend is initialized. */
    par_status_t (*is_init)(bool * const p_is_init);
    /** @brief Read raw bytes from the storage backend. */
    par_status_t (*read)(const uint32_t addr, const uint32_t size, uint8_t * const p_buf);
    /** @brief Write raw bytes to the storage backend. */
    par_status_t (*write)(const uint32_t addr, const uint32_t size, const uint8_t * const p_buf);
    /** @brief Erase raw bytes in the storage backend. */
    par_status_t (*erase)(const uint32_t addr, const uint32_t size);
    /** @brief Flush pending backend data to the final storage medium. */
    par_status_t (*sync)(void);
    /** @brief Optional backend name for diagnostics. */
    const char *name;
} par_store_backend_api_t;

/**
 * @brief Resolve the active parameter-storage backend API.
 *
 * @details Link exactly one concrete implementation when `PAR_CFG_NVM_EN = 1`.
 * The package can provide the GeneralEmbeddedCLibraries/nvm adapter, or the
 * application can provide its own implementation.
 *
 * @return Pointer to backend API, or NULL if no backend is available.
 */
const par_store_backend_api_t * par_store_backend_get_api(void);
#endif /* 1 == PAR_CFG_NVM_EN */

#endif /* _PAR_STORE_BACKEND_H_ */
