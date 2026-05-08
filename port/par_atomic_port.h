/**
 * @file par_atomic_port.h
 * @brief Provide the port atomic backend contract.
 * @details Defines atomic wrapper types and inline helper functions for 8-bit, 16-bit,
 * 32-bit, and 32-bit floating-point values using the RT-Thread atomic API.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-03-09
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-03-09 1.0     wdfk-prog    first version
 */
#ifndef PAR_ATOMIC_PORT_H
#define PAR_ATOMIC_PORT_H

#include <rtthread.h>

/**
 * @brief Atomic unsigned 8-bit value type.
 */
typedef rt_atomic8_t par_atomic_u8_t;
/**
 * @brief Atomic signed 8-bit value type.
 */
typedef rt_atomic8_t par_atomic_i8_t;
/**
 * @brief Atomic unsigned 16-bit value type.
 */
typedef rt_atomic16_t par_atomic_u16_t;
/**
 * @brief Atomic signed 16-bit value type.
 */
typedef rt_atomic16_t par_atomic_i16_t;
/**
 * @brief Atomic unsigned 32-bit value type.
 */
typedef rt_atomic_t par_atomic_u32_t;
/**
 * @brief Atomic signed 32-bit value type.
 */
typedef rt_atomic_t par_atomic_i32_t;
/**
 * @brief Atomic 32-bit floating-point value type.
 * @details Floating-point values are stored through the underlying
 * atomic integer type by preserving their bit representation.
 */
typedef rt_atomic_t par_atomic_f32_t;

PAR_STATIC_ASSERT(float_atomic_size_match, sizeof(float) == sizeof(rt_atomic_t));
PAR_STATIC_ASSERT(u32_atomic_size_match, sizeof(uint32_t) == sizeof(rt_atomic_t));
/**
 * @brief Convert a 32-bit floating-point value to its atomic storage
 * representation.
 *
 * @details This helper preserves the exact IEEE-754 bit pattern of a
 * @c float by copying its object representation into an integer-sized
 * atomic storage type. A normal C cast must not be used here. Casting
 * from @c float to @c rt_atomic_t performs a numeric conversion, not a
 * bitwise reinterpretation. For example, the bit pattern of @c 1.0f
 * must be stored as @c 0x3f800000, while a numeric cast would produce
 * the integer value @c 1, which is not equivalent.
 *
 * @note This function intentionally uses @c rt_memcpy to perform a
 * bitwise copy. This avoids strict-aliasing violations and keeps the
 * operation well-defined under compiler optimization.
 *
 * @note This design differs from implementations based on compiler
 * built-ins such as @c __atomic_store, where the compiler atomically
 * copies bytes directly into a @c float object. Here, the floating-point
 * value is first converted to the integer atomic storage format, and the
 * actual atomic operation is then performed by RT-Thread's integer
 * atomic backend via @c rt_atomic_store().
 *
 * @param value Floating-point value to convert.
 * @return Atomic integer value containing the exact bitwise
 * representation of @p value.
 */
static inline rt_atomic_t par_atomic_f32_to_atomic(float value)
{
    rt_atomic_t bits;

    rt_memcpy(&bits, &value, sizeof(bits));

    return bits;
}

/**
 * @brief Convert an atomic storage representation to a 32-bit
 * floating-point value.
 *
 * @details This helper reconstructs a @c float from the raw bit pattern
 * stored in an integer-sized atomic storage object. A normal C cast must
 * not be used here. Casting from @c rt_atomic_t to @c float performs a
 * numeric conversion, not a bitwise reinterpretation. For example, the
 * stored bit pattern @c 0x3f800000 must be reconstructed as @c 1.0f,
 * while a numeric cast would instead yield @c 1065353216.0f.
 *
 * @note This function intentionally uses @c rt_memcpy to perform a
 * bitwise copy. This avoids strict-aliasing violations and keeps the
 * operation well-defined under compiler optimization.
 *
 * @note This design differs from implementations based on compiler
 * built-ins such as @c __atomic_load, where the compiler atomically
 * copies bytes directly from the atomic object into a @c float variable.
 * Here, RT-Thread first loads the raw integer atomic value via
 * @c rt_atomic_load(), and this helper then converts that raw storage
 * representation back into a floating-point value.
 *
 * @param value Atomic integer value containing a floating-point bit pattern.
 * @return Floating-point value reconstructed from @p value.
 */
static inline float par_atomic_atomic_to_f32(rt_atomic_t value)
{
    float bits;

    rt_memcpy(&bits, &value, sizeof(bits));

    return bits;
}

/**
 * @brief Load an unsigned 8-bit atomic value.
 * @param ptr Pointer to the atomic value.
 * @return Current value stored at @p ptr.
 */
static inline uint8_t par_atomic_load_u8(const par_atomic_u8_t *ptr)
{
    return (uint8_t)(*ptr);
}

/**
 * @brief Store an unsigned 8-bit atomic value.
 * @param ptr Pointer to the atomic value.
 * @param value Value to store.
 */
static inline void par_atomic_store_u8(par_atomic_u8_t *ptr, uint8_t value)
{
    *ptr = (par_atomic_u8_t)value;
}

/**
 * @brief Load a signed 8-bit atomic value.
 * @param ptr Pointer to the atomic value.
 * @return Current value stored at @p ptr.
 */
static inline int8_t par_atomic_load_i8(const par_atomic_i8_t *ptr)
{
    return (int8_t)(*ptr);
}

/**
 * @brief Store a signed 8-bit atomic value.
 * @param ptr Pointer to the atomic value.
 * @param value Value to store.
 */
static inline void par_atomic_store_i8(par_atomic_i8_t *ptr, int8_t value)
{
    *ptr = (par_atomic_i8_t)value;
}

/**
 * @brief Load an unsigned 16-bit atomic value.
 * @param ptr Pointer to the atomic value.
 * @return Current value stored at @p ptr.
 */
static inline uint16_t par_atomic_load_u16(const par_atomic_u16_t *ptr)
{
    return (uint16_t)(*ptr);
}

/**
 * @brief Store an unsigned 16-bit atomic value.
 * @param ptr Pointer to the atomic value.
 * @param value Value to store.
 */
static inline void par_atomic_store_u16(par_atomic_u16_t *ptr, uint16_t value)
{
    *ptr = (par_atomic_u16_t)value;
}

/**
 * @brief Load a signed 16-bit atomic value.
 * @param ptr Pointer to the atomic value.
 * @return Current value stored at @p ptr.
 */
static inline int16_t par_atomic_load_i16(const par_atomic_i16_t *ptr)
{
    return (int16_t)(*ptr);
}

/**
 * @brief Store a signed 16-bit atomic value.
 * @param ptr Pointer to the atomic value.
 * @param value Value to store.
 */
static inline void par_atomic_store_i16(par_atomic_i16_t *ptr, int16_t value)
{
    *ptr = (par_atomic_i16_t)value;
}

/**
 * @brief Load an unsigned 32-bit atomic value.
 * @param ptr Pointer to the atomic value.
 * @return Current value stored at @p ptr.
 */
static inline uint32_t par_atomic_load_u32(const par_atomic_u32_t *ptr)
{
    return (uint32_t)rt_atomic_load((volatile rt_atomic_t *)ptr);
}

/**
 * @brief Store an unsigned 32-bit atomic value.
 * @param ptr Pointer to the atomic value.
 * @param value Value to store.
 */
static inline void par_atomic_store_u32(par_atomic_u32_t *ptr, uint32_t value)
{
    rt_atomic_store((volatile rt_atomic_t *)ptr, (rt_atomic_t)value);
}

/**
 * @brief Load a signed 32-bit atomic value.
 * @param ptr Pointer to the atomic value.
 * @return Current value stored at @p ptr.
 */
static inline int32_t par_atomic_load_i32(const par_atomic_i32_t *ptr)
{
    return (int32_t)rt_atomic_load((volatile rt_atomic_t *)ptr);
}

/**
 * @brief Store a signed 32-bit atomic value.
 * @param ptr Pointer to the atomic value.
 * @param value Value to store.
 */
static inline void par_atomic_store_i32(par_atomic_i32_t *ptr, int32_t value)
{
    rt_atomic_store((volatile rt_atomic_t *)ptr, (rt_atomic_t)value);
}

/**
 * @brief Load a 32-bit floating-point atomic value.
 * @param ptr Pointer to the atomic value.
 * @return Current floating-point value stored at @p ptr.
 */
static inline float par_atomic_load_f32(const par_atomic_f32_t *ptr)
{
    return par_atomic_atomic_to_f32(rt_atomic_load((volatile rt_atomic_t *)ptr));
}

/**
 * @brief Store a 32-bit floating-point atomic value.
 * @param ptr Pointer to the atomic value.
 * @param value Value to store.
 */
static inline void par_atomic_store_f32(par_atomic_f32_t *ptr, float value)
{
    rt_atomic_store((volatile rt_atomic_t *)ptr, par_atomic_f32_to_atomic(value));
}

/**
 * @brief Atomically apply a bitwise AND to an unsigned 8-bit value.
 * @param ptr Pointer to the atomic value.
 * @param value Operand used for the bitwise AND operation.
 * @return Previous value stored at @p ptr before the operation.
 */
static inline uint8_t par_atomic_fetch_and_u8(par_atomic_u8_t *ptr, uint8_t value)
{
    return (uint8_t)rt_atomic_and8((volatile rt_atomic8_t *)ptr, (rt_atomic8_t)value);
}

/**
 * @brief Atomically apply a bitwise AND to an unsigned 16-bit value.
 * @param ptr Pointer to the atomic value.
 * @param value Operand used for the bitwise AND operation.
 * @return Previous value stored at @p ptr before the operation.
 */
static inline uint16_t par_atomic_fetch_and_u16(par_atomic_u16_t *ptr, uint16_t value)
{
    return (uint16_t)rt_atomic_and16((volatile rt_atomic16_t *)ptr, (rt_atomic16_t)value);
}

/**
 * @brief Atomically apply a bitwise AND to an unsigned 32-bit value.
 * @param ptr Pointer to the atomic value.
 * @param value Operand used for the bitwise AND operation.
 * @return Previous value stored at @p ptr before the operation.
 */
static inline uint32_t par_atomic_fetch_and_u32(par_atomic_u32_t *ptr, uint32_t value)
{
    return (uint32_t)rt_atomic_and((volatile rt_atomic_t *)ptr, (rt_atomic_t)value);
}

/**
 * @brief Atomically apply a bitwise OR to an unsigned 8-bit value.
 * @param ptr Pointer to the atomic value.
 * @param value Operand used for the bitwise OR operation.
 * @return Previous value stored at @p ptr before the operation.
 */
static inline uint8_t par_atomic_fetch_or_u8(par_atomic_u8_t *ptr, uint8_t value)
{
    return (uint8_t)rt_atomic_or8((volatile rt_atomic8_t *)ptr, (rt_atomic8_t)value);
}

/**
 * @brief Atomically apply a bitwise OR to an unsigned 16-bit value.
 * @param ptr Pointer to the atomic value.
 * @param value Operand used for the bitwise OR operation.
 * @return Previous value stored at @p ptr before the operation.
 */
static inline uint16_t par_atomic_fetch_or_u16(par_atomic_u16_t *ptr, uint16_t value)
{
    return (uint16_t)rt_atomic_or16((volatile rt_atomic16_t *)ptr, (rt_atomic16_t)value);
}

/**
 * @brief Atomically apply a bitwise OR to an unsigned 32-bit value.
 * @param ptr Pointer to the atomic value.
 * @param value Operand used for the bitwise OR operation.
 * @return Previous value stored at @p ptr before the operation.
 */
static inline uint32_t par_atomic_fetch_or_u32(par_atomic_u32_t *ptr, uint32_t value)
{
    return (uint32_t)rt_atomic_or((volatile rt_atomic_t *)ptr, (rt_atomic_t)value);
}

#endif /* !defined(PAR_ATOMIC_PORT_H) */
