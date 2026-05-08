/**
 * @file par_store_backend.h
 * @brief Declare the abstract parameter-storage backend interface.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-03-29
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 * 
 * @details par_nvm.c uses this interface instead of depending directly on a
 * concrete NVM repository layout. Integrators may provide any storage backend
 * that supports the required byte-addressable operations.
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-03-29 1.0     wdfk-prog     first version
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
 *
 * If the selected storage medium provides ECC or other read-health reporting,
 * policy handling is intentionally left above this abstraction. The backend
 * may log or expose such information through its own implementation-specific
 * means, but the parameter core does not mandate whether the application must
 * rebuild, reset, continue running, or only report the event. That recovery
 * decision belongs to the business layer because acceptable behavior is
 * product-specific.
 */
typedef struct
{
    /**
     * @brief Initialize the storage backend.
     *
     * @details Prepare the reserved storage region for later byte-addressable
     * access. This may mount a partition, initialize a driver, or verify the
     * backend context required by the concrete implementation.
     *
     * @return ePAR_OK on success, otherwise an implementation-defined error.
     */
    par_status_t (*init)(void);
    /**
     * @brief Deinitialize the storage backend.
     *
     * @details Release resources acquired by @ref init when the parameter
     * module owns backend initialization.
     *
     * @return ePAR_OK on success, otherwise an implementation-defined error.
     */
    par_status_t (*deinit)(void);
    /**
     * @brief Query whether the storage backend is initialized.
     *
     * @param[out] p_is_init Receives the backend initialization state.
     * Must not be NULL. Invalid implementations shall report false
     * rather than returning an error status.
     */
    void (*is_init)(bool * const p_is_init);
    /**
     * @brief Read raw bytes from the storage backend.
     *
     * @param[in] addr Byte offset inside the reserved parameter-storage region.
     * @param[in] size Number of bytes to read.
     * @param[out] p_buf Destination buffer that receives @p size bytes.
     * Must not be NULL.
     *
     * @return ePAR_OK on success, otherwise an implementation-defined error.
     */
    par_status_t (*read)(const uint32_t addr, const uint32_t size, uint8_t * const p_buf);
    /**
     * @brief Write raw bytes to the storage backend.
     *
     * @param[in] addr Byte offset inside the reserved parameter-storage region.
     * @param[in] size Number of bytes to write.
     * @param[in] p_buf Source buffer that provides @p size bytes.
     * Must not be NULL.
     *
     * @return ePAR_OK on success, otherwise an implementation-defined error.
     */
    par_status_t (*write)(const uint32_t addr, const uint32_t size, const uint8_t * const p_buf);
    /**
     * @brief Erase raw bytes in the storage backend.
     *
     * @param[in] addr Byte offset inside the reserved parameter-storage region.
     * @param[in] size Number of bytes to erase.
     *
     * @return ePAR_OK on success, otherwise an implementation-defined error.
     */
    par_status_t (*erase)(const uint32_t addr, const uint32_t size);
    /**
     * @brief Flush pending backend data to the final storage medium.
     *
     * @details Call this after writes or erases when the backend may stage data
     * in RAM, caches, controller FIFOs, or deferred commit queues.
     *
     * @return ePAR_OK on success, otherwise an implementation-defined error.
     */
    par_status_t (*sync)(void);
    /**
     * @brief Optional backend name for diagnostics.
     *
     * @details Set to NULL when no human-readable backend name is available.
     */
    const char *name;
} par_store_backend_api_t;

/**
 * @brief Bind or prepare the active parameter-storage backend.
 *
 * @details Call this before @ref par_store_backend_get_api when the selected
 * backend needs to attach one concrete storage port, partition, or device
 * context. Pure backends that do not need a pre-bind step should return
 * @ref ePAR_OK without side effects.
 *
 * @return ePAR_OK on success, otherwise an implementation-defined error.
 */
par_status_t par_store_backend_bind(void);

/**
 * @brief Resolve the active parameter-storage backend API.
 *
 * @details Link exactly one concrete implementation when `PAR_CFG_NVM_EN = 1`.
 * The package can provide the GeneralEmbeddedCLibraries/nvm adapter, or the
 * application can provide its own implementation. This accessor must be side
 * effect free; any backend-specific binding work belongs in
 * @ref par_store_backend_bind.
 *
 * @return Pointer to backend API, or NULL if no backend is available.
 */
const par_store_backend_api_t * par_store_backend_get_api(void);

#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
/**
 * @brief Bind or prepare the dedicated object parameter-storage backend.
 *
 * @details Required only when PAR_CFG_NVM_OBJECT_STORE_MODE selects
 * PAR_CFG_NVM_OBJECT_STORE_DEDICATED. Ports that use shared object storage do
 * not need to provide a strong implementation.
 *
 * @return ePAR_OK on success, otherwise an implementation-defined error.
 */
par_status_t par_object_store_backend_bind(void);

/**
 * @brief Resolve the dedicated object parameter-storage backend API.
 *
 * @details Required only when PAR_CFG_NVM_OBJECT_STORE_MODE selects
 * PAR_CFG_NVM_OBJECT_STORE_DEDICATED. The returned API addresses the object
 * backend or partition selected by @ref par_object_store_backend_bind.
 *
 * @return Pointer to dedicated object backend API, or NULL if unavailable.
 */
const par_store_backend_api_t * par_object_store_backend_get_api(void);
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */
#endif /* (1 == PAR_CFG_NVM_EN) */

#endif /* !defined(_PAR_STORE_BACKEND_H_) */
