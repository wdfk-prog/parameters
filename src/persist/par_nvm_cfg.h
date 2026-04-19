/**
 * @file par_nvm_cfg.h
 * @brief Provide compile-time configuration defaults for parameter persistence.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-04-19
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-04-19 1.0     wdfk-prog    first version
 */
#ifndef _PAR_NVM_CFG_H_
#define _PAR_NVM_CFG_H_

/**
 * @brief Enable/Disable storing persistent parameters to NVM.
 *
 * @note This switch is also the single compile-time gate for persistence
 * metadata in the parameter table. There is no separate PAR_CFG_ENABLE_PERSIST
 * override anymore.
 */
#ifndef PAR_CFG_NVM_EN
#define PAR_CFG_NVM_EN (1)
#endif

/**
 * @brief Enable/Disable the legacy GeneralEmbeddedCLibraries/nvm backend.
 *
 * @note Keep disabled when the application provides an out-of-package storage
 * backend, such as the RT-Thread AT24CXX adapter.
 */
#ifndef PAR_CFG_NVM_BACKEND_GEL_EN
#define PAR_CFG_NVM_BACKEND_GEL_EN (0)
#endif

/**
 * @brief Enable/Disable parameter-table compatibility checking.
 *
 * @note The stored NVM image header carries a table-ID digest that covers
 * PAR_CFG_TABLE_ID_SCHEMA_VER, selected record layout, and the stored
 * persistent prefix. Self-describing layouts include parameter IDs in that
 * digest. Payload-only layouts intentionally exclude external parameter IDs
 * and hash only prefix count, persistent order, and parameter type so stored
 * prefixes with identical byte layout remain compatible.
 *
 * When enabled, any persisted-layout incompatibility is treated as a managed
 * schema change: startup restores defaults and rebuilds the managed NVM image.
 * Layouts with stable prefix addresses allow compatible tail-slot growth when
 * the stored prefix still matches the live prefix. The grouped payload-only
 * layout is excluded from that repair path and rebuilds on any stored/live
 * count mismatch. Payload-only layouts therefore still require the integrator
 * to bump PAR_CFG_TABLE_ID_SCHEMA_VER whenever a prefix parameter is
 * semantically remapped without changing its serialized byte layout.
 *
 * @pre "PAR_CFG_NVM_EN" must be enabled, otherwise table-ID checking does.
 * not apply.
 */
#ifndef PAR_CFG_TABLE_ID_CHECK_EN
#define PAR_CFG_TABLE_ID_CHECK_EN (0)
#endif

/**
 * @brief Parameter-table ID schema version.
 *
 * @note Increase this value when the serialized table-ID composition changes.
 * The integrator owns this version number and may override it in
 * port/par_cfg_port.h before this header provides the default.
 */
#ifndef PAR_CFG_TABLE_ID_SCHEMA_VER
#define PAR_CFG_TABLE_ID_SCHEMA_VER (1U)
#endif

#include "persist/backend/par_store_backend_flash_ee_cfg.h"

/**
 * @brief Enable/Disable write-path readback verification for persisted data.
 *
 * @details When enabled, each persisted-record write and each header write are
 * followed by a backend sync and a readback verification step. This improves
 * reliability at the cost of additional latency and backend traffic.
 */
#ifndef PAR_CFG_NVM_WRITE_VERIFY_EN
#define PAR_CFG_NVM_WRITE_VERIFY_EN (0)
#endif

/**
 * @brief Select persisted record layout.
 *
 * @note The chosen layout is also included in the table-ID digest so layout
 * changes are treated as managed compatibility changes.
 */
#define PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE (0U)
#define PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE   (1U)
#define PAR_CFG_NVM_RECORD_LAYOUT_COMPACT_PAYLOAD      (2U)
#define PAR_CFG_NVM_RECORD_LAYOUT_FIXED_PAYLOAD_ONLY   (3U)
#define PAR_CFG_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY (4U)

#ifndef PAR_CFG_NVM_RECORD_LAYOUT
#define PAR_CFG_NVM_RECORD_LAYOUT (PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE)
#endif

/**
 * @brief Derived layout capability: serialized records store a parameter ID.
 */
#if (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE) || \
    (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE) ||   \
    (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_COMPACT_PAYLOAD)
#define PAR_CFG_NVM_RECORD_LAYOUT_HAS_STORED_ID (1)
#else
#define PAR_CFG_NVM_RECORD_LAYOUT_HAS_STORED_ID (0)
#endif

/**
 * @brief Derived layout capability: serialized records store a size descriptor.
 */
#if (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE) || \
    (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_COMPACT_PAYLOAD)
#define PAR_CFG_NVM_RECORD_LAYOUT_HAS_SIZE_DESC (1)
#else
#define PAR_CFG_NVM_RECORD_LAYOUT_HAS_SIZE_DESC (0)
#endif

#endif /* _PAR_NVM_CFG_H_ */
