/**
 * @file par_object_api.c
 * @brief Implement public object-parameter API entry points.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-05-01
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include <string.h>

#include "par.h"
#include "par_core.h"
#include "par_object.h"
#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
#include "par_nvm.h"
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
#if (1 == PAR_CFG_ENABLE_ID)
/**
 * @brief Resolve an external object parameter ID to an internal parameter number.
 * @param id External parameter ID.
 * @param[out] p_par_num Pointer to the resolved parameter number.
 * @return Operation status.
 */
static par_status_t par_object_resolve_id(const uint16_t id,
                                          par_num_t * const p_par_num)
{
    return par_get_num_by_id(id, p_par_num);
}
#endif /* (1 == PAR_CFG_ENABLE_ID) */
/**
 * @brief Set one object parameter through the shared checked object path.
 * @details Object setters support validation callbacks only; object on-change
 * callbacks are intentionally not part of the core API.
 * @param par_num Parameter number.
 * @param expected_type Expected object parameter type.
 * @param p_data Pointer to source payload bytes.
 * @param len Source payload length in bytes.
 * @return Operation status.
 */
static par_status_t par_set_obj_checked_core(const par_num_t par_num, const par_type_list_t expected_type, const uint8_t * const p_data, const uint16_t len)
{
    const par_cfg_t *par_cfg = NULL;
    par_status_t status = par_core_resolve_runtime(par_num, p_data, (len > 0U), &par_cfg);

    if (ePAR_OK != status)
    {
        return status;
    }
    if (expected_type != par_cfg->type)
    {
        return ePAR_ERROR_TYPE;
    }
#if (1 == PAR_CFG_ENABLE_ACCESS)
    if (false == par_core_access_has_write(par_cfg->access))
    {
        return ePAR_ERROR_ACCESS;
    }
#endif /* (1 == PAR_CFG_ENABLE_ACCESS) */
    if (true == par_object_source_overlaps_pool(p_data, len))
    {
        PAR_ASSERT(0);
        return ePAR_ERROR_PARAM;
    }
    if (false == par_object_len_is_valid(&par_cfg->value_cfg.object, len))
    {
        return ePAR_ERROR_VALUE;
    }
    if (ePAR_OK != par_acquire_mutex(par_num))
    {
        return ePAR_ERROR_MUTEX;
    }

#if (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION)
    if (false == par_core_object_validation_accepts(par_num, p_data, len))
    {
        par_release_mutex(par_num);
        return ePAR_ERROR_VALUE;
    }
#endif /* (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION) */

    status = par_object_write_payload(par_num, p_data, len);

    par_release_mutex(par_num);
    return status;
}

/**
 * @brief Set one fixed-capacity raw byte-array parameter.
 * @param par_num Parameter number.
 * @param p_data Pointer to the source bytes.
 * @param len Payload length in bytes.
 * @return Operation status.
 */
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
par_status_t par_set_bytes(const par_num_t par_num, const uint8_t *p_data, const uint16_t len)
{
    return par_set_obj_checked_core(par_num, ePAR_TYPE_BYTES, p_data, len);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */

/**
 * @brief Set one fixed-capacity string parameter.
 * @param par_num Parameter number.
 * @param p_str Pointer to a NUL-terminated source string, or NULL
 * for an empty string.
 * @return Operation status.
 */
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
par_status_t par_set_str(const par_num_t par_num, const char *p_str)
{
    const par_cfg_t *par_cfg = NULL;
    size_t len = 0U;
    size_t max_len = 0U;
    par_status_t status = par_core_resolve_runtime(par_num, NULL, false, &par_cfg);

    if (ePAR_OK != status)
    {
        return status;
    }
    if (ePAR_TYPE_STR != par_cfg->type)
    {
        return ePAR_ERROR_TYPE;
    }

    max_len = (size_t)par_cfg->value_cfg.object.range.max_len;
    if (NULL != p_str)
    {
        while ((len <= max_len) && ('\0' != p_str[len]))
        {
            len++;
        }
        if (len > max_len)
        {
            return ePAR_ERROR_VALUE;
        }
    }

    return par_set_obj_checked_core(par_num,
                                    ePAR_TYPE_STR,
                                    (const uint8_t *)p_str,
                                    (uint16_t)len);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */

/**
 * @brief Set one fixed-size unsigned array parameter through the shared object core.
 * @param par_num Parameter number.
 * @param expected_type Expected array type.
 * @param p_data Pointer to the source elements.
 * @param count Number of source elements.
 * @param elem_size Size of one array element in bytes.
 * @return Operation status.
 */
#if ((1 == PAR_CFG_ENABLE_TYPE_ARR_U8) || (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) || (1 == PAR_CFG_ENABLE_TYPE_ARR_U32))
static par_status_t par_set_array_checked_core(const par_num_t par_num,
                                               const par_type_list_t expected_type,
                                               const void *p_data,
                                               const uint16_t count,
                                               const uint16_t elem_size)
{
    uint32_t len = (uint32_t)count * (uint32_t)elem_size;

    if (len > UINT16_MAX)
    {
        return ePAR_ERROR_PARAM;
    }

    return par_set_obj_checked_core(par_num, expected_type, (const uint8_t *)p_data, (uint16_t)len);
}
#endif /* ((1 == PAR_CFG_ENABLE_TYPE_ARR_U8) || (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) || (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)) */

/**
 * @brief Set one fixed-size unsigned 8-bit array parameter.
 * @param par_num Parameter number.
 * @param p_data Pointer to the source elements.
 * @param count Number of source elements.
 * @return Operation status.
 */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
par_status_t par_set_arr_u8(const par_num_t par_num, const uint8_t *p_data, const uint16_t count)
{
    return par_set_array_checked_core(par_num, ePAR_TYPE_ARR_U8, p_data, count, 1U);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */

/**
 * @brief Set one fixed-size unsigned 16-bit array parameter.
 * @param par_num Parameter number.
 * @param p_data Pointer to the source elements.
 * @param count Number of source elements.
 * @return Operation status.
 */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
par_status_t par_set_arr_u16(const par_num_t par_num, const uint16_t *p_data, const uint16_t count)
{
    return par_set_array_checked_core(par_num, ePAR_TYPE_ARR_U16, p_data, count, (uint16_t)sizeof(uint16_t));
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */

/**
 * @brief Set one fixed-size unsigned 32-bit array parameter.
 * @param par_num Parameter number.
 * @param p_data Pointer to the source elements.
 * @param count Number of source elements.
 * @return Operation status.
 */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
par_status_t par_set_arr_u32(const par_num_t par_num, const uint32_t *p_data, const uint16_t count)
{
    return par_set_array_checked_core(par_num, ePAR_TYPE_ARR_U32, p_data, count, (uint16_t)sizeof(uint32_t));
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */


#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
#if ((1 == PAR_CFG_ENABLE_TYPE_ARR_U8) || (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) || (1 == PAR_CFG_ENABLE_TYPE_ARR_U32))
/**
 * @brief Read one fixed-size unsigned array parameter from the shared object pool.
 * @param par_num Parameter number.
 * @param expected_type Expected array type.
 * @param p_buf Pointer to the destination array buffer.
 * @param buf_count Destination capacity in elements.
 * @param elem_size Size of one element in bytes.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status.
 */
static par_status_t par_get_array_runtime_core(const par_num_t par_num,
                                               const par_type_list_t expected_type,
                                               void *p_buf,
                                               const uint16_t buf_count,
                                               const uint16_t elem_size,
                                               uint16_t *p_out_count)
{
    const par_cfg_t *par_cfg = NULL;
    const uint8_t *p_payload = NULL;
    const par_status_t status = par_core_resolve_runtime(par_num,
                                                         (NULL != p_buf) ? p_buf : (void *)p_out_count,
                                                         ((NULL == p_buf) && (NULL == p_out_count)),
                                                         &par_cfg);
    uint16_t len = 0U;
    uint16_t capacity = 0U;
    uint16_t count = 0U;
    par_status_t obj_status = ePAR_OK;

    if (ePAR_OK != status)
    {
        return status;
    }
    if (expected_type != par_cfg->type)
    {
        return ePAR_ERROR_TYPE;
    }
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

    obj_status = par_object_get_view(par_num, &p_payload, &len, &capacity);
    (void)capacity;
    if (ePAR_OK != obj_status)
    {
        par_release_mutex(par_num);
        return obj_status;
    }
    if ((0U == elem_size) || ((len % elem_size) != 0U))
    {
        par_release_mutex(par_num);
        return ePAR_ERROR;
    }

    count = (uint16_t)(len / elem_size);
    if (NULL != p_out_count)
    {
        *p_out_count = count;
    }
    if (NULL == p_buf)
    {
        par_release_mutex(par_num);
        return ePAR_OK;
    }
    if (buf_count < count)
    {
        par_release_mutex(par_num);
        return ePAR_ERROR_PARAM;
    }

    if (len > 0U)
    {
        memcpy(p_buf, p_payload, len);
    }

    par_release_mutex(par_num);
    return ePAR_OK;
}

/**
 * @brief Read one fixed-size unsigned array default payload from table metadata.
 * @param par_num Parameter number.
 * @param expected_type Expected array type.
 * @param p_buf Pointer to the destination array buffer.
 * @param buf_count Destination capacity in elements.
 * @param elem_size Size of one element in bytes.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status.
 */
static par_status_t par_get_array_default_core(const par_num_t par_num,
                                               const par_type_list_t expected_type,
                                               void *p_buf,
                                               const uint16_t buf_count,
                                               const uint16_t elem_size,
                                               uint16_t *p_out_count)
{
    const par_cfg_t *par_cfg = NULL;
    const par_status_t status = par_core_resolve_metadata(par_num,
                                                          (NULL != p_buf) ? p_buf : (void *)p_out_count,
                                                          ((NULL == p_buf) && (NULL == p_out_count)),
                                                          &par_cfg);
    uint16_t count = 0U;

    if (ePAR_OK != status)
    {
        return status;
    }
    if (expected_type != par_cfg->type)
    {
        return ePAR_ERROR_TYPE;
    }
    if (false == par_object_default_cfg_is_valid(&par_cfg->value_cfg.object))
    {
        return ePAR_ERROR_INIT;
    }
    if ((0U == elem_size) || ((par_cfg->value_cfg.object.def.len % elem_size) != 0U))
    {
        return ePAR_ERROR;
    }

    count = (uint16_t)(par_cfg->value_cfg.object.def.len / elem_size);
    if (NULL != p_out_count)
    {
        *p_out_count = count;
    }
    if (NULL == p_buf)
    {
        return ePAR_OK;
    }
    if (buf_count < count)
    {
        return ePAR_ERROR_PARAM;
    }

    if (par_cfg->value_cfg.object.def.len > 0U)
    {
        memcpy(p_buf, par_cfg->value_cfg.object.def.p_data, par_cfg->value_cfg.object.def.len);
    }
    return ePAR_OK;
}
#endif /* ((1 == PAR_CFG_ENABLE_TYPE_ARR_U8) || (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) || (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)) */
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

/**
 * @brief Return the current payload length of one object parameter.
 * @param par_num Parameter number.
 * @param[out] p_len Pointer to the returned payload length in bytes.
 * @return Operation status.
 */
#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
par_status_t par_get_obj_len(const par_num_t par_num, uint16_t *p_len)
{
    const par_cfg_t *par_cfg = NULL;
    const uint8_t *p_payload = NULL;
    const par_status_t status = par_core_resolve_runtime(par_num, p_len, true, &par_cfg);
    uint16_t capacity = 0U;
    par_status_t obj_status = ePAR_OK;

    if (ePAR_OK != status)
    {
        return status;
    }
    if (false == par_object_type_is_object(par_cfg->type))
    {
        return ePAR_ERROR_TYPE;
    }
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

    obj_status = par_object_get_view(par_num, &p_payload, p_len, &capacity);
    (void)p_payload;
    (void)capacity;
    par_release_mutex(par_num);
    return obj_status;
}

/**
 * @brief Return the configured payload capacity of one object parameter.
 * @param par_num Parameter number.
 * @param[out] p_capacity Pointer to the returned payload capacity in bytes.
 * @return Operation status.
 */
par_status_t par_get_obj_capacity(const par_num_t par_num, uint16_t *p_capacity)
{
    const par_cfg_t *par_cfg = NULL;
    const par_status_t status = par_core_resolve_metadata(par_num, p_capacity, true, &par_cfg);

    if (ePAR_OK != status)
    {
        return status;
    }
    if (false == par_object_type_is_object(par_cfg->type))
    {
        return ePAR_ERROR_TYPE;
    }

    *p_capacity = par_cfg->value_cfg.object.range.max_len;
    return ePAR_OK;
}

#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

/**
 * @brief Read one fixed-capacity raw byte-array parameter.
 * @param par_num Parameter number.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_size Destination buffer size in bytes.
 * @param[out] p_out_len Pointer to the returned payload length in bytes.
 * @return Operation status.
 */
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
par_status_t par_get_bytes(const par_num_t par_num, uint8_t *p_buf, const uint16_t buf_size, uint16_t *p_out_len)
{
    const par_cfg_t *par_cfg = NULL;
    const uint8_t *p_payload = NULL;
    const par_status_t status = par_core_resolve_runtime(par_num, (NULL != p_buf) ? (const void *)p_buf : (const void *)p_out_len, ((NULL == p_buf) && (NULL == p_out_len)), &par_cfg);
    uint16_t len = 0U;
    uint16_t capacity = 0U;
    par_status_t obj_status = ePAR_OK;

    if (ePAR_OK != status)
    {
        return status;
    }
    if (ePAR_TYPE_BYTES != par_cfg->type)
    {
        return ePAR_ERROR_TYPE;
    }
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

    obj_status = par_object_get_view(par_num, &p_payload, &len, &capacity);
    (void)capacity;
    if (ePAR_OK != obj_status)
    {
        par_release_mutex(par_num);
        return obj_status;
    }
    if (NULL != p_out_len)
    {
        *p_out_len = len;
    }
    if (NULL == p_buf)
    {
        par_release_mutex(par_num);
        return ePAR_OK;
    }
    if (buf_size < len)
    {
        par_release_mutex(par_num);
        return ePAR_ERROR_PARAM;
    }

    if (len > 0U)
    {
        memcpy(p_buf, p_payload, len);
    }
    par_release_mutex(par_num);
    return ePAR_OK;
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */

/**
 * @brief Read one fixed-capacity string parameter.
 * @param par_num Parameter number.
 * @param p_buf Pointer to the destination character buffer.
 * @param buf_size Destination buffer size in bytes, including the trailing '\0'.
 * Must be at least payload length + 1.
 * @param[out] p_out_len Pointer to the returned string length without the trailing '\0'.
 * @return Operation status.
 */
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
par_status_t par_get_str(const par_num_t par_num, char *p_buf, const uint16_t buf_size, uint16_t *p_out_len)
{
    const par_cfg_t *par_cfg = NULL;
    const uint8_t *p_payload = NULL;
    const par_status_t status = par_core_resolve_runtime(par_num, (NULL != p_buf) ? (const void *)p_buf : (const void *)p_out_len, ((NULL == p_buf) && (NULL == p_out_len)), &par_cfg);
    uint16_t len = 0U;
    uint16_t capacity = 0U;
    par_status_t obj_status = ePAR_OK;

    if (ePAR_OK != status)
    {
        return status;
    }
    if (ePAR_TYPE_STR != par_cfg->type)
    {
        return ePAR_ERROR_TYPE;
    }
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

    obj_status = par_object_get_view(par_num, &p_payload, &len, &capacity);
    (void)capacity;
    if (ePAR_OK != obj_status)
    {
        par_release_mutex(par_num);
        return obj_status;
    }
    if (NULL != p_out_len)
    {
        *p_out_len = len;
    }
    if (NULL == p_buf)
    {
        par_release_mutex(par_num);
        return ePAR_OK;
    }
    if (buf_size <= len)
    {
        par_release_mutex(par_num);
        return ePAR_ERROR_PARAM;
    }

    if (len > 0U)
    {
        memcpy(p_buf, p_payload, len);
    }
    p_buf[len] = '\0';
    par_release_mutex(par_num);
    return ePAR_OK;
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */

/**
 * @brief Read one fixed-size unsigned 8-bit array parameter.
 * @param par_num Parameter number.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status.
 */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
par_status_t par_get_arr_u8(const par_num_t par_num, uint8_t *p_buf, const uint16_t buf_count, uint16_t *p_out_count)
{
    return par_get_array_runtime_core(par_num, ePAR_TYPE_ARR_U8, p_buf, buf_count, 1U, p_out_count);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */

/**
 * @brief Read one fixed-size unsigned 16-bit array parameter.
 * @param par_num Parameter number.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status.
 */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
par_status_t par_get_arr_u16(const par_num_t par_num, uint16_t *p_buf, const uint16_t buf_count, uint16_t *p_out_count)
{
    return par_get_array_runtime_core(par_num, ePAR_TYPE_ARR_U16, p_buf, buf_count, (uint16_t)sizeof(uint16_t), p_out_count);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */

/**
 * @brief Read one fixed-size unsigned 32-bit array parameter.
 * @param par_num Parameter number.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status.
 */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
par_status_t par_get_arr_u32(const par_num_t par_num, uint32_t *p_buf, const uint16_t buf_count, uint16_t *p_out_count)
{
    return par_get_array_runtime_core(par_num, ePAR_TYPE_ARR_U32, p_buf, buf_count, (uint16_t)sizeof(uint32_t), p_out_count);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */


/**
 * @brief Read the default raw byte-array payload for one object parameter.
 * @param par_num Parameter number.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_size Destination buffer size in bytes.
 * @param[out] p_out_len Pointer to the returned payload length in bytes.
 * @return Operation status.
 */
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
par_status_t par_get_default_bytes(const par_num_t par_num, uint8_t *p_buf, const uint16_t buf_size, uint16_t *p_out_len)
{
    const par_cfg_t *par_cfg = NULL;
    const par_status_t status = par_core_resolve_metadata(par_num, (NULL != p_buf) ? (const void *)p_buf : (const void *)p_out_len, ((NULL == p_buf) && (NULL == p_out_len)), &par_cfg);

    if (ePAR_OK != status)
    {
        return status;
    }
    if (ePAR_TYPE_BYTES != par_cfg->type)
    {
        return ePAR_ERROR_TYPE;
    }
    if (false == par_object_default_cfg_is_valid(&par_cfg->value_cfg.object))
    {
        return ePAR_ERROR_INIT;
    }
    if (NULL != p_out_len)
    {
        *p_out_len = par_cfg->value_cfg.object.def.len;
    }
    if (NULL == p_buf)
    {
        return ePAR_OK;
    }
    if (buf_size < par_cfg->value_cfg.object.def.len)
    {
        return ePAR_ERROR_PARAM;
    }
    if (par_cfg->value_cfg.object.def.len > 0U)
    {
        memcpy(p_buf, par_cfg->value_cfg.object.def.p_data, par_cfg->value_cfg.object.def.len);
    }
    return ePAR_OK;
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */

/**
 * @brief Read the default string payload for one object parameter.
 * @param par_num Parameter number.
 * @param p_buf Pointer to the destination character buffer.
 * @param buf_size Destination buffer size in bytes, including the trailing '\0'.
 * Must be at least payload length + 1.
 * @param[out] p_out_len Pointer to the returned string length without the trailing '\0'.
 * @return Operation status.
 */
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
par_status_t par_get_default_str(const par_num_t par_num, char *p_buf, const uint16_t buf_size, uint16_t *p_out_len)
{
    const par_cfg_t *par_cfg = NULL;
    const par_status_t status = par_core_resolve_metadata(par_num, (NULL != p_buf) ? (const void *)p_buf : (const void *)p_out_len, ((NULL == p_buf) && (NULL == p_out_len)), &par_cfg);

    if (ePAR_OK != status)
    {
        return status;
    }
    if (ePAR_TYPE_STR != par_cfg->type)
    {
        return ePAR_ERROR_TYPE;
    }
    if (false == par_object_default_cfg_is_valid(&par_cfg->value_cfg.object))
    {
        return ePAR_ERROR_INIT;
    }
    if (NULL != p_out_len)
    {
        *p_out_len = par_cfg->value_cfg.object.def.len;
    }
    if (NULL == p_buf)
    {
        return ePAR_OK;
    }
    if (buf_size <= par_cfg->value_cfg.object.def.len)
    {
        return ePAR_ERROR_PARAM;
    }
    if (par_cfg->value_cfg.object.def.len > 0U)
    {
        memcpy(p_buf, par_cfg->value_cfg.object.def.p_data, par_cfg->value_cfg.object.def.len);
    }
    p_buf[par_cfg->value_cfg.object.def.len] = '\0';
    return ePAR_OK;
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */

/**
 * @brief Read the default unsigned 8-bit array payload for one object parameter.
 * @param par_num Parameter number.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status.
 */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
par_status_t par_get_default_arr_u8(const par_num_t par_num, uint8_t *p_buf, const uint16_t buf_count, uint16_t *p_out_count)
{
    return par_get_array_default_core(par_num, ePAR_TYPE_ARR_U8, p_buf, buf_count, 1U, p_out_count);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */

/**
 * @brief Read the default unsigned 16-bit array payload for one object parameter.
 * @param par_num Parameter number.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status.
 */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
par_status_t par_get_default_arr_u16(const par_num_t par_num, uint16_t *p_buf, const uint16_t buf_count, uint16_t *p_out_count)
{
    return par_get_array_default_core(par_num, ePAR_TYPE_ARR_U16, p_buf, buf_count, (uint16_t)sizeof(uint16_t), p_out_count);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */

/**
 * @brief Read the default unsigned 32-bit array payload for one object parameter.
 * @param par_num Parameter number.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status.
 */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
par_status_t par_get_default_arr_u32(const par_num_t par_num, uint32_t *p_buf, const uint16_t buf_count, uint16_t *p_out_count)
{
    return par_get_array_default_core(par_num, ePAR_TYPE_ARR_U32, p_buf, buf_count, (uint16_t)sizeof(uint32_t), p_out_count);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */


#if (1 == PAR_CFG_ENABLE_ID)
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
/**
 * @brief Set one fixed-capacity raw byte-array parameter by external ID.
 * @param id External parameter ID.
 * @param p_data Pointer to the source bytes.
 * @param len Payload length in bytes.
 * @return Operation status.
 */
par_status_t par_set_bytes_by_id(const uint16_t id,
                                 const uint8_t *p_data,
                                 const uint16_t len)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_object_resolve_id(id, &par_num);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_set_bytes(par_num, p_data, len);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */

#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
/**
 * @brief Read one fixed-capacity raw byte-array parameter by external ID.
 * @param id External parameter ID.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_size Destination buffer size in bytes.
 * @param[out] p_out_len Pointer to the returned payload length in bytes.
 * @return Operation status.
 */
par_status_t par_get_bytes_by_id(const uint16_t id,
                                 uint8_t *p_buf,
                                 const uint16_t buf_size,
                                 uint16_t *p_out_len)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_object_resolve_id(id, &par_num);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_get_bytes(par_num, p_buf, buf_size, p_out_len);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */

#if (1 == PAR_CFG_ENABLE_TYPE_STR)
/**
 * @brief Set one fixed-capacity string parameter by external ID.
 * @param id External parameter ID.
 * @param p_str Pointer to a NUL-terminated source string, or NULL
 * for an empty string.
 * @return Operation status.
 */
par_status_t par_set_str_by_id(const uint16_t id, const char *p_str)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_object_resolve_id(id, &par_num);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_set_str(par_num, p_str);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */

#if (1 == PAR_CFG_ENABLE_TYPE_STR)
/**
 * @brief Read one fixed-capacity string parameter by external ID.
 * @param id External parameter ID.
 * @param p_buf Pointer to the destination character buffer.
 * @param buf_size Destination buffer size in bytes, including the trailing NUL.
 * @param[out] p_out_len Pointer to the returned string length without the trailing NUL.
 * @return Operation status.
 */
par_status_t par_get_str_by_id(const uint16_t id,
                               char *p_buf,
                               const uint16_t buf_size,
                               uint16_t *p_out_len)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_object_resolve_id(id, &par_num);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_get_str(par_num, p_buf, buf_size, p_out_len);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */

#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
/**
 * @brief Set one fixed-size unsigned 8-bit array parameter by external ID.
 * @param id External parameter ID.
 * @param p_data Pointer to the source elements.
 * @param count Number of source elements.
 * @return Operation status.
 */
par_status_t par_set_arr_u8_by_id(const uint16_t id,
                                  const uint8_t *p_data,
                                  const uint16_t count)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_object_resolve_id(id, &par_num);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_set_arr_u8(par_num, p_data, count);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */

#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
/**
 * @brief Read one fixed-size unsigned 8-bit array parameter by external ID.
 * @param id External parameter ID.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status.
 */
par_status_t par_get_arr_u8_by_id(const uint16_t id,
                                  uint8_t *p_buf,
                                  const uint16_t buf_count,
                                  uint16_t *p_out_count)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_object_resolve_id(id, &par_num);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_get_arr_u8(par_num, p_buf, buf_count, p_out_count);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */

#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
/**
 * @brief Set one fixed-size unsigned 16-bit array parameter by external ID.
 * @param id External parameter ID.
 * @param p_data Pointer to the source elements.
 * @param count Number of source elements.
 * @return Operation status.
 */
par_status_t par_set_arr_u16_by_id(const uint16_t id,
                                   const uint16_t *p_data,
                                   const uint16_t count)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_object_resolve_id(id, &par_num);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_set_arr_u16(par_num, p_data, count);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */

#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
/**
 * @brief Read one fixed-size unsigned 16-bit array parameter by external ID.
 * @param id External parameter ID.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status.
 */
par_status_t par_get_arr_u16_by_id(const uint16_t id,
                                   uint16_t *p_buf,
                                   const uint16_t buf_count,
                                   uint16_t *p_out_count)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_object_resolve_id(id, &par_num);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_get_arr_u16(par_num, p_buf, buf_count, p_out_count);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */

#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
/**
 * @brief Set one fixed-size unsigned 32-bit array parameter by external ID.
 * @param id External parameter ID.
 * @param p_data Pointer to the source elements.
 * @param count Number of source elements.
 * @return Operation status.
 */
par_status_t par_set_arr_u32_by_id(const uint16_t id,
                                   const uint32_t *p_data,
                                   const uint16_t count)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_object_resolve_id(id, &par_num);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_set_arr_u32(par_num, p_data, count);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */

#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
/**
 * @brief Read one fixed-size unsigned 32-bit array parameter by external ID.
 * @param id External parameter ID.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status.
 */
par_status_t par_get_arr_u32_by_id(const uint16_t id,
                                   uint32_t *p_buf,
                                   const uint16_t buf_count,
                                   uint16_t *p_out_count)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_object_resolve_id(id, &par_num);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_get_arr_u32(par_num, p_buf, buf_count, p_out_count);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */

#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
/**
 * @brief Return the current payload length of one object parameter by external ID.
 * @param id External parameter ID.
 * @param[out] p_len Pointer to the returned payload length in bytes.
 * @return Operation status.
 */
par_status_t par_get_obj_len_by_id(const uint16_t id, uint16_t *p_len)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_object_resolve_id(id, &par_num);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_get_obj_len(par_num, p_len);
}

/**
 * @brief Return the configured object payload capacity by external ID.
 * @param id External parameter ID.
 * @param[out] p_capacity Pointer to the returned payload capacity in bytes.
 * @return Operation status.
 */
par_status_t par_get_obj_capacity_by_id(const uint16_t id, uint16_t *p_capacity)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_object_resolve_id(id, &par_num);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_get_obj_capacity(par_num, p_capacity);
}
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
/**
 * @brief Read the default byte-array payload by external ID.
 * @param id External parameter ID.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_size Destination buffer size in bytes.
 * @param[out] p_out_len Pointer to the returned default payload length in bytes.
 * @return Operation status.
 */
par_status_t par_get_default_bytes_by_id(const uint16_t id,
                                         uint8_t *p_buf,
                                         const uint16_t buf_size,
                                         uint16_t *p_out_len)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_object_resolve_id(id, &par_num);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_get_default_bytes(par_num, p_buf, buf_size, p_out_len);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */

#if (1 == PAR_CFG_ENABLE_TYPE_STR)
/**
 * @brief Read the default string payload by external ID.
 * @param id External parameter ID.
 * @param p_buf Pointer to the destination character buffer.
 * @param buf_size Destination buffer size in bytes, including the trailing NUL.
 * @param[out] p_out_len Pointer to the returned default string length without the trailing NUL.
 * @return Operation status.
 */
par_status_t par_get_default_str_by_id(const uint16_t id,
                                       char *p_buf,
                                       const uint16_t buf_size,
                                       uint16_t *p_out_len)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_object_resolve_id(id, &par_num);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_get_default_str(par_num, p_buf, buf_size, p_out_len);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */

#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
/**
 * @brief Read the default unsigned 8-bit array payload by external ID.
 * @param id External parameter ID.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status.
 */
par_status_t par_get_default_arr_u8_by_id(const uint16_t id,
                                          uint8_t *p_buf,
                                          const uint16_t buf_count,
                                          uint16_t *p_out_count)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_object_resolve_id(id, &par_num);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_get_default_arr_u8(par_num, p_buf, buf_count, p_out_count);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */

#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
/**
 * @brief Read the default unsigned 16-bit array payload by external ID.
 * @param id External parameter ID.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status.
 */
par_status_t par_get_default_arr_u16_by_id(const uint16_t id,
                                           uint16_t *p_buf,
                                           const uint16_t buf_count,
                                           uint16_t *p_out_count)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_object_resolve_id(id, &par_num);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_get_default_arr_u16(par_num, p_buf, buf_count, p_out_count);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */

#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
/**
 * @brief Read the default unsigned 32-bit array payload by external ID.
 * @param id External parameter ID.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status.
 */
par_status_t par_get_default_arr_u32_by_id(const uint16_t id,
                                           uint32_t *p_buf,
                                           const uint16_t buf_count,
                                           uint16_t *p_out_count)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_object_resolve_id(id, &par_num);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_get_default_arr_u32(par_num, p_buf, buf_count, p_out_count);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */
#endif /* (1 == PAR_CFG_ENABLE_ID) */


#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
/**
 * @brief Set one object payload and save to NVM if changed.
 * @param par_num Parameter number.
 * @param p_data Pointer to payload bytes, or NULL when len is zero.
 * @param len Payload length in bytes.
 * @return Status of operation. Returns ePAR_ERROR_TYPE for scalar rows.
 */
par_status_t par_set_obj_n_save(const par_num_t par_num,
                                const uint8_t *p_data,
                                const uint16_t len)
{
    const par_cfg_t *par_cfg = NULL;
    const uint8_t *p_payload = NULL;
    uint16_t cur_len = 0U;
    uint16_t capacity = 0U;
    bool value_change = false;
    bool locked = false;
    par_status_t status = par_core_resolve_runtime(par_num, p_data, (len > 0U), &par_cfg);

    if (ePAR_OK != status)
    {
        return status;
    }
    if (false == par_object_type_is_object(par_cfg->type))
    {
        return ePAR_ERROR_TYPE;
    }
#if (1 == PAR_CFG_ENABLE_ACCESS)
    if (false == par_core_access_has_write(par_cfg->access))
    {
        return ePAR_ERROR_ACCESS;
    }
#endif /* (1 == PAR_CFG_ENABLE_ACCESS) */
    if (true == par_object_source_overlaps_pool(p_data, len))
    {
        PAR_ASSERT(0);
        return ePAR_ERROR_PARAM;
    }
    if (false == par_object_len_is_valid(&par_cfg->value_cfg.object, len))
    {
        return ePAR_ERROR_VALUE;
    }
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
    if ((ePAR_TYPE_STR == par_cfg->type) && (len > 0U) &&
        (NULL != memchr(p_data, '\0', (size_t)len)))
    {
        return ePAR_ERROR_VALUE;
    }
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */

    if (ePAR_OK != par_acquire_mutex(par_num))
    {
        return ePAR_ERROR_MUTEX;
    }
    locked = true;

    status = par_object_get_view(par_num, &p_payload, &cur_len, &capacity);
    (void)capacity;
    if (ePAR_OK != status)
    {
        goto out;
    }

    if (cur_len != len)
    {
        value_change = true;
    }
    else if (0U == len)
    {
        value_change = false;
    }
    else
    {
        value_change = (0 != memcmp(p_payload, p_data, len));
    }

#if (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION)
    if (false == par_core_object_validation_accepts(par_num, p_data, len))
    {
        status = ePAR_ERROR_VALUE;
        goto out;
    }
#endif /* (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION) */

    status = par_object_write_payload(par_num, p_data, len);
    if (ePAR_OK != status)
    {
        goto out;
    }

    if (true == value_change)
    {
        PAR_DBG_PRINT("PAR: obj_n_save detected payload change, par_num=%u",
                      (unsigned)par_num);
        status |= par_nvm_write_object_locked(par_num, true);
    }
    else
    {
        PAR_DBG_PRINT("PAR: obj_n_save skipped NVM write because payload is "
                      "unchanged, par_num=%u",
                      (unsigned)par_num);
    }

out:
    if (true == locked)
    {
        par_release_mutex(par_num);
    }

    return status;
}
#if (1 == PAR_CFG_ENABLE_ID)
/**
 * @brief Set one object payload by external ID and persist it if changed.
 * @param id External parameter ID.
 * @param p_data Pointer to payload bytes, or NULL when len is zero.
 * @param len Payload length in bytes.
 * @return Operation status. Returns ePAR_ERROR_TYPE for scalar rows.
 */
par_status_t par_set_obj_n_save_by_id(const uint16_t id,
                                      const uint8_t *p_data,
                                      const uint16_t len)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_object_resolve_id(id, &par_num);

    if (ePAR_OK != status)
    {
        return status;
    }

    return par_set_obj_n_save(par_num, p_data, len);
}
#endif /* (1 == PAR_CFG_ENABLE_ID) */
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

#if (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
/**
 * @brief Register one object validation callback.
 * @param par_num Parameter number.
 * @param validation Object validation callback function pointer.
 */
void par_register_obj_validation(const par_num_t par_num, const pf_par_obj_validation_t validation)
{
    par_core_register_obj_validation(par_num, validation);
}
#endif /* (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */
