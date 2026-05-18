/**
 * @file test_par_generated_runtime_consistency.c
 * @brief Check generated layout metadata against the C runtime table.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include <stdint.h>
#include <string.h>

#include "par.h"
#include "par_cfg.h"
#include "par_layout_static.h"
#include "test_host_common.h"
#include "par_generated_info.h"

/** @brief Verify generated scalar metadata and runtime lookup agree. */
static bool test_generated_runtime_scalar_rows(void)
{
    par_num_t par_num = ePAR_NUM_OF;
    uint8_t mode = 0U;
    uint16_t rate = 0U;
    uint32_t flags = 0UL;

#if defined(PAR_HOST_TEST_GENERATED_SCALAR_ONLY)
    TEST_ASSERT(g_par_generated_info.param_count == 3U);
    TEST_ASSERT(g_par_generated_info.count_obj == 0U);
    TEST_ASSERT(g_par_generated_info.obj_pool_bytes == 0UL);
#elif defined(PAR_HOST_TEST_GENERATED_CONDITIONAL_DISABLED)
    TEST_ASSERT(g_par_generated_info.param_count == 5U);
    TEST_ASSERT(g_par_generated_info.count_obj == 2U);
    TEST_ASSERT(g_par_generated_info.obj_pool_bytes == 12UL);
#else
    TEST_ASSERT(g_par_generated_info.param_count == 7U);
    TEST_ASSERT(g_par_generated_info.count_obj == 4U);
    TEST_ASSERT(g_par_generated_info.obj_pool_bytes == 19UL);
#endif /* defined(PAR_HOST_TEST_GENERATED_SCALAR_ONLY) */
    TEST_ASSERT(g_par_generated_info.count8 == 1U);
    TEST_ASSERT(g_par_generated_info.count16 == 1U);
    TEST_ASSERT(g_par_generated_info.count32 == 1U);
    TEST_ASSERT(g_par_generated_info.param_count == (uint16_t)ePAR_NUM_OF);
    TEST_ASSERT(g_par_generated_info.count8 == (uint16_t)PAR_LAYOUT_STATIC_COUNT8);
    TEST_ASSERT(g_par_generated_info.count16 == (uint16_t)PAR_LAYOUT_STATIC_COUNT16);
    TEST_ASSERT(g_par_generated_info.count32 == (uint16_t)PAR_LAYOUT_STATIC_COUNT32);
    TEST_ASSERT(g_par_generated_info.count_obj == (uint16_t)PAR_LAYOUT_STATIC_COUNTOBJ);
    TEST_ASSERT(g_par_generated_info.obj_pool_bytes == (uint32_t)PAR_LAYOUT_STATIC_OBJ_POOL_BYTES);
    TEST_ASSERT(g_par_generated_info.layout_signature == (uint32_t)PAR_LAYOUT_STATIC_SIGNATURE);

    TEST_ASSERT_OK(par_init());
    TEST_ASSERT(par_cfg_get_table_size() == ((uint32_t)sizeof(par_cfg_t) * (uint32_t)ePAR_NUM_OF));
    TEST_ASSERT_OK(par_get_num_by_id(0U, &par_num));
    TEST_ASSERT(par_num == ePAR_GEN_MODE);
    TEST_ASSERT_OK(par_get_num_by_id(2U, &par_num));
    TEST_ASSERT(par_num == ePAR_GEN_FLAGS);
    TEST_ASSERT_OK(par_get_u8(ePAR_GEN_MODE, &mode));
    TEST_ASSERT_OK(par_get_u16(ePAR_GEN_RATE, &rate));
    TEST_ASSERT_OK(par_get_u32(ePAR_GEN_FLAGS, &flags));
    TEST_ASSERT(mode == 2U);
    TEST_ASSERT(rate == 100U);
    TEST_ASSERT(flags == 0x10UL);
    TEST_ASSERT_OK(par_set_u16(ePAR_GEN_RATE, 321U));
    TEST_ASSERT_OK(par_get_u16(ePAR_GEN_RATE, &rate));
    TEST_ASSERT(rate == 321U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify every generated external ID resolves to the expected enum. */
static bool test_generated_id_lookup_matches_runtime_table(void)
{
    par_num_t par_num = ePAR_NUM_OF;
    uint16_t id = 0U;

    TEST_ASSERT_OK(par_init());
    TEST_ASSERT_OK(par_get_num_by_id(0U, &par_num));
    TEST_ASSERT(par_num == ePAR_GEN_MODE);
    TEST_ASSERT_OK(par_get_id_by_num(ePAR_GEN_MODE, &id));
    TEST_ASSERT(id == 0U);
    TEST_ASSERT_OK(par_get_num_by_id(1U, &par_num));
    TEST_ASSERT(par_num == ePAR_GEN_RATE);
    TEST_ASSERT_OK(par_get_id_by_num(ePAR_GEN_RATE, &id));
    TEST_ASSERT(id == 1U);
    TEST_ASSERT_OK(par_get_num_by_id(2U, &par_num));
    TEST_ASSERT(par_num == ePAR_GEN_FLAGS);
    TEST_ASSERT_OK(par_get_id_by_num(ePAR_GEN_FLAGS, &id));
    TEST_ASSERT(id == 2U);
#if !defined(PAR_HOST_TEST_GENERATED_SCALAR_ONLY)
    TEST_ASSERT_OK(par_get_num_by_id(3U, &par_num));
    TEST_ASSERT(par_num == ePAR_GEN_NAME);
    TEST_ASSERT_OK(par_get_id_by_num(ePAR_GEN_NAME, &id));
    TEST_ASSERT(id == 3U);
#if defined(PAR_HOST_TEST_GENERATED_CONDITIONAL_DISABLED)
    TEST_ASSERT((par_get_num_by_id(4U, &par_num) & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
#else
    TEST_ASSERT_OK(par_get_num_by_id(4U, &par_num));
    TEST_ASSERT(par_num == ePAR_GEN_KEY);
    TEST_ASSERT_OK(par_get_id_by_num(ePAR_GEN_KEY, &id));
    TEST_ASSERT(id == 4U);
#endif /* defined(PAR_HOST_TEST_GENERATED_CONDITIONAL_DISABLED) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
    TEST_ASSERT_OK(par_get_num_by_id(5U, &par_num));
    TEST_ASSERT(par_num == ePAR_GEN_SKIP8);
    TEST_ASSERT_OK(par_get_id_by_num(ePAR_GEN_SKIP8, &id));
    TEST_ASSERT(id == 5U);
#else
    TEST_ASSERT((par_get_num_by_id(5U, &par_num) & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */
    TEST_ASSERT_OK(par_get_num_by_id(6U, &par_num));
    TEST_ASSERT(par_num == ePAR_GEN_ARR16);
    TEST_ASSERT_OK(par_get_id_by_num(ePAR_GEN_ARR16, &id));
    TEST_ASSERT(id == 6U);
#endif /* !defined(PAR_HOST_TEST_GENERATED_SCALAR_ONLY) */
    TEST_ASSERT((par_get_num_by_id(0xFFFFU, &par_num) & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

#if defined(PAR_HOST_TEST_MANIFEST_PARAM_COUNT_MAX)
/** @brief Verify manifest-derived count macros match generated runtime metadata. */
static bool test_generated_manifest_counts_match_runtime_info(void)
{
    TEST_ASSERT(PAR_HOST_TEST_MANIFEST_PARAM_COUNT_MAX == g_par_generated_info.param_count);
    TEST_ASSERT(PAR_HOST_TEST_MANIFEST_COUNT8 == g_par_generated_info.count8);
    TEST_ASSERT(PAR_HOST_TEST_MANIFEST_COUNT16 == g_par_generated_info.count16);
    TEST_ASSERT(PAR_HOST_TEST_MANIFEST_COUNT32 == g_par_generated_info.count32);
    TEST_ASSERT(PAR_HOST_TEST_MANIFEST_COUNTOBJ == g_par_generated_info.count_obj);
    TEST_ASSERT(PAR_HOST_TEST_MANIFEST_OBJ_POOL_BYTES == g_par_generated_info.obj_pool_bytes);
    TEST_ASSERT(PAR_HOST_TEST_MANIFEST_LAYOUT_SIGNATURE == g_par_generated_info.layout_signature);
    return true;
}
#endif /* defined(PAR_HOST_TEST_MANIFEST_PARAM_COUNT_MAX) */

#if !defined(PAR_HOST_TEST_GENERATED_SCALAR_ONLY) && \
    !defined(PAR_HOST_TEST_GENERATED_CONDITIONAL_DISABLED)
/** @brief Verify generated object rows round-trip through the runtime object APIs. */
static bool test_generated_runtime_object_rows_roundtrip(void)
{
    par_num_t par_num = ePAR_NUM_OF;
    char name[9] = { 0 };
    uint8_t bytes[4] = { 0U };
    uint8_t arr8[3] = { 0U };
    uint16_t arr16[2] = { 0U };
    uint16_t out_len = 0U;
    uint16_t out_count = 0U;
    const uint8_t key[] = { 0xAAU, 0x55U, 0x11U, 0x22U };
    const uint8_t arr8_payload[] = { 3U, 2U, 1U };
    const uint16_t arr16_payload[] = { 30U, 40U };

    TEST_ASSERT_OK(par_init());
    TEST_ASSERT_OK(par_get_num_by_id(3U, &par_num));
    TEST_ASSERT(par_num == ePAR_GEN_NAME);
    TEST_ASSERT_OK(par_get_str(ePAR_GEN_NAME, name, sizeof(name), &out_len));
    TEST_ASSERT(out_len == 2U);
    TEST_ASSERT(0 == strcmp(name, "ap"));
    TEST_ASSERT_OK(par_get_bytes(ePAR_GEN_KEY, bytes, sizeof(bytes), &out_len));
    TEST_ASSERT(out_len == 3U);
    TEST_ASSERT(bytes[0] == 0x01U);
    TEST_ASSERT(bytes[1] == 0x02U);
    TEST_ASSERT(bytes[2] == 0x03U);
    TEST_ASSERT_OK(par_get_arr_u8(ePAR_GEN_SKIP8, arr8, 3U, &out_count));
    TEST_ASSERT(out_count == 3U);
    TEST_ASSERT(arr8[0] == 1U);
    TEST_ASSERT(arr8[1] == 2U);
    TEST_ASSERT(arr8[2] == 3U);
    TEST_ASSERT_OK(par_get_arr_u16(ePAR_GEN_ARR16, arr16, 2U, &out_count));
    TEST_ASSERT(out_count == 2U);
    TEST_ASSERT(arr16[0] == 10U);
    TEST_ASSERT(arr16[1] == 20U);

    TEST_ASSERT_OK(par_set_str(ePAR_GEN_NAME, "rt"));
    TEST_ASSERT_OK(par_set_bytes(ePAR_GEN_KEY, key, (uint16_t)sizeof(key)));
    TEST_ASSERT_OK(par_set_arr_u8(ePAR_GEN_SKIP8, arr8_payload, 3U));
    TEST_ASSERT_OK(par_set_arr_u16(ePAR_GEN_ARR16, arr16_payload, 2U));
    memset(name, 0, sizeof(name));
    memset(bytes, 0, sizeof(bytes));
    memset(arr8, 0, sizeof(arr8));
    memset(arr16, 0, sizeof(arr16));
    TEST_ASSERT_OK(par_get_str(ePAR_GEN_NAME, name, sizeof(name), &out_len));
    TEST_ASSERT(out_len == 2U);
    TEST_ASSERT(0 == strcmp(name, "rt"));
    TEST_ASSERT_OK(par_get_bytes(ePAR_GEN_KEY,
                                 bytes,
                                 (uint16_t)sizeof(bytes),
                                 &out_len));
    TEST_ASSERT(out_len == (uint16_t)sizeof(key));
    TEST_ASSERT(0 == memcmp(bytes, key, sizeof(key)));
    TEST_ASSERT_OK(par_get_arr_u8(ePAR_GEN_SKIP8, arr8, 3U, &out_count));
    TEST_ASSERT(out_count == 3U);
    TEST_ASSERT(0 == memcmp(arr8, arr8_payload, sizeof(arr8_payload)));
    TEST_ASSERT_OK(par_get_arr_u16(ePAR_GEN_ARR16, arr16, 2U, &out_count));
    TEST_ASSERT(out_count == 2U);
    TEST_ASSERT(arr16[0] == 30U);
    TEST_ASSERT(arr16[1] == 40U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* !defined(PAR_HOST_TEST_GENERATED_SCALAR_ONLY) && !defined(PAR_HOST_TEST_GENERATED_CONDITIONAL_DISABLED) */

#if defined(PAR_HOST_TEST_GENERATED_CONDITIONAL_DISABLED)
/** @brief Verify disabled conditional rows do not shift later object rows. */
static bool test_generated_conditional_disabled_rows_keep_later_offsets(void)
{
    par_num_t par_num = ePAR_NUM_OF;
    char name[9] = { 0 };
    uint16_t arr16[2] = { 0U };
    uint16_t out_len = 0U;
    uint16_t out_count = 0U;

    TEST_ASSERT_OK(par_init());
    TEST_ASSERT((par_get_num_by_id(4U, &par_num) & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    TEST_ASSERT((par_get_num_by_id(5U, &par_num) & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    TEST_ASSERT_OK(par_get_num_by_id(6U, &par_num));
    TEST_ASSERT(par_num == ePAR_GEN_ARR16);
    TEST_ASSERT_OK(par_get_str(ePAR_GEN_NAME, name, sizeof(name), &out_len));
    TEST_ASSERT(out_len == 2U);
    TEST_ASSERT(0 == strcmp(name, "ap"));
    TEST_ASSERT_OK(par_get_arr_u16(ePAR_GEN_ARR16, arr16, 2U, &out_count));
    TEST_ASSERT(out_count == 2U);
    TEST_ASSERT(arr16[0] == 10U);
    TEST_ASSERT(arr16[1] == 20U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify enabled object rows do not overlap after disabled object holes. */
static bool test_generated_conditional_disabled_object_pool_offsets_do_not_overlap(void)
{
    char name[9] = { 0 };
    uint16_t arr16[2] = { 0U };
    uint16_t out_len = 0U;
    uint16_t out_count = 0U;
    const uint16_t arr_payload[2] = { 101U, 202U };

    TEST_ASSERT_OK(par_init());
    TEST_ASSERT_OK(par_set_str(ePAR_GEN_NAME, "12345678"));
    TEST_ASSERT_OK(par_set_arr_u16(ePAR_GEN_ARR16, arr_payload, 2U));
    TEST_ASSERT_OK(par_get_str(ePAR_GEN_NAME, name, sizeof(name), &out_len));
    TEST_ASSERT(out_len == 8U);
    TEST_ASSERT(0 == strcmp(name, "12345678"));
    TEST_ASSERT_OK(par_get_arr_u16(ePAR_GEN_ARR16, arr16, 2U, &out_count));
    TEST_ASSERT(out_count == 2U);
    TEST_ASSERT(arr16[0] == 101U);
    TEST_ASSERT(arr16[1] == 202U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* defined(PAR_HOST_TEST_GENERATED_CONDITIONAL_DISABLED) */

/** @brief Entrypoint for generated-output/runtime consistency tests. */
int main(void)
{
    static const par_host_test_case_t cases[] = {
        { "generated_runtime_scalar_rows", test_generated_runtime_scalar_rows },
        { "generated_id_lookup_matches_runtime_table", test_generated_id_lookup_matches_runtime_table },
#if defined(PAR_HOST_TEST_MANIFEST_PARAM_COUNT_MAX)
        { "generated_manifest_counts_match_runtime_info", test_generated_manifest_counts_match_runtime_info },
#endif /* defined(PAR_HOST_TEST_MANIFEST_PARAM_COUNT_MAX) */
#if !defined(PAR_HOST_TEST_GENERATED_SCALAR_ONLY) && \
    !defined(PAR_HOST_TEST_GENERATED_CONDITIONAL_DISABLED)
        { "generated_runtime_object_rows_roundtrip", test_generated_runtime_object_rows_roundtrip },
#endif /* !defined(PAR_HOST_TEST_GENERATED_SCALAR_ONLY) && !defined(PAR_HOST_TEST_GENERATED_CONDITIONAL_DISABLED) */
#if defined(PAR_HOST_TEST_GENERATED_CONDITIONAL_DISABLED)
        { "generated_conditional_disabled_rows_keep_later_offsets", test_generated_conditional_disabled_rows_keep_later_offsets },
        { "generated_conditional_disabled_object_pool_offsets_do_not_overlap", test_generated_conditional_disabled_object_pool_offsets_do_not_overlap },
#endif /* defined(PAR_HOST_TEST_GENERATED_CONDITIONAL_DISABLED) */
    };

    return par_host_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
}
