/**
 * @file test_par_generated_runtime_consistency.c
 * @brief Check generated layout metadata against the C runtime table.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
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
#else
    TEST_ASSERT(g_par_generated_info.param_count == 6U);
    TEST_ASSERT(g_par_generated_info.count_obj == 3U);
    TEST_ASSERT(g_par_generated_info.obj_pool_bytes == 16UL);
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

#if !defined(PAR_HOST_TEST_GENERATED_SCALAR_ONLY)
/** @brief Verify generated object rows round-trip through the runtime object APIs. */
static bool test_generated_runtime_object_rows_roundtrip(void)
{
    par_num_t par_num = ePAR_NUM_OF;
    char name[9] = { 0 };
    uint8_t bytes[4] = { 0U };
    uint16_t arr16[2] = { 0U };
    uint16_t out_len = 0U;
    uint16_t out_count = 0U;
    const uint8_t key[] = { 0xAAU, 0x55U, 0x11U, 0x22U };
    const uint16_t arr_payload[] = { 30U, 40U };

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
    TEST_ASSERT_OK(par_get_arr_u16(ePAR_GEN_ARR16, arr16, 2U, &out_count));
    TEST_ASSERT(out_count == 2U);
    TEST_ASSERT(arr16[0] == 10U);
    TEST_ASSERT(arr16[1] == 20U);

    TEST_ASSERT_OK(par_set_str(ePAR_GEN_NAME, "runtime"));
    TEST_ASSERT_OK(par_set_bytes(ePAR_GEN_KEY, key, (uint16_t)sizeof(key)));
    TEST_ASSERT_OK(par_set_arr_u16(ePAR_GEN_ARR16, arr_payload, 2U));
    memset(name, 0, sizeof(name));
    memset(bytes, 0, sizeof(bytes));
    memset(arr16, 0, sizeof(arr16));
    TEST_ASSERT_OK(par_get_str(ePAR_GEN_NAME, name, sizeof(name), &out_len));
    TEST_ASSERT(out_len == 7U);
    TEST_ASSERT(0 == strcmp(name, "runtime"));
    TEST_ASSERT_OK(par_get_bytes(ePAR_GEN_KEY, bytes, sizeof(bytes), &out_len));
    TEST_ASSERT(out_len == sizeof(key));
    TEST_ASSERT(0 == memcmp(bytes, key, sizeof(key)));
    TEST_ASSERT_OK(par_get_arr_u16(ePAR_GEN_ARR16, arr16, 2U, &out_count));
    TEST_ASSERT(out_count == 2U);
    TEST_ASSERT(arr16[0] == 30U);
    TEST_ASSERT(arr16[1] == 40U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* !defined(PAR_HOST_TEST_GENERATED_SCALAR_ONLY) */

/** @brief Entrypoint for generated-output/runtime consistency tests. */
int main(void)
{
    static const par_host_test_case_t cases[] = {
        { "generated_runtime_scalar_rows", test_generated_runtime_scalar_rows },
#if !defined(PAR_HOST_TEST_GENERATED_SCALAR_ONLY)
        { "generated_runtime_object_rows_roundtrip", test_generated_runtime_object_rows_roundtrip },
#endif /* !defined(PAR_HOST_TEST_GENERATED_SCALAR_ONLY) */
    };

    return par_host_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
}
