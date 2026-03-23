/*
 * Private include fragment. Included only by par.c.
 */

#define PAR_FOR_EACH_BITWISE_OP(OP, NAME, CTYPE, ETYPE, FIELD, STORAGE) \
    OP(bitand, AND, NAME, CTYPE, ETYPE, FIELD, STORAGE)                 \
    OP(bitor, OR, NAME, CTYPE, ETYPE, FIELD, STORAGE)

#if (1 == PAR_CFG_ENABLE_RANGE)
#define PAR_BITWISE_FAST_SET_BODY(ATOMIC_OP, NAME, FIELD, STORAGE, par_num, val)                    \
    do                                                                                              \
    {                                                                                               \
        const par_range_t range = par_get_range(par_num);                                           \
        if ((val) > range.max.FIELD)                                                                \
        {                                                                                           \
            PAR_ATOMIC_FETCH_##ATOMIC_OP(NAME, &(STORAGE)[g_par_offset[par_num]], range.max.FIELD); \
            return ePAR_WAR_LIMITED;                                                                \
        }                                                                                           \
        else if ((val) < range.min.FIELD)                                                           \
        {                                                                                           \
            PAR_ATOMIC_FETCH_##ATOMIC_OP(NAME, &(STORAGE)[g_par_offset[par_num]], range.min.FIELD); \
            return ePAR_WAR_LIMITED;                                                                \
        }                                                                                           \
        else                                                                                        \
        {                                                                                           \
            PAR_ATOMIC_FETCH_##ATOMIC_OP(NAME, &(STORAGE)[g_par_offset[par_num]], (val));           \
            return ePAR_OK;                                                                         \
        }                                                                                           \
    } while (0)
#else
#define PAR_BITWISE_FAST_SET_BODY(ATOMIC_OP, NAME, FIELD, STORAGE, par_num, val)      \
    do                                                                                \
    {                                                                                 \
        PAR_ATOMIC_FETCH_##ATOMIC_OP(NAME, &(STORAGE)[g_par_offset[par_num]], (val)); \
        return ePAR_OK;                                                               \
    } while (0)
#endif

#define PAR_IMPL_BITWISE_FAST_SETTER(OP_NAME, ATOMIC_OP, NAME, CTYPE, ETYPE, FIELD, STORAGE) \
    par_status_t par_##OP_NAME##_set_##NAME##_fast(const par_num_t par_num, const CTYPE val) \
    {                                                                                        \
        PAR_ASSERT(true == par_is_init());                                                   \
        PAR_ASSERT(ETYPE == par_get_type(par_num));                                          \
        PAR_BITWISE_FAST_SET_BODY(ATOMIC_OP, NAME, FIELD, STORAGE, par_num, val);            \
    }

#define PAR_BITWISE_ROWS(OP)                               \
    OP(u8, uint8_t, ePAR_TYPE_U8, u8, gpu8_par_value)      \
    OP(u16, uint16_t, ePAR_TYPE_U16, u16, gpu16_par_value) \
    OP(u32, uint32_t, ePAR_TYPE_U32, u32, gpu32_par_value)

#define PAR_IMPL_BITWISE_WIDTH(NAME, CTYPE, ETYPE, FIELD, STORAGE) \
    PAR_FOR_EACH_BITWISE_OP(PAR_IMPL_BITWISE_FAST_SETTER, NAME, CTYPE, ETYPE, FIELD, STORAGE)

PAR_BITWISE_ROWS(PAR_IMPL_BITWISE_WIDTH)

#undef PAR_IMPL_BITWISE_WIDTH
#undef PAR_BITWISE_ROWS

#undef PAR_IMPL_BITWISE_FAST_SETTER
#undef PAR_BITWISE_FAST_SET_BODY
#undef PAR_FOR_EACH_BITWISE_OP
