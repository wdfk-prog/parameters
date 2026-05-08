/**
 * @file par_bitwise_impl.inc
 * @brief Generate bitwise fast setter implementations.
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
 * @brief Private include fragment. Included only by par.c.
 */

/**
 * @brief Expand each supported bitwise operation for one scalar width.
 * @param M Operation handler macro.
 * @param ... Arguments forwarded to the operation handler macro.
 */
#define PAR_FOR_EACH_BITWISE_OP(M, ...) \
    M(bitand, AND, __VA_ARGS__)         \
    M(bitor, OR, __VA_ARGS__)

/**
 * @brief Emit an atomic bitwise fast setter body.
 * @param ATOMIC_OP Atomic operation suffix.
 * @param NAME Scalar width suffix.
 * @param FIELD par_type_t union field.
 * @param STORAGE Typed backing storage pointer.
 * @param par_num Parameter number.
 * @param val Bit mask value.
 */
#define PAR_BITWISE_FAST_SET_BODY(ATOMIC_OP, NAME, FIELD, STORAGE, par_num, val)      \
    do                                                                                \
    {                                                                                 \
        PAR_ATOMIC_FETCH_##ATOMIC_OP(NAME, &(STORAGE)[g_par_offset[par_num]], (val)); \
        return ePAR_OK;                                                               \
    } while (0)

/**
 * @brief Emit one bitwise fast setter implementation.
 * @param OP_NAME Public operation name suffix.
 * @param ATOMIC_OP Atomic operation suffix.
 * @param NAME Scalar width suffix.
 * @param CTYPE C scalar type.
 * @param ETYPE Expected parameter type enum.
 * @param FIELD par_type_t union field.
 * @param STORAGE Typed backing storage pointer.
 */
#define PAR_IMPL_BITWISE_FAST_SETTER(OP_NAME, ATOMIC_OP, NAME, CTYPE, ETYPE, FIELD, STORAGE) \
    par_status_t par_##OP_NAME##_set_##NAME##_fast(const par_num_t par_num, const CTYPE val) \
    {                                                                                        \
        par_assert_fast_typed_preconditions(par_num, ETYPE);                                 \
        PAR_BITWISE_FAST_SET_BODY(ATOMIC_OP, NAME, FIELD, STORAGE, par_num, val);            \
    }

/**
 * @brief Expand unsigned scalar widths that support bitwise fast setters.
 * @param OP Width handler macro.
 */
#define PAR_BITWISE_ROWS(OP)                               \
    OP(u8, uint8_t, ePAR_TYPE_U8, u8, gpu8_par_value)      \
    OP(u16, uint16_t, ePAR_TYPE_U16, u16, gpu16_par_value) \
    OP(u32, uint32_t, ePAR_TYPE_U32, u32, gpu32_par_value)

/**
 * @brief Emit all bitwise operations for one scalar width.
 * @param NAME Scalar width suffix.
 * @param CTYPE C scalar type.
 * @param ETYPE Expected parameter type enum.
 * @param FIELD par_type_t union field.
 * @param STORAGE Typed backing storage pointer.
 */
#define PAR_IMPL_BITWISE_WIDTH(NAME, CTYPE, ETYPE, FIELD, STORAGE) \
    PAR_FOR_EACH_BITWISE_OP(PAR_IMPL_BITWISE_FAST_SETTER, NAME, CTYPE, ETYPE, FIELD, STORAGE)

PAR_BITWISE_ROWS(PAR_IMPL_BITWISE_WIDTH)

#undef PAR_IMPL_BITWISE_WIDTH
#undef PAR_BITWISE_ROWS

#undef PAR_IMPL_BITWISE_FAST_SETTER
#undef PAR_BITWISE_FAST_SET_BODY
#undef PAR_FOR_EACH_BITWISE_OP
