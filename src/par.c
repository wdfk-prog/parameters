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
 * 2026-01-29 V3.0.1  Ziga Miklosic
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
#include "port/par_atomic.h"
#include "layout/par_layout.h"
#include "def/par_id_map_static.h"
#include "persist/par_nvm.h"
#include "port/par_if.h"
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
    pf_par_validation_t validation; /**< Validation callback function (or NULL). */
#endif
#if (1 == PAR_CFG_ENABLE_CHANGE_CALLBACK)
    pf_par_on_change_cb_t on_change; /**< On change callback function (or NULL). */
#endif
} g_par_cb_table[ePAR_NUM_OF];
#endif

/**
 * @brief Grouped typed storage backing parameter live values.
 *
 * @note Storage is organized as U8/U16/U32 typed members inside one private.
 * grouped storage object.
 *
 * @note Zero-length groups are mapped to size 1 arrays for compiler portability.
 *
 * @note Private implementation fragment below must not be included outside par.c.
 */
typedef struct
{
    par_atomic_u8_t u8[PAR_STORAGE_NONZERO(PAR_STORAGE_COUNT8)];
    par_atomic_u16_t u16[PAR_STORAGE_NONZERO(PAR_STORAGE_COUNT16)];
    par_atomic_u32_t u32[PAR_STORAGE_NONZERO(PAR_STORAGE_COUNT32)];
} par_storage_groups_t;
/**
 * @brief Private implementation fragment. Do not include outside par.c.
 * @details Defines gs_par_storage with grouped typed initializers.
 */
#include "detail/par_storage_init.inc"

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
#endif

/**
 * @brief Typed live-value access pointers into grouped storage.
 */
static par_atomic_u8_t * const gpu8_par_value = gs_par_storage.u8;
static par_atomic_i8_t * const gpi8_par_value = (par_atomic_i8_t *)gs_par_storage.u8;
static par_atomic_u16_t * const gpu16_par_value = gs_par_storage.u16;
static par_atomic_i16_t * const gpi16_par_value = (par_atomic_i16_t *)gs_par_storage.u16;
static par_atomic_u32_t * const gpu32_par_value = gs_par_storage.u32;
static par_atomic_i32_t * const gpi32_par_value = (par_atomic_i32_t *)gs_par_storage.u32;
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
static par_atomic_f32_t * const gpf32_par_value = (par_atomic_f32_t *)gs_par_storage.u32;
#endif

/**
 * @brief Offset table compatibility alias.
 *
 * @note Layout offsets are now owned by par_layout and accessed via getter.
 * Keep this local alias so existing indexed access sites remain unchanged.
 */
#define g_par_offset (par_layout_get_offset_table())

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
#endif

#define PAR_SET_U8_PRIV(par_num, val)  PAR_ATOMIC_STORE(u8, &gpu8_par_value[g_par_offset[par_num]], (val))
#define PAR_SET_I8_PRIV(par_num, val)  PAR_ATOMIC_STORE(i8, &gpi8_par_value[g_par_offset[par_num]], (val))
#define PAR_SET_U16_PRIV(par_num, val) PAR_ATOMIC_STORE(u16, &gpu16_par_value[g_par_offset[par_num]], (val))
#define PAR_SET_I16_PRIV(par_num, val) PAR_ATOMIC_STORE(i16, &gpi16_par_value[g_par_offset[par_num]], (val))
#define PAR_SET_U32_PRIV(par_num, val) PAR_ATOMIC_STORE(u32, &gpu32_par_value[g_par_offset[par_num]], (val))
#define PAR_SET_I32_PRIV(par_num, val) PAR_ATOMIC_STORE(i32, &gpi32_par_value[g_par_offset[par_num]], (val))
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
#define PAR_SET_F32_PRIV(par_num, val) PAR_ATOMIC_STORE(f32, &gpf32_par_value[g_par_offset[par_num]], (val))
#endif

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
#endif
/**
 * @brief Function declarations.
 */
static par_status_t par_set_checked_core(const par_num_t par_num, const par_type_list_t expected_type, const par_type_t * const p_typed_val, const void * const p_ptr_val);

/**
 * @brief Function declarations and definitions.
 */
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
#endif
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
static par_status_t par_resolve_metadata(const par_num_t par_num, const void * const p_arg, const bool require_arg, const par_cfg_t ** const pp_cfg)
{
    const par_cfg_t *p_cfg = NULL;

    if ((true == require_arg) && (NULL == p_arg))
    {
        return ePAR_ERROR_PARAM;
    }

    PAR_ASSERT(par_num < ePAR_NUM_OF);
    if (par_num >= ePAR_NUM_OF)
    {
        return ePAR_ERROR_PAR_NUM;
    }

    p_cfg = par_get_config(par_num);
    if (NULL == p_cfg)
    {
        return ePAR_ERROR;
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
static par_status_t par_resolve_runtime(const par_num_t par_num, const void * const p_arg, const bool require_arg, const par_cfg_t ** const pp_cfg)
{
    if (true != par_is_init())
    {
        return ePAR_ERROR_INIT;
    }

    return par_resolve_metadata(par_num, p_arg, require_arg, pp_cfg);
}

/**
 * @brief Compare two F32 values by raw bit pattern.
 *
 * @note This helper intentionally uses memcpy() instead of pointer.
 * casting or union type-punning. memcpy() preserves the exact.
 * IEEE-754 bit pattern while remaining strict-aliasing safe.
 * Bitwise comparison keeps NaN payloads and signed-zero handling.
 * deterministic for parameter storage use cases.
 *
 * @param lhs Left-hand float value.
 * @param rhs Right-hand float value.
 * @return true if raw 32-bit representations are equal.
 */
static bool par_f32_bits_equal(const float32_t lhs, const float32_t rhs)
{
    uint32_t lhs_bits = 0U;
    uint32_t rhs_bits = 0U;

    memcpy(&lhs_bits, &lhs, sizeof(lhs_bits));
    memcpy(&rhs_bits, &rhs, sizeof(rhs_bits));

    return (lhs_bits == rhs_bits);
}

#if (1 == PAR_CFG_ENABLE_TYPE_F32)
/**
 * @brief Patch F32 defaults into shared u32 storage as bit-patterns.
 *
 * @note Integer defaults are already initialized at definition time.
 * F32 defaults are patched once after layout offsets are available.
 */
static void par_patch_f32_defaults_from_table(void)
{
    for (par_num_t par_num = 0; par_num < ePAR_NUM_OF; par_num++)
    {
        const par_cfg_t * const p_cfg = par_cfg_get(par_num);
        if (ePAR_TYPE_F32 == p_cfg->type)
        {
            PAR_SET_F32_PRIV(par_num, p_cfg->def.f32);
        }
    }
}
#endif
/**
 * @brief Bind static space for live parameter values.
 */
static void par_bind_storage_layout(void)
{
    par_layout_init();
    PAR_DBG_PRINT("Total RAM consumption for parameters value: %u bytes", (unsigned)(((uint32_t)par_layout_get_count().count32 * 4u) +
                                                                                     ((uint32_t)par_layout_get_count().count16 * 2u) +
                                                                                     ((uint32_t)par_layout_get_count().count8)));
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
            PAR_DBG_PRINT("ERR, Duplicate parameter ID %u!", (unsigned)id);
            PAR_ASSERT(0);
            return ePAR_ERROR_INIT;
        }
#endif

#if (1 == PAR_CFG_ENABLE_RUNTIME_ID_HASH_COLLISION_CHECK)
        if (bucket->id != id)
        {
            PAR_DBG_PRINT("ERR, Hash collision: ID %u conflicts with ID %u at bucket %u!",
                          (unsigned)id, (unsigned)bucket->id, (unsigned)bucket_idx);
            PAR_ASSERT(0);
            return ePAR_ERROR_INIT;
        }
#endif
    }

    return ePAR_OK;
}
#endif
#endif
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
#endif

    for (uint32_t i = 0; i < ePAR_NUM_OF; i++)
    {
#if (1 == PAR_CFG_ENABLE_RANGE)
        /*
         * Keep F32 range/default validation at runtime.
         * Some embedded GCC variants do not treat these float comparisons as
         * constant expressions in file-scope static assertions.
         */
        PAR_ASSERT((ePAR_TYPE_F32 == p_par_cfg[i].type) ? (((p_par_cfg[i].range.min.f32 < p_par_cfg[i].range.max.f32) &&
                                                            (p_par_cfg[i].def.f32 <= p_par_cfg[i].range.max.f32)) &&
                                                           (p_par_cfg[i].range.min.f32 <= p_par_cfg[i].def.f32))
                                                        : (1));
#endif

#if (1 == PAR_CFG_ENABLE_NAME)
        if (NULL == p_par_cfg[i].name)
        {
            status = ePAR_ERROR_INIT;
            PAR_DBG_PRINT("ERR, Parameter %d name missing!", i);
            PAR_ASSERT(0);
            break;
        }
#endif

#if (1 == PAR_CFG_ENABLE_DESC)
        if (NULL == p_par_cfg[i].desc)
        {
            status = ePAR_ERROR_INIT;
            PAR_DBG_PRINT("ERR, Parameter %d description missing!", i);
            PAR_ASSERT(0);
            break;
        }
#endif

#if (1 == PAR_CFG_ENABLE_DESC) && (1 == PAR_CFG_ENABLE_DESC_CHECK)
        if (false == par_port_is_desc_valid(p_par_cfg[i].desc))
        {
            status = ePAR_ERROR_INIT;
            PAR_DBG_PRINT("ERR, Parameter %d description is invalid!", i);
            PAR_ASSERT(0);
            break;
        }
#endif
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
 * @brief Device Parameters initialization.
 *
 * At init parameter table is being check for correct definition, allocation.
 * in RAM space for parameters live values and additionaly interface to.
 * platform is being done.
 *
 * @return Status of initialization.
 */
par_status_t par_init(void)
{
    par_status_t status = ePAR_OK;

    PAR_ASSERT(false == par_is_init());
    if (false != par_is_init())
        return ePAR_ERROR_INIT;
    status |= par_check_table_validity(par_cfg_get_table());
    par_bind_storage_layout();
    status |= par_if_init();
    PAR_ASSERT(ePAR_OK == status);
    if (ePAR_OK == status)
    {
        gb_is_init = true;
        /* Patch F32 defaults after layout offsets become available. */
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
        par_patch_f32_defaults_from_table();
#endif

#if (1 == PAR_CFG_ENABLE_RESET_ALL_RAW)
        /* Snapshot defaults before optional NVM restore. */
        memcpy(&gs_par_default_mirror, &gs_par_storage, sizeof(gs_par_storage));
#endif

#if (1 == PAR_CFG_NVM_EN)
        /* Restore persisted values after default initialization. */
        status |= par_nvm_init();
#endif
    }

    PAR_DBG_PRINT("PAR: Parameters initialized with status: %s", par_get_status_str(status));

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

#if (1 == PAR_CFG_NVM_EN)
    deinit_status = par_nvm_deinit();
    status |= deinit_status;
#endif

    deinit_status = par_if_deinit();
    status |= deinit_status;
    gb_is_init = false;

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
    PAR_ASSERT(par_num < ePAR_NUM_OF);

    return par_if_aquire_mutex(par_num);
}
/**
 * @brief Try to acquire mutex for specified parameter.
 *
 * @param par_num Parameter number (enumeration).
 */
void par_release_mutex(const par_num_t par_num)
{
    PAR_ASSERT(par_num < ePAR_NUM_OF);

    par_if_release_mutex(par_num);
}
/**
 * @brief Set parameter value.
 *
 * @note Mandatory to cast input argument to appropriate type. E.g.:
 *
 * @code
 * float32_t my_val = 1.234f;.
 * par_set( ePAR_MY_VAR, (float32_t*) &my_val );.
 * @endcode
 *
 * @note Input is the internal parameter number (`par_num_t`) from `par_def.h`,
 * not the external parameter ID.
 *
 * @param par_num Parameter number (enumeration).
 * @param p_val Pointer to value.
 * @return Status of operation.
 */
par_status_t par_set(const par_num_t par_num, const void *p_val)
{
    const par_cfg_t *par_cfg = NULL;
    const par_status_t status = par_resolve_runtime(par_num, p_val, true, &par_cfg);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_set_checked_core(par_num, par_cfg->type, NULL, p_val);
}
/**
 * @brief Set parameter value through the generic unchecked fast path.
 *
 * @param par_num Parameter number (enumeration).
 * @param p_val Pointer to value.
 * @return Status of operation.
 */
par_status_t par_set_fast(const par_num_t par_num, const void *p_val)
{
    const par_cfg_t *par_cfg = NULL;
    const par_status_t status = par_resolve_runtime(par_num, p_val, true, &par_cfg);

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
#endif

    case ePAR_TYPE_NUM_OF:
    default:
        PAR_ASSERT(0);
        return ePAR_ERROR_TYPE;
    }
}
/**
 * @brief Set parameter value by ID.
 *
 * @param id Parameter ID number.
 * @param p_val Pointer to value.
 * @return Status of operation.
 */
#if (1 == PAR_CFG_ENABLE_ID)
par_status_t par_set_by_id(const uint16_t id, const void *p_val)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_get_num_by_id(id, &par_num);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_set(par_num, p_val);
}
#endif
/**
 * @brief Typed parameter API implementation expansion point.
 *
 * The following public APIs are not handwritten one by one in this file.
 * They are emitted by macro expansion from "par_typed_impl.inc":
 *
 * - par_set_u8 / i8 / u16 / i16 / u32 / i32 (/ f32).
 * - par_set_u8_fast / i8_fast / u16_fast / i16_fast / u32_fast / i32_fast (/ f32_fast).
 * - par_get_u8 / i8 / u16 / i16 / u32 / i32 (/ f32).
 *
 * If IDE navigation cannot jump from par.h declarations to concrete bodies,.
 * inspect this include point and then open par_typed_impl.inc.
 *
 * @note Private implementation fragment. Do not include outside par.c.
 */
#include "detail/par_typed_impl.inc"
/**
 * @brief Bitwise fast setter implementation.
 * @note Private implementation fragments. Do not include outside par.c.
 */
#include "detail/par_bitwise_impl.inc"
/**
 * @brief Set parameter to default value.
 *
 * @pre    Parameters must be initialised before usage!
 *
 * @param par_num Parameter number (enumeration).
 * @return Status of operation.
 */
par_status_t par_set_to_default(const par_num_t par_num)
{
    const par_cfg_t *par_cfg = NULL;
    par_status_t status = par_resolve_runtime(par_num, NULL, false, &par_cfg);

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
        status = par_set_u8_fast(par_num, par_cfg->def.u8);
        break;

    case ePAR_TYPE_I8:
        status = par_set_i8_fast(par_num, par_cfg->def.i8);
        break;

    case ePAR_TYPE_U16:
        status = par_set_u16_fast(par_num, par_cfg->def.u16);
        break;

    case ePAR_TYPE_I16:
        status = par_set_i16_fast(par_num, par_cfg->def.i16);
        break;

    case ePAR_TYPE_U32:
        status = par_set_u32_fast(par_num, par_cfg->def.u32);
        break;

    case ePAR_TYPE_I32:
        status = par_set_i32_fast(par_num, par_cfg->def.i32);
        break;

#if (1 == PAR_CFG_ENABLE_TYPE_F32)
    case ePAR_TYPE_F32:
        status = par_set_f32_fast(par_num, par_cfg->def.f32);
        break;
#endif

    case ePAR_TYPE_NUM_OF:
    default:
        PAR_ASSERT(0);
        status = ePAR_ERROR_TYPE;
        break;
    }

    par_release_mutex(par_num);
    return status;
}

#if (1 == PAR_CFG_ENABLE_RESET_ALL_RAW)
/**
 * @brief Reset all parameters to default values via raw storage restore.
 *
 * @note Unlike par_set_all_to_default(), this API restores grouped.
 * storage directly from default mirrors instead of iterating over.
 * all parameters through the normal setter path.
 *
 * @note This path is therefore typically faster for bulk reset, because.
 * it avoids per-parameter runtime validation, on-change callback,.
 * and setter-side range handling.
 *
 * @note Restore is performed as one grouped storage snapshot copy.
 * Internal U8/U16/U32 width-group storage semantics are preserved.
 *
 * @pre          Parameters must be initialized before usage.
 *
 * @return Status of operation.
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

    par_release_mutex((par_num_t)0);

    PAR_DBG_PRINT("PAR: Raw reset all parameters to default");
    return ePAR_OK;
}
#endif
/**
 * @brief Set all parameters to default value.
 *
 * @pre          Parameters must be initialised before usage!
 * @note When PAR_CFG_ENABLE_RESET_ALL_RAW = 1, this public API forwards.
 * to par_reset_all_to_default_raw() for maximum reset speed.
 * @note Otherwise it iterates through parameters and resets them via.
 * par_set_to_default(), preserving fast default-restore semantics.
 *
 * @return Status of operation.
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

    PAR_DBG_PRINT("PAR: Setting all parameters to default");
    return status;
#endif
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

    status = par_resolve_runtime(par_num, p_has_changed, true, &par_cfg);
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
        *p_has_changed = (cur != par_cfg->def.u8);
        break;
    }

    case ePAR_TYPE_I8:
    {
        int8_t cur = 0;
        const par_status_t status = par_get_i8(par_num, &cur);
        if (ePAR_OK != status)
            return status;
        *p_has_changed = (cur != par_cfg->def.i8);
        break;
    }

    case ePAR_TYPE_U16:
    {
        uint16_t cur = 0U;
        const par_status_t status = par_get_u16(par_num, &cur);
        if (ePAR_OK != status)
            return status;
        *p_has_changed = (cur != par_cfg->def.u16);
        break;
    }

    case ePAR_TYPE_I16:
    {
        int16_t cur = 0;
        const par_status_t status = par_get_i16(par_num, &cur);
        if (ePAR_OK != status)
            return status;
        *p_has_changed = (cur != par_cfg->def.i16);
        break;
    }

    case ePAR_TYPE_U32:
    {
        uint32_t cur = 0U;
        const par_status_t status = par_get_u32(par_num, &cur);
        if (ePAR_OK != status)
            return status;
        *p_has_changed = (cur != par_cfg->def.u32);
        break;
    }

    case ePAR_TYPE_I32:
    {
        int32_t cur = 0;
        const par_status_t status = par_get_i32(par_num, &cur);
        if (ePAR_OK != status)
            return status;
        *p_has_changed = (cur != par_cfg->def.i32);
        break;
    }

#if (1 == PAR_CFG_ENABLE_TYPE_F32)
    case ePAR_TYPE_F32:
    {
        float32_t cur = 0.0f;
        const par_status_t status = par_get_f32(par_num, &cur);
        if (ePAR_OK != status)
            return status;
        *p_has_changed = !par_f32_bits_equal(cur, par_cfg->def.f32);
        break;
    }
#endif

    case ePAR_TYPE_NUM_OF:
    default:
        PAR_ASSERT(0);
        return ePAR_ERROR_TYPE;
    }

    return ePAR_OK;
}
/**
 * @brief Get parameter value.
 *
 * @note Mandatory to cast input argument to appropriate type. E.g.:
 *
 * @code
 * float32_t my_val = 0.0f;.
 * par_get( ePAR_MY_VAR, (float32_t*) &my_val );.
 * @endcode
 *
 * @note Input is the internal parameter number (`par_num_t`) from `par_def.h`,
 * not the external parameter ID.
 *
 * @param par_num Parameter number (enumeration).
 * @param p_val Parameter value.
 * @return Status of operation.
 */
par_status_t par_get(const par_num_t par_num, void * const p_val)
{
    const par_cfg_t *par_cfg = NULL;
    par_status_t status = ePAR_OK;

    status = par_resolve_runtime(par_num, p_val, true, &par_cfg);
    if (ePAR_OK != status)
    {
        return status;
    }

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
#endif

    case ePAR_TYPE_NUM_OF:
    default:
        PAR_ASSERT(0);
        return ePAR_ERROR_TYPE;
    }
}
/**
 * @brief Get parameter value by ID.
 *
 * @param id Parameter ID number.
 * @param p_val Pointer to value.
 * @return Status of operation.
 */
#if (1 == PAR_CFG_ENABLE_ID)
par_status_t par_get_by_id(const uint16_t id, void * const p_val)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_get_num_by_id(id, &par_num);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_get(par_num, p_val);
}
#endif
/**
 * @brief Get parameter default value.
 * *.
 * @param par_num Parameter number (enumeration).
 * @param p_val Parameter default value.
 * @return Status of operation.
 */
par_status_t par_get_default(const par_num_t par_num, void * const p_val)
{
    const par_cfg_t *par_cfg = NULL;
    par_status_t status = ePAR_OK;

    status = par_resolve_metadata(par_num, p_val, true, &par_cfg);
    if (ePAR_OK != status)
    {
        return status;
    }

    switch (par_cfg->type)
    {
    case ePAR_TYPE_U8:
        *(uint8_t *)p_val = (uint8_t)par_cfg->def.u8;
        break;

    case ePAR_TYPE_I8:
        *(int8_t *)p_val = (int8_t)par_cfg->def.i8;
        break;

    case ePAR_TYPE_U16:
        *(uint16_t *)p_val = (uint16_t)par_cfg->def.u16;
        break;

    case ePAR_TYPE_I16:
        *(int16_t *)p_val = (int16_t)par_cfg->def.i16;
        break;

    case ePAR_TYPE_U32:
        *(uint32_t *)p_val = (uint32_t)par_cfg->def.u32;
        break;

    case ePAR_TYPE_I32:
        *(int32_t *)p_val = (int32_t)par_cfg->def.i32;
        break;

#if (1 == PAR_CFG_ENABLE_TYPE_F32)
    case ePAR_TYPE_F32:
        *(float32_t *)p_val = (float32_t)par_cfg->def.f32;
        break;
#endif

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
    PAR_ASSERT(par_num < ePAR_NUM_OF);
    if (par_num >= ePAR_NUM_OF)
        return NULL;

    return par_cfg_get(par_num);
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
    const par_cfg_t *par_cfg = NULL;

    if (ePAR_OK == par_resolve_metadata(par_num, NULL, false, &par_cfg))
    {
        return par_cfg->name;
    }

    return NULL;
}
#endif
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
    const par_cfg_t *par_cfg = NULL;

    if (ePAR_OK == par_resolve_metadata(par_num, NULL, false, &par_cfg))
    {
        return par_cfg->range;
    }

    return range;
}
#endif
/**
 * @brief Get parameter unit.
 *
 * @param par_num Parameter number (enumeration).
 * @return Parameter unit.
 */
#if (1 == PAR_CFG_ENABLE_UNIT)
const char *par_get_unit(const par_num_t par_num)
{
    const par_cfg_t *par_cfg = NULL;

    if (ePAR_OK == par_resolve_metadata(par_num, NULL, false, &par_cfg))
    {
        return par_cfg->unit;
    }

    return NULL;
}
#endif
/**
 * @brief Get parameter description.
 *
 * @param par_num Parameter number (enumeration).
 * @return Parameter description.
 */
#if (1 == PAR_CFG_ENABLE_DESC)
const char *par_get_desc(const par_num_t par_num)
{
    const par_cfg_t *par_cfg = NULL;

    if (ePAR_OK == par_resolve_metadata(par_num, NULL, false, &par_cfg))
    {
        return par_cfg->desc;
    }

    return NULL;
}
#endif
/**
 * @brief Get parameter type.
 *
 * @param par_num Parameter number (enumeration).
 * @return Parameter data type.
 */
par_type_list_t par_get_type(const par_num_t par_num)
{
    const par_cfg_t *par_cfg = NULL;

    if (ePAR_OK == par_resolve_metadata(par_num, NULL, false, &par_cfg))
    {
        return par_cfg->type;
    }

    return ePAR_TYPE_NUM_OF;
}
/**
 * @brief Get parameter access (RO, RW).
 *
 * @param par_num Parameter number (enumeration).
 * @return Parameter access.
 */
#if (1 == PAR_CFG_ENABLE_ACCESS)
par_access_t par_get_access(const par_num_t par_num)
{
    const par_cfg_t *par_cfg = NULL;

    if (ePAR_OK == par_resolve_metadata(par_num, NULL, false, &par_cfg))
    {
        return par_cfg->access;
    }

    return ePAR_ACCESS_RO;
}
#endif
/**
 * @brief Is parameter persistent (does it store to NVM).
 *
 * @param par_num Parameter number (enumeration).
 * @return True if parameter is persistent.
 */
#if (1 == PAR_CFG_ENABLE_PERSIST)
bool par_is_persistent(const par_num_t par_num)
{
    const par_cfg_t *par_cfg = NULL;

    if (ePAR_OK == par_resolve_metadata(par_num, NULL, false, &par_cfg))
    {
        return par_cfg->persistent;
    }

    return false;
}
#endif
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
#endif

#if (1 == PAR_CFG_NVM_EN)
/**
 * @brief Is parameter value changed.
 *
 * @param par_num Parameter number.
 * @param p_val Parameter value.
 * @return True if parameter value is different from current.
 */
static par_status_t par_is_value_changed(const par_num_t par_num, const void *p_val, bool * const p_value_changed)
{
    const par_cfg_t *par_cfg = NULL;
    par_status_t status = ePAR_OK;

    if ((NULL == p_val) || (NULL == p_value_changed))
    {
        return ePAR_ERROR_PARAM;
    }

    status = par_resolve_runtime(par_num, NULL, false, &par_cfg);
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
        *p_value_changed = (cur != *(const uint8_t *)p_val);
        break;
    }

    case ePAR_TYPE_I8:
    {
        int8_t cur = 0;
        const par_status_t status = par_get_i8(par_num, &cur);
        if (ePAR_OK != status)
            return status;
        *p_value_changed = (cur != *(const int8_t *)p_val);
        break;
    }

    case ePAR_TYPE_U16:
    {
        uint16_t cur = 0U;
        const par_status_t status = par_get_u16(par_num, &cur);
        if (ePAR_OK != status)
            return status;
        *p_value_changed = (cur != *(const uint16_t *)p_val);
        break;
    }

    case ePAR_TYPE_I16:
    {
        int16_t cur = 0;
        const par_status_t status = par_get_i16(par_num, &cur);
        if (ePAR_OK != status)
            return status;
        *p_value_changed = (cur != *(const int16_t *)p_val);
        break;
    }

    case ePAR_TYPE_U32:
    {
        uint32_t cur = 0U;
        const par_status_t status = par_get_u32(par_num, &cur);
        if (ePAR_OK != status)
            return status;
        *p_value_changed = (cur != *(const uint32_t *)p_val);
        break;
    }

    case ePAR_TYPE_I32:
    {
        int32_t cur = 0;
        const par_status_t status = par_get_i32(par_num, &cur);
        if (ePAR_OK != status)
            return status;
        *p_value_changed = (cur != *(const int32_t *)p_val);
        break;
    }

#if (1 == PAR_CFG_ENABLE_TYPE_F32)
    case ePAR_TYPE_F32:
    {
        float32_t cur = 0.0f;
        const par_status_t status = par_get_f32(par_num, &cur);
        if (ePAR_OK != status)
            return status;
        *p_value_changed = !par_f32_bits_equal(cur, *(const float32_t *)p_val);
        break;
    }
#endif

    case ePAR_TYPE_NUM_OF:
    default:
        PAR_ASSERT(0);
        return ePAR_ERROR_TYPE;
    }

    return ePAR_OK;
}
/**
 * @brief Set parameter value and save to NVM if value changed.
 *
 * @note Mandatory to cast input argument to appropriate type. E.g.:
 *
 * @code
 * float32_t my_val = 1.234f;.
 * par_set( ePAR_MY_VAR, (float32_t*) &my_val );.
 * @endcode
 *
 * @note Input is the internal parameter number (`par_num_t`) from `par_def.h`,
 * not the external parameter ID.
 *
 * @param par_num Parameter number (enumeration).
 * @param p_val Pointer to value.
 * @return Status of operation.
 */
par_status_t par_set_n_save(const par_num_t par_num, const void *p_val)
{
    bool value_change = false;
    par_status_t status = par_is_value_changed(par_num, p_val, &value_change);

    if (ePAR_OK != status)
    {
        PAR_DBG_PRINT("PAR: failed to read current value before set_n_save for par_num=%u with status=%s", (unsigned)par_num, par_get_status_str(status));
        PAR_ASSERT(0);
        return status;
    }

    status = par_set(par_num, p_val);
    /* Persist only when the value actually changed. */
    if ((ePAR_OK == status) && value_change)
    {
        status |= par_save(par_num);
    }

    return status;
}
/**
 * @brief Store all parameters value to NVM.
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

    return par_nvm_write_all();
}
/**
 * @brief Store single parameter value to NVM.
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

    return par_nvm_write(par_num, true);
}
/**
 * @brief Store single parameter value to NVM by its ID value.
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

    return ePAR_ERROR;
}
#endif
/**
 * @brief Clean all stored parameters inside NVM.
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

#endif
/**
 * @brief Register parameter on change callback.
 *
 * @param par_num Parameter number (enumeration).
 * @param cb Callback.
 */
#if (1 == PAR_CFG_ENABLE_CHANGE_CALLBACK)
void par_register_on_change_cb(const par_num_t par_num, const pf_par_on_change_cb_t cb)
{
    PAR_ASSERT(par_num < ePAR_NUM_OF);

    g_par_cb_table[par_num].on_change = cb;
}
#endif
/**
 * @brief Register parameter value validation function.
 *
 * @param par_num Parameter number (enumeration).
 * @param validation Validation.
 */
#if (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION)
void par_register_validation(const par_num_t par_num, const pf_par_validation_t validation)
{
    PAR_ASSERT(par_num < ePAR_NUM_OF);

    g_par_cb_table[par_num].validation = validation;
}
#endif

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
#endif
/**
 * @} <!-- END GROUP -->
 */
