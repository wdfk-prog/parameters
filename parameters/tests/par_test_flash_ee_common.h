/**
 * @file par_test_flash_ee_common.h
 * @brief Declare reusable Flash EE backend test flows.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#ifndef PAR_TEST_FLASH_EE_COMMON_H
#define PAR_TEST_FLASH_EE_COMMON_H

#include "par_test.h"
#include "par_store_backend.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/**
 * @brief Flash EE test-platform operations.
 */
typedef struct
{
    const char *name; /**< Platform name printed in failure details. */
    par_test_result_t (*reset)(par_test_context_t *ctx); /**< Reset the physical fake or hardware partition. */
    par_test_result_t (*open)(par_test_context_t *ctx, const par_store_backend_api_t **pp_api); /**< Bind and initialize backend. */
    par_test_result_t (*close)(par_test_context_t *ctx, const par_store_backend_api_t *p_api); /**< Deinitialize backend. */
    par_test_result_t (*get_capacity)(par_test_context_t *ctx, uint32_t *p_capacity); /**< Return append capacity per bank. */
} par_test_flash_ee_ops_t;

/**
 * @brief Fill a buffer with a deterministic Flash EE test pattern.
 * @param p_buf Destination buffer.
 * @param size Number of bytes to fill.
 * @param seed Pattern seed.
 */
void par_test_flash_ee_fill_pattern(uint8_t *p_buf, uint32_t size, uint8_t seed);

/**
 * @brief Return true when a buffer contains only logical erased bytes.
 * @param p_buf Buffer to inspect.
 * @param size Number of bytes to inspect.
 * @return true when every byte is erased.
 */
bool par_test_flash_ee_is_erased(const uint8_t *p_buf, uint32_t size);

/**
 * @brief Write one full logical line with a deterministic pattern.
 * @param ctx Test context.
 * @param p_api Backend API.
 * @param line_index Logical line index.
 * @param tag Pattern tag encoded across the line.
 * @param p_expected Optional expected-pattern output buffer.
 * @param p_step Failure-detail prefix.
 * @return Case result.
 */
par_test_result_t par_test_flash_ee_write_line_pattern(par_test_context_t *ctx,
                                                       const par_store_backend_api_t *p_api,
                                                       uint32_t line_index,
                                                       uint32_t tag,
                                                       uint8_t *p_expected,
                                                       const char *p_step);

/**
 * @brief Read and compare one full logical line.
 * @param ctx Test context.
 * @param p_api Backend API.
 * @param line_index Logical line index.
 * @param p_expected Expected line payload.
 * @param p_step Failure-detail prefix.
 * @return Case result.
 */
par_test_result_t par_test_flash_ee_expect_line(par_test_context_t *ctx,
                                                const par_store_backend_api_t *p_api,
                                                uint32_t line_index,
                                                const uint8_t *p_expected,
                                                const char *p_step);

/**
 * @brief Validate that a Flash EE platform can bind and initialize.
 * @param ctx Test context.
 * @param p_ops Platform operations.
 * @return Case result.
 */
par_test_result_t par_test_flash_ee_common_bind_init(par_test_context_t *ctx, const par_test_flash_ee_ops_t *p_ops);

/**
 * @brief Validate write/read data survives backend reinitialization.
 * @param ctx Test context.
 * @param p_ops Platform operations.
 * @return Case result.
 */
par_test_result_t par_test_flash_ee_common_write_read_persists_after_reinit(par_test_context_t *ctx,
                                                                           const par_test_flash_ee_ops_t *p_ops);

/**
 * @brief Validate logical erase survives backend reinitialization.
 * @param ctx Test context.
 * @param p_ops Platform operations.
 * @return Case result.
 */
par_test_result_t par_test_flash_ee_common_erase_persists_after_reinit(par_test_context_t *ctx,
                                                                       const par_test_flash_ee_ops_t *p_ops);

/**
 * @brief Validate rollover preserves newest lines and remains writable.
 * @param ctx Test context.
 * @param p_ops Platform operations.
 * @return Case result.
 */
par_test_result_t par_test_flash_ee_common_wrap_after_bank_full_preserves_latest(par_test_context_t *ctx,
                                                                                const par_test_flash_ee_ops_t *p_ops);

/**
 * @brief Validate multiple rollover cycles preserve the latest logical image.
 * @param ctx Test context.
 * @param p_ops Platform operations.
 * @return Case result.
 */
par_test_result_t par_test_flash_ee_common_wrap_multiple_cycles_preserves_latest(par_test_context_t *ctx,
                                                                                const par_test_flash_ee_ops_t *p_ops);

/**
 * @brief Validate a full bank is writable after reboot-like reinit.
 * @param ctx Test context.
 * @param p_ops Platform operations.
 * @return Case result.
 */
par_test_result_t par_test_flash_ee_common_full_bank_then_reinit_remains_writable(par_test_context_t *ctx,
                                                                                 const par_test_flash_ee_ops_t *p_ops);

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* !defined(PAR_TEST_FLASH_EE_COMMON_H) */
