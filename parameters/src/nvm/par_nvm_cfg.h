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
#endif /* !defined(PAR_CFG_NVM_EN) */

/**
 * @brief Enable/Disable managed persistence for scalar parameters.
 *
 * @details Defaults to the top-level NVM enable state. When disabled, scalar
 * rows with pers_ == 1 are rejected by compile-time table checks. Object
 * persistence can still be enabled and may use either a shared backend address
 * space or a dedicated object backend.
 */
#ifndef PAR_CFG_NVM_SCALAR_EN
#define PAR_CFG_NVM_SCALAR_EN (PAR_CFG_NVM_EN)
#endif /* !defined(PAR_CFG_NVM_SCALAR_EN) */

/**
 * @brief Enable/Disable the legacy GeneralEmbeddedCLibraries/nvm backend.
 *
 * @note Keep disabled when the application provides an out-of-package storage
 * backend, such as the RT-Thread AT24CXX adapter.
 */
#ifndef PAR_CFG_NVM_BACKEND_GEL_EN
#define PAR_CFG_NVM_BACKEND_GEL_EN (0)
#endif /* !defined(PAR_CFG_NVM_BACKEND_GEL_EN) */

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
#endif /* !defined(PAR_CFG_TABLE_ID_CHECK_EN) */

/**
 * @brief Parameter-table ID schema version.
 *
 * @note Increase this value when the serialized table-ID composition changes.
 * The integrator owns this version number and may override it in
 * port/par_cfg_port.h before this header provides the default.
 */
#ifndef PAR_CFG_TABLE_ID_SCHEMA_VER
#define PAR_CFG_TABLE_ID_SCHEMA_VER (1U)
#endif /* !defined(PAR_CFG_TABLE_ID_SCHEMA_VER) */

#include "par_store_backend_flash_ee_cfg.h"

/**
 * @brief Enable/Disable write-path readback verification for persisted data.
 *
 * @details When enabled, each persisted-record write and each header write are
 * followed by a backend sync and a readback verification step. This improves
 * reliability at the cost of additional latency and backend traffic.
 */
#ifndef PAR_CFG_NVM_WRITE_VERIFY_EN
#define PAR_CFG_NVM_WRITE_VERIFY_EN (0)
#endif /* !defined(PAR_CFG_NVM_WRITE_VERIFY_EN) */

/**
 * @brief Enable/Disable managed persistence for object parameters.
 *
 * @details When disabled, object rows with pers_ == 1 are rejected by
 * compile-time table checks. When enabled, object payloads are stored in a
 * dedicated object persistence block.
 */
#ifndef PAR_CFG_NVM_OBJECT_EN
#define PAR_CFG_NVM_OBJECT_EN (0)
#endif /* !defined(PAR_CFG_NVM_OBJECT_EN) */

/**
 * @brief Use the same backend and address space for scalar and object persistence.
 *
 * @details In shared-store mode, the object block is addressed through the
 * scalar NVM backend. The selected object address mode decides whether the
 * object block follows the scalar block or starts at a fixed address.
 */
#ifndef PAR_CFG_NVM_OBJECT_STORE_SHARED
#define PAR_CFG_NVM_OBJECT_STORE_SHARED (0U)
#endif /* !defined(PAR_CFG_NVM_OBJECT_STORE_SHARED) */

/**
 * @brief Use a dedicated object backend or partition for object persistence.
 *
 * @details This mode lets scalar and object persistence use different media or
 * partitions. The port must provide par_object_store_backend_bind() and
 * par_object_store_backend_get_api(). Object addresses are relative to that
 * dedicated object backend address space, so scalar-layout growth cannot move
 * or overwrite object records.
 */
#ifndef PAR_CFG_NVM_OBJECT_STORE_DEDICATED
#define PAR_CFG_NVM_OBJECT_STORE_DEDICATED (1U)
#endif /* !defined(PAR_CFG_NVM_OBJECT_STORE_DEDICATED) */

/**
 * @brief Select the object persistence storage target.
 *
 * @details Defaults to PAR_CFG_NVM_OBJECT_STORE_SHARED to preserve the compact
 * single-backend behavior. Select PAR_CFG_NVM_OBJECT_STORE_DEDICATED when a
 * product reserves a separate EEPROM device, flash partition, or backend
 * instance for object payloads.
 */
#ifndef PAR_CFG_NVM_OBJECT_STORE_MODE
#define PAR_CFG_NVM_OBJECT_STORE_MODE PAR_CFG_NVM_OBJECT_STORE_SHARED
#endif /* !defined(PAR_CFG_NVM_OBJECT_STORE_MODE) */

/**
 * @brief Place the object persistence block immediately after the scalar NVM block.
 *
 * @details This compact shared-store mode requires no extra address
 * configuration. When the scalar persistent layout grows, the object block
 * base can move. The generic object persistence core intentionally does not
 * migrate object blocks between addresses because flash-class media requires
 * product-specific scratch or double-buffer storage, commit state,
 * power-loss recovery, and erase-block planning. Use fixed or dedicated
 * placement before release when object values must survive scalar-layout growth.
 */
#ifndef PAR_CFG_NVM_OBJECT_ADDR_AFTER_SCALAR
#define PAR_CFG_NVM_OBJECT_ADDR_AFTER_SCALAR (0U)
#endif /* !defined(PAR_CFG_NVM_OBJECT_ADDR_AFTER_SCALAR) */

/**
 * @brief Place the object persistence block at a fixed configured backend address.
 *
 * @details Use this mode when object values must remain independent from
 * scalar persistent-layout growth. The configured address must not overlap
 * the scalar NVM block in the same backend address space.
 */
#ifndef PAR_CFG_NVM_OBJECT_ADDR_FIXED
#define PAR_CFG_NVM_OBJECT_ADDR_FIXED (1U)
#endif /* !defined(PAR_CFG_NVM_OBJECT_ADDR_FIXED) */

/**
 * @brief Select object persistence block address placement mode.
 *
 * @details Defaults to PAR_CFG_NVM_OBJECT_ADDR_AFTER_SCALAR for backward
 * compatibility with the compact scalar-plus-object layout. Select
 * PAR_CFG_NVM_OBJECT_ADDR_FIXED for product layouts that reserve an
 * independent object address range.
 */
#ifndef PAR_CFG_NVM_OBJECT_ADDR_MODE
#define PAR_CFG_NVM_OBJECT_ADDR_MODE PAR_CFG_NVM_OBJECT_ADDR_AFTER_SCALAR
#endif /* !defined(PAR_CFG_NVM_OBJECT_ADDR_MODE) */

/**
 * @brief Fixed object persistence block base address in backend address space.
 *
 * @details Used only when PAR_CFG_NVM_OBJECT_STORE_MODE is shared and
 * PAR_CFG_NVM_OBJECT_ADDR_MODE is PAR_CFG_NVM_OBJECT_ADDR_FIXED. Use this when
 * scalar and object records share one backend but must not move together.
 */
#ifndef PAR_CFG_NVM_OBJECT_FIXED_ADDR
#define PAR_CFG_NVM_OBJECT_FIXED_ADDR (0U)
#endif /* !defined(PAR_CFG_NVM_OBJECT_FIXED_ADDR) */

/**
 * @brief Optional reserved size for the fixed object persistence region.
 *
 * @details Used only in fixed mode when non-zero. Initialization rejects the
 * current object block if it does not fit in this reserved region. Leave zero
 * to skip the region-size check and rely on backend capacity probing.
 */
#ifndef PAR_CFG_NVM_OBJECT_REGION_SIZE
#define PAR_CFG_NVM_OBJECT_REGION_SIZE (0U)
#endif /* !defined(PAR_CFG_NVM_OBJECT_REGION_SIZE) */

/**
 * @brief Base address of the object block in a dedicated object backend.
 *
 * @details Used only when PAR_CFG_NVM_OBJECT_STORE_MODE is
 * PAR_CFG_NVM_OBJECT_STORE_DEDICATED. The address is relative to the object
 * backend or partition selected by the port-level object backend hooks. Keep
 * this at zero when the object partition begins with the object persistence block.
 */
#ifndef PAR_CFG_NVM_OBJECT_DEDICATED_BASE_ADDR
#define PAR_CFG_NVM_OBJECT_DEDICATED_BASE_ADDR (0U)
#endif /* !defined(PAR_CFG_NVM_OBJECT_DEDICATED_BASE_ADDR) */

/**
 * @brief Enable/Disable write-path readback verification for object records.
 *
 * @details Defaults to scalar NVM write verification so object and scalar
 * persistence have matching reliability/latency behavior unless overridden.
 */
#ifndef PAR_CFG_NVM_OBJECT_WRITE_VERIFY_EN
#define PAR_CFG_NVM_OBJECT_WRITE_VERIFY_EN PAR_CFG_NVM_WRITE_VERIFY_EN
#endif /* !defined(PAR_CFG_NVM_OBJECT_WRITE_VERIFY_EN) */

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
#endif /* !defined(PAR_CFG_NVM_RECORD_LAYOUT) */

/**
 * @brief Derived layout capability: serialized records store a parameter ID.
 */
#if (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE) || \
    (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE) ||   \
    (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_COMPACT_PAYLOAD)
#define PAR_CFG_NVM_RECORD_LAYOUT_HAS_STORED_ID (1)
#else
#define PAR_CFG_NVM_RECORD_LAYOUT_HAS_STORED_ID (0)
#endif /* (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE) || (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_NO_SIZE) || (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_COMPACT_PAYLOAD) */

/**
 * @brief Derived layout capability: serialized records store a size descriptor.
 */
#if (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE) || \
    (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_COMPACT_PAYLOAD)
#define PAR_CFG_NVM_RECORD_LAYOUT_HAS_SIZE_DESC (1)
#else
#define PAR_CFG_NVM_RECORD_LAYOUT_HAS_SIZE_DESC (0)
#endif /* (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_FIXED_SLOT_WITH_SIZE) || (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_COMPACT_PAYLOAD) */

#endif /* !defined(_PAR_NVM_CFG_H_) */
