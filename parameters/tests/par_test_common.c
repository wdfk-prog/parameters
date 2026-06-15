/**
 * @file par_test_common.c
 * @brief Implement reusable helpers for parameter runtime tests.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#include "par_test.h"

#include <limits.h>
#include <string.h>

#include "par_def.h"

/**
 * @brief Return whether a status contains no error bits.
 * @param status Parameter API status value.
 * @return true when no error bit is set; otherwise false.
 */
static bool par_test_status_ok(const par_status_t status)
{
    return PAR_TEST_STATUS_HAS_NO_ERROR(status);
}

/**
 * @brief Return true when an unsigned value can be decremented without wrap.
 * @param value Current unsigned value.
 * @return true when value is greater than zero.
 */
static bool par_test_u32_has_below(const uint32_t value)
{
    return (value > 0UL);
}

/**
 * @brief Return true when an unsigned value can be incremented without wrap.
 * @param value Current unsigned value.
 * @return true when value is lower than UINT32_MAX.
 */
static bool par_test_u32_has_above(const uint32_t value)
{
    return (value < UINT32_MAX);
}

/**
 * @brief Return true when a signed value can be decremented without overflow.
 * @param value Current signed value.
 * @return true when value is greater than INT32_MIN.
 */
static bool par_test_i32_has_below(const int32_t value)
{
    return (value > INT32_MIN);
}

/**
 * @brief Return true when a signed value can be incremented without overflow.
 * @param value Current signed value.
 * @return true when value is lower than INT32_MAX.
 */
static bool par_test_i32_has_above(const int32_t value)
{
    return (value < INT32_MAX);
}

/**
 * @brief Return whether a parameter type uses scalar storage.
 * @param type Parameter type identifier.
 * @return true for scalar types; otherwise false.
 */
bool par_test_type_is_scalar(const par_type_list_t type)
{
    switch (type)
    {
    case ePAR_TYPE_U8:
    case ePAR_TYPE_U16:
    case ePAR_TYPE_U32:
    case ePAR_TYPE_I8:
    case ePAR_TYPE_I16:
    case ePAR_TYPE_I32:
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
    case ePAR_TYPE_F32:
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */
        return true;

    default:
        return false;
    }
}

/**
 * @brief Return whether a parameter configuration permits normal writes.
 * @param cfg Parameter configuration pointer.
 * @return true when normal setter APIs may write the parameter.
 */
bool par_test_cfg_is_writable(const par_cfg_t *cfg)
{
    if (NULL == cfg)
    {
        return false;
    }

#if (1 == PAR_CFG_ENABLE_ACCESS)
    return (0U != ((uint32_t)cfg->access & (uint32_t)ePAR_ACCESS_WRITE));
#else
    return true;
#endif /* (1 == PAR_CFG_ENABLE_ACCESS) */
}

/**
 * @brief Read a scalar parameter through the generic public getter.
 * @param par_num Parameter number.
 * @param value Destination scalar union.
 * @return Operation status.
 */
par_status_t par_test_read_scalar(const par_num_t par_num, par_type_t *value)
{
    if (NULL == value)
    {
        return ePAR_ERROR_PARAM;
    }

    (void)memset(value, 0, sizeof(*value));
    return par_get_scalar(par_num, value);
}

/**
 * @brief Write a scalar parameter through the generic public setter.
 * @param par_num Parameter number.
 * @param value Source scalar union.
 * @return Operation status.
 */
par_status_t par_test_set_scalar(const par_num_t par_num, const par_type_t *value)
{
    if (NULL == value)
    {
        return ePAR_ERROR_PARAM;
    }

    return par_set_scalar(par_num, value);
}

/**
 * @brief Restore a scalar parameter through the generic fast setter.
 * @param par_num Parameter number.
 * @param value Source scalar union.
 * @return Operation status.
 */
par_status_t par_test_set_scalar_fast(const par_num_t par_num, const par_type_t *value)
{
    if (NULL == value)
    {
        return ePAR_ERROR_PARAM;
    }

    return par_set_scalar_fast(par_num, value);
}

/**
 * @brief Compare two scalar unions using the selected parameter type.
 * @param type Scalar parameter type.
 * @param lhs Left-hand scalar value.
 * @param rhs Right-hand scalar value.
 * @return true when both scalar values are equal for the selected type.
 */
bool par_test_scalar_equal(const par_type_list_t type, const par_type_t *lhs, const par_type_t *rhs)
{
    if ((NULL == lhs) || (NULL == rhs))
    {
        return false;
    }

    switch (type)
    {
    case ePAR_TYPE_U8:
        return (lhs->u8 == rhs->u8);
    case ePAR_TYPE_U16:
        return (lhs->u16 == rhs->u16);
    case ePAR_TYPE_U32:
        return (lhs->u32 == rhs->u32);
    case ePAR_TYPE_I8:
        return (lhs->i8 == rhs->i8);
    case ePAR_TYPE_I16:
        return (lhs->i16 == rhs->i16);
    case ePAR_TYPE_I32:
        return (lhs->i32 == rhs->i32);
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
    case ePAR_TYPE_F32:
        return (lhs->f32 == rhs->f32);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */
    default:
        return false;
    }
}

/**
 * @brief Build a different in-range scalar value for mutation tests.
 * @param par_num Parameter number.
 * @param current Current scalar value.
 * @param alternate Destination alternate scalar value.
 * @return true when a different value could be generated.
 */
bool par_test_make_alternate_scalar(const par_num_t par_num, const par_type_t *current, par_type_t *alternate)
{
    const par_cfg_t *cfg = par_get_config(par_num);

    if ((NULL == cfg) || (NULL == current) || (NULL == alternate) || (false == par_test_type_is_scalar(cfg->type)))
    {
        return false;
    }

    *alternate = *current;

    switch (cfg->type)
    {
    case ePAR_TYPE_U8:
#if (1 == PAR_CFG_ENABLE_RANGE)
        if (cfg->value_cfg.scalar.range.min.u8 >= cfg->value_cfg.scalar.range.max.u8)
        {
            return false;
        }
        alternate->u8 = (current->u8 != cfg->value_cfg.scalar.range.min.u8) ?
                        cfg->value_cfg.scalar.range.min.u8 :
                        cfg->value_cfg.scalar.range.max.u8;
#else
        alternate->u8 = (uint8_t)(current->u8 + 1U);
#endif /* (1 == PAR_CFG_ENABLE_RANGE) */
        return (alternate->u8 != current->u8);

    case ePAR_TYPE_U16:
#if (1 == PAR_CFG_ENABLE_RANGE)
        if (cfg->value_cfg.scalar.range.min.u16 >= cfg->value_cfg.scalar.range.max.u16)
        {
            return false;
        }
        alternate->u16 = (current->u16 != cfg->value_cfg.scalar.range.min.u16) ?
                         cfg->value_cfg.scalar.range.min.u16 :
                         cfg->value_cfg.scalar.range.max.u16;
#else
        alternate->u16 = (uint16_t)(current->u16 + 1U);
#endif /* (1 == PAR_CFG_ENABLE_RANGE) */
        return (alternate->u16 != current->u16);

    case ePAR_TYPE_U32:
#if (1 == PAR_CFG_ENABLE_RANGE)
        if (cfg->value_cfg.scalar.range.min.u32 >= cfg->value_cfg.scalar.range.max.u32)
        {
            return false;
        }
        alternate->u32 = (current->u32 != cfg->value_cfg.scalar.range.min.u32) ?
                         cfg->value_cfg.scalar.range.min.u32 :
                         cfg->value_cfg.scalar.range.max.u32;
#else
        alternate->u32 = current->u32 + 1UL;
#endif /* (1 == PAR_CFG_ENABLE_RANGE) */
        return (alternate->u32 != current->u32);

    case ePAR_TYPE_I8:
#if (1 == PAR_CFG_ENABLE_RANGE)
        if (cfg->value_cfg.scalar.range.min.i8 >= cfg->value_cfg.scalar.range.max.i8)
        {
            return false;
        }
        alternate->i8 = (current->i8 != cfg->value_cfg.scalar.range.min.i8) ?
                        cfg->value_cfg.scalar.range.min.i8 :
                        cfg->value_cfg.scalar.range.max.i8;
#else
        alternate->i8 = (int8_t)(current->i8 + 1);
#endif /* (1 == PAR_CFG_ENABLE_RANGE) */
        return (alternate->i8 != current->i8);

    case ePAR_TYPE_I16:
#if (1 == PAR_CFG_ENABLE_RANGE)
        if (cfg->value_cfg.scalar.range.min.i16 >= cfg->value_cfg.scalar.range.max.i16)
        {
            return false;
        }
        alternate->i16 = (current->i16 != cfg->value_cfg.scalar.range.min.i16) ?
                         cfg->value_cfg.scalar.range.min.i16 :
                         cfg->value_cfg.scalar.range.max.i16;
#else
        alternate->i16 = (int16_t)(current->i16 + 1);
#endif /* (1 == PAR_CFG_ENABLE_RANGE) */
        return (alternate->i16 != current->i16);

    case ePAR_TYPE_I32:
#if (1 == PAR_CFG_ENABLE_RANGE)
        if (cfg->value_cfg.scalar.range.min.i32 >= cfg->value_cfg.scalar.range.max.i32)
        {
            return false;
        }
        alternate->i32 = (current->i32 != cfg->value_cfg.scalar.range.min.i32) ?
                         cfg->value_cfg.scalar.range.min.i32 :
                         cfg->value_cfg.scalar.range.max.i32;
#else
        alternate->i32 = (current->i32 != INT32_MAX) ? (current->i32 + 1L) : (current->i32 - 1L);
#endif /* (1 == PAR_CFG_ENABLE_RANGE) */
        return (alternate->i32 != current->i32);

#if (1 == PAR_CFG_ENABLE_TYPE_F32)
    case ePAR_TYPE_F32:
#if (1 == PAR_CFG_ENABLE_RANGE)
        if (!(cfg->value_cfg.scalar.range.min.f32 < cfg->value_cfg.scalar.range.max.f32))
        {
            return false;
        }
        alternate->f32 = (current->f32 != cfg->value_cfg.scalar.range.min.f32) ?
                         cfg->value_cfg.scalar.range.min.f32 :
                         cfg->value_cfg.scalar.range.max.f32;
#else
        alternate->f32 = current->f32 + 1.0f;
#endif /* (1 == PAR_CFG_ENABLE_RANGE) */
        return (alternate->f32 != current->f32);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */

    default:
        return false;
    }
}

/**
 * @brief Build a scalar value below the configured lower range limit.
 * @param par_num Parameter number.
 * @param value Destination scalar value.
 * @return true when a representable below-range value could be generated.
 */
bool par_test_make_below_range_scalar(const par_num_t par_num, par_type_t *value)
{
#if (1 == PAR_CFG_ENABLE_RANGE)
    const par_cfg_t *cfg = par_get_config(par_num);

    if ((NULL == cfg) || (NULL == value) || (false == par_test_type_is_scalar(cfg->type)))
    {
        return false;
    }

    (void)memset(value, 0, sizeof(*value));
    switch (cfg->type)
    {
    case ePAR_TYPE_U8:
        if (false == par_test_u32_has_below((uint32_t)cfg->value_cfg.scalar.range.min.u8))
        {
            return false;
        }
        value->u8 = (uint8_t)(cfg->value_cfg.scalar.range.min.u8 - 1U);
        return true;
    case ePAR_TYPE_U16:
        if (false == par_test_u32_has_below((uint32_t)cfg->value_cfg.scalar.range.min.u16))
        {
            return false;
        }
        value->u16 = (uint16_t)(cfg->value_cfg.scalar.range.min.u16 - 1U);
        return true;
    case ePAR_TYPE_U32:
        if (false == par_test_u32_has_below(cfg->value_cfg.scalar.range.min.u32))
        {
            return false;
        }
        value->u32 = cfg->value_cfg.scalar.range.min.u32 - 1UL;
        return true;
    case ePAR_TYPE_I8:
        if (false == par_test_i32_has_below((int32_t)cfg->value_cfg.scalar.range.min.i8))
        {
            return false;
        }
        value->i8 = (int8_t)(cfg->value_cfg.scalar.range.min.i8 - 1);
        return true;
    case ePAR_TYPE_I16:
        if (false == par_test_i32_has_below((int32_t)cfg->value_cfg.scalar.range.min.i16))
        {
            return false;
        }
        value->i16 = (int16_t)(cfg->value_cfg.scalar.range.min.i16 - 1);
        return true;
    case ePAR_TYPE_I32:
        if (false == par_test_i32_has_below(cfg->value_cfg.scalar.range.min.i32))
        {
            return false;
        }
        value->i32 = cfg->value_cfg.scalar.range.min.i32 - 1L;
        return true;
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
    case ePAR_TYPE_F32:
        value->f32 = cfg->value_cfg.scalar.range.min.f32 - 1.0f;
        return (value->f32 < cfg->value_cfg.scalar.range.min.f32);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */
    default:
        return false;
    }
#else
    (void)par_num;
    (void)value;
    return false;
#endif /* (1 == PAR_CFG_ENABLE_RANGE) */
}

/**
 * @brief Build a scalar value above the configured upper range limit.
 * @param par_num Parameter number.
 * @param value Destination scalar value.
 * @return true when a representable above-range value could be generated.
 */
bool par_test_make_above_range_scalar(const par_num_t par_num, par_type_t *value)
{
#if (1 == PAR_CFG_ENABLE_RANGE)
    const par_cfg_t *cfg = par_get_config(par_num);

    if ((NULL == cfg) || (NULL == value) || (false == par_test_type_is_scalar(cfg->type)))
    {
        return false;
    }

    (void)memset(value, 0, sizeof(*value));
    switch (cfg->type)
    {
    case ePAR_TYPE_U8:
        if (false == par_test_u32_has_above((uint32_t)cfg->value_cfg.scalar.range.max.u8))
        {
            return false;
        }
        value->u8 = (uint8_t)(cfg->value_cfg.scalar.range.max.u8 + 1U);
        return true;
    case ePAR_TYPE_U16:
        if (false == par_test_u32_has_above((uint32_t)cfg->value_cfg.scalar.range.max.u16))
        {
            return false;
        }
        value->u16 = (uint16_t)(cfg->value_cfg.scalar.range.max.u16 + 1U);
        return true;
    case ePAR_TYPE_U32:
        if (false == par_test_u32_has_above(cfg->value_cfg.scalar.range.max.u32))
        {
            return false;
        }
        value->u32 = cfg->value_cfg.scalar.range.max.u32 + 1UL;
        return true;
    case ePAR_TYPE_I8:
        if (false == par_test_i32_has_above((int32_t)cfg->value_cfg.scalar.range.max.i8))
        {
            return false;
        }
        value->i8 = (int8_t)(cfg->value_cfg.scalar.range.max.i8 + 1);
        return true;
    case ePAR_TYPE_I16:
        if (false == par_test_i32_has_above((int32_t)cfg->value_cfg.scalar.range.max.i16))
        {
            return false;
        }
        value->i16 = (int16_t)(cfg->value_cfg.scalar.range.max.i16 + 1);
        return true;
    case ePAR_TYPE_I32:
        if (false == par_test_i32_has_above(cfg->value_cfg.scalar.range.max.i32))
        {
            return false;
        }
        value->i32 = cfg->value_cfg.scalar.range.max.i32 + 1L;
        return true;
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
    case ePAR_TYPE_F32:
        value->f32 = cfg->value_cfg.scalar.range.max.f32 + 1.0f;
        return (value->f32 > cfg->value_cfg.scalar.range.max.f32);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */
    default:
        return false;
    }
#else
    (void)par_num;
    (void)value;
    return false;
#endif /* (1 == PAR_CFG_ENABLE_RANGE) */
}

/**
 * @brief Read one scalar range boundary from the parameter configuration.
 * @param par_num Parameter number.
 * @param upper Select upper limit when true, lower limit when false.
 * @param limit Destination scalar limit value.
 * @return true when the range limit is available for the selected scalar type.
 */
bool par_test_get_range_limit_scalar(const par_num_t par_num, const bool upper, par_type_t *limit)
{
#if (1 == PAR_CFG_ENABLE_RANGE)
    const par_cfg_t *cfg = par_get_config(par_num);

    if ((NULL == cfg) || (NULL == limit) || (false == par_test_type_is_scalar(cfg->type)))
    {
        return false;
    }

    (void)memset(limit, 0, sizeof(*limit));
    switch (cfg->type)
    {
    case ePAR_TYPE_U8:
        limit->u8 = (true == upper) ? cfg->value_cfg.scalar.range.max.u8 : cfg->value_cfg.scalar.range.min.u8;
        return true;
    case ePAR_TYPE_U16:
        limit->u16 = (true == upper) ? cfg->value_cfg.scalar.range.max.u16 : cfg->value_cfg.scalar.range.min.u16;
        return true;
    case ePAR_TYPE_U32:
        limit->u32 = (true == upper) ? cfg->value_cfg.scalar.range.max.u32 : cfg->value_cfg.scalar.range.min.u32;
        return true;
    case ePAR_TYPE_I8:
        limit->i8 = (true == upper) ? cfg->value_cfg.scalar.range.max.i8 : cfg->value_cfg.scalar.range.min.i8;
        return true;
    case ePAR_TYPE_I16:
        limit->i16 = (true == upper) ? cfg->value_cfg.scalar.range.max.i16 : cfg->value_cfg.scalar.range.min.i16;
        return true;
    case ePAR_TYPE_I32:
        limit->i32 = (true == upper) ? cfg->value_cfg.scalar.range.max.i32 : cfg->value_cfg.scalar.range.min.i32;
        return true;
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
    case ePAR_TYPE_F32:
        limit->f32 = (true == upper) ? cfg->value_cfg.scalar.range.max.f32 : cfg->value_cfg.scalar.range.min.f32;
        return true;
#endif /* (1 == PAR_CFG_ENABLE_TYPE_F32) */
    default:
        return false;
    }
#else
    (void)par_num;
    (void)upper;
    (void)limit;
    return false;
#endif /* (1 == PAR_CFG_ENABLE_RANGE) */
}

/**
 * @brief Find the first writable scalar parameter matching persistence needs.
 * @param require_persistent Require NVM persistence when true.
 * @param par_num Destination parameter number.
 * @return true when a matching parameter is found.
 */
bool par_test_find_writable_scalar(const bool require_persistent, par_num_t *par_num)
{
    par_num_t it;

    if (NULL == par_num)
    {
        return false;
    }

    for (it = 0U; it < (par_num_t)ePAR_NUM_OF; it++)
    {
        const par_cfg_t *cfg = par_get_config(it);
        if ((NULL == cfg) || (false == par_test_type_is_scalar(cfg->type)) || (false == par_test_cfg_is_writable(cfg)))
        {
            continue;
        }

#if (1 == PAR_CFG_NVM_EN)
        if ((true == require_persistent) && (false == cfg->persistent))
        {
            continue;
        }
#else
        if (true == require_persistent)
        {
            continue;
        }
#endif /* (1 == PAR_CFG_NVM_EN) */

        *par_num = it;
        return true;
    }

    return false;
}

/**
 * @brief Find a writable scalar parameter with a usable alternate value.
 * @param require_persistent Require NVM persistence when true.
 * @param par_num Destination parameter number.
 * @param original Destination current scalar value.
 * @param alternate Destination alternate scalar value.
 * @return true when a mutable writable scalar is found.
 */
bool par_test_find_mutable_writable_scalar(const bool require_persistent,
                                           par_num_t *par_num,
                                           par_type_t *original,
                                           par_type_t *alternate)
{
    par_num_t it;

    if ((NULL == par_num) || (NULL == original) || (NULL == alternate))
    {
        return false;
    }

    for (it = 0U; it < (par_num_t)ePAR_NUM_OF; it++)
    {
        const par_cfg_t *cfg = par_get_config(it);
        par_status_t status;

        if ((NULL == cfg) || (false == par_test_type_is_scalar(cfg->type)) || (false == par_test_cfg_is_writable(cfg)))
        {
            continue;
        }

#if (1 == PAR_CFG_NVM_EN)
        if ((true == require_persistent) && (false == cfg->persistent))
        {
            continue;
        }
#else
        if (true == require_persistent)
        {
            continue;
        }
#endif /* (1 == PAR_CFG_NVM_EN) */

        status = par_test_read_scalar(it, original);
        if (false == par_test_status_ok(status))
        {
            continue;
        }

        if (false == par_test_make_alternate_scalar(it, original, alternate))
        {
            continue;
        }

        *par_num = it;
        return true;
    }

    return false;
}
