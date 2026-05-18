/**
 * @file test_par_config_smoke.c
 * @brief Provide a small runtime smoke test for legal feature-off matrices.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include "test_host_common.h"

/** @brief Verify core scalar APIs remain usable in a legal feature-off build. */
static bool test_config_scalar_smoke(void)
{
    uint8_t value = 0U;

    TEST_ASSERT_OK(par_init());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 1U);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 4U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}



#if defined(PAR_HOST_TEST_CONFIG_OBJECT_TYPE_SMOKE) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
/** @brief Verify enabled object APIs remain usable in legal feature-off builds. */
static bool test_config_object_type_smoke(void)
{
    TEST_ASSERT_OK(par_init());
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
    {
        char out[9] = { 0 };
        uint16_t len = 0U;

        TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "cfg"));
        TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, out, sizeof(out), &len));
        TEST_ASSERT(len == 3U);
        TEST_ASSERT(0 == strcmp(out, "cfg"));
    }
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
    {
        const uint8_t payload[4] = { 1U, 2U, 3U, 4U };
        uint8_t out[4] = { 0U };
        uint16_t len = 0U;

        TEST_ASSERT_OK(par_set_bytes(ePAR_TEST_BYTES, payload, (uint16_t)sizeof(payload)));
        TEST_ASSERT_OK(par_get_bytes(ePAR_TEST_BYTES, out, (uint16_t)sizeof(out), &len));
        TEST_ASSERT(len == sizeof(payload));
        TEST_ASSERT(0 == memcmp(out, payload, sizeof(payload)));
    }
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
    {
        const uint8_t payload[3] = { 8U, 7U, 6U };
        uint8_t out[3] = { 0U };
        uint16_t count = 0U;

        TEST_ASSERT_OK(par_set_arr_u8(ePAR_TEST_ARR_U8, payload, 3U));
        TEST_ASSERT_OK(par_get_arr_u8(ePAR_TEST_ARR_U8, out, 3U, &count));
        TEST_ASSERT(count == 3U);
        TEST_ASSERT(0 == memcmp(out, payload, sizeof(payload)));
    }
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
    {
        const uint16_t payload[2] = { 11U, 22U };
        uint16_t out[2] = { 0U };
        uint16_t count = 0U;

        TEST_ASSERT_OK(par_set_arr_u16(ePAR_TEST_ARR_U16, payload, 2U));
        TEST_ASSERT_OK(par_get_arr_u16(ePAR_TEST_ARR_U16, out, 2U, &count));
        TEST_ASSERT(count == 2U);
        TEST_ASSERT(0 == memcmp(out, payload, sizeof(payload)));
    }
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
    {
        const uint32_t payload[2] = { 111U, 222U };
        uint32_t out[2] = { 0U };
        uint16_t count = 0U;

        TEST_ASSERT_OK(par_set_arr_u32(ePAR_TEST_ARR_U32, payload, 2U));
        TEST_ASSERT_OK(par_get_arr_u32(ePAR_TEST_ARR_U32, out, 2U, &count));
        TEST_ASSERT(count == 2U);
        TEST_ASSERT(0 == memcmp(out, payload, sizeof(payload)));
    }
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* defined(PAR_HOST_TEST_CONFIG_OBJECT_TYPE_SMOKE) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

#if defined(PAR_HOST_TEST_CONFIG_ACCESS_DISABLED) && (0 == PAR_CFG_ENABLE_ACCESS)
/** @brief Verify disabled access checks allow writing a read-only row. */
static bool test_config_access_disabled_allows_read_only_row_write(void)
{
    uint8_t value = 0U;

    TEST_ASSERT_OK(par_init());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_RO, 4U));
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_RO, &value));
    TEST_ASSERT(value == 4U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* defined(PAR_HOST_TEST_CONFIG_ACCESS_DISABLED) && (0 == PAR_CFG_ENABLE_ACCESS) */

#if defined(PAR_HOST_TEST_CONFIG_RANGE_DISABLED) && (0 == PAR_CFG_ENABLE_RANGE)
/** @brief Verify disabled range checks allow nominally out-of-range values. */
static bool test_config_range_disabled_accepts_out_of_range_value(void)
{
    uint8_t value = 0U;

    TEST_ASSERT_OK(par_init());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 250U));
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 250U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* defined(PAR_HOST_TEST_CONFIG_RANGE_DISABLED) && (0 == PAR_CFG_ENABLE_RANGE) */

#if defined(PAR_HOST_TEST_CONFIG_ROLE_POLICY_DISABLED) && (0 == PAR_CFG_ENABLE_ROLE_POLICY)
/** @brief Verify disabled role-policy builds keep scalar API behavior stable. */
static bool test_config_role_policy_disabled_scalar_runtime_smoke(void)
{
    uint8_t value = 0U;

    TEST_ASSERT_OK(par_init());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 6U));
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 6U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* defined(PAR_HOST_TEST_CONFIG_ROLE_POLICY_DISABLED) && (0 == PAR_CFG_ENABLE_ROLE_POLICY) */

#if defined(PAR_HOST_TEST_CONFIG_NO_CALLBACK_VALIDATION) && \
    (0 == PAR_CFG_ENABLE_CHANGE_CALLBACK) && (0 == PAR_CFG_ENABLE_RUNTIME_VALIDATION)
/** @brief Verify callback and validation feature-off builds keep setters usable. */
static bool test_config_no_callback_validation_runtime_smoke(void)
{
    uint8_t value = 0U;

    TEST_ASSERT_OK(par_init());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 7U));
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 7U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* defined(PAR_HOST_TEST_CONFIG_NO_CALLBACK_VALIDATION) && (0 == PAR_CFG_ENABLE_CHANGE_CALLBACK) && (0 == PAR_CFG_ENABLE_RUNTIME_VALIDATION) */

/** @brief Entrypoint for feature-off configuration smoke tests. */
int main(void)
{
    static const par_host_test_case_t cases[] = {
        { "config_scalar_smoke", test_config_scalar_smoke },
#if defined(PAR_HOST_TEST_CONFIG_OBJECT_TYPE_SMOKE) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
        { "config_object_type_smoke", test_config_object_type_smoke },
#endif /* defined(PAR_HOST_TEST_CONFIG_OBJECT_TYPE_SMOKE) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */
#if defined(PAR_HOST_TEST_CONFIG_ACCESS_DISABLED) && (0 == PAR_CFG_ENABLE_ACCESS)
        { "config_access_disabled_allows_read_only_row_write", test_config_access_disabled_allows_read_only_row_write },
#endif /* defined(PAR_HOST_TEST_CONFIG_ACCESS_DISABLED) && (0 == PAR_CFG_ENABLE_ACCESS) */
#if defined(PAR_HOST_TEST_CONFIG_RANGE_DISABLED) && (0 == PAR_CFG_ENABLE_RANGE)
        { "config_range_disabled_accepts_out_of_range_value", test_config_range_disabled_accepts_out_of_range_value },
#endif /* defined(PAR_HOST_TEST_CONFIG_RANGE_DISABLED) && (0 == PAR_CFG_ENABLE_RANGE) */
#if defined(PAR_HOST_TEST_CONFIG_ROLE_POLICY_DISABLED) && (0 == PAR_CFG_ENABLE_ROLE_POLICY)
        { "config_role_policy_disabled_scalar_runtime_smoke", test_config_role_policy_disabled_scalar_runtime_smoke },
#endif /* defined(PAR_HOST_TEST_CONFIG_ROLE_POLICY_DISABLED) && (0 == PAR_CFG_ENABLE_ROLE_POLICY) */
#if defined(PAR_HOST_TEST_CONFIG_NO_CALLBACK_VALIDATION) && \
    (0 == PAR_CFG_ENABLE_CHANGE_CALLBACK) && (0 == PAR_CFG_ENABLE_RUNTIME_VALIDATION)
        { "config_no_callback_validation_runtime_smoke", test_config_no_callback_validation_runtime_smoke },
#endif /* defined(PAR_HOST_TEST_CONFIG_NO_CALLBACK_VALIDATION) && (0 == PAR_CFG_ENABLE_CHANGE_CALLBACK) && (0 == PAR_CFG_ENABLE_RUNTIME_VALIDATION) */
    };

    return par_host_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
}
