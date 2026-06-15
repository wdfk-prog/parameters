/**
 * @file par_test.h
 * @brief Declare reusable runtime-test helpers for the parameter package.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#ifndef _PAR_TEST_H_
#define _PAR_TEST_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>

#include "par.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/**
 * @brief Number of elements in a fixed-size array.
 * @param array Fixed-size array expression.
 */
#define PAR_TEST_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

/**
 * @brief Return true when a parameter status contains no error bits.
 * @param status Parameter API status value.
 */
#define PAR_TEST_STATUS_HAS_NO_ERROR(status) (((uint32_t)(status) & (uint32_t)ePAR_STATUS_ERROR_MASK) == 0U)

/**
 * @brief Mark the current test case as failed and return from the case body.
 * @param ctx Test execution context.
 * @param ... printf-style detail string and optional arguments.
 */
#define PAR_TEST_FAIL(ctx, ...)                  \
    do                                           \
    {                                            \
        par_test_set_detail((ctx), __VA_ARGS__); \
        return ePAR_TEST_FAIL;                   \
    } while (0)

/**
 * @brief Mark the current test case as skipped and return from the case body.
 * @param ctx Test execution context.
 * @param ... printf-style detail string and optional arguments.
 */
#define PAR_TEST_SKIP(ctx, ...)                  \
    do                                           \
    {                                            \
        par_test_set_detail((ctx), __VA_ARGS__); \
        return ePAR_TEST_SKIP;                   \
    } while (0)

/**
 * @brief Assert a test condition and fail the current case when it is false.
 * @param ctx Test execution context.
 * @param condition Boolean expression that must evaluate to true.
 * @param ... printf-style detail string and optional arguments.
 */
#define PAR_TEST_ASSERT(ctx, condition, ...)         \
    do                                               \
    {                                                \
        if (!(condition))                            \
        {                                            \
            par_test_set_detail((ctx), __VA_ARGS__); \
            return ePAR_TEST_FAIL;                   \
        }                                            \
    } while (0)

/**
 * @brief Runtime-test case result.
 */
typedef enum
{
    ePAR_TEST_PASS = 0, /**< Case passed. */
    ePAR_TEST_FAIL,     /**< Case failed. */
    ePAR_TEST_SKIP      /**< Case was skipped because prerequisites were absent. */
} par_test_result_t;

/**
 * @brief Runtime-test context passed to each case.
 */
typedef struct
{
    const char *suite_name; /**< Active suite name. */
    const char *case_name;  /**< Active case name. */
    char detail[160];       /**< Bounded failure or skip detail buffer. */
} par_test_context_t;

/**
 * @brief Runtime-test case callback.
 * @param ctx Test execution context.
 * @return Case result.
 */
typedef par_test_result_t (*par_test_case_fn_t)(par_test_context_t *ctx);

/**
 * @brief Runtime-test case flags.
 */
typedef enum
{
    ePAR_TEST_CASE_FLAG_NONE = 0x00000000U,       /**< No special case behavior. */
    ePAR_TEST_CASE_FLAG_DESTRUCTIVE = 0x00000001U /**< Case may rewrite persistent storage. */
} par_test_case_flags_t;

/**
 * @brief Runtime-test case descriptor.
 */
typedef struct
{
    const char *name;          /**< Case name printed in CI logs. */
    par_test_case_fn_t run;    /**< Case callback. */
    uint32_t flags;            /**< Bitwise OR of par_test_case_flags_t values. */
} par_test_case_t;

/**
 * @brief Runtime-test suite descriptor.
 */
typedef struct
{
    const char *name;                 /**< Suite name accepted by the par_test shell command. */
    const par_test_case_t *cases;     /**< Case descriptor table. */
    uint32_t case_count;              /**< Number of entries in cases. */
} par_test_suite_t;

/**
 * @brief Runtime-test aggregate counters.
 */
typedef struct
{
    uint32_t total; /**< Number of executed or skipped cases. */
    uint32_t pass;  /**< Number of passed cases. */
    uint32_t fail;  /**< Number of failed cases. */
    uint32_t skip;  /**< Number of skipped cases. */
} par_test_summary_t;

/**
 * @brief Test log printer callback used by hardware and host harnesses.
 * @param p_ctx Opaque context registered with par_test_set_vprint().
 * @param fmt printf-style format string.
 * @param args printf-style argument list.
 */
typedef void (*par_test_vprint_fn_t)(void *p_ctx, const char *fmt, va_list args);

/**
 * @brief Install a test log printer for the current execution harness.
 * @details Hardware wrappers normally bind this to rt_kprintf(), while host
 *          harnesses may bind it to stdout or a capture buffer. Passing NULL
 *          restores the default stdout printer used by host builds.
 * @param p_vprint Log printer callback, or NULL for the default printer.
 * @param p_ctx Opaque context passed to p_vprint.
 */
void par_test_set_vprint(par_test_vprint_fn_t p_vprint, void *p_ctx);

/**
 * @brief Print one formatted test log line through the active harness printer.
 * @param fmt printf-style format string.
 */
void par_test_print(const char *fmt, ...);

/**
 * @brief Print one formatted test log line from an existing va_list.
 * @param fmt printf-style format string.
 * @param args printf-style argument list.
 */
void par_test_vprint(const char *fmt, va_list args);

/**
 * @brief Return the number of compiled runtime-test suites.
 * @return Number of suites registered in this build.
 */
uint32_t par_test_get_suite_count(void);

/**
 * @brief Return one compiled runtime-test suite by dense index.
 * @param index Dense suite index in the range [0, par_test_get_suite_count()).
 * @return Suite descriptor, or NULL when index is out of range.
 */
const par_test_suite_t *par_test_get_suite(uint32_t index);

/**
 * @brief Run one runtime-test suite.
 * @param suite Suite descriptor.
 * @return Aggregate test summary for the suite.
 */
par_test_summary_t par_test_run_suite(const par_test_suite_t *suite);

/**
 * @brief Print the compiled suite list through the active harness printer.
 */
void par_test_print_list(void);

/**
 * @brief Bind the active test log printer to the RT-Thread console.
 * @details RT-Thread MSH wrappers call this before dispatching reusable test
 *          helpers. Host harnesses should not depend on this function.
 */
void par_test_bind_rt_console(void);

/**
 * @brief Set the bounded detail string for the active test case.
 * @param ctx Test execution context.
 * @param fmt printf-style format string.
 */
void par_test_set_detail(par_test_context_t *ctx, const char *fmt, ...);

/**
 * @brief Ensure the parameter manager is initialized before an API-level case runs.
 * @return Operation status from the parameter manager.
 */
par_status_t par_test_ensure_parameter_init(void);

/**
 * @brief Run one named suite or all registered suites.
 * @param name Suite name, "all", or NULL for all suites.
 * @return Aggregate test summary.
 */
par_test_summary_t par_test_run_by_name(const char *name);

/**
 * @brief Return true when a parameter type is scalar-backed.
 * @param type Parameter type.
 * @return true for scalar types; otherwise false.
 */
bool par_test_type_is_scalar(par_type_list_t type);

/**
 * @brief Return true when a parameter is writable through the public setter path.
 * @param cfg Parameter configuration entry.
 * @return true when writes are allowed by compiled access metadata.
 */
bool par_test_cfg_is_writable(const par_cfg_t *cfg);

/**
 * @brief Read a scalar parameter into a generic scalar value union.
 * @param par_num Parameter number.
 * @param value Output scalar value.
 * @return Operation status.
 */
par_status_t par_test_read_scalar(par_num_t par_num, par_type_t *value);

/**
 * @brief Set a scalar parameter through the normal public setter path.
 * @param par_num Parameter number.
 * @param value Input scalar value.
 * @return Operation status.
 */
par_status_t par_test_set_scalar(par_num_t par_num, const par_type_t *value);

/**
 * @brief Set a scalar parameter through the fast restore path.
 * @param par_num Parameter number.
 * @param value Input scalar value.
 * @return Operation status.
 */
par_status_t par_test_set_scalar_fast(par_num_t par_num, const par_type_t *value);

/**
 * @brief Compare two scalar values according to one parameter type.
 * @param type Parameter type.
 * @param lhs First scalar value.
 * @param rhs Second scalar value.
 * @return true when both values are equal for the selected type.
 */
bool par_test_scalar_equal(par_type_list_t type, const par_type_t *lhs, const par_type_t *rhs);

/**
 * @brief Build an in-range scalar value different from the current value.
 * @param par_num Parameter number.
 * @param current Current scalar value.
 * @param alternate Output alternate scalar value.
 * @return true when an alternate value is available; otherwise false.
 */
bool par_test_make_alternate_scalar(par_num_t par_num, const par_type_t *current, par_type_t *alternate);


/**
 * @brief Build one scalar value below the configured minimum when possible.
 * @param par_num Parameter number.
 * @param value Output out-of-range scalar value.
 * @return true when a below-minimum value is available; otherwise false.
 */
bool par_test_make_below_range_scalar(par_num_t par_num, par_type_t *value);

/**
 * @brief Build one scalar value above the configured maximum when possible.
 * @param par_num Parameter number.
 * @param value Output out-of-range scalar value.
 * @return true when an above-maximum value is available; otherwise false.
 */
bool par_test_make_above_range_scalar(par_num_t par_num, par_type_t *value);

/**
 * @brief Return the configured scalar range boundary for one parameter.
 * @param par_num Parameter number.
 * @param upper true for maximum, false for minimum.
 * @param limit Output boundary value.
 * @return true when the boundary value is available; otherwise false.
 */
bool par_test_get_range_limit_scalar(par_num_t par_num, bool upper, par_type_t *limit);

/**
 * @brief Find a writable scalar parameter, optionally requiring persistence.
 * @param require_persistent true to require persistent metadata.
 * @param par_num Output parameter number.
 * @return true when a matching parameter was found; otherwise false.
 */
bool par_test_find_writable_scalar(bool require_persistent, par_num_t *par_num);

/**
 * @brief Find a writable scalar parameter with an alternate test value.
 * @param require_persistent true to require persistent metadata.
 * @param par_num Output parameter number.
 * @param original Output current scalar value.
 * @param alternate Output in-range value different from original.
 * @return true when a mutable matching parameter was found; otherwise false.
 */
bool par_test_find_mutable_writable_scalar(bool require_persistent,
                                           par_num_t *par_num,
                                           par_type_t *original,
                                           par_type_t *alternate);

#if defined(AUTOGEN_PM_TEST_NVM_RAW_HELPER)
/**
 * @brief Execute a raw NVM helper command using generic argc/argv input.
 * @param argc Argument count, including the command name.
 * @param argv Argument vector.
 * @return 0 on success; otherwise a negative error code.
 */
int par_test_nvm_raw_exec(int argc, char **argv);
#endif /* defined(AUTOGEN_PM_TEST_NVM_RAW_HELPER) */

#if defined(AUTOGEN_PM_TEST_NVM_FIXED_SLOT_WITH_SIZE)
/**
 * @brief Execute a fixed-slot-with-size NVM helper command using generic argc/argv input.
 * @param argc Argument count, including the command name.
 * @param argv Argument vector.
 * @return 0 on success; otherwise a negative error code.
 */
int par_test_nvm_fslot_exec(int argc, char **argv);
#endif /* defined(AUTOGEN_PM_TEST_NVM_FIXED_SLOT_WITH_SIZE) */


#if defined(AUTOGEN_PM_TEST_NVM_FIXED_SLOT_NO_SIZE)
/**
 * @brief Execute a fixed-slot-no-size NVM helper command using generic argc/argv input.
 * @param argc Argument count, including the command name.
 * @param argv Argument vector.
 * @return 0 on success; otherwise a negative error code.
 */
int par_test_nvm_fslot_no_size_exec(int argc, char **argv);
#endif /* defined(AUTOGEN_PM_TEST_NVM_FIXED_SLOT_NO_SIZE) */

#if defined(AUTOGEN_PM_TEST_NVM_COMPACT_PAYLOAD)
/**
 * @brief Execute a compact-payload NVM helper command using generic argc/argv input.
 * @param argc Argument count, including the command name.
 * @param argv Argument vector.
 * @return 0 on success; otherwise a negative error code.
 */
int par_test_nvm_compact_payload_exec(int argc, char **argv);
#endif /* defined(AUTOGEN_PM_TEST_NVM_COMPACT_PAYLOAD) */

#if defined(AUTOGEN_PM_TEST_NVM_FIXED_PAYLOAD_ONLY)
/**
 * @brief Execute a fixed-payload-only NVM helper command using generic argc/argv input.
 * @param argc Argument count, including the command name.
 * @param argv Argument vector.
 * @return 0 on success; otherwise a negative error code.
 */
int par_test_nvm_fixed_payload_only_exec(int argc, char **argv);
#endif /* defined(AUTOGEN_PM_TEST_NVM_FIXED_PAYLOAD_ONLY) */

#if defined(AUTOGEN_PM_TEST_NVM_GROUPED_PAYLOAD_ONLY)
/**
 * @brief Execute a grouped-payload-only NVM helper command using generic argc/argv input.
 * @param argc Argument count, including the command name.
 * @param argv Argument vector.
 * @return 0 on success; otherwise a negative error code.
 */
int par_test_nvm_grouped_payload_only_exec(int argc, char **argv);
#endif /* defined(AUTOGEN_PM_TEST_NVM_GROUPED_PAYLOAD_ONLY) */

#if defined(AUTOGEN_PM_TEST_NVM_OBJECT_HELPER)
/**
 * @brief Execute an object NVM helper command using generic argc/argv input.
 * @param argc Argument count, including the command name.
 * @param argv Argument vector.
 * @return 0 on success; otherwise a negative error code.
 */
int par_test_nvm_obj_exec(int argc, char **argv);
#endif /* defined(AUTOGEN_PM_TEST_NVM_OBJECT_HELPER) */

#if defined(AUTOGEN_PM_TEST_USING_RAM_CONFIG)
/**
 * @brief RAM configuration validation suite.
 */
extern const par_test_suite_t g_par_test_suite_ram;
#endif /* defined(AUTOGEN_PM_TEST_USING_RAM_CONFIG) */

#if defined(AUTOGEN_PM_TEST_USING_AT24CXX)
/**
 * @brief AT24CXX persistence validation suite.
 */
extern const par_test_suite_t g_par_test_suite_at24;
#endif /* defined(AUTOGEN_PM_TEST_USING_AT24CXX) */

#if defined(AUTOGEN_PM_TEST_USING_FLASH_EE_FAL)
/**
 * @brief Flash-ee real FAL partition validation suite.
 */
extern const par_test_suite_t g_par_test_suite_flash_ee_fal;
#endif /* defined(AUTOGEN_PM_TEST_USING_FLASH_EE_FAL) */

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* !defined(_PAR_TEST_H_) */
