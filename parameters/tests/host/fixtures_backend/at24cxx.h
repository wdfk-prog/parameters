/**
 * @file at24cxx.h
 * @brief Minimal AT24CXX declarations for host backend adapter smoke tests.
 */
#ifndef PAR_TEST_AT24CXX_H
#define PAR_TEST_AT24CXX_H

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#ifndef RT_NULL
#define RT_NULL ((void *)0)
#endif /* !defined(RT_NULL) */
#ifndef RT_EOK
#define RT_EOK (0)
#endif /* !defined(RT_EOK) */
#ifndef RT_FALSE
#define RT_FALSE (0)
#endif /* !defined(RT_FALSE) */
#ifndef RT_TRUE
#define RT_TRUE (1)
#endif /* !defined(RT_TRUE) */
#ifndef RT_STATIC_ASSERT
#define RT_STATIC_ASSERT(name_, expr_) typedef char rt_static_assert_##name_[(expr_) ? 1 : -1]
#endif /* !defined(RT_STATIC_ASSERT) */

/** @brief RT-Thread boolean stub. */
typedef int rt_bool_t;
/** @brief AT24CXX opaque device stub. */
typedef struct par_test_at24_device *at24cxx_device_t;

/** @brief Stub EEPROM memory size. */
#define AT24CXX_MAX_MEM_ADDRESS (256U)
/** @brief Stub EEPROM page size. */
#define AT24CXX_PAGE_BYTE (8U)

#define rt_memset memset

at24cxx_device_t at24cxx_init(const char *i2c_bus_name, uint8_t addr_input);
void at24cxx_deinit(at24cxx_device_t dev);
int at24cxx_check(at24cxx_device_t dev);
int at24cxx_page_read(at24cxx_device_t dev, uint32_t addr, uint8_t *buf, uint16_t size);
int at24cxx_page_write(at24cxx_device_t dev, uint32_t addr, const uint8_t *buf, uint16_t size);

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* !defined(PAR_TEST_AT24CXX_H) */
