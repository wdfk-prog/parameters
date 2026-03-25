/*
 * Private include fragment. Included only by par.c.
 *
 * This file generates the typed public APIs declared in par.h.
 *
 * Generated setter family:
 *   par_set_u8/i8/u16/i16/u32/i32(/f32)
 *
 * Generated fast-setter family:
 *   par_set_u8_fast/i8_fast/u16_fast/i16_fast/u32_fast/i32_fast(/f32_fast)
 *
 * Generated getter family:
 *   par_get_u8/i8/u16/i16/u32/i32(/f32)
 *
 * Expansion mapping:
 *   PAR_TYPED_ROWS(...)           -> selects enabled scalar types
 *   PAR_TYPED_IMPL_SETTER         -> normal setters
 *   PAR_TYPED_IMPL_FAST_SETTER    -> fast setters
 *   PAR_TYPED_IMPL_GETTER         -> getters
 */

#define PAR_TYPED_ROWS_BASE(OP)                \
    OP(u8, U8, uint8_t, ePAR_TYPE_U8, u8)      \
    OP(i8, I8, int8_t, ePAR_TYPE_I8, i8)       \
    OP(u16, U16, uint16_t, ePAR_TYPE_U16, u16) \
    OP(i16, I16, int16_t, ePAR_TYPE_I16, i16)  \
    OP(u32, U32, uint32_t, ePAR_TYPE_U32, u32) \
    OP(i32, I32, int32_t, ePAR_TYPE_I32, i32)

#if (1 == PAR_CFG_ENABLE_TYPE_F32)
#define PAR_TYPED_ROWS_F32(OP) OP(f32, F32, float32_t, ePAR_TYPE_F32, f32)
#else
#define PAR_TYPED_ROWS_F32(OP)
#endif

#define PAR_TYPED_ROWS(OP)  \
    PAR_TYPED_ROWS_BASE(OP) \
    PAR_TYPED_ROWS_F32(OP)

#define PAR_TYPED_FIELD_CHANGED_u8(new_val, old_val)     ((new_val).u8 != (old_val).u8)
#define PAR_TYPED_FIELD_CHANGED_i8(new_val, old_val)     ((new_val).i8 != (old_val).i8)
#define PAR_TYPED_FIELD_CHANGED_u16(new_val, old_val)    ((new_val).u16 != (old_val).u16)
#define PAR_TYPED_FIELD_CHANGED_i16(new_val, old_val)    ((new_val).i16 != (old_val).i16)
#define PAR_TYPED_FIELD_CHANGED_u32(new_val, old_val)    ((new_val).u32 != (old_val).u32)
#define PAR_TYPED_FIELD_CHANGED_i32(new_val, old_val)    ((new_val).i32 != (old_val).i32)
#define PAR_TYPED_FIELD_CHANGED_f32(new_val, old_val)    (!par_f32_bits_equal((new_val).f32, (old_val).f32))
#define PAR_TYPED_VALUE_CHANGED(FIELD, new_val, old_val) PAR_TYPED_FIELD_CHANGED_##FIELD((new_val), (old_val))

#if (1 == PAR_CFG_ENABLE_CHANGE_CALLBACK)
#define PAR_TYPED_SETTER_OLD_VAL_DECL(PRIV, FIELD, par_num) const par_type_t old_val = { .FIELD = PAR_GET_##PRIV##_PRIV(par_num) };
#define PAR_TYPED_SETTER_ON_CHANGE(PRIV, FIELD, par_num)                             \
    do                                                                               \
    {                                                                                \
        pf_par_on_change_cb_t on_change = g_par_cb_table[(par_num)].on_change;       \
        const par_type_t new_val = { .FIELD = PAR_GET_##PRIV##_PRIV((par_num)) };    \
        if ((NULL != on_change) && PAR_TYPED_VALUE_CHANGED(FIELD, new_val, old_val)) \
        {                                                                            \
            on_change((par_num), new_val, old_val);                                  \
        }                                                                            \
    } while (0)
#else
#define PAR_TYPED_SETTER_OLD_VAL_DECL(PRIV, FIELD, par_num)
#define PAR_TYPED_SETTER_ON_CHANGE(PRIV, FIELD, par_num) \
    do                                                   \
    {                                                    \
        (void)(par_num);                                 \
    } while (0)
#endif

#if (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION)
#define PAR_TYPED_SETTER_VALIDATE_OR_SET(NAME, FIELD, par_num, val, status_var)                       \
    do                                                                                                \
    {                                                                                                 \
        pf_par_validation_t validation = g_par_cb_table[(par_num)].validation;                        \
        if ((NULL != validation) && (false == validation((par_num), (par_type_t){ .FIELD = (val) }))) \
        {                                                                                             \
            (status_var) = ePAR_ERROR_VALUE;                                                          \
        }                                                                                             \
        else                                                                                          \
        {                                                                                             \
            (status_var) = par_set_##NAME##_fast((par_num), (val));                                   \
        }                                                                                             \
    } while (0)
#else
#define PAR_TYPED_SETTER_VALIDATE_OR_SET(NAME, FIELD, par_num, val, status_var) \
    do                                                                          \
    {                                                                           \
        (status_var) = par_set_##NAME##_fast((par_num), (val));                 \
    } while (0)
#endif

#if (1 == PAR_CFG_ENABLE_RANGE)
#define PAR_TYPED_FAST_SET_BODY(PRIV, FIELD, par_num, val)     \
    do                                                         \
    {                                                          \
        const par_range_t range = par_get_range(par_num);      \
        if ((val) > range.max.FIELD)                           \
        {                                                      \
            PAR_SET_##PRIV##_PRIV((par_num), range.max.FIELD); \
            return ePAR_WAR_LIMITED;                           \
        }                                                      \
        else if ((val) < range.min.FIELD)                      \
        {                                                      \
            PAR_SET_##PRIV##_PRIV((par_num), range.min.FIELD); \
            return ePAR_WAR_LIMITED;                           \
        }                                                      \
        else                                                   \
        {                                                      \
            PAR_SET_##PRIV##_PRIV((par_num), (val));           \
            return ePAR_OK;                                    \
        }                                                      \
    } while (0)
#else
#define PAR_TYPED_FAST_SET_BODY(PRIV, FIELD, par_num, val) \
    do                                                     \
    {                                                      \
        PAR_SET_##PRIV##_PRIV((par_num), (val));           \
        return ePAR_OK;                                    \
    } while (0)
#endif

#define PAR_TYPED_IMPL_SETTER(NAME, PRIV, CTYPE, ETYPE, FIELD)                   \
    par_status_t par_set_##NAME(const par_num_t par_num, const CTYPE val)        \
    {                                                                            \
        par_status_t status = ePAR_OK;                                           \
        PAR_ASSERT(true == par_is_init());                                       \
        if (true != par_is_init())                                               \
            return ePAR_ERROR_INIT;                                              \
        PAR_ASSERT(par_num < ePAR_NUM_OF);                                       \
        if (par_num >= ePAR_NUM_OF)                                              \
            return ePAR_ERROR_PAR_NUM;                                           \
        PAR_ASSERT(ETYPE == par_get_type(par_num));                              \
        if (ETYPE != par_get_type(par_num))                                      \
            return ePAR_ERROR_TYPE;                                              \
        if (ePAR_OK == par_acquire_mutex(par_num))                               \
        {                                                                        \
            PAR_TYPED_SETTER_OLD_VAL_DECL(PRIV, FIELD, par_num)                  \
            PAR_TYPED_SETTER_VALIDATE_OR_SET(NAME, FIELD, par_num, val, status); \
            PAR_TYPED_SETTER_ON_CHANGE(PRIV, FIELD, par_num);                    \
            par_release_mutex(par_num);                                          \
        }                                                                        \
        else                                                                     \
        {                                                                        \
            status = ePAR_ERROR_MUTEX;                                           \
        }                                                                        \
        return status;                                                           \
    }

PAR_TYPED_ROWS(PAR_TYPED_IMPL_SETTER)
#undef PAR_TYPED_IMPL_SETTER

#define PAR_TYPED_IMPL_FAST_SETTER(NAME, PRIV, CTYPE, ETYPE, FIELD)              \
    par_status_t par_set_##NAME##_fast(const par_num_t par_num, const CTYPE val) \
    {                                                                            \
        PAR_ASSERT(true == par_is_init());                                       \
        PAR_ASSERT(par_num < ePAR_NUM_OF);                                       \
        PAR_ASSERT(ETYPE == par_get_type(par_num));                              \
        PAR_TYPED_FAST_SET_BODY(PRIV, FIELD, par_num, val);                      \
    }

PAR_TYPED_ROWS(PAR_TYPED_IMPL_FAST_SETTER)
#undef PAR_TYPED_IMPL_FAST_SETTER

/* Generate typed getters: par_get_xxx() */
#define PAR_TYPED_IMPL_GETTER(NAME, PRIV, CTYPE, ETYPE, FIELD)                \
    par_status_t par_get_##NAME(const par_num_t par_num, CTYPE * const p_val) \
    {                                                                         \
        PAR_ASSERT(true == par_is_init());                                    \
        if (true != par_is_init())                                            \
            return ePAR_ERROR_INIT;                                           \
        PAR_ASSERT(par_num < ePAR_NUM_OF);                                    \
        if (par_num >= ePAR_NUM_OF)                                           \
            return ePAR_ERROR_PAR_NUM;                                        \
        PAR_ASSERT(ETYPE == par_get_type(par_num));                           \
        if (ETYPE != par_get_type(par_num))                                   \
            return ePAR_ERROR_TYPE;                                           \
        PAR_ASSERT(NULL != p_val);                                            \
        if (NULL == p_val)                                                    \
            return ePAR_ERROR_PARAM;                                          \
        *p_val = PAR_GET_##PRIV##_PRIV(par_num);                              \
        return ePAR_OK;                                                       \
    }

PAR_TYPED_ROWS(PAR_TYPED_IMPL_GETTER)
#undef PAR_TYPED_IMPL_GETTER

#undef PAR_TYPED_FAST_SET_BODY
#undef PAR_TYPED_SETTER_VALIDATE_OR_SET
#undef PAR_TYPED_SETTER_ON_CHANGE
#undef PAR_TYPED_SETTER_OLD_VAL_DECL
#undef PAR_TYPED_VALUE_CHANGED
#undef PAR_TYPED_FIELD_CHANGED_u8
#undef PAR_TYPED_FIELD_CHANGED_i8
#undef PAR_TYPED_FIELD_CHANGED_u16
#undef PAR_TYPED_FIELD_CHANGED_i16
#undef PAR_TYPED_FIELD_CHANGED_u32
#undef PAR_TYPED_FIELD_CHANGED_i32
#undef PAR_TYPED_FIELD_CHANGED_f32
#undef PAR_TYPED_ROWS
#undef PAR_TYPED_ROWS_F32
#undef PAR_TYPED_ROWS_BASE
