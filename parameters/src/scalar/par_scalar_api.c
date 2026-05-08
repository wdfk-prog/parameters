/**
 * @file par_scalar_api.c
 * @brief Implement public scalar-parameter API entry points.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-05-01
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include <stdbool.h>
#include <string.h>

#include "par.h"
#include "par_atomic.h"
#include "par_layout.h"
#include "par_object.h"
#include "par_core.h"

/**
 * @brief Compile-time definitions.
 */
PAR_STATIC_ASSERT(par_atomic_u8_i8_same_size, sizeof(par_atomic_u8_t) == sizeof(par_atomic_i8_t));
PAR_STATIC_ASSERT(par_atomic_u8_i8_same_align, PAR_ALIGNOF(par_atomic_u8_t) == PAR_ALIGNOF(par_atomic_i8_t));
PAR_STATIC_ASSERT(par_atomic_u16_i16_same_size, sizeof(par_atomic_u16_t) == sizeof(par_atomic_i16_t));
PAR_STATIC_ASSERT(par_atomic_u16_i16_same_align, PAR_ALIGNOF(par_atomic_u16_t) == PAR_ALIGNOF(par_atomic_i16_t));
PAR_STATIC_ASSERT(par_atomic_u32_i32_same_size, sizeof(par_atomic_u32_t) == sizeof(par_atomic_i32_t));
PAR_STATIC_ASSERT(par_atomic_u32_i32_same_align, PAR_ALIGNOF(par_atomic_u32_t) == PAR_ALIGNOF(par_atomic_i32_t));
PAR_STATIC_ASSERT(par_atomic_u32_f32_same_size, sizeof(par_atomic_u32_t) == sizeof(par_atomic_f32_t));
PAR_STATIC_ASSERT(par_atomic_u32_f32_same_align, PAR_ALIGNOF(par_atomic_u32_t) == PAR_ALIGNOF(par_atomic_f32_t));
/**
 * @brief Grouped typed storage backing parameter live values.
 *
 * @note Storage is organized as U8/U16/U32 typed members inside one private.
 * grouped storage object.
 *
 * @note Zero-length groups are mapped to size 1 arrays for compiler portability.
 *
 * @note Private implementation fragment below must not be included outside par_scalar_api.c.
 */
typedef struct
{
    par_atomic_u8_t u8[PAR_STORAGE_NONZERO(PAR_STORAGE_COUNT8)];
    par_atomic_u16_t u16[PAR_STORAGE_NONZERO(PAR_STORAGE_COUNT16)];
    par_atomic_u32_t u32[PAR_STORAGE_NONZERO(PAR_STORAGE_COUNT32)];
} par_storage_groups_t;
/**
 * @brief Private implementation fragment. Do not include outside par_scalar_api.c.
 * @details Defines gs_par_storage with grouped typed initializers.
 */
#include "par_storage_init.inc"

#if (1 == PAR_CFG_ENABLE_RESET_ALL_RAW)
/**
 * @brief Runtime grouped default mirror storage for raw reset-all API.
 *
 * @note Mirrors are initialized in par_init() from current live defaults.
 * after F32 startup patch and before optional NVM load.
 *
 * @note Mirror layout matches the grouped live storage object.
 */
static par_storage_groups_t gs_par_default_mirror = { 0 };
#endif /* (1 == PAR_CFG_ENABLE_RESET_ALL_RAW) */


/**
 * @brief U8 live-value access pointer into grouped storage.
 */
static par_atomic_u8_t * const gpu8_par_value = gs_par_storage.u8;
/**
 * @brief I8 live-value access pointer into grouped storage.
 */
static par_atomic_i8_t * const gpi8_par_value = (par_atomic_i8_t *)gs_par_storage.u8;
/**
 * @brief U16 live-value access pointer into grouped storage.
 */
static par_atomic_u16_t * const gpu16_par_value = gs_par_storage.u16;
/**
 * @brief I16 live-value access pointer into grouped storage.
 */
static par_atomic_i16_t * const gpi16_par_value = (par_atomic_i16_t *)gs_par_storage.u16;
/**
 * @brief U32 live-value access pointer into grouped storage.
 */
static par_atomic_u32_t * const gpu32_par_value = gs_par_storage.u32;
/**
 * @brief I32 live-value access pointer into grouped storage.
 */
static par_atomic_i32_t * const gpi32_par_value = (par_atomic_i32_t *)gs_par_storage.u32;
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
/**
 * @brief F32 live-value access pointer into grouped storage.
 */
static par_atomic_f32_t * const gpf32_par_value = (par_atomic_f32_t *)gs_par_storage.u32;
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */

/**
 * @brief Offset table compatibility alias.
 *
 * @note Layout offsets are now owned by par_layout and accessed via getter.
 * Keep this local alias so existing indexed access sites remain unchanged.
 */
#define g_par_offset             (par_layout_get_offset_table())
#define PAR_CFG_SCALAR_DEF(cfg_) ((cfg_)->value_cfg.scalar.def)
#if (1 == PAR_CFG_ENABLE_RANGE)
#define PAR_CFG_SCALAR_RANGE(cfg_) ((cfg_)->value_cfg.scalar.range)
#endif /* (1 == PAR_CFG_ENABLE_RANGE) */
/**
 * @brief Private getters and setters.
 */
#define PAR_GET_U8_PRIV(par_num)  PAR_ATOMIC_LOAD(u8, &gpu8_par_value[g_par_offset[par_num]])
#define PAR_GET_I8_PRIV(par_num)  PAR_ATOMIC_LOAD(i8, &gpi8_par_value[g_par_offset[par_num]])
#define PAR_GET_U16_PRIV(par_num) PAR_ATOMIC_LOAD(u16, &gpu16_par_value[g_par_offset[par_num]])
#define PAR_GET_I16_PRIV(par_num) PAR_ATOMIC_LOAD(i16, &gpi16_par_value[g_par_offset[par_num]])
#define PAR_GET_U32_PRIV(par_num) PAR_ATOMIC_LOAD(u32, &gpu32_par_value[g_par_offset[par_num]])
#define PAR_GET_I32_PRIV(par_num) PAR_ATOMIC_LOAD(i32, &gpi32_par_value[g_par_offset[par_num]])
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
#define PAR_GET_F32_PRIV(par_num) PAR_ATOMIC_LOAD(f32, &gpf32_par_value[g_par_offset[par_num]])
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */

#define PAR_SET_U8_PRIV(par_num, val)  PAR_ATOMIC_STORE(u8, &gpu8_par_value[g_par_offset[par_num]], (val))
#define PAR_SET_I8_PRIV(par_num, val)  PAR_ATOMIC_STORE(i8, &gpi8_par_value[g_par_offset[par_num]], (val))
#define PAR_SET_U16_PRIV(par_num, val) PAR_ATOMIC_STORE(u16, &gpu16_par_value[g_par_offset[par_num]], (val))
#define PAR_SET_I16_PRIV(par_num, val) PAR_ATOMIC_STORE(i16, &gpi16_par_value[g_par_offset[par_num]], (val))
#define PAR_SET_U32_PRIV(par_num, val) PAR_ATOMIC_STORE(u32, &gpu32_par_value[g_par_offset[par_num]], (val))
#define PAR_SET_I32_PRIV(par_num, val) PAR_ATOMIC_STORE(i32, &gpi32_par_value[g_par_offset[par_num]], (val))
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
#define PAR_SET_F32_PRIV(par_num, val) PAR_ATOMIC_STORE(f32, &gpf32_par_value[g_par_offset[par_num]], (val))
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */


#if (1 == PAR_CFG_ENABLE_TYPE_F32)
/**
 * @brief Patch F32 defaults into shared u32 storage as bit-patterns.
 *
 * @note Integer defaults are already initialized at definition time. F32
 * defaults are patched once after layout offsets are available.
 */
void par_core_scalar_patch_f32_defaults_from_table(void)
{
    for (par_num_t par_num = 0; par_num < ePAR_NUM_OF; par_num++)
    {
        const par_cfg_t * const p_cfg = par_cfg_get(par_num);
        if (ePAR_TYPE_F32 == p_cfg->type)
        {
            PAR_SET_F32_PRIV(par_num, p_cfg->value_cfg.scalar.def.f32);
        }
    }
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */

#if (1 == PAR_CFG_ENABLE_RESET_ALL_RAW)
/**
 * @brief Capture the current scalar defaults for the raw reset path.
 */
void par_core_scalar_snapshot_default_mirror(void)
{
    memcpy(&gs_par_default_mirror, &gs_par_storage, sizeof(gs_par_storage));
}
#endif /* (1 == PAR_CFG_ENABLE_RESET_ALL_RAW) */

/**
 * @brief Typed parameter API implementation expansion point.
 *
 * The following public APIs are emitted by macro expansion from
 * "par_typed_impl.inc":
 *
 * - par_set_u8 / i8 / u16 / i16 / u32 / i32 (/ f32).
 * - par_set_u8_fast / i8_fast / u16_fast / i16_fast / u32_fast / i32_fast (/ f32_fast).
 * - par_get_u8 / i8 / u16 / i16 / u32 / i32 (/ f32).
 *
 * If IDE navigation cannot jump from par.h declarations to concrete bodies,
 * inspect this include point and then open par_typed_impl.inc.
 *
 * @note Private implementation fragment. Do not include outside par_scalar_api.c.
 */
#include "par_typed_impl.inc"

/**
 * @brief Bitwise fast setter implementation.
 * @note Private implementation fragment. Do not include outside par_scalar_api.c.
 */
#include "par_bitwise_impl.inc"

/**
 * @brief Set one scalar parameter value.
 *
 * @note Mandatory to cast input argument to appropriate type. E.g.:
 *
 * @code
 * float32_t my_val = 1.234f;.
 * par_set_scalar( ePAR_MY_VAR, (float32_t*) &my_val );.
 * @endcode
 *
 * @note Input is the internal parameter number (`par_num_t`) from `par_def.h`,
 * not the external parameter ID.
 *
 * @param par_num Parameter number (enumeration).
 * @param p_val Pointer to value.
 * @return Status of operation.
 */
par_status_t par_set_scalar(const par_num_t par_num, const void *p_val)
{
    const par_cfg_t *par_cfg = NULL;
    const par_status_t status = par_core_resolve_runtime(par_num, p_val, true, &par_cfg);

    if (ePAR_OK != status)
    {
        return status;
    }
#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
    if (true == par_object_type_is_object(par_cfg->type))
    {
        return ePAR_ERROR_TYPE;
    }
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

    return par_set_checked_core(par_num, par_cfg->type, NULL, p_val, NULL);
}
/**
 * @brief Set parameter value through the generic unchecked fast path.
 *
 * @param par_num Parameter number (enumeration).
 * @param p_val Pointer to value.
 * @return Status of operation.
 */
par_status_t par_set_scalar_fast(const par_num_t par_num, const void *p_val)
{
    const par_cfg_t *par_cfg = NULL;
    const par_status_t status = par_core_resolve_runtime(par_num, p_val, true, &par_cfg);

    if (ePAR_OK != status)
    {
        return status;
    }

    switch (par_cfg->type)
    {
    case ePAR_TYPE_U8:
        return par_set_u8_fast(par_num, *(const uint8_t *)p_val);

    case ePAR_TYPE_I8:
        return par_set_i8_fast(par_num, *(const int8_t *)p_val);

    case ePAR_TYPE_U16:
        return par_set_u16_fast(par_num, *(const uint16_t *)p_val);

    case ePAR_TYPE_I16:
        return par_set_i16_fast(par_num, *(const int16_t *)p_val);

    case ePAR_TYPE_U32:
        return par_set_u32_fast(par_num, *(const uint32_t *)p_val);

    case ePAR_TYPE_I32:
        return par_set_i32_fast(par_num, *(const int32_t *)p_val);

#if (1 == PAR_CFG_ENABLE_TYPE_F32)
    case ePAR_TYPE_F32:
        return par_set_f32_fast(par_num, *(const float32_t *)p_val);
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
        return ePAR_ERROR_TYPE;
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

    case ePAR_TYPE_NUM_OF:
    default:
        PAR_ASSERT(0);
        return ePAR_ERROR_TYPE;
    }
}


/**
 * @brief Set one scalar parameter value by ID.
 *
 * @param id Parameter ID number.
 * @param p_val Pointer to value.
 * @return Status of operation.
 * @note Object-backed rows are not supported by this generic ID-based setter.
 */
#if (1 == PAR_CFG_ENABLE_ID)
par_status_t par_set_scalar_by_id(const uint16_t id, const void *p_val)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_get_num_by_id(id, &par_num);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_set_scalar(par_num, p_val);
}
#endif /* (1 == PAR_CFG_ENABLE_ID) */
#if (1 == PAR_CFG_ENABLE_RESET_ALL_RAW)
/**
 * @brief Reset all parameters to default values via raw storage restore.
 *
 * @pre          Parameters must be initialized before usage.
 * @return Status of operation.
 * @note This API restores grouped storage directly from default mirrors instead
 * of iterating over all parameters through the normal setter path.
 * @note Raw default restore is a maintenance/recovery operation. It intentionally
 * bypasses external write access and role-policy checks, plus runtime validation,
 * on-change callbacks, and setter-side range handling.
 * @note Restore is performed as one grouped storage snapshot copy. Internal
 * U8/U16/U32 width-group storage semantics are preserved.
 */
par_status_t par_reset_all_to_default_raw(void)
{
    PAR_ASSERT(true == par_is_init());
    if (true != par_is_init())
        return ePAR_ERROR_INIT;

    if (ePAR_OK != par_acquire_mutex((par_num_t)0))
    {
        return ePAR_ERROR_MUTEX;
    }

    memcpy(&gs_par_storage, &gs_par_default_mirror, sizeof(gs_par_storage));
#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
    par_object_restore_default_mirror();
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

    par_release_mutex((par_num_t)0);

    PAR_DBG_PRINT("PAR: raw reset all parameters to defaults");
    return ePAR_OK;
}
#endif /* (1 == PAR_CFG_ENABLE_RESET_ALL_RAW) */
/**
 * @brief Get one scalar parameter value.
 *
 * @note Mandatory to cast input argument to appropriate type. E.g.:
 *
 * @code
 * float32_t my_val = 0.0f;.
 * par_get_scalar( ePAR_MY_VAR, (float32_t*) &my_val );.
 * @endcode
 *
 * @note Input is the internal parameter number (`par_num_t`) from `par_def.h`,
 * not the external parameter ID.
 *
 * @param par_num Parameter number (enumeration).
 * @param p_val Parameter value.
 * @return Status of operation.
 */
par_status_t par_get_scalar(const par_num_t par_num, void * const p_val)
{
    const par_cfg_t *par_cfg = NULL;
    par_status_t status = ePAR_OK;

    status = par_core_resolve_runtime(par_num, p_val, true, &par_cfg);
    if (ePAR_OK != status)
    {
        return status;
    }
#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
    if (true == par_object_type_is_object(par_cfg->type))
    {
        return ePAR_ERROR_TYPE;
    }
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

    switch (par_cfg->type)
    {
    case ePAR_TYPE_U8:
        return par_get_u8(par_num, (uint8_t *)p_val);

    case ePAR_TYPE_I8:
        return par_get_i8(par_num, (int8_t *)p_val);

    case ePAR_TYPE_U16:
        return par_get_u16(par_num, (uint16_t *)p_val);

    case ePAR_TYPE_I16:
        return par_get_i16(par_num, (int16_t *)p_val);

    case ePAR_TYPE_U32:
        return par_get_u32(par_num, (uint32_t *)p_val);

    case ePAR_TYPE_I32:
        return par_get_i32(par_num, (int32_t *)p_val);

#if (1 == PAR_CFG_ENABLE_TYPE_F32)
    case ePAR_TYPE_F32:
        return par_get_f32(par_num, (float32_t *)p_val);
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
        return ePAR_ERROR_TYPE;
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

    case ePAR_TYPE_NUM_OF:
    default:
        PAR_ASSERT(0);
        return ePAR_ERROR_TYPE;
    }
}

/**
 * @brief Get one scalar parameter value by ID.
 *
 * @param id Parameter ID number.
 * @param p_val Pointer to value.
 * @return Status of operation.
 * @note Object-backed rows are not supported by this generic ID-based getter.
 */
#if (1 == PAR_CFG_ENABLE_ID)
par_status_t par_get_scalar_by_id(const uint16_t id, void * const p_val)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_get_num_by_id(id, &par_num);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_get_scalar(par_num, p_val);
}
#endif /* (1 == PAR_CFG_ENABLE_ID) */
/**
 * @brief Get one scalar parameter default value from table metadata.
 * @param par_num Parameter number (enumeration).
 * @param p_val Parameter default value.
 * @return Status of operation.
 */
par_status_t par_get_scalar_default(const par_num_t par_num, void * const p_val)
{
    const par_cfg_t *par_cfg = NULL;
    par_status_t status = ePAR_OK;

    status = par_core_resolve_metadata(par_num, p_val, true, &par_cfg);
    if (ePAR_OK != status)
    {
        return status;
    }

    switch (par_cfg->type)
    {
    case ePAR_TYPE_U8:
        *(uint8_t *)p_val = (uint8_t)par_cfg->value_cfg.scalar.def.u8;
        break;

    case ePAR_TYPE_I8:
        *(int8_t *)p_val = (int8_t)par_cfg->value_cfg.scalar.def.i8;
        break;

    case ePAR_TYPE_U16:
        *(uint16_t *)p_val = (uint16_t)par_cfg->value_cfg.scalar.def.u16;
        break;

    case ePAR_TYPE_I16:
        *(int16_t *)p_val = (int16_t)par_cfg->value_cfg.scalar.def.i16;
        break;

    case ePAR_TYPE_U32:
        *(uint32_t *)p_val = (uint32_t)par_cfg->value_cfg.scalar.def.u32;
        break;

    case ePAR_TYPE_I32:
        *(int32_t *)p_val = (int32_t)par_cfg->value_cfg.scalar.def.i32;
        break;

#if (1 == PAR_CFG_ENABLE_TYPE_F32)
    case ePAR_TYPE_F32:
        *(float32_t *)p_val = (float32_t)par_cfg->value_cfg.scalar.def.f32;
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
        return ePAR_ERROR_TYPE;
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

    case ePAR_TYPE_NUM_OF:
    default:
        PAR_ASSERT(0);
        return ePAR_ERROR_TYPE;
    }

    return ePAR_OK;
}


#if (1 == PAR_CFG_NVM_EN)
/**
 * @brief Set one scalar parameter value and save to NVM if changed.
 *
 * @note Mandatory to cast input argument to appropriate type. E.g.:
 *
 * @code
 * float32_t my_val = 1.234f;.
 * par_set_scalar_n_save( ePAR_MY_VAR, (float32_t*) &my_val );.
 * @endcode
 *
 * @note Input is the internal parameter number (`par_num_t`) from `par_def.h`,
 * not the external parameter ID.
 * @note This API uses the normal scalar setter path first. If an on-change
 * callback is registered, the callback is dispatched before the subsequent NVM
 * save step and follows the normal callback reentrancy restrictions.
 *
 * @param par_num Parameter number (enumeration).
 * @param p_val Pointer to scalar value.
 * @return Status of operation. Returns ePAR_ERROR_TYPE for object rows.
 */
par_status_t par_set_scalar_n_save(const par_num_t par_num, const void *p_val)
{
    const par_cfg_t *par_cfg = NULL;
    bool value_change = false;
    par_status_t status = par_core_resolve_runtime(par_num, p_val, true, &par_cfg);

    if (ePAR_OK != status)
    {
        return status;
    }
#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
    if (true == par_object_type_is_object(par_cfg->type))
    {
        return ePAR_ERROR_TYPE;
    }
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

    status = par_set_checked_core(par_num, par_cfg->type, NULL, p_val, &value_change);
    /* Persist successful writes even when par_set_scalar reports a warning, such as
     * range limiting. The changed flag is produced by the locked setter core
     * after it has written the actual live value. */
    if ((ePAR_OK == (status & ePAR_STATUS_ERROR_MASK)) && value_change)
    {
        PAR_DBG_PRINT("PAR: scalar_n_save detected value change, par_num=%u",
                      (unsigned)par_num);
        status |= par_save(par_num);
    }
    else if (ePAR_OK == (status & ePAR_STATUS_ERROR_MASK))
    {
        PAR_DBG_PRINT("PAR: scalar_n_save skipped NVM write because value is "
                      "unchanged, par_num=%u",
                      (unsigned)par_num);
    }

    return status;
}
#endif /* (1 == PAR_CFG_NVM_EN) */


