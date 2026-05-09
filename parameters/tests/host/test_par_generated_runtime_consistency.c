/**
 * @file test_par_generated_runtime_consistency.c
 * @brief Check generated layout metadata against the C runtime table.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include "test_host_common.h"
#include "par_generated_info.h"

/** @brief Verify generated metadata and runtime lookup agree. */
static bool test_generated_runtime_consistency(void)
{
    par_num_t par_num = ePAR_NUM_OF;
    uint8_t ctrl = 0U;

    TEST_ASSERT(g_par_generated_info.param_count == (uint16_t)ePAR_NUM_OF);
    TEST_ASSERT(g_par_generated_info.count8 == (uint16_t)PAR_LAYOUT_STATIC_COUNT8);
    TEST_ASSERT(g_par_generated_info.count16 == (uint16_t)PAR_LAYOUT_STATIC_COUNT16);
    TEST_ASSERT(g_par_generated_info.count32 == (uint16_t)PAR_LAYOUT_STATIC_COUNT32);
    TEST_ASSERT(g_par_generated_info.count_obj == (uint16_t)PAR_LAYOUT_STATIC_COUNTOBJ);
    TEST_ASSERT(g_par_generated_info.obj_pool_bytes == (uint32_t)PAR_LAYOUT_STATIC_OBJ_POOL_BYTES);
    TEST_ASSERT_OK(par_init());
    TEST_ASSERT(par_cfg_get_table_size() == ((uint32_t)sizeof(par_cfg_t) * (uint32_t)ePAR_NUM_OF));
    TEST_ASSERT_OK(par_get_num_by_id(0U, &par_num));
    TEST_ASSERT(par_num == ePAR_CH1_CTRL);
    TEST_ASSERT_OK(par_get_u8(ePAR_CH1_CTRL, &ctrl));
    TEST_ASSERT(ctrl == 2U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Entrypoint for generated-output/runtime consistency tests. */
int main(void)
{
    static const par_host_test_case_t cases[] = {
        { "generated_runtime_consistency", test_generated_runtime_consistency },
    };

    return par_host_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
}
