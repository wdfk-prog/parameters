/**
 * @file par_atomic.h
 * @brief Define atomic helper types and macros for parameter storage.
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
#ifndef PAR_ATOMIC_H
#define PAR_ATOMIC_H

#include <stdint.h>
/**
 * @brief Compile-time definitions.
 */
/**
 * @brief C11 atomic backend selector.
 */
#define PAR_ATOMIC_BACKEND_C11 1

/**
 * @brief Port-specific atomic backend selector.
 */
#define PAR_ATOMIC_BACKEND_PORT 2

#ifndef PAR_ATOMIC_BACKEND
/**
 * @brief Default atomic backend selection.
 */
#define PAR_ATOMIC_BACKEND PAR_ATOMIC_BACKEND_C11
#endif

/**
 * @brief Atomic shared-storage contract for backend implementers.
 *
 * @note Backends used with static shared storage mode must guarantee identical.
 * object representation for the following type groups:
 * - par_atomic_u8_t and par_atomic_i8_t.
 * - par_atomic_u16_t and par_atomic_i16_t.
 * - par_atomic_u32_t, par_atomic_i32_t and par_atomic_f32_t.
 *
 * @note If a port backend cannot satisfy this contract, static shared storage.
 * mode is not supported for that backend.
 */

/**
 * @brief List of integral types supported by atomic load/store helpers.
 *
 * @param X Macro invoked as X(tag, type).
 */
#define PAR_ATOMIC_INTEGRAL_TYPE_LIST(X) \
    X(u8, uint8_t)                       \
    X(i8, int8_t)                        \
    X(u16, uint16_t)                     \
    X(i16, int16_t)                      \
    X(u32, uint32_t)                     \
    X(i32, int32_t)

/**
 * @brief List of all scalar types supported by atomic helpers.
 *
 * @param X Macro invoked as X(tag, type).
 */
#define PAR_ATOMIC_TYPE_LIST(X)      \
    PAR_ATOMIC_INTEGRAL_TYPE_LIST(X) \
    X(f32, float)

/**
 * @brief List of types supported by atomic fetch-and and fetch-or helpers.
 *
 * @param X Macro invoked as X(tag, type).
 */
#define PAR_ATOMIC_FETCH_TYPE_LIST(X) \
    X(u8, uint8_t)                    \
    X(u16, uint16_t)                  \
    X(u32, uint32_t)

#if (PAR_ATOMIC_BACKEND == PAR_ATOMIC_BACKEND_C11)

#include <stdatomic.h>

/**
 * @brief Declare atomic typedef for selected scalar type.
 *
 * @param tag Type tag suffix.
 * @param type Scalar type wrapped by _Atomic.
 */
#define PAR_ATOMIC_DECLARE_TYPE(tag, type) \
    typedef _Atomic type par_atomic_##tag##_t;

PAR_ATOMIC_TYPE_LIST(PAR_ATOMIC_DECLARE_TYPE)

#undef PAR_ATOMIC_DECLARE_TYPE

/**
 * @brief Define atomic load and store helper functions.
 *
 * @param tag Type tag suffix.
 * @param type Scalar type of generated helpers.
 */
#define PAR_ATOMIC_DEFINE_LOAD_STORE(tag, type)                               \
    static inline type par_atomic_load_##tag(const par_atomic_##tag##_t *ptr) \
    {                                                                         \
        return atomic_load_explicit(ptr, memory_order_relaxed);               \
    }                                                                         \
                                                                              \
    static inline void par_atomic_store_##tag(par_atomic_##tag##_t *ptr,      \
                                              type value)                     \
    {                                                                         \
        atomic_store_explicit(ptr, value, memory_order_relaxed);              \
    }

PAR_ATOMIC_INTEGRAL_TYPE_LIST(PAR_ATOMIC_DEFINE_LOAD_STORE)

#undef PAR_ATOMIC_DEFINE_LOAD_STORE
/**
 * @brief Load floating-point atomic value.
 *
 * @note "atomic_load_explicit" does not support float data type in this.
 * implementation, therefore GCC/Clang built-in primitive.
 * "__atomic_load" is used instead.
 *
 * @param ptr Pointer to atomic floating-point object.
 * @return Current floating-point value.
 */
static inline float par_atomic_load_f32(const par_atomic_f32_t *ptr)
{
    float value;

    __atomic_load(ptr, &value, __ATOMIC_RELAXED);

    return value;
}
/**
 * @brief Store floating-point atomic value.
 *
 * @note "atomic_store_explicit" does not support float data type in this.
 * implementation, therefore GCC/Clang built-in primitive.
 * "__atomic_store" is used instead.
 *
 * @param ptr Pointer to atomic floating-point object.
 * @param value Value to store.
 */
static inline void par_atomic_store_f32(par_atomic_f32_t *ptr, float value)
{
    __atomic_store(ptr, &value, __ATOMIC_RELAXED);
}

/**
 * @brief Define atomic fetch-and helper function.
 *
 * @param tag Type tag suffix.
 * @param type Scalar type of generated helper.
 */
#define PAR_ATOMIC_DEFINE_FETCH_AND(tag, type)                               \
    static inline type par_atomic_fetch_and_##tag(par_atomic_##tag##_t *ptr, \
                                                  type value)                \
    {                                                                        \
        return atomic_fetch_and_explicit(ptr, value, memory_order_relaxed);  \
    }

PAR_ATOMIC_FETCH_TYPE_LIST(PAR_ATOMIC_DEFINE_FETCH_AND)

#undef PAR_ATOMIC_DEFINE_FETCH_AND

/**
 * @brief Define atomic fetch-or helper function.
 *
 * @param tag Type tag suffix.
 * @param type Scalar type of generated helper.
 */
#define PAR_ATOMIC_DEFINE_FETCH_OR(tag, type)                               \
    static inline type par_atomic_fetch_or_##tag(par_atomic_##tag##_t *ptr, \
                                                 type value)                \
    {                                                                       \
        return atomic_fetch_or_explicit(ptr, value, memory_order_relaxed);  \
    }

PAR_ATOMIC_FETCH_TYPE_LIST(PAR_ATOMIC_DEFINE_FETCH_OR)

#undef PAR_ATOMIC_DEFINE_FETCH_OR

#elif (PAR_ATOMIC_BACKEND == PAR_ATOMIC_BACKEND_PORT)

#include "../../par_atomic_port.h"

#else

#error "Unsupported PAR_ATOMIC_BACKEND"

#endif
/**
 * @brief Load atomic value by type tag.
 *
 * @param tag Type tag suffix.
 * @param ptr Pointer to atomic object.
 * @return Current atomic value.
 */
#define PAR_ATOMIC_LOAD(tag, ptr) par_atomic_load_##tag((ptr))
/**
 * @brief Store atomic value by type tag.
 *
 * @param tag Type tag suffix.
 * @param ptr Pointer to atomic object.
 * @param value Value to store.
 */
#define PAR_ATOMIC_STORE(tag, ptr, value) par_atomic_store_##tag((ptr), (value))
/**
 * @brief Perform atomic fetch-and by type tag.
 *
 * @param tag Type tag suffix.
 * @param ptr Pointer to atomic object.
 * @param value Operand for bitwise AND.
 * @return Previous atomic value.
 */
#define PAR_ATOMIC_FETCH_AND(tag, ptr, value) \
    par_atomic_fetch_and_##tag((ptr), (value))
/**
 * @brief Perform atomic fetch-or by type tag.
 *
 * @param tag Type tag suffix.
 * @param ptr Pointer to atomic object.
 * @param value Operand for bitwise OR.
 * @return Previous atomic value.
 */
#define PAR_ATOMIC_FETCH_OR(tag, ptr, value) \
    par_atomic_fetch_or_##tag((ptr), (value))

#endif
