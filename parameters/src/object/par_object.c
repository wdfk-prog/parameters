/**
 * @file par_object.c
 * @brief Implement fixed-capacity object-parameter storage helpers.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-04-28
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include "par_object.h"

#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)

#include <string.h>

#include "par_layout.h"
#include "par_if.h"

/**
 * @brief Runtime descriptor for one fixed-capacity object parameter.
 * @details Payload bytes live in `gs_par_object_pool`, while this slot keeps
 * the mapping from a parameter number to its assigned byte range inside that
 * pool. This avoids heap allocation and keeps object values deterministic.
 */
typedef struct
{
    uint32_t pool_offset; /**< Byte offset of the assigned payload window. */
    uint16_t len;         /**< Current payload length in bytes. */
    uint16_t capacity;    /**< Reserved payload capacity in bytes. */
} par_obj_slot_t;

/**
 * @brief Backing storage for all fixed-capacity object payload bytes.
 */
static uint8_t gs_par_object_pool[PAR_STORAGE_NONZERO(PAR_STORAGE_OBJ_POOL_BYTES)] = { 0 };

/**
 * @brief Slot table mapping object parameters to payload-pool windows.
 */
static par_obj_slot_t gs_par_object_slots[PAR_STORAGE_NONZERO(PAR_STORAGE_COUNTOBJ)] = { 0 };

#if (1 == PAR_CFG_ENABLE_RESET_ALL_RAW)
/**
 * @brief Default mirror of the shared object payload pool used by raw reset-all.
 */
static uint8_t gs_par_object_pool_default_mirror[PAR_STORAGE_NONZERO(PAR_STORAGE_OBJ_POOL_BYTES)] = { 0 };

/**
 * @brief Default mirror of the object slot table used by raw reset-all.
 */
static par_obj_slot_t gs_par_object_slots_default_mirror[PAR_STORAGE_NONZERO(PAR_STORAGE_COUNTOBJ)] = { 0 };
#endif /* (1 == PAR_CFG_ENABLE_RESET_ALL_RAW) */

/**
 * @brief Return the runtime object slot for one object parameter.
 *
 * @param par_num Parameter number.
 * @return Pointer to the assigned object slot.
 */
static par_obj_slot_t *par_object_get_slot(const par_num_t par_num)
{
    return &gs_par_object_slots[par_layout_get_offset_table()[par_num]];
}

/**
 * @brief Return whether one object slot range is inside the payload pool.
 *
 * @param p_slot Object slot to validate.
 * @return true when the slot window is fully inside the pool.
 */
static bool par_object_slot_range_is_valid(const par_obj_slot_t * const p_slot)
{
    if (NULL == p_slot)
    {
        return false;
    }
    if (p_slot->pool_offset > (uint32_t)sizeof(gs_par_object_pool))
    {
        return false;
    }
    if ((uint32_t)p_slot->capacity > ((uint32_t)sizeof(gs_par_object_pool) - p_slot->pool_offset))
    {
        return false;
    }
    if (p_slot->len > p_slot->capacity)
    {
        return false;
    }

    return true;
}

/**
 * @brief Report whether a parameter type is handled by object storage.
 * @param type Parameter type tag.
 * @return True when the type is a fixed-capacity object type.
 */
bool par_object_type_is_object(const par_type_list_t type)
{
    switch (type)
    {
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
        return true;

    default:
        return false;
    }
}

/**
 * @brief Validate an object payload length against object configuration.
 * @param p_obj_cfg Object parameter configuration.
 * @param len Payload length in bytes.
 * @return True when the length is inside range and element-aligned.
 */
bool par_object_len_is_valid(const par_object_cfg_t * const p_obj_cfg,
                             const uint16_t len)
{
    if (NULL == p_obj_cfg)
    {
        return false;
    }
    if ((len < p_obj_cfg->range.min_len) || (len > p_obj_cfg->range.max_len))
    {
        return false;
    }
    if ((p_obj_cfg->elem_size > 0U) && ((len % p_obj_cfg->elem_size) != 0U))
    {
        return false;
    }

    return true;
}

/**
 * @brief Validate default payload metadata for one object parameter.
 * @param p_obj_cfg Object parameter configuration.
 * @return True when default length, range, and source pointer are valid.
 */
bool par_object_default_cfg_is_valid(const par_object_cfg_t * const p_obj_cfg)
{
    if (NULL == p_obj_cfg)
    {
        return false;
    }
    if (p_obj_cfg->range.min_len > p_obj_cfg->range.max_len)
    {
        return false;
    }
    if (false == par_object_len_is_valid(p_obj_cfg, p_obj_cfg->def.len))
    {
        return false;
    }
    if ((p_obj_cfg->def.len > 0U) && (NULL == p_obj_cfg->def.p_data))
    {
        return false;
    }

    return true;
}

/**
 * @brief Calculate object slot table storage size.
 * @param object_count Number of object parameter slots.
 * @return Required slot table size in bytes.
 */
uint32_t par_object_slot_table_bytes(const uint16_t object_count)
{
    return (uint32_t)object_count * (uint32_t)sizeof(par_obj_slot_t);
}

/**
 * @brief Initialize object slots and payload pool from par_table.def defaults.
 * @return Operation status.
 */
par_status_t par_object_init_defaults_from_table(void)
{
    uint32_t expected_pool_off = 0U;

    memset(gs_par_object_pool, 0, sizeof(gs_par_object_pool));
    memset(gs_par_object_slots, 0, sizeof(gs_par_object_slots));

    for (par_num_t par_num = 0; par_num < ePAR_NUM_OF; par_num++)
    {
        const par_cfg_t * const p_cfg = par_cfg_get(par_num);
        par_obj_slot_t *p_slot = NULL;

        if (false == par_object_type_is_object(p_cfg->type))
        {
            continue;
        }
        if (false == par_object_default_cfg_is_valid(&p_cfg->value_cfg.object))
        {
            PAR_ERR_PRINT("PAR: invalid object default configuration at par_num=%u", (unsigned)par_num);
            PAR_ASSERT(0);
            return ePAR_ERROR_INIT;
        }
        if (par_layout_get_offset_table()[par_num] >= (uint16_t)PAR_STORAGE_COUNTOBJ)
        {
            PAR_ERR_PRINT("PAR: object slot offset out of range at par_num=%u", (unsigned)par_num);
            PAR_ASSERT(0);
            return ePAR_ERROR_INIT;
        }

        p_slot = par_object_get_slot(par_num);
        p_slot->pool_offset = par_layout_get_obj_pool_offset(par_num);
        p_slot->capacity = p_cfg->value_cfg.object.range.max_len;
        p_slot->len = p_cfg->value_cfg.object.def.len;

        if (expected_pool_off != p_slot->pool_offset)
        {
            PAR_ERR_PRINT("PAR: object layout offset mismatch at par_num=%u, expected=%lu got=%lu",
                          (unsigned)par_num,
                          (unsigned long)expected_pool_off,
                          (unsigned long)p_slot->pool_offset);
            PAR_ASSERT(0);
            return ePAR_ERROR_INIT;
        }
        if (false == par_object_slot_range_is_valid(p_slot))
        {
            PAR_ERR_PRINT("PAR: object layout range exceeds pool at par_num=%u", (unsigned)par_num);
            PAR_ASSERT(0);
            return ePAR_ERROR_INIT;
        }

        expected_pool_off += (uint32_t)p_slot->capacity;

        if ((NULL != p_cfg->value_cfg.object.def.p_data) && (p_slot->len > 0U))
        {
            memcpy(&gs_par_object_pool[p_slot->pool_offset], p_cfg->value_cfg.object.def.p_data, p_slot->len);
        }
    }

    if (expected_pool_off != (uint32_t)PAR_STORAGE_OBJ_POOL_BYTES)
    {
        PAR_ERR_PRINT("PAR: object pool size mismatch, expected=%lu got=%lu",
                      (unsigned long)PAR_STORAGE_OBJ_POOL_BYTES,
                      (unsigned long)expected_pool_off);
        PAR_ASSERT(0);
        return ePAR_ERROR_INIT;
    }

    return ePAR_OK;
}

#if (1 == PAR_CFG_ENABLE_RESET_ALL_RAW)
/**
 * @brief Snapshot current object defaults into the raw reset-all mirror.
 */
void par_object_snapshot_default_mirror(void)
{
    memcpy(&gs_par_object_pool_default_mirror, &gs_par_object_pool, sizeof(gs_par_object_pool));
    memcpy(&gs_par_object_slots_default_mirror, &gs_par_object_slots, sizeof(gs_par_object_slots));
}

/**
 * @brief Restore object payloads and slots from the raw reset-all mirror.
 */
void par_object_restore_default_mirror(void)
{
    memcpy(&gs_par_object_pool, &gs_par_object_pool_default_mirror, sizeof(gs_par_object_pool));
    memcpy(&gs_par_object_slots, &gs_par_object_slots_default_mirror, sizeof(gs_par_object_slots));
}
#endif /* (1 == PAR_CFG_ENABLE_RESET_ALL_RAW) */

/**
 * @brief Check whether source bytes overlap the internal object pool.
 * @param p_data Source payload pointer.
 * @param len Source payload length in bytes.
 * @return True when the source range overlaps object backing storage.
 */
bool par_object_source_overlaps_pool(const uint8_t * const p_data,
                                     const uint16_t len)
{
    const uintptr_t pool_start = (uintptr_t)&gs_par_object_pool[0];
    const uintptr_t pool_end = pool_start + (uintptr_t)sizeof(gs_par_object_pool);
    const uintptr_t src_start = (uintptr_t)p_data;
    const uintptr_t src_end = src_start + (uintptr_t)len;

    if ((NULL == p_data) || (0U == len))
    {
        return false;
    }

    return ((src_start < pool_end) && (src_end > pool_start));
}

/**
 * @brief Get a read-only view of one object payload window.
 * @param par_num Parameter number.
 * @param[out] pp_data Pointer receiving the payload address.
 * @param[out] p_len Pointer receiving current payload length in bytes.
 * @param[out] p_capacity Pointer receiving payload capacity in bytes.
 * @return Operation status.
 */
par_status_t par_object_get_view(const par_num_t par_num,
                                 const uint8_t **pp_data,
                                 uint16_t * const p_len,
                                 uint16_t * const p_capacity)
{
    const par_cfg_t *p_cfg = NULL;
    const par_obj_slot_t *p_slot = NULL;

    if ((NULL == pp_data) || (NULL == p_len) || (NULL == p_capacity))
    {
        return ePAR_ERROR_PARAM;
    }
    *pp_data = NULL;
    *p_len = 0U;
    *p_capacity = 0U;

    if (par_num >= ePAR_NUM_OF)
    {
        return ePAR_ERROR_PAR_NUM;
    }

    p_cfg = par_get_config(par_num);
    if ((NULL == p_cfg) || (false == par_object_type_is_object(p_cfg->type)))
    {
        return ePAR_ERROR_TYPE;
    }

    p_slot = par_object_get_slot(par_num);
    if (false == par_object_slot_range_is_valid(p_slot))
    {
        return ePAR_ERROR_INIT;
    }

    *pp_data = &gs_par_object_pool[p_slot->pool_offset];
    *p_len = p_slot->len;
    *p_capacity = p_slot->capacity;
    return ePAR_OK;
}

/**
 * @brief Get a mutable view of one object payload window.
 * @param par_num Parameter number.
 * @param[out] pp_data Pointer receiving the payload address.
 * @param[out] p_capacity Pointer receiving payload capacity in bytes.
 * @return Operation status.
 */
par_status_t par_object_get_mutable_view(const par_num_t par_num,
                                         uint8_t **pp_data,
                                         uint16_t * const p_capacity)
{
    const par_cfg_t *p_cfg = NULL;
    const par_obj_slot_t *p_slot = NULL;

    if ((NULL == pp_data) || (NULL == p_capacity))
    {
        return ePAR_ERROR_PARAM;
    }
    *pp_data = NULL;
    *p_capacity = 0U;

    if (par_num >= ePAR_NUM_OF)
    {
        return ePAR_ERROR_PAR_NUM;
    }

    p_cfg = par_get_config(par_num);
    if ((NULL == p_cfg) || (false == par_object_type_is_object(p_cfg->type)))
    {
        return ePAR_ERROR_TYPE;
    }

    p_slot = par_object_get_slot(par_num);
    if (false == par_object_slot_range_is_valid(p_slot))
    {
        return ePAR_ERROR_INIT;
    }

    *pp_data = &gs_par_object_pool[p_slot->pool_offset];
    *p_capacity = p_slot->capacity;
    return ePAR_OK;
}

/**
 * @brief Commit a new current length for one object payload.
 * @param par_num Parameter number.
 * @param len New payload length in bytes.
 * @param clear_tail True to clear bytes after the committed payload.
 * @return Operation status.
 */
par_status_t par_object_commit_len(const par_num_t par_num,
                                   const uint16_t len,
                                   const bool clear_tail)
{
    const par_cfg_t *p_cfg = NULL;
    par_obj_slot_t *p_slot = NULL;

    if (par_num >= ePAR_NUM_OF)
    {
        return ePAR_ERROR_PAR_NUM;
    }

    p_cfg = par_get_config(par_num);
    if ((NULL == p_cfg) || (false == par_object_type_is_object(p_cfg->type)))
    {
        return ePAR_ERROR_TYPE;
    }
    if (false == par_object_len_is_valid(&p_cfg->value_cfg.object, len))
    {
        return ePAR_ERROR_VALUE;
    }

    p_slot = par_object_get_slot(par_num);
    if ((false == par_object_slot_range_is_valid(p_slot)) || (len > p_slot->capacity))
    {
        return ePAR_ERROR_INIT;
    }
    if ((true == clear_tail) && (len < p_slot->capacity))
    {
        (void)memset(&gs_par_object_pool[p_slot->pool_offset + (uint32_t)len], 0, (size_t)(p_slot->capacity - len));
    }
    p_slot->len = len;
    return ePAR_OK;
}

/**
 * @brief Replace one object payload and commit its length.
 * @param par_num Parameter number.
 * @param p_data Source payload bytes.
 * @param len Source payload length in bytes.
 * @return Operation status.
 */
par_status_t par_object_write_payload(const par_num_t par_num,
                                      const uint8_t * const p_data,
                                      const uint16_t len)
{
    const par_cfg_t *p_cfg = NULL;
    uint8_t *p_payload = NULL;
    uint16_t capacity = 0U;
    par_status_t status = ePAR_OK;

    if ((NULL == p_data) && (len > 0U))
    {
        return ePAR_ERROR_PARAM;
    }
    if (par_num >= ePAR_NUM_OF)
    {
        return ePAR_ERROR_PAR_NUM;
    }

    p_cfg = par_get_config(par_num);
    if ((NULL == p_cfg) || (false == par_object_type_is_object(p_cfg->type)))
    {
        return ePAR_ERROR_TYPE;
    }
    if (false == par_object_len_is_valid(&p_cfg->value_cfg.object, len))
    {
        return ePAR_ERROR_VALUE;
    }

    status = par_object_get_mutable_view(par_num, &p_payload, &capacity);
    if (ePAR_OK != status)
    {
        return status;
    }
    if (len > capacity)
    {
        return ePAR_ERROR_VALUE;
    }

    if (capacity > 0U)
    {
        memset(p_payload, 0, capacity);
    }
    if (len > 0U)
    {
        memcpy(p_payload, p_data, len);
    }

    return par_object_commit_len(par_num, len, false);
}

/**
 * @brief Restore one object parameter to its configured default payload.
 * @param par_num Parameter number.
 * @return Operation status.
 */
par_status_t par_object_write_default(const par_num_t par_num)
{
    const par_cfg_t *p_cfg = NULL;

    if (par_num >= ePAR_NUM_OF)
    {
        return ePAR_ERROR_PAR_NUM;
    }
    p_cfg = par_get_config(par_num);
    if ((NULL == p_cfg) || (false == par_object_type_is_object(p_cfg->type)))
    {
        return ePAR_ERROR_TYPE;
    }
    if (false == par_object_default_cfg_is_valid(&p_cfg->value_cfg.object))
    {
        return ePAR_ERROR_INIT;
    }

    return par_object_write_payload(par_num,
                                    p_cfg->value_cfg.object.def.p_data,
                                    p_cfg->value_cfg.object.def.len);
}

/**
 * @brief Compare one live object payload with its configured default.
 * @param par_num Parameter number.
 * @param[out] p_has_changed Pointer receiving the changed flag.
 * @return Operation status.
 */
par_status_t par_object_payload_changed_from_default(const par_num_t par_num,
                                                     bool * const p_has_changed)
{
    const par_cfg_t *p_cfg = NULL;
    const uint8_t *p_payload = NULL;
    uint16_t len = 0U;
    uint16_t capacity = 0U;
    par_status_t status = ePAR_OK;

    if (NULL == p_has_changed)
    {
        return ePAR_ERROR_PARAM;
    }
    *p_has_changed = false;

    if (par_num >= ePAR_NUM_OF)
    {
        return ePAR_ERROR_PAR_NUM;
    }
    p_cfg = par_get_config(par_num);
    if ((NULL == p_cfg) || (false == par_object_type_is_object(p_cfg->type)))
    {
        return ePAR_ERROR_TYPE;
    }
    if (false == par_object_default_cfg_is_valid(&p_cfg->value_cfg.object))
    {
        return ePAR_ERROR_INIT;
    }

    status = par_object_get_view(par_num, &p_payload, &len, &capacity);
    (void)capacity;
    if (ePAR_OK != status)
    {
        return status;
    }

    if (len != p_cfg->value_cfg.object.def.len)
    {
        *p_has_changed = true;
    }
    else if (0U == len)
    {
        *p_has_changed = false;
    }
    else
    {
        *p_has_changed = (0 != memcmp(p_payload, p_cfg->value_cfg.object.def.p_data, len));
    }

    return ePAR_OK;
}

#if (1 == PAR_CFG_NVM_EN)
/**
 * @brief Export a read-only object payload view for NVM persistence.
 * @param par_num Parameter number.
 * @param[out] pp_data Pointer receiving the payload address.
 * @param[out] p_len Pointer receiving current payload length in bytes.
 * @param[out] p_capacity Pointer receiving payload capacity in bytes.
 * @return Operation status.
 */
par_status_t par_obj_nvm_export(const par_num_t par_num,
                                const uint8_t **pp_data,
                                uint16_t * const p_len,
                                uint16_t * const p_capacity)
{
    return par_object_get_view(par_num, pp_data, p_len, p_capacity);
}

/**
 * @brief Export a mutable object payload window for NVM restore.
 * @param par_num Parameter number.
 * @param[out] pp_data Pointer receiving the payload address.
 * @param[out] p_capacity Pointer receiving payload capacity in bytes.
 * @return Operation status.
 */
par_status_t par_obj_nvm_restore_window(const par_num_t par_num,
                                        uint8_t **pp_data,
                                        uint16_t * const p_capacity)
{
    return par_object_get_mutable_view(par_num, pp_data, p_capacity);
}

/**
 * @brief Commit the length of a restored object payload.
 * @param par_num Parameter number.
 * @param len Restored payload length in bytes.
 * @return Operation status.
 */
par_status_t par_obj_nvm_commit_restore(const par_num_t par_num,
                                        const uint16_t len)
{
    return par_object_commit_len(par_num, len, true);
}

/**
 * @brief Restore one object payload from NVM data.
 * @param par_num Parameter number.
 * @param p_data Restored payload bytes.
 * @param len Restored payload length in bytes.
 * @return Operation status.
 */
par_status_t par_obj_nvm_restore(const par_num_t par_num,
                                 const uint8_t * const p_data,
                                 const uint16_t len)
{
    const par_cfg_t *p_cfg = NULL;
    uint8_t *p_payload = NULL;
    uint16_t capacity = 0U;
    bool locked = false;
    par_status_t status = ePAR_OK;

    if ((NULL == p_data) && (len > 0U))
    {
        return ePAR_ERROR_PARAM;
    }
    if (par_num >= ePAR_NUM_OF)
    {
        return ePAR_ERROR_PAR_NUM;
    }

    p_cfg = par_get_config(par_num);
    if ((NULL == p_cfg) || (false == par_object_type_is_object(p_cfg->type)))
    {
        return ePAR_ERROR_TYPE;
    }
    if (false == par_object_len_is_valid(&p_cfg->value_cfg.object, len))
    {
        return ePAR_ERROR_VALUE;
    }

    if (ePAR_OK != par_acquire_mutex(par_num))
    {
        return ePAR_ERROR_MUTEX;
    }
    locked = true;

    status = par_object_get_mutable_view(par_num, &p_payload, &capacity);
    if (ePAR_OK != status)
    {
        goto out;
    }
    if (len > capacity)
    {
        status = ePAR_ERROR_VALUE;
        goto out;
    }
    if (len > 0U)
    {
        memcpy(p_payload, p_data, len);
    }
    status = par_object_commit_len(par_num, len, true);

out:
    if (true == locked)
    {
        par_release_mutex(par_num);
    }
    return status;
}
#endif /* (1 == PAR_CFG_NVM_EN) */
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */
