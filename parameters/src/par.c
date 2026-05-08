/**
 * @file par.c
 * @brief Implement the public device-parameter API.
 * @author Ziga Miklosic
 * @version V3.0.1
 * @date 2026-01-29
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-01-29 V3.0.1  Ziga Miklosic first version
 */
/**
 * @addtogroup PARAMETERS_API
 * @{ <!-- BEGIN GROUP -->
 */
/**
 * @brief Include dependencies.
 */
#include <stdbool.h>
#include <string.h>

#include "par.h"
#include "par_atomic.h"
#include "par_layout.h"
#include "par_id_map_static.h"
#include "par_object.h"
#include "par_nvm.h"
#include "par_if.h"
#include "par_core.h"
/**
 * @brief Compile-time definitions.
 */
#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
#define PAR_OBJECT_SLOT_TABLE_BYTES(layout_count_) (par_object_slot_table_bytes((layout_count_).count_obj))
#else
#define PAR_OBJECT_SLOT_TABLE_BYTES(layout_count_) (0u)
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */
/**
 * @brief Module-scope variables.
 */
/**
 * @brief Initialization guard.
 */
static bool gb_is_init = false;
/**
 * @brief Parameter callback table.
 * @note Keep runtime hooks separate from par_cfg_t metadata table.
 */
#if ((1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION) || (1 == PAR_CFG_ENABLE_CHANGE_CALLBACK))
static struct
{
#if (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION)
    union
    {
        pf_par_validation_t scalar;     /**< Scalar validation callback function (or NULL). */
#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
        pf_par_obj_validation_t object; /**< Object validation callback function (or NULL). */
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */
    } validation;                       /**< Shared validation callback function slot. */
#endif /* (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION) */
#if (1 == PAR_CFG_ENABLE_CHANGE_CALLBACK)
    pf_par_on_change_cb_t on_change;    /**< Scalar on-change callback function (or NULL). */
#endif /* (1 == PAR_CFG_ENABLE_CHANGE_CALLBACK) */
} g_par_cb_table[ePAR_NUM_OF];
#endif /* ((1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION) || (1 == PAR_CFG_ENABLE_CHANGE_CALLBACK)) */


#if (PAR_CFG_DEBUG_EN)

/**
 * @brief Status strings.
 */
static const char *gs_status[] = {
    "OK",

    "ERROR",
    "ERROR INIT",
    "ERROR NVM",
    "ERROR CRC",
    "ERROR TYPE",
    "ERROR MUTEX",
    "ERROR VALUE",
    "ERROR PARAM",
    "ERROR PAR NUM",
    "ERROR ACCESS",
    "ERROR TABLE ID",
    "WARN SET TO DEF",
    "WARN NVM REWRITTEN",
    "NO PERSISTENT",
    "LIMITED",
    "N/A",
};
#endif /* (PAR_CFG_DEBUG_EN) */
/**
 * @brief Function declarations and definitions.
 */
#if (1 == PAR_CFG_ENABLE_ACCESS)
/**
 * @brief Return whether the access mask contains read capability.
 */
bool par_core_access_has_read(const par_access_t access)
{
    return (0U != ((uint32_t)access & (uint32_t)ePAR_ACCESS_READ));
}

/**
 * @brief Return whether the access mask grants external write capability.
 * @details Writable parameters must also expose read capability; write-only
 * access masks are intentionally not supported.
 */
bool par_core_access_has_write(const par_access_t access)
{
    return (((uint32_t)ePAR_ACCESS_RW) == ((uint32_t)access & (uint32_t)ePAR_ACCESS_RW));
}
#endif /* (1 == PAR_CFG_ENABLE_ACCESS) */
#if (1 == PAR_CFG_ENABLE_DESC) && (1 == PAR_CFG_ENABLE_DESC_CHECK)
/**
 * @brief Validate parameter description string.
 *
 * @note Default weak implementation only prohibits comma character.
 * Application may override this symbol with stronger policy.
 *
 * @param p_desc Parameter description.
 * @return true if description is valid.
 */
PAR_PORT_WEAK bool par_port_is_desc_valid(const char * const p_desc)
{
    return ((NULL == p_desc) || (NULL == strchr(p_desc, ',')));
}
#endif /* (1 == PAR_CFG_ENABLE_DESC) && (1 == PAR_CFG_ENABLE_DESC_CHECK) */
/**
 * @brief Return one static parameter-table entry after number validation.
 *
 * @param par_num Parameter number.
 * @return Parameter configuration pointer, or NULL when @p par_num is invalid.
 */
static const par_cfg_t *par_core_get_table_entry(const par_num_t par_num)
{
    PAR_ASSERT(par_num < ePAR_NUM_OF);
    if (par_num >= (par_num_t)ePAR_NUM_OF)
    {
        return NULL;
    }

    return par_cfg_get(par_num);
}
/**
 * @brief Resolve metadata entry for parameter identified by number.
 *
 * @note Metadata resolution only checks parameter number and table.
 * entry. It intentionally does not require the runtime module.
 * to be initialized, because callers only read compile-time.
 * metadata.
 *
 * @param par_num Parameter number (enumeration).
 * @param p_arg Optional pointer argument to validate.
 * @param require_arg True if p_arg must not be NULL.
 * @param pp_cfg Optional output pointer to parameter configuration.
 * @return Status of operation.
 */
par_status_t par_core_resolve_metadata(const par_num_t par_num, const void * const p_arg, const bool require_arg, const par_cfg_t ** const pp_cfg)
{
    const par_cfg_t *p_cfg = NULL;

    if ((true == require_arg) && (NULL == p_arg))
    {
        return ePAR_ERROR_PARAM;
    }

    p_cfg = par_core_get_table_entry(par_num);
    if (NULL == p_cfg)
    {
        return ePAR_ERROR_PAR_NUM;
    }

    if (NULL != pp_cfg)
    {
        *pp_cfg = p_cfg;
    }

    return ePAR_OK;
}
/**
 * @brief Resolve runtime metadata entry for parameter identified by number.
 *
 * @note Runtime resolution extends metadata resolution with module.
 * init state validation, because live parameter storage is only.
 * valid after successful par_init().
 *
 * @param par_num Parameter number (enumeration).
 * @param p_arg Optional pointer argument to validate.
 * @param require_arg True if p_arg must not be NULL.
 * @param pp_cfg Optional output pointer to parameter configuration.
 * @return Status of operation.
 */
par_status_t par_core_resolve_runtime(const par_num_t par_num, const void * const p_arg, const bool require_arg, const par_cfg_t ** const pp_cfg)
{
    if (true != par_is_init())
    {
        return ePAR_ERROR_INIT;
    }

    return par_core_resolve_metadata(par_num, p_arg, require_arg, pp_cfg);
}
#if (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION)
/**
 * @brief Run the registered scalar validation callback, when present.
 *
 * @param par_num Parameter number.
 * @param val Candidate scalar value.
 * @return true when no callback is registered or the callback accepts the value.
 */
bool par_core_scalar_validation_accepts(const par_num_t par_num, const par_type_t val)
{
    PAR_ASSERT(par_num < ePAR_NUM_OF);
    if (par_num >= (par_num_t)ePAR_NUM_OF)
    {
        return false;
    }

    if (NULL == g_par_cb_table[par_num].validation.scalar)
    {
        return true;
    }

    return g_par_cb_table[par_num].validation.scalar(par_num, val);
}
#endif /* (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION) */

#if (1 == PAR_CFG_ENABLE_CHANGE_CALLBACK)
/**
 * @brief Notify the registered scalar on-change callback when the value changed.
 *
 * @param par_num Parameter number.
 * @param new_val New scalar value.
 * @param old_val Previous scalar value.
 * @param p_value_changed Optional precomputed changed-state pointer.
 * @note This helper is called after a successful checked scalar write while
 * the setter still owns the parameter lock. Registered callbacks must not
 * re-enter parameter APIs for the same parameter. Cross-parameter updates must
 * be protected by an application-defined lock-order policy.
 */
void par_core_notify_scalar_change_if_changed(const par_num_t par_num,
                                              const par_type_t new_val,
                                              const par_type_t old_val,
                                              const bool * const p_value_changed)
{
    const par_cfg_t *p_cfg = NULL;
    bool value_changed = false;

    PAR_ASSERT(par_num < ePAR_NUM_OF);
    if ((par_num >= (par_num_t)ePAR_NUM_OF) ||
        (NULL == g_par_cb_table[par_num].on_change))
    {
        return;
    }

    if (NULL != p_value_changed)
    {
        value_changed = *p_value_changed;
    }
    else
    {
        p_cfg = par_core_get_table_entry(par_num);
        if (NULL == p_cfg)
        {
            return;
        }

        switch (p_cfg->type)
        {
        case ePAR_TYPE_U8:
            value_changed = (new_val.u8 != old_val.u8);
            break;

        case ePAR_TYPE_I8:
            value_changed = (new_val.i8 != old_val.i8);
            break;

        case ePAR_TYPE_U16:
            value_changed = (new_val.u16 != old_val.u16);
            break;

        case ePAR_TYPE_I16:
            value_changed = (new_val.i16 != old_val.i16);
            break;

        case ePAR_TYPE_U32:
            value_changed = (new_val.u32 != old_val.u32);
            break;

        case ePAR_TYPE_I32:
            value_changed = (new_val.i32 != old_val.i32);
            break;

#if (1 == PAR_CFG_ENABLE_TYPE_F32)
        case ePAR_TYPE_F32:
            value_changed = !par_core_scalar_f32_bits_equal(new_val.f32, old_val.f32);
            break;
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */

        case ePAR_TYPE_NUM_OF:
        default:
            PAR_ASSERT(0);
            break;
        }
    }

    if (true == value_changed)
    {
        g_par_cb_table[par_num].on_change(par_num, new_val, old_val);
    }
}
#endif /* (1 == PAR_CFG_ENABLE_CHANGE_CALLBACK) */

#if (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
/**
 * @brief Run the registered object validation callback, when present.
 *
 * @param par_num Parameter number.
 * @param p_data Pointer to candidate object payload bytes.
 * @param len Candidate object payload length in bytes.
 * @return true when no callback is registered or the callback accepts the payload.
 */
bool par_core_object_validation_accepts(const par_num_t par_num,
                                        const uint8_t * const p_data,
                                        const uint16_t len)
{
    PAR_ASSERT(par_num < ePAR_NUM_OF);
    if (par_num >= (par_num_t)ePAR_NUM_OF)
    {
        return false;
    }

    if (NULL == g_par_cb_table[par_num].validation.object)
    {
        return true;
    }

    return g_par_cb_table[par_num].validation.object(par_num, p_data, len);
}

/**
 * @brief Register one object validation callback in the shared callback table.
 *
 * @param par_num Parameter number.
 * @param validation Object validation callback function pointer.
 */
void par_core_register_obj_validation(const par_num_t par_num,
                                      const pf_par_obj_validation_t validation)
{
    const par_cfg_t * const p_cfg = par_core_get_table_entry(par_num);

    if (NULL == p_cfg)
    {
        return;
    }

    if (false == par_object_type_is_object(p_cfg->type))
    {
        PAR_ASSERT(0);
        return;
    }

    g_par_cb_table[par_num].validation.object = validation;
}
#endif /* (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

#if (1 == PAR_CFG_ENABLE_TYPE_F32)
/**
 * @brief Compare two scalar F32 values by raw bit pattern.
 *
 * @note This helper intentionally uses memcpy() instead of pointer.
 * casting or union type-punning. memcpy() preserves the exact.
 * IEEE-754 bit pattern while remaining strict-aliasing safe.
 * Bitwise comparison keeps NaN payloads and signed-zero handling.
 * deterministic for parameter storage use cases.
 *
 * @param lhs Left-hand scalar float value.
 * @param rhs Right-hand scalar float value.
 * @return true if raw 32-bit representations are equal.
 */
bool par_core_scalar_f32_bits_equal(const float32_t lhs, const float32_t rhs)
{
    uint32_t lhs_bits = 0U;
    uint32_t rhs_bits = 0U;

    memcpy(&lhs_bits, &lhs, sizeof(lhs_bits));
    memcpy(&rhs_bits, &rhs, sizeof(rhs_bits));

    return (lhs_bits == rhs_bits);
}


#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */
/**
 * @brief Bind static space for live parameter values.
 */
static void par_bind_storage_layout(void)
{
    par_layout_init();
    const par_layout_count_t layout_count = par_layout_get_count();
    (void)layout_count;

    PAR_DBG_PRINT("PAR: layout bound, count8=%u count16=%u count32=%u count_obj=%u obj_pool=%lu",
                  (unsigned)layout_count.count8,
                  (unsigned)layout_count.count16,
                  (unsigned)layout_count.count32,
                  (unsigned)layout_count.count_obj,
                  (unsigned long)layout_count.obj_pool_bytes);
    PAR_DBG_PRINT("PAR: total RAM consumption for parameter values: %lu bytes",
                  (unsigned long)(((uint32_t)layout_count.count32 * 4u) +
                                  ((uint32_t)layout_count.count16 * 2u) +
                                  ((uint32_t)layout_count.count8) +
                                  layout_count.obj_pool_bytes +
                                  PAR_OBJECT_SLOT_TABLE_BYTES(layout_count)));
}
/**
 * @brief Hash parameter ID to bucket index.
 *
 * @param id Parameter ID.
 * @return hash index.
 */
#if (1 == PAR_CFG_ENABLE_ID)
static inline uint32_t par_hash_id(const uint16_t id)
{
    return PAR_HASH_ID_CONST(id);
}

#if ((1 == PAR_CFG_ENABLE_RUNTIME_ID_DUP_CHECK) || (1 == PAR_CFG_ENABLE_RUNTIME_ID_HASH_COLLISION_CHECK))
/**
 * @brief Run optional runtime diagnostics on the compiled parameter ID table.
 *
 * @note Static ID-map generation and compile-time conflict checks are.
 * always enabled when PAR_CFG_ENABLE_ID = 1. This function exists.
 * only to provide runtime diagnostics and clearer conflict logs.
 *
 * @param p_par_cfg Pointer to parameters table.
 * @return Status of operation.
 */
static par_status_t par_runtime_validate_id_table(const par_cfg_t * const p_par_cfg)
{
    par_id_map_entry_t diag_map[PAR_ID_HASH_SIZE];
    memset(diag_map, 0, sizeof(diag_map));

    for (par_num_t par_num = 0; par_num < ePAR_NUM_OF; par_num++)
    {
        const uint16_t id = p_par_cfg[par_num].id;
        const uint32_t bucket_idx = par_hash_id(id);
        par_id_map_entry_t * const bucket = &diag_map[bucket_idx];

        if (0u == bucket->used)
        {
            bucket->used = 1u;
            bucket->id = id;
            bucket->par_num = par_num;
            continue;
        }

#if (1 == PAR_CFG_ENABLE_RUNTIME_ID_DUP_CHECK)
        if (bucket->id == id)
        {
            PAR_ERR_PRINT("PAR: duplicate parameter ID %u detected during runtime diagnostic scan", (unsigned)id);
            PAR_ASSERT(0);
            return ePAR_ERROR_INIT;
        }
#endif /* (1 == PAR_CFG_ENABLE_RUNTIME_ID_DUP_CHECK) */

#if (1 == PAR_CFG_ENABLE_RUNTIME_ID_HASH_COLLISION_CHECK)
        if (bucket->id != id)
        {
            PAR_ERR_PRINT("PAR: hash collision detected during runtime diagnostic scan, id=%u conflicts_with=%u bucket=%u",
                          (unsigned)id,
                          (unsigned)bucket->id,
                          (unsigned)bucket_idx);
            PAR_ASSERT(0);
            return ePAR_ERROR_INIT;
        }
#endif /* (1 == PAR_CFG_ENABLE_RUNTIME_ID_HASH_COLLISION_CHECK) */
    }

    return ePAR_OK;
}
#endif /* ((1 == PAR_CFG_ENABLE_RUNTIME_ID_DUP_CHECK) || (1 == PAR_CFG_ENABLE_RUNTIME_ID_HASH_COLLISION_CHECK)) */
#endif /* (1 == PAR_CFG_ENABLE_ID) */
/**
 * @brief Check that parameter table is correctly defined.
 *
 * @param p_par_cfg Pointer to parameters table.
 * @return Status of operation.
 */
static par_status_t par_check_table_validity(const par_cfg_t * const p_par_cfg)
{
    par_status_t status = ePAR_OK;

#if (1 == PAR_CFG_ENABLE_ID) && ((1 == PAR_CFG_ENABLE_RUNTIME_ID_DUP_CHECK) || (1 == PAR_CFG_ENABLE_RUNTIME_ID_HASH_COLLISION_CHECK))
    status = par_runtime_validate_id_table(p_par_cfg);
    if (ePAR_OK != status)
    {
        return status;
    }
#endif /* (1 == PAR_CFG_ENABLE_ID) && ((1 == PAR_CFG_ENABLE_RUNTIME_ID_DUP_CHECK) || (1 == PAR_CFG_ENABLE_RUNTIME_ID_HASH_COLLISION_CHECK)) */

    for (uint32_t i = 0; i < ePAR_NUM_OF; i++)
    {
#if (1 == PAR_CFG_ENABLE_RUNTIME_TABLE_CHECK) && (1 == PAR_CFG_ENABLE_RANGE) && (1 == PAR_CFG_ENABLE_TYPE_F32)
        /*
         * Keep optional F32 range/default validation at runtime.
         */
        if (ePAR_TYPE_F32 == p_par_cfg[i].type)
        {
            PAR_ASSERT(((p_par_cfg[i].value_cfg.scalar.range.min.f32 < p_par_cfg[i].value_cfg.scalar.range.max.f32) &&
                        (p_par_cfg[i].value_cfg.scalar.def.f32 <= p_par_cfg[i].value_cfg.scalar.range.max.f32)) &&
                       (p_par_cfg[i].value_cfg.scalar.range.min.f32 <= p_par_cfg[i].value_cfg.scalar.def.f32));
        }
#endif /* (1 == PAR_CFG_ENABLE_RUNTIME_TABLE_CHECK) && (1 == PAR_CFG_ENABLE_RANGE) && (1 == PAR_CFG_ENABLE_TYPE_F32) */
#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
        if ((true == par_object_type_is_object(p_par_cfg[i].type)) &&
            (false == par_object_default_cfg_is_valid(&p_par_cfg[i].value_cfg.object)))
        {
            status = ePAR_ERROR_INIT;
            PAR_ERR_PRINT("PAR: object parameter %u metadata invalid", (unsigned)i);
            PAR_ASSERT(0);
            break;
        }
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

#if (1 == PAR_CFG_ENABLE_NAME)
        if (NULL == p_par_cfg[i].name)
        {
            status = ePAR_ERROR_INIT;
            PAR_ERR_PRINT("PAR: parameter %u name missing", (unsigned)i);
            PAR_ASSERT(0);
            break;
        }
#endif /* (1 == PAR_CFG_ENABLE_NAME) */

#if (1 == PAR_CFG_ENABLE_DESC)
        if (NULL == p_par_cfg[i].desc)
        {
            status = ePAR_ERROR_INIT;
            PAR_ERR_PRINT("PAR: parameter %u description missing", (unsigned)i);
            PAR_ASSERT(0);
            break;
        }
#endif /* (1 == PAR_CFG_ENABLE_DESC) */

#if (1 == PAR_CFG_ENABLE_DESC) && (1 == PAR_CFG_ENABLE_DESC_CHECK)
        if (false == par_port_is_desc_valid(p_par_cfg[i].desc))
        {
            status = ePAR_ERROR_INIT;
            PAR_ERR_PRINT("PAR: parameter %u description invalid", (unsigned)i);
            PAR_ASSERT(0);
            break;
        }
#endif /* (1 == PAR_CFG_ENABLE_DESC) && (1 == PAR_CFG_ENABLE_DESC_CHECK) */
    }

    return status;
}
/**
 * @} <!-- END GROUP -->
 */

/**
 * @addtogroup API_FUNCTIONS
 * @{ <!-- BEGIN GROUP -->
 *
 * @brief Following function are part of Device Parameter module API.
 */

/**
 * @brief Initialize the Device Parameters module.
 *
 * @details Initialization validates the parameter table, binds storage layout,
 * prepares object defaults when object rows are enabled, and initializes the
 * platform interface only after local storage setup has succeeded.
 *
 * @return Status of initialization.
 */
par_status_t par_init(void)
{
    par_status_t status = ePAR_OK;

    PAR_ASSERT(false == par_is_init());
    if (false != par_is_init())
        return ePAR_ERROR_INIT;

    PAR_DBG_PRINT("PAR: initialization started");
    status |= par_check_table_validity(par_cfg_get_table());
    par_bind_storage_layout();
#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
    if (ePAR_OK == status)
    {
        status |= par_object_init_defaults_from_table();
    }
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */
    if (ePAR_OK == status)
    {
        status |= par_if_init();
    }
    PAR_ASSERT(ePAR_OK == status);
    if (ePAR_OK == status)
    {
        gb_is_init = true;
        /* Patch F32 defaults after layout offsets become available. */
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
        par_core_scalar_patch_f32_defaults_from_table();
        PAR_DBG_PRINT("PAR: F32 defaults patched into 32-bit storage group");
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */

#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
        PAR_DBG_PRINT("PAR: object defaults patched into object storage pool");
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

#if (1 == PAR_CFG_ENABLE_RESET_ALL_RAW)
        /* Snapshot defaults before optional NVM restore. */
        par_core_scalar_snapshot_default_mirror();
#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
        par_object_snapshot_default_mirror();
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */
        PAR_DBG_PRINT("PAR: default mirror snapshot captured for raw reset path");
#endif /* (1 == PAR_CFG_ENABLE_RESET_ALL_RAW) */

#if (1 == PAR_CFG_NVM_EN)
        /* Restore persisted values after default initialization. */
        PAR_DBG_PRINT("PAR: restoring persistent values from NVM");
        status |= par_nvm_init();
#endif /* (1 == PAR_CFG_NVM_EN) */
    }

    PAR_INFO_PRINT("PAR: initialization finished with status=%s", par_get_status_str(status));

    return status;
}
/**
 * @brief De-initialize Device Parameters.
 *
 * @return Status of de-initialization.
 */
par_status_t par_deinit(void)
{
    par_status_t status = ePAR_OK;
    par_status_t deinit_status = ePAR_OK;

    PAR_ASSERT(true == par_is_init());
    if (true != par_is_init())
        return ePAR_ERROR_INIT;

    PAR_DBG_PRINT("PAR: deinitialization started");
#if (1 == PAR_CFG_NVM_EN)
    deinit_status = par_nvm_deinit();
    status |= deinit_status;
#endif /* (1 == PAR_CFG_NVM_EN) */

    deinit_status = par_if_deinit();
    status |= deinit_status;
    gb_is_init = false;
    PAR_INFO_PRINT("PAR: deinitialization finished with status=%s", par_get_status_str(status));

    return status;
}
/**
 * @brief Get initialization flag.
 *
 * @return Initialization state.
 */
bool par_is_init(void)
{
    return gb_is_init;
}
/**
 * @brief Try to acquire mutex for specified parameter.
 *
 * @param par_num Parameter number (enumeration).
 * @return Status of operation.
 */
par_status_t par_acquire_mutex(const par_num_t par_num)
{
    return par_if_aquire_mutex(par_num);
}
/**
 * @brief Try to acquire mutex for specified parameter.
 *
 * @param par_num Parameter number (enumeration).
 */
void par_release_mutex(const par_num_t par_num)
{
    par_if_release_mutex(par_num);
}
/**
 * @brief Set parameter to default value.
 *
 * @pre    Parameters must be initialized before usage.
 *
 * @param par_num Parameter number.
 * @return Status of operation.
 * @note Default restore is a maintenance/recovery operation. It intentionally
 * bypasses external write access and role-policy checks, plus runtime validation
 * and on-change callbacks.
 */
par_status_t par_set_to_default(const par_num_t par_num)
{
    const par_cfg_t *par_cfg = NULL;
    par_status_t status = par_core_resolve_runtime(par_num, NULL, false, &par_cfg);

    if (ePAR_OK != status)
    {
        return status;
    }

    if (ePAR_OK != par_acquire_mutex(par_num))
    {
        return ePAR_ERROR_MUTEX;
    }

    switch (par_cfg->type)
    {
    case ePAR_TYPE_U8:
        status = par_set_u8_fast(par_num, par_cfg->value_cfg.scalar.def.u8);
        break;

    case ePAR_TYPE_I8:
        status = par_set_i8_fast(par_num, par_cfg->value_cfg.scalar.def.i8);
        break;

    case ePAR_TYPE_U16:
        status = par_set_u16_fast(par_num, par_cfg->value_cfg.scalar.def.u16);
        break;

    case ePAR_TYPE_I16:
        status = par_set_i16_fast(par_num, par_cfg->value_cfg.scalar.def.i16);
        break;

    case ePAR_TYPE_U32:
        status = par_set_u32_fast(par_num, par_cfg->value_cfg.scalar.def.u32);
        break;

    case ePAR_TYPE_I32:
        status = par_set_i32_fast(par_num, par_cfg->value_cfg.scalar.def.i32);
        break;

#if (1 == PAR_CFG_ENABLE_TYPE_F32)
    case ePAR_TYPE_F32:
        status = par_set_f32_fast(par_num, par_cfg->value_cfg.scalar.def.f32);
        break;
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */

#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
    case ePAR_TYPE_STR:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
    case ePAR_TYPE_BYTES:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
    case ePAR_TYPE_ARR_U8:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
    case ePAR_TYPE_ARR_U16:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
    case ePAR_TYPE_ARR_U32:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */
    {
        status = par_object_write_default(par_num);
        break;
    }
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

    case ePAR_TYPE_NUM_OF:
    default:
        PAR_ASSERT(0);
        status = ePAR_ERROR_TYPE;
        break;
    }

    par_release_mutex(par_num);

    if (ePAR_OK == (status & ePAR_STATUS_ERROR_MASK))
    {
        PAR_DBG_PRINT("PAR: restored default value, par_num=%u", (unsigned)par_num);
    }
    else
    {
        PAR_ERR_PRINT("PAR: failed to restore default value, par_num=%u status=%s", (unsigned)par_num, par_get_status_str(status));
    }

    return status;
}

/**
 * @brief Set all parameters to default value.
 *
 * @pre          Parameters must be initialized before usage.
 * @return Status of operation.
 * @note Bulk default restore is a maintenance/recovery operation. It intentionally
 * bypasses external write access and role-policy checks.
 * @note When PAR_CFG_ENABLE_RESET_ALL_RAW = 1, this public API forwards to
 * par_reset_all_to_default_raw() for maximum reset speed. Otherwise it iterates
 * through par_set_to_default(), preserving fast default-restore semantics.
 */
par_status_t par_set_all_to_default(void)
{
#if (1 == PAR_CFG_ENABLE_RESET_ALL_RAW)
    return par_reset_all_to_default_raw();
#else
    par_status_t status = ePAR_OK;

    if (true != par_is_init())
    {
        return ePAR_ERROR_INIT;
    }

    for (par_num_t par_num = 0; par_num < ePAR_NUM_OF; par_num++)
    {
        status |= par_set_to_default(par_num);
    }

    PAR_DBG_PRINT("PAR: setting all parameters to defaults");
    return status;
#endif /* (1 == PAR_CFG_ENABLE_RESET_ALL_RAW) */
}
/**
 * @brief Check if parameter changed from its default value.
 *
 * @param par_num Parameter number (enumeration).
 * @param p_has_changed Pointer to changed indication.
 * @return Status of operation.
 */
par_status_t par_has_changed(const par_num_t par_num, bool * const p_has_changed)
{
    const par_cfg_t *par_cfg = NULL;
    par_status_t status = ePAR_OK;

    status = par_core_resolve_runtime(par_num, p_has_changed, true, &par_cfg);
    if (ePAR_OK != status)
    {
        return status;
    }

    switch (par_cfg->type)
    {
    case ePAR_TYPE_U8:
    {
        uint8_t cur = 0U;
        const par_status_t status = par_get_u8(par_num, &cur);
        if (ePAR_OK != status)
            return status;
        *p_has_changed = (cur != par_cfg->value_cfg.scalar.def.u8);
        break;
    }

    case ePAR_TYPE_I8:
    {
        int8_t cur = 0;
        const par_status_t status = par_get_i8(par_num, &cur);
        if (ePAR_OK != status)
            return status;
        *p_has_changed = (cur != par_cfg->value_cfg.scalar.def.i8);
        break;
    }

    case ePAR_TYPE_U16:
    {
        uint16_t cur = 0U;
        const par_status_t status = par_get_u16(par_num, &cur);
        if (ePAR_OK != status)
            return status;
        *p_has_changed = (cur != par_cfg->value_cfg.scalar.def.u16);
        break;
    }

    case ePAR_TYPE_I16:
    {
        int16_t cur = 0;
        const par_status_t status = par_get_i16(par_num, &cur);
        if (ePAR_OK != status)
            return status;
        *p_has_changed = (cur != par_cfg->value_cfg.scalar.def.i16);
        break;
    }

    case ePAR_TYPE_U32:
    {
        uint32_t cur = 0U;
        const par_status_t status = par_get_u32(par_num, &cur);
        if (ePAR_OK != status)
            return status;
        *p_has_changed = (cur != par_cfg->value_cfg.scalar.def.u32);
        break;
    }

    case ePAR_TYPE_I32:
    {
        int32_t cur = 0;
        const par_status_t status = par_get_i32(par_num, &cur);
        if (ePAR_OK != status)
            return status;
        *p_has_changed = (cur != par_cfg->value_cfg.scalar.def.i32);
        break;
    }

#if (1 == PAR_CFG_ENABLE_TYPE_F32)
    case ePAR_TYPE_F32:
    {
        float32_t cur = 0.0f;
        const par_status_t status = par_get_f32(par_num, &cur);
        if (ePAR_OK != status)
            return status;
        *p_has_changed = !par_core_scalar_f32_bits_equal(cur, par_cfg->value_cfg.scalar.def.f32);
        break;
    }
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */

#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
    case ePAR_TYPE_STR:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
    case ePAR_TYPE_BYTES:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
    case ePAR_TYPE_ARR_U8:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
    case ePAR_TYPE_ARR_U16:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
    case ePAR_TYPE_ARR_U32:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */
    {
#if (1 == PAR_CFG_ENABLE_ACCESS)
        if (false == par_core_access_has_read(par_cfg->access))
        {
            return ePAR_ERROR_ACCESS;
        }
#endif /* (1 == PAR_CFG_ENABLE_ACCESS) */
        if (ePAR_OK != par_acquire_mutex(par_num))
        {
            return ePAR_ERROR_MUTEX;
        }

        status = par_object_payload_changed_from_default(par_num, p_has_changed);
        par_release_mutex(par_num);
        if (ePAR_OK != status)
        {
            return status;
        }
        break;
    }
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

    case ePAR_TYPE_NUM_OF:
    default:
        PAR_ASSERT(0);
        return ePAR_ERROR_TYPE;
    }

    return ePAR_OK;
}
/**
 * @brief Get parameter configurations.
 *
 * @note In case parameter is not found it return NULL!
 *
 * @param par_num Parameter number (enumeration).
 * @return Parameter configuration.
 */
const par_cfg_t *par_get_config(const par_num_t par_num)
{
    return par_core_get_table_entry(par_num);
}
/**
 * @brief Get parameter name.
 *
 * @param par_num Parameter number (enumeration).
 * @return Parameter name.
 */
#if (1 == PAR_CFG_ENABLE_NAME)
const char *par_get_name(const par_num_t par_num)
{
    const par_cfg_t * const par_cfg = par_core_get_table_entry(par_num);

    if (NULL != par_cfg)
    {
        return par_cfg->name;
    }

    return NULL;
}
#endif /* (1 == PAR_CFG_ENABLE_NAME) */
/**
 * @brief Get parameter value range.
 *
 * @param par_num Parameter number (enumeration).
 * @return Parameter min/max range.
 */
#if (1 == PAR_CFG_ENABLE_RANGE)
par_range_t par_get_range(const par_num_t par_num)
{
    par_range_t range = { 0 };
    const par_cfg_t * const par_cfg = par_core_get_table_entry(par_num);

    if (NULL == par_cfg)
    {
        return range;
    }
#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
    if (true == par_object_type_is_object(par_cfg->type))
    {
        return range;
    }
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

    return par_cfg->value_cfg.scalar.range;
}
#endif /* (1 == PAR_CFG_ENABLE_RANGE) */
/**
 * @brief Get parameter unit.
 *
 * @param par_num Parameter number (enumeration).
 * @return Parameter unit.
 */
#if (1 == PAR_CFG_ENABLE_UNIT)
const char *par_get_unit(const par_num_t par_num)
{
    const par_cfg_t * const par_cfg = par_core_get_table_entry(par_num);

    if (NULL != par_cfg)
    {
        return par_cfg->unit;
    }

    return NULL;
}
#endif /* (1 == PAR_CFG_ENABLE_UNIT) */
/**
 * @brief Get parameter description.
 *
 * @param par_num Parameter number (enumeration).
 * @return Parameter description.
 */
#if (1 == PAR_CFG_ENABLE_DESC)
const char *par_get_desc(const par_num_t par_num)
{
    const par_cfg_t * const par_cfg = par_core_get_table_entry(par_num);

    if (NULL != par_cfg)
    {
        return par_cfg->desc;
    }

    return NULL;
}
#endif /* (1 == PAR_CFG_ENABLE_DESC) */
/**
 * @brief Get parameter type.
 *
 * @param par_num Parameter number (enumeration).
 * @return Parameter data type.
 */
par_type_list_t par_get_type(const par_num_t par_num)
{
    const par_cfg_t * const par_cfg = par_core_get_table_entry(par_num);

    if (NULL != par_cfg)
    {
        return par_cfg->type;
    }

    return ePAR_TYPE_NUM_OF;
}
/**
 * @brief Get parameter external capability mask.
 *
 * @param par_num Parameter number (enumeration).
 * @return Parameter access.
 */
#if (1 == PAR_CFG_ENABLE_ACCESS)
par_access_t par_get_access(const par_num_t par_num)
{
    const par_cfg_t * const par_cfg = par_core_get_table_entry(par_num);

    if (NULL != par_cfg)
    {
        return par_cfg->access;
    }

    return ePAR_ACCESS_NONE;
}
#endif /* (1 == PAR_CFG_ENABLE_ACCESS) */
#if (1 == PAR_CFG_ENABLE_ROLE_POLICY)
/**
 * @brief Get the configured read-role mask for one parameter.
 * @param par_num Parameter number.
 * @return Configured read-role mask, or ePAR_ROLE_NONE on lookup failure.
 */
par_role_t par_get_read_roles(const par_num_t par_num)
{
    const par_cfg_t * const par_cfg = par_core_get_table_entry(par_num);

    if (NULL != par_cfg)
    {
        return par_cfg->read_roles;
    }

    return ePAR_ROLE_NONE;
}

/**
 * @brief Get the configured write-role mask for one parameter.
 * @param par_num Parameter number.
 * @return Configured write-role mask, or ePAR_ROLE_NONE on lookup failure.
 */
par_role_t par_get_write_roles(const par_num_t par_num)
{
    const par_cfg_t * const par_cfg = par_core_get_table_entry(par_num);

    if (NULL != par_cfg)
    {
        return par_cfg->write_roles;
    }

    return ePAR_ROLE_NONE;
}

/**
 * @brief Validate that a role mask contains only supported role bits.
 * @param roles Role mask to validate.
 * @return True when the role mask contains only supported bits.
 */
static bool par_roles_are_valid(const par_role_t roles)
{
    return (0U == ((uint32_t)roles & (uint32_t)(~((uint32_t)ePAR_ROLE_ALL))));
}

/**
 * @brief Check whether a role mask can read one parameter.
 * @param par_num Parameter number.
 * @param roles Candidate external role mask.
 * @return True when the role mask is valid and read access is allowed.
 */
bool par_can_read(const par_num_t par_num, const par_role_t roles)
{
    const par_cfg_t *par_cfg = NULL;

    if (false == par_roles_are_valid(roles))
    {
        return false;
    }

    par_cfg = par_core_get_table_entry(par_num);
    if (NULL == par_cfg)
    {
        return false;
    }

#if (1 == PAR_CFG_ENABLE_ACCESS)
    if (false == par_core_access_has_read(par_cfg->access))
    {
        return false;
    }
#endif /* (1 == PAR_CFG_ENABLE_ACCESS) */

    return (0U != ((uint32_t)par_cfg->read_roles & (uint32_t)roles));
}

/**
 * @brief Check whether a role mask can write one parameter.
 * @param par_num Parameter number.
 * @param roles Candidate external role mask.
 * @return True when the role mask is valid and write access is allowed.
 */
bool par_can_write(const par_num_t par_num, const par_role_t roles)
{
    const par_cfg_t *par_cfg = NULL;

    if (false == par_roles_are_valid(roles))
    {
        return false;
    }

    par_cfg = par_core_get_table_entry(par_num);
    if (NULL == par_cfg)
    {
        return false;
    }

#if (1 == PAR_CFG_ENABLE_ACCESS)
    if (false == par_core_access_has_write(par_cfg->access))
    {
        return false;
    }
#endif /* (1 == PAR_CFG_ENABLE_ACCESS) */

    return (0U != ((uint32_t)par_cfg->write_roles & (uint32_t)roles));
}

#endif /* (1 == PAR_CFG_ENABLE_ROLE_POLICY) */
/**
 * @brief Is parameter persistent (does it store to NVM).
 *
 * @param par_num Parameter number (enumeration).
 * @return True if parameter is persistent.
 */
#if (1 == PAR_CFG_NVM_EN)
bool par_is_persistent(const par_num_t par_num)
{
    const par_cfg_t * const par_cfg = par_core_get_table_entry(par_num);

    if (NULL != par_cfg)
    {
        return par_cfg->persistent;
    }

    return false;
}
#endif /* (1 == PAR_CFG_NVM_EN) */
/**
 * @brief Get parameter number (enumeration) by ID.
 *
 * @note This API reads the compile-time static ID map only.
 * It does not require par_init(), because it does not access.
 * runtime parameter storage.
 *
 * @param id Parameter ID.
 * @param p_par_num Pointer to parameter enumeration number.
 * @return Status of operation.
 */
#if (1 == PAR_CFG_ENABLE_ID)
par_status_t par_get_num_by_id(const uint16_t id, par_num_t * const p_par_num)
{
    if (NULL == p_par_num)
    {
        return ePAR_ERROR_PARAM;
    }

    const uint32_t bucket_idx = par_hash_id(id);
    const par_id_map_entry_t * const bucket = &g_par_id_map_static[bucket_idx];

    if ((0u != bucket->used) && (id == bucket->id))
    {
        if (bucket->par_num >= ePAR_NUM_OF)
        {
            return ePAR_ERROR_PAR_NUM;
        }

        *p_par_num = bucket->par_num;
        return ePAR_OK;
    }

    return ePAR_ERROR;
}
/**
 * @brief Get parameter ID by number (enumeration).
 *
 * @param par_num Parameter number.
 * @param p_id Pointer to parameter ID.
 * @return Status of operation.
 */
par_status_t par_get_id_by_num(const par_num_t par_num, uint16_t * const p_id)
{
    if (NULL == p_id)
    {
        return ePAR_ERROR_PARAM;
    }

    if (par_num >= ePAR_NUM_OF)
    {
        return ePAR_ERROR_PAR_NUM;
    }

    const par_cfg_t * const par_cfg = par_get_config(par_num);

    if (NULL != par_cfg)
    {
        *p_id = par_cfg->id;
        return ePAR_OK;
    }

    return ePAR_ERROR;
}
#endif /* (1 == PAR_CFG_ENABLE_ID) */

#if (1 == PAR_CFG_NVM_EN)
/**
 * @brief Store all scalar and object persistent parameters to NVM.
 *
 * @pre        NVM storage must be initialized first and "PAR_CFG_NVM_EN"
 * settings must be enabled.
 *
 * @return Status of operation.
 */
par_status_t par_save_all(void)
{
    PAR_ASSERT(true == par_is_init());
    if (true != par_is_init())
        return ePAR_ERROR_INIT;

    PAR_DBG_PRINT("PAR: persisting all configured parameters to NVM");
    return par_nvm_write_all();
}
/**
 * @brief Store one scalar or object parameter value to NVM.
 *
 * @pre        NVM storage must be initialized first and "PAR_CFG_NVM_EN"
 * settings must be enabled.
 *
 * @param par_num Parameter number (enumeration).
 * @return Status of operation.
 */
par_status_t par_save(const par_num_t par_num)
{
    PAR_ASSERT(true == par_is_init());
    if (true != par_is_init())
        return ePAR_ERROR_INIT;

    PAR_DBG_PRINT("PAR: persisting par_num=%u to NVM", (unsigned)par_num);
    return par_nvm_write(par_num, true);
}
/**
 * @brief Store one scalar or object parameter value to NVM by ID.
 *
 * @pre        NVM storage must be initialized first and "PAR_CFG_NVM_EN"
 * settings must be enabled.
 *
 * @code
 * // Use case.
 * // Store par from ID 10 to 32.
 * uint8_t par_id;.
 *
 * for ( par_id = 10; par_id < 32; par_id++ ).
 * {.
 * status |= par_save_by_id( par_id ).
 * }.
 *
 * @endcode
 *
 * @param par_id Parameter ID number.
 * @return Status of operation.
 */
#if (1 == PAR_CFG_ENABLE_ID)
par_status_t par_save_by_id(const uint16_t par_id)
{
    par_num_t par_num = 0;
    PAR_ASSERT(true == par_is_init());
    if (true != par_is_init())
        return ePAR_ERROR_INIT;

    if (ePAR_OK == par_get_num_by_id(par_id, &par_num))
    {
        return par_save(par_num);
    }

    PAR_WARN_PRINT("PAR: persist-by-id could not resolve parameter id=%u", (unsigned)par_id);
    return ePAR_ERROR;
}
#endif /* (1 == PAR_CFG_ENABLE_ID) */
/**
 * @brief Rewrite the full managed parameter NVM area.
 *
 * @note This function shall be locked as it will erase complete parameter.
 * region of NVM space. Shall be used only during.
 *
 * @pre      NVM storage must be initialized first and "PAR_CFG_NVM_EN"
 * settings must be enabled.
 *
 * @return Status of operation.
 */
par_status_t par_save_clean(void)
{
    PAR_ASSERT(true == par_is_init());
    if (true != par_is_init())
        return ePAR_ERROR_INIT;

    return par_nvm_reset_all();
}

#endif /* (1 == PAR_CFG_NVM_EN) */
#if (1 == PAR_CFG_ENABLE_CHANGE_CALLBACK)
/**
 * @brief Register parameter on change callback.
 *
 * @param par_num Parameter number (enumeration).
 * @param cb Callback.
 * @note Register callbacks during single-threaded setup or under application
 * synchronization because this table write is not serialized against callback
 * dispatch.
 */
void par_register_on_change_cb(const par_num_t par_num, const pf_par_on_change_cb_t cb)
{
    if (NULL == par_core_get_table_entry(par_num))
    {
        return;
    }

    g_par_cb_table[par_num].on_change = cb;
}
#endif /* (1 == PAR_CFG_ENABLE_CHANGE_CALLBACK) */
/**
 * @brief Register parameter value validation function.
 *
 * @param par_num Parameter number (enumeration).
 * @param validation Validation.
 */
#if (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION)
void par_register_validation(const par_num_t par_num, const pf_par_validation_t validation)
{
    const par_cfg_t * const p_cfg = par_core_get_table_entry(par_num);

    if (NULL == p_cfg)
    {
        return;
    }
#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
    if (true == par_object_type_is_object(p_cfg->type))
    {
        PAR_ASSERT(0);
        return;
    }
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

    g_par_cb_table[par_num].validation.scalar = validation;
}
#endif /* (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION) */


#if (PAR_CFG_DEBUG_EN)
/**
 * @brief Get status string description.
 *
 * @param status Parameter status.
 * @return Parameter status description.
 */
const char *par_get_status_str(const par_status_t status)
{
    uint8_t i = 0;
    const char *str = "N/A";

    if (ePAR_OK == status)
    {
        str = (const char *)gs_status[0];
    }
    else
    {
        for (i = 0; i < 16; i++)
        {
            if (status & (1 << i))
            {
                str = (const char *)gs_status[i + 1];
                break;
            }
        }
    }

    return str;
}
#endif /* (PAR_CFG_DEBUG_EN) */
/**
 * @} <!-- END GROUP -->
 */
