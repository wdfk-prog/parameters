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

/** @brief Environment variable used to select the host test group. */
#define PAR_HOST_TEST_GROUP_ENV "PAR_HOST_TEST_GROUP"

/** @brief Environment variable requiring at least one selected host test. */
#define PAR_HOST_REQUIRE_SELECTED_ENV "PAR_HOST_REQUIRE_SELECTED"

/** @brief Compile-time switch for non-blocking current-policy test bodies. */
#ifndef PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
#define PAR_HOST_ENABLE_CURRENT_POLICY_TESTS 0
#endif /* !defined(PAR_HOST_ENABLE_CURRENT_POLICY_TESTS) */

/** @brief Case-name fragment used for non-blocking current-policy tests. */
#define PAR_HOST_CURRENT_POLICY_MARKER "current_policy"

/** @brief Host test case function signature. */
typedef bool (*par_host_test_fn_t)(void);

/** @brief Host test group selected for a test run. */
typedef enum
{
    ePAR_HOST_TEST_GROUP_MANDATORY = 0,     /**< Run mandatory invariant tests. */
    ePAR_HOST_TEST_GROUP_CURRENT_POLICY,    /**< Run current-policy compatibility tests. */
    ePAR_HOST_TEST_GROUP_ALL,               /**< Run every registered host test. */
    ePAR_HOST_TEST_GROUP_INVALID            /**< Invalid group selector. */
} par_host_test_group_t;

/** @brief One named host test case. */
typedef struct
{
    const char *name;              /**< Stable grep-friendly case name. */
    par_host_test_fn_t test_fn;    /**< Test function pointer. */
} par_host_test_case_t;

/**
 * @brief Convert a selected host test group to its log label.
 * @param group Selected test group.
 * @return Stable group label string.
 */
static const char *par_host_test_group_name(const par_host_test_group_t group)
{
    switch (group)
    {
        case ePAR_HOST_TEST_GROUP_MANDATORY:
            return "mandatory";
        case ePAR_HOST_TEST_GROUP_CURRENT_POLICY:
            return "current-policy";
        case ePAR_HOST_TEST_GROUP_ALL:
            return "all";
        default:
            return "invalid";
    }
}

/**
 * @brief Read the requested host test group from the environment.
 * @return Selected group, or ePAR_HOST_TEST_GROUP_INVALID for unsupported input.
 */
static par_host_test_group_t par_host_selected_test_group(void)
{
    const char *group = getenv(PAR_HOST_TEST_GROUP_ENV);

    if ((NULL == group) || ('\0' == group[0]) || (0 == strcmp(group, "mandatory")))
    {
        return ePAR_HOST_TEST_GROUP_MANDATORY;
    }
    if ((0 == strcmp(group, "current-policy")) || (0 == strcmp(group, "current_policy")))
    {
        return ePAR_HOST_TEST_GROUP_CURRENT_POLICY;
    }
    if (0 == strcmp(group, "all"))
    {
        return ePAR_HOST_TEST_GROUP_ALL;
    }
    return ePAR_HOST_TEST_GROUP_INVALID;
}

/**
 * @brief Check whether a case belongs to the current-policy group.
 * @param case_name Stable host test case name.
 * @return true when @p case_name contains the current-policy marker.
 */
static bool par_host_case_is_current_policy(const char *case_name)
{
    return (NULL != case_name) && (NULL != strstr(case_name, PAR_HOST_CURRENT_POLICY_MARKER));
}

/**
 * @brief Check whether at least one selected case is required.
 * @return true when a zero-selected run should fail.
 */
static bool par_host_require_selected_case(void)
{
    const char *require_selected = getenv(PAR_HOST_REQUIRE_SELECTED_ENV);

    return (NULL != require_selected) && (0 == strcmp(require_selected, "1"));
}

/**
 * @brief Check whether a case should run for the selected group.
 * @param group Selected test group.
 * @param case_name Stable host test case name.
 * @return true when the case belongs to @p group.
 */
static bool par_host_should_run_case(const par_host_test_group_t group, const char *case_name)
{
    const bool is_current_policy = par_host_case_is_current_policy(case_name);

    switch (group)
    {
        case ePAR_HOST_TEST_GROUP_MANDATORY:
            return !is_current_policy;
        case ePAR_HOST_TEST_GROUP_CURRENT_POLICY:
            return is_current_policy;
        case ePAR_HOST_TEST_GROUP_ALL:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Run a list of named host test cases.
 * @param cases Host test case array.
 * @param count Number of elements in @p cases.
 * @return 0 when all selected tests pass; otherwise 1.
 */
static int par_host_run_tests(const par_host_test_case_t *cases, const size_t count)
{
    const par_host_test_group_t group = par_host_selected_test_group();
    size_t passed = 0U;
    size_t selected = 0U;
    size_t skipped = 0U;

    if (ePAR_HOST_TEST_GROUP_INVALID == group)
    {
        fprintf(stderr,
                "PAR_HOST_GROUP_FAIL unsupported %s value; use mandatory, current-policy, or all\n",
                PAR_HOST_TEST_GROUP_ENV);
        return 1;
    }

    if ((NULL == cases) && (count > 0U))
    {
        fprintf(stderr, "PAR_HOST_CASE_FAIL invalid test table\n");
        return 1;
    }

    for (size_t i = 0U; i < count; i++)
    {
        const char *case_name = (NULL != cases[i].name) ? cases[i].name : "<unnamed>";

        if (!par_host_should_run_case(group, case_name))
        {
            printf("PAR_HOST_CASE_SKIP %s group=%s\n",
                   case_name,
                   par_host_test_group_name(group));
            skipped++;
            continue;
        }

        selected++;
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

    if ((0U == selected) && par_host_require_selected_case())
    {
        fprintf(stderr,
                "PAR_HOST_GROUP_FAIL no selected cases for group=%s\n",
                par_host_test_group_name(group));
        return 1;
    }

    printf("PAR_HOST_SUMMARY Passed %lu/%lu Skipped %lu/%lu group=%s\n",
           (unsigned long)passed,
           (unsigned long)selected,
           (unsigned long)skipped,
           (unsigned long)count,
           par_host_test_group_name(group));
    return 0;
}

#endif /* !defined(PAR_TEST_HOST_COMMON_H) */
