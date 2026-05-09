/**
 * @file rtthread.h
 * @brief Minimal RT-Thread host-test stub used by shell command tests.
 */
#ifndef PAR_TEST_RTTHREAD_H
#define PAR_TEST_RTTHREAD_H

#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/** @brief RT-Thread size type stub. */
typedef size_t rt_size_t;

/** @brief RT-Thread null pointer stub. */
#define RT_NULL ((void *)0)

/** @brief Mark intentionally unused host-test arguments. */
#define RT_UNUSED(x_) ((void)(x_))

/** @brief Map RT-Thread strncpy wrapper to libc for host tests. */
#define rt_strncpy strncpy

int rt_kprintf(const char *fmt, ...);
int rt_snprintf(char *buf, rt_size_t size, const char *fmt, ...);
void *rt_malloc(rt_size_t size);
void rt_free(void *ptr);

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* !defined(PAR_TEST_RTTHREAD_H) */
