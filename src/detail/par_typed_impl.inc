/**
 * @file par_typed_impl.inc
 * @brief Generate typed getter and setter implementations.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-03-27
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-03-27 1.0     wdfk-prog    first version
 */
/**
 * @brief Macro-generated typed parameter APIs and checked write core.
 * @details This fragment is included only by par.c and expands the typed setter/getter.
 * families from a single type table.
 * Generated API groups:
 * par_set_u8/i8/u16/i16/u32/i32(/f32).
 * par_set_u8_fast/i8_fast/u16_fast/i16_fast/u32_fast/i32_fast(/f32_fast).
 * par_get_u8/i8/u16/i16/u32/i32(/f32).
 */

#define PAR_TYPED_ROWS_BASE(X)                \
    X(u8, U8, uint8_t, ePAR_TYPE_U8, u8)      \
    X(i8, I8, int8_t, ePAR_TYPE_I8, i8)       \
    X(u16, U16, uint16_t, ePAR_TYPE_U16, u16) \
    X(i16, I16, int16_t, ePAR_TYPE_I16, i16)  \
    X(u32, U32, uint32_t, ePAR_TYPE_U32, u32) \
    X(i32, I32, int32_t, ePAR_TYPE_I32, i32)

#if (1 == PAR_CFG_ENABLE_TYPE_F32)
#define PAR_TYPED_ROWS_F32(X) \
    X(f32, F32, float32_t, ePAR_TYPE_F32, f32)
#else
#define PAR_TYPED_ROWS_F32(X)
#endif

#define PAR_TYPED_ROWS(X)  \
    PAR_TYPED_ROWS_BASE(X) \
    PAR_TYPED_ROWS_F32(X)

#if (1 == PAR_CFG_ENABLE_RANGE)
#define PAR_TYPED_FAST_SET_BODY(PRIV, FIELD, par_num, val)                         \
    do                                                                             \
    {                                                                              \
        const par_range_t range = par_get_range(par_num);                          \
        if ((val) > range.max.FIELD)                                               \
        {                                                                          \
            PAR_SET_##PRIV##_PRIV((par_num), range.max.FIELD);                     \
            PAR_WARN_PRINT("PAR: value limited to configured maximum, par_num=%u", \
                           (unsigned)(par_num));                                   \
            return ePAR_WAR_LIMITED;                                               \
        }                                                                          \
        else if ((val) < range.min.FIELD)                                          \
        {                                                                          \
            PAR_SET_##PRIV##_PRIV((par_num), range.min.FIELD);                     \
            PAR_WARN_PRINT("PAR: value limited to configured minimum, par_num=%u", \
                           (unsigned)(par_num));                                   \
            return ePAR_WAR_LIMITED;                                               \
        }                                                                          \
        else                                                                       \
        {                                                                          \
            PAR_SET_##PRIV##_PRIV((par_num), (val));                               \
            return ePAR_OK;                                                        \
        }                                                                          \
    } while (0)
#else
#define PAR_TYPED_FAST_SET_BODY(PRIV, FIELD, par_num, val) \
    do                                                     \
    {                                                      \
        PAR_SET_##PRIV##_PRIV((par_num), (val));           \
        return ePAR_OK;                                    \
    } while (0)
#endif

#define PAR_CHECKED_VALUE_CHANGED_u8(new_val, old_val)     ((new_val).u8 != (old_val).u8)
#define PAR_CHECKED_VALUE_CHANGED_i8(new_val, old_val)     ((new_val).i8 != (old_val).i8)
#define PAR_CHECKED_VALUE_CHANGED_u16(new_val, old_val)    ((new_val).u16 != (old_val).u16)
#define PAR_CHECKED_VALUE_CHANGED_i16(new_val, old_val)    ((new_val).i16 != (old_val).i16)
#define PAR_CHECKED_VALUE_CHANGED_u32(new_val, old_val)    ((new_val).u32 != (old_val).u32)
#define PAR_CHECKED_VALUE_CHANGED_i32(new_val, old_val)    ((new_val).i32 != (old_val).i32)
#define PAR_CHECKED_VALUE_CHANGED_f32(new_val, old_val)    (!par_f32_bits_equal((new_val).f32, (old_val).f32))
#define PAR_CHECKED_VALUE_CHANGED(FIELD, new_val, old_val) PAR_CHECKED_VALUE_CHANGED_##FIELD((new_val), (old_val))

#if (1 == PAR_CFG_ENABLE_CHANGE_CALLBACK)
#define PAR_CHECKED_OLD_VAL_DECL(PRIV, FIELD, par_num) \
    const par_type_t old_val = { .FIELD = PAR_GET_##PRIV##_PRIV(par_num) };
#define PAR_CHECKED_ON_CHANGE(PRIV, FIELD, par_num, status_var)                     \
    do                                                                              \
    {                                                                               \
        pf_par_on_change_cb_t on_change = g_par_cb_table[(par_num)].on_change;      \
        if (((ePAR_OK == (status_var)) || (ePAR_WAR_LIMITED == (status_var))) &&    \
            (NULL != on_change))                                                    \
        {                                                                           \
            const par_type_t new_val = { .FIELD = PAR_GET_##PRIV##_PRIV(par_num) }; \
            if (PAR_CHECKED_VALUE_CHANGED(FIELD, new_val, old_val))                 \
            {                                                                       \
                on_change((par_num), new_val, old_val);                             \
            }                                                                       \
        }                                                                           \
    } while (0)
#else
#define PAR_CHECKED_OLD_VAL_DECL(PRIV, FIELD, par_num)
#define PAR_CHECKED_ON_CHANGE(PRIV, FIELD, par_num, status_var) \
    do                                                          \
    {                                                           \
        (void)(par_num);                                        \
        (void)(status_var);                                     \
    } while (0)
#endif

#if (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION)
#define PAR_CHECKED_VALIDATE_OR_SET(NAME, FIELD, par_num, val, status_var)     \
    do                                                                         \
    {                                                                          \
        pf_par_validation_t validation = g_par_cb_table[(par_num)].validation; \
        if ((NULL != validation) && (false == validation((par_num), (val))))   \
        {                                                                      \
            (status_var) = ePAR_ERROR_VALUE;                                   \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            (status_var) = par_set_##NAME##_fast((par_num), (val).FIELD);      \
        }                                                                      \
    } while (0)
#else
#define PAR_CHECKED_VALIDATE_OR_SET(NAME, FIELD, par_num, val, status_var) \
    do                                                                     \
    {                                                                      \
        (status_var) = par_set_##NAME##_fast((par_num), (val).FIELD);      \
    } while (0)
#endif

#define PAR_CHECKED_LOAD_FROM_PTR_CASE(ETYPE, CTYPE, FIELD) \
    case ETYPE:                                             \
        val.FIELD = *(const CTYPE *)p_ptr_val;              \
        break;

#define PAR_CHECKED_SET_CASE(NAME, PRIV, ETYPE, FIELD)                  \
    case ETYPE:                                                         \
    {                                                                   \
        PAR_CHECKED_OLD_VAL_DECL(PRIV, FIELD, par_num);                 \
        PAR_CHECKED_VALIDATE_OR_SET(NAME, FIELD, par_num, val, status); \
        PAR_CHECKED_ON_CHANGE(PRIV, FIELD, par_num, status);            \
        break;                                                          \
    }
/**
 * @brief Assert fast typed setter preconditions.
 *
 * @note Fast typed APIs are internal trusted paths. They keep assert.
 * based precondition checks only and intentionally avoid the.
 * public metadata getter path.
 *
 * @param par_num Parameter number (enumeration).
 * @param expected_type Expected parameter type for operation.
 */
static void par_assert_fast_typed_preconditions(const par_num_t par_num,
                                                const par_type_list_t expected_type)
{
    const par_cfg_t *p_cfg = NULL;

    PAR_ASSERT(true == par_is_init());
    PAR_ASSERT(par_num < ePAR_NUM_OF);

    if ((true == par_is_init()) && (par_num < ePAR_NUM_OF))
    {
        p_cfg = par_get_config(par_num);
        PAR_ASSERT(NULL != p_cfg);

        if (NULL != p_cfg)
        {
            PAR_ASSERT(expected_type == p_cfg->type);
        }
    }
}
/**
 * @brief Validate expected parameter type against resolved metadata.
 *
 * @note Callers shall resolve metadata first and then validate the.
 * expected parameter type against the resolved configuration.
 *
 * @param p_cfg Resolved parameter configuration entry.
 * @param expected_type Expected parameter type for operation.
 * @return Status of operation.
 */
static par_status_t par_validate_expected_type(const par_cfg_t * const p_cfg,
                                               const par_type_list_t expected_type)
{
    if (NULL == p_cfg)
    {
        return ePAR_ERROR;
    }

    PAR_ASSERT(expected_type == p_cfg->type);
    if (expected_type != p_cfg->type)
    {
        PAR_ERR_PRINT("PAR: type mismatch, expected=%u actual=%u",
                      (unsigned)expected_type,
                      (unsigned)p_cfg->type);
        return ePAR_ERROR_TYPE;
    }

    return ePAR_OK;
}
/**
 * @brief Checked parameter write core for all public setter APIs.
 *
 * @note This is the single enforcement point for all public write.
 * paths. It performs runtime access checks, type validation,.
 * access enforcement, optional runtime validation, fast setter.
 * dispatch, and optional on-change callback dispatch.
 *
 * @note Exactly one value source shall be provided:
 * - p_typed_val: used by typed setters par_set_xxx().
 * - p_ptr_val : used by generic par_set().
 *
 * @note Unchecked internal paths such as par_set_xxx_fast(), NVM.
 * restore helpers, and other explicit fast paths do not call.
 * this function.
 *
 * @param par_num Parameter number (enumeration).
 * @param expected_type Expected parameter type for write.
 * @param p_typed_val Optional typed union value source.
 * @param p_ptr_val Optional pointer value source.
 *
 * @return Status of operation.
 */
static par_status_t par_set_checked_core(const par_num_t par_num, const par_type_list_t expected_type, const par_type_t * const p_typed_val, const void * const p_ptr_val)
{
    const par_cfg_t *par_cfg = NULL;
    par_type_t val = { 0 };
    par_status_t status = par_resolve_runtime(par_num, p_ptr_val, (NULL == p_typed_val), &par_cfg);

    if (ePAR_OK != status)
    {
        return status;
    }

    status = par_validate_expected_type(par_cfg, expected_type);
    if (ePAR_OK != status)
    {
        return status;
    }

#if (1 == PAR_CFG_ENABLE_ACCESS)
    if (false == par_access_has_write(par_cfg->access))
    {
        PAR_INFO_PRINT("PAR: write denied by access capability, par_num=%u", (unsigned)par_num);
        return ePAR_ERROR_ACCESS;
    }
#endif

    if (NULL != p_typed_val)
    {
        val = *p_typed_val;
    }
    else
    {
        switch (expected_type)
        {
            PAR_CHECKED_LOAD_FROM_PTR_CASE(ePAR_TYPE_U8,  uint8_t,   u8)
            PAR_CHECKED_LOAD_FROM_PTR_CASE(ePAR_TYPE_I8,  int8_t,    i8)
            PAR_CHECKED_LOAD_FROM_PTR_CASE(ePAR_TYPE_U16, uint16_t,  u16)
            PAR_CHECKED_LOAD_FROM_PTR_CASE(ePAR_TYPE_I16, int16_t,   i16)
            PAR_CHECKED_LOAD_FROM_PTR_CASE(ePAR_TYPE_U32, uint32_t,  u32)
            PAR_CHECKED_LOAD_FROM_PTR_CASE(ePAR_TYPE_I32, int32_t,   i32)
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
            PAR_CHECKED_LOAD_FROM_PTR_CASE(ePAR_TYPE_F32, float32_t, f32)
#endif
        case ePAR_TYPE_NUM_OF:
        default:
            PAR_ASSERT(0);
            return ePAR_ERROR_TYPE;
        }
    }

    if (ePAR_OK != par_acquire_mutex(par_num))
    {
        return ePAR_ERROR_MUTEX;
    }

    switch (expected_type)
    {
        PAR_CHECKED_SET_CASE(u8,  U8,  ePAR_TYPE_U8,  u8)
        PAR_CHECKED_SET_CASE(i8,  I8,  ePAR_TYPE_I8,  i8)
        PAR_CHECKED_SET_CASE(u16, U16, ePAR_TYPE_U16, u16)
        PAR_CHECKED_SET_CASE(i16, I16, ePAR_TYPE_I16, i16)
        PAR_CHECKED_SET_CASE(u32, U32, ePAR_TYPE_U32, u32)
        PAR_CHECKED_SET_CASE(i32, I32, ePAR_TYPE_I32, i32)
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
        PAR_CHECKED_SET_CASE(f32, F32, ePAR_TYPE_F32, f32)
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

/**
 * @brief Generate normal typed setters: par_set_xxx().
 */
#define PAR_TYPED_IMPL_SETTER(NAME, PRIV, CTYPE, ETYPE, FIELD)            \
    par_status_t par_set_##NAME(const par_num_t par_num, const CTYPE val) \
    {                                                                     \
        const par_type_t typed_val = { .FIELD = val };                    \
        return par_set_checked_core(par_num, ETYPE, &typed_val, NULL);    \
    }

PAR_TYPED_ROWS(PAR_TYPED_IMPL_SETTER)
#undef PAR_TYPED_IMPL_SETTER

/**
 * @brief Generate fast typed setters: par_set_xxx_fast().
 */
#define PAR_TYPED_IMPL_FAST_SETTER(NAME, PRIV, CTYPE, ETYPE, FIELD)              \
    par_status_t par_set_##NAME##_fast(const par_num_t par_num, const CTYPE val) \
    {                                                                            \
        par_assert_fast_typed_preconditions(par_num, ETYPE);                     \
        PAR_TYPED_FAST_SET_BODY(PRIV, FIELD, par_num, val);                      \
    }

PAR_TYPED_ROWS(PAR_TYPED_IMPL_FAST_SETTER)
#undef PAR_TYPED_IMPL_FAST_SETTER

/**
 * @brief Generate typed getters: par_get_xxx().
 */
#define PAR_TYPED_IMPL_GETTER(NAME, PRIV, CTYPE, ETYPE, FIELD)                     \
    par_status_t par_get_##NAME(const par_num_t par_num, CTYPE * const p_val)      \
    {                                                                              \
        const par_cfg_t *par_cfg = NULL;                                           \
        par_status_t status = par_resolve_runtime(par_num, p_val, true, &par_cfg); \
        if (ePAR_OK != status)                                                     \
            return status;                                                         \
        status = par_validate_expected_type(par_cfg, ETYPE);                       \
        if (ePAR_OK != status)                                                     \
            return status;                                                         \
#if (1 == PAR_CFG_ENABLE_ACCESS)                                                   \
        if (false == par_access_has_read(par_cfg->access))                         \
            return ePAR_ERROR_ACCESS;                                              \
#endif                                                                             \
        *p_val = PAR_GET_##PRIV##_PRIV(par_num);                                   \
        return ePAR_OK;                                                            \
    }

PAR_TYPED_ROWS(PAR_TYPED_IMPL_GETTER)
#undef PAR_TYPED_IMPL_GETTER

#undef PAR_CHECKED_SET_CASE
#undef PAR_CHECKED_LOAD_FROM_PTR_CASE
#undef PAR_CHECKED_VALIDATE_OR_SET
#undef PAR_CHECKED_ON_CHANGE
#undef PAR_CHECKED_OLD_VAL_DECL
#undef PAR_CHECKED_VALUE_CHANGED
#undef PAR_CHECKED_VALUE_CHANGED_f32
#undef PAR_CHECKED_VALUE_CHANGED_i32
#undef PAR_CHECKED_VALUE_CHANGED_u32
#undef PAR_CHECKED_VALUE_CHANGED_i16
#undef PAR_CHECKED_VALUE_CHANGED_u16
#undef PAR_CHECKED_VALUE_CHANGED_i8
#undef PAR_CHECKED_VALUE_CHANGED_u8

#undef PAR_TYPED_FAST_SET_BODY
#undef PAR_TYPED_ROWS
#undef PAR_TYPED_ROWS_F32
#undef PAR_TYPED_ROWS_BASE
