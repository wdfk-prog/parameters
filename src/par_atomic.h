// Copyright (c) 2026 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      par_atomic.h
*@brief     Atomic operations API macros and type declarations
*@author    wdfk-prog
*@email     1425075683@qq.com
*@date      09.03.2026
*@version   V3.0.1
*@details   This header provides atomic type aliases and helper macros for load,
*           store, fetch-and, and fetch-or operations. It supports either the
*           C11 atomic backend or a port-specific backend selected by
*           PAR_ATOMIC_BACKEND.
*/

#ifndef PAR_ATOMIC_H
#define PAR_ATOMIC_H

#include <stdint.h>

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 *     C11 atomic backend selector
 */
#define PAR_ATOMIC_BACKEND_C11   1

/**
 *     Port-specific atomic backend selector
 */
#define PAR_ATOMIC_BACKEND_PORT  2

#ifndef PAR_ATOMIC_BACKEND
    /**
     *     Default atomic backend selection
     */
    #define PAR_ATOMIC_BACKEND PAR_ATOMIC_BACKEND_C11
#endif

/**
 *     Atomic shared-storage contract for backend implementers
 *
 * @note  Backends used with static shared storage mode must guarantee identical
 *        object representation for the following type groups:
 *        - par_atomic_u8_t  and par_atomic_i8_t
 *        - par_atomic_u16_t and par_atomic_i16_t
 *        - par_atomic_u32_t, par_atomic_i32_t and par_atomic_f32_t
 *
 * @note  If a port backend cannot satisfy this contract, static shared storage
 *        mode is not supported for that backend.
 */

/**
 *     List of integral types supported by atomic load/store helpers
 *
 * @param[in]    X - Macro invoked as X(tag, type)
 */
#define PAR_ATOMIC_INTEGRAL_TYPE_LIST(X) \
    X(u8, uint8_t)                       \
    X(i8, int8_t)                        \
    X(u16, uint16_t)                     \
    X(i16, int16_t)                      \
    X(u32, uint32_t)                     \
    X(i32, int32_t)

/**
 *     List of all scalar types supported by atomic helpers
 *
 * @param[in]    X - Macro invoked as X(tag, type)
 */
#define PAR_ATOMIC_TYPE_LIST(X) \
    PAR_ATOMIC_INTEGRAL_TYPE_LIST(X) \
    X(f32, float)

/**
 *     List of types supported by atomic fetch-and and fetch-or helpers
 *
 * @param[in]    X - Macro invoked as X(tag, type)
 */
#define PAR_ATOMIC_FETCH_TYPE_LIST(X) \
    X(u8, uint8_t)                    \
    X(u16, uint16_t)                  \
    X(u32, uint32_t)

#if (PAR_ATOMIC_BACKEND == PAR_ATOMIC_BACKEND_C11)

#include <stdatomic.h>

    /**
     *     Declare atomic typedef for selected scalar type
     *
     * @param[in]    tag  - Type tag suffix
     * @param[in]    type - Scalar type wrapped by _Atomic
     */
    #define PAR_ATOMIC_DECLARE_TYPE(tag, type) \
        typedef _Atomic type par_atomic_##tag##_t;

PAR_ATOMIC_TYPE_LIST(PAR_ATOMIC_DECLARE_TYPE)

    #undef PAR_ATOMIC_DECLARE_TYPE

    /**
     *     Define atomic load and store helper functions
     *
     * @param[in]    tag  - Type tag suffix
     * @param[in]    type - Scalar type of generated helpers
     */
    #define PAR_ATOMIC_DEFINE_LOAD_STORE(tag, type)                                \
        static inline type par_atomic_load_##tag(const par_atomic_##tag##_t *ptr)  \
        {                                                                          \
            return atomic_load_explicit(ptr, memory_order_relaxed);                \
        }                                                                          \
                                                                                   \
        static inline void par_atomic_store_##tag(par_atomic_##tag##_t *ptr,       \
            type value)                                                            \
        {                                                                          \
            atomic_store_explicit(ptr, value, memory_order_relaxed);               \
        }

PAR_ATOMIC_INTEGRAL_TYPE_LIST(PAR_ATOMIC_DEFINE_LOAD_STORE)

    #undef PAR_ATOMIC_DEFINE_LOAD_STORE

////////////////////////////////////////////////////////////////////////////////
/**
*        Load floating-point atomic value
*
* @note     "atomic_load_explicit" does not support float data type in this
*           implementation, therefore GCC/Clang built-in primitive
*           "__atomic_load" is used instead.
*
* @param[in]    ptr - Pointer to atomic floating-point object
* @return       value - Current floating-point value
*/
////////////////////////////////////////////////////////////////////////////////
static inline float par_atomic_load_f32(const par_atomic_f32_t *ptr)
{
    float value;

    __atomic_load(ptr, &value, __ATOMIC_RELAXED);

    return value;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Store floating-point atomic value
*
* @note     "atomic_store_explicit" does not support float data type in this
*           implementation, therefore GCC/Clang built-in primitive
*           "__atomic_store" is used instead.
*
* @param[in]    ptr   - Pointer to atomic floating-point object
* @param[in]    value - Value to store
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static inline void par_atomic_store_f32(par_atomic_f32_t *ptr, float value)
{
    __atomic_store(ptr, &value, __ATOMIC_RELAXED);
}

    /**
     *     Define atomic fetch-and helper function
     *
     * @param[in]    tag  - Type tag suffix
     * @param[in]    type - Scalar type of generated helper
     */
    #define PAR_ATOMIC_DEFINE_FETCH_AND(tag, type)                                 \
        static inline type par_atomic_fetch_and_##tag(par_atomic_##tag##_t *ptr,   \
            type value)                                                            \
        {                                                                          \
            return atomic_fetch_and_explicit(ptr, value, memory_order_relaxed);    \
        }

PAR_ATOMIC_FETCH_TYPE_LIST(PAR_ATOMIC_DEFINE_FETCH_AND)

    #undef PAR_ATOMIC_DEFINE_FETCH_AND

    /**
     *     Define atomic fetch-or helper function
     *
     * @param[in]    tag  - Type tag suffix
     * @param[in]    type - Scalar type of generated helper
     */
    #define PAR_ATOMIC_DEFINE_FETCH_OR(tag, type)                                  \
        static inline type par_atomic_fetch_or_##tag(par_atomic_##tag##_t *ptr,    \
            type value)                                                            \
        {                                                                          \
            return atomic_fetch_or_explicit(ptr, value, memory_order_relaxed);     \
        }

PAR_ATOMIC_FETCH_TYPE_LIST(PAR_ATOMIC_DEFINE_FETCH_OR)

    #undef PAR_ATOMIC_DEFINE_FETCH_OR

#elif (PAR_ATOMIC_BACKEND == PAR_ATOMIC_BACKEND_PORT)

#include "../../par_atomic_port.h"

#else

    #error "Unsupported PAR_ATOMIC_BACKEND"

#endif

////////////////////////////////////////////////////////////////////////////////
/**
*        Load atomic value by type tag
*
* @param[in]    tag - Type tag suffix
* @param[in]    ptr - Pointer to atomic object
* @return       value - Current atomic value
*/
////////////////////////////////////////////////////////////////////////////////
#define PAR_ATOMIC_LOAD(tag, ptr)           par_atomic_load_##tag((ptr))

////////////////////////////////////////////////////////////////////////////////
/**
*        Store atomic value by type tag
*
* @param[in]    tag   - Type tag suffix
* @param[in]    ptr   - Pointer to atomic object
* @param[in]    value - Value to store
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
#define PAR_ATOMIC_STORE(tag, ptr, value)   par_atomic_store_##tag((ptr), (value))

////////////////////////////////////////////////////////////////////////////////
/**
*        Perform atomic fetch-and by type tag
*
* @param[in]    tag   - Type tag suffix
* @param[in]    ptr   - Pointer to atomic object
* @param[in]    value - Operand for bitwise AND
* @return       value - Previous atomic value
*/
////////////////////////////////////////////////////////////////////////////////
#define PAR_ATOMIC_FETCH_AND(tag, ptr, value) \
    par_atomic_fetch_and_##tag((ptr), (value))

////////////////////////////////////////////////////////////////////////////////
/**
*        Perform atomic fetch-or by type tag
*
* @param[in]    tag   - Type tag suffix
* @param[in]    ptr   - Pointer to atomic object
* @param[in]    value - Operand for bitwise OR
* @return       value - Previous atomic value
*/
////////////////////////////////////////////////////////////////////////////////
#define PAR_ATOMIC_FETCH_OR(tag, ptr, value) \
    par_atomic_fetch_or_##tag((ptr), (value))

#endif
