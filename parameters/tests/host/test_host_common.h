/**
 * @file test_host_common.h
 * @brief Provide a minimal C test harness for host parameter tests.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#ifndef PAR_TEST_HOST_COMMON_H
#define PAR_TEST_HOST_COMMON_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "par.h"

/** @brief Assert that a condition is true inside a host test case. */
#define TEST_ASSERT(cond_)                                                                      \
    do                                                                                          \
    {                                                                                           \
        if (!(cond_))                                                                           \
        {                                                                                       \
            fprintf(stderr, "%s:%d: assertion failed: %s\n", __FILE__, __LINE__, #cond_);      \
            return false;                                                                       \
        }                                                                                       \
    } while (0)

/** @brief Assert that a status has no error bits. */
#define TEST_ASSERT_OK(expr_)                                                                   \
    do                                                                                          \
    {                                                                                           \
        const par_status_t _status = (expr_);                                                    \
        if (ePAR_OK != (_status & ePAR_STATUS_ERROR_MASK))                                      \
        {                                                                                       \
            fprintf(stderr, "%s:%d: status failed: %s = 0x%04X\n",                             \
                    __FILE__, __LINE__, #expr_, (unsigned)_status);                             \
            return false;                                                                       \
        }                                                                                       \
    } while (0)

/** @brief Assert that a status matches an expected error value exactly. */
#define TEST_ASSERT_STATUS(expr_, expected_)                                                     \
    do                                                                                          \
    {                                                                                           \
        const par_status_t _status = (expr_);                                                    \
        if ((expected_) != _status)                                                             \
        {                                                                                       \
            fprintf(stderr, "%s:%d: status mismatch: %s = 0x%04X expected 0x%04X\n",           \
                    __FILE__, __LINE__, #expr_, (unsigned)_status, (unsigned)(expected_));      \
            return false;                                                                       \
        }                                                                                       \
    } while (0)

/** @brief Host test case function signature. */
typedef bool (*par_host_test_fn_t)(void);

/** @brief One named host test case. */
typedef struct
{
    const char *name;              /**< Stable grep-friendly case name. */
    par_host_test_fn_t test_fn;    /**< Test function pointer. */
} par_host_test_case_t;

/**
 * @brief Run a list of named host test cases.
 * @param cases Host test case array.
 * @param count Number of elements in @p cases.
 * @return 0 when all tests pass; otherwise 1.
 */
static int par_host_run_tests(const par_host_test_case_t *cases, const size_t count)
{
    size_t passed = 0U;

    if ((NULL == cases) && (count > 0U))
    {
        fprintf(stderr, "PAR_HOST_CASE_FAIL invalid test table\n");
        return 1;
    }

    for (size_t i = 0U; i < count; i++)
    {
        const char *case_name = (NULL != cases[i].name) ? cases[i].name : "<unnamed>";

        if (NULL == cases[i].test_fn)
        {
            fprintf(stderr, "PAR_HOST_CASE_FAIL %s missing test function\n", case_name);
            return 1;
        }

        if (cases[i].test_fn())
        {
            printf("PAR_HOST_CASE_PASS %s\n", case_name);
            passed++;
        }
        else
        {
            fprintf(stderr, "PAR_HOST_CASE_FAIL %s\n", case_name);
            return 1;
        }
    }

    printf("PAR_HOST_SUMMARY Passed %lu/%lu\n", (unsigned long)passed, (unsigned long)count);
    return 0;
}

#endif /* !defined(PAR_TEST_HOST_COMMON_H) */
