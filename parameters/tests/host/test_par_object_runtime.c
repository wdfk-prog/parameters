/**
 * @file test_par_object_runtime.c
 * @brief Exercise STR, BYTES, and typed-array object parameter behavior.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include "test_host_common.h"
#include "par_registration_api.h"
#include "par_object.h"

/** @brief Object validation acceptance switch. */
static bool g_obj_validation_accept = true;
/** @brief Last object payload observed by validation tests. */
static uint8_t g_obj_validation_seen[8];
/** @brief Last object payload length observed by validation tests. */
static uint16_t g_obj_validation_seen_len;

/**
 * @brief Conditionally accept object payload writes for validation tests.
 * @param par_num Parameter number being written.
 * @param p_data Candidate payload bytes.
 * @param len Candidate payload length in bytes.
 * @return true when the payload is accepted.
 */
static bool object_validation(const par_num_t par_num,
                              const uint8_t *p_data,
                              const uint16_t len)
{
    (void)par_num;
    g_obj_validation_seen_len = len;
    if ((NULL != p_data) && (len <= (uint16_t)sizeof(g_obj_validation_seen)))
    {
        memcpy(g_obj_validation_seen, p_data, len);
    }
    return g_obj_validation_accept;
}

/**
 * @brief Update another object while validating a string object.
 * @param par_num Parameter number being written.
 * @param p_data Candidate payload bytes.
 * @param len Candidate payload length in bytes.
 * @return true when the reentrant object update succeeds.
 */
static bool object_validation_reentrant_update_bytes(const par_num_t par_num,
                                                     const uint8_t *p_data,
                                                     const uint16_t len)
{
    const uint8_t payload[4] = { 7U, 8U, 9U, 10U };

    (void)par_num;
    (void)p_data;
    (void)len;
    return (ePAR_OK == par_set_bytes(ePAR_TEST_BYTES, payload, (uint16_t)sizeof(payload)));
}

/**
 * @brief Initialize the parameter module for one object test case.
 * @return true when initialization succeeds.
 */
static bool init_module(void)
{
    if (par_is_init())
    {
        TEST_ASSERT_OK(par_deinit());
    }

    TEST_ASSERT_OK(par_init());
    return true;
}

#include "object_runtime/par_object_runtime_basic_cases.inc"
#include "object_runtime/par_object_runtime_policy_cases.inc"

/** @brief Entrypoint for object host runtime tests. */
int main(void)
{
    static const par_host_test_case_t cases[] = {
        { "object_api_use_before_init_returns_init_error", test_object_api_use_before_init_returns_init_error },
        { "object_api_after_deinit_returns_init_error", test_object_api_after_deinit_returns_init_error },
        { "object_default_api_use_before_init_reports_metadata", test_object_default_api_use_before_init_reports_metadata },
        { "object_str_boundaries", test_object_str_boundaries },
        { "object_bytes_boundaries", test_object_bytes_boundaries },
        { "object_arr_u8_roundtrip", test_object_arr_u8_roundtrip },
        { "object_arr_u16_count_to_byte_length", test_object_arr_u16_count_to_byte_length },
        { "object_arr_u32_count_to_byte_length", test_object_arr_u32_count_to_byte_length },
        { "object_by_id_and_capacity", test_object_by_id_and_capacity },
        { "object_bytes_and_arrays_by_id_wrappers", test_object_bytes_and_arrays_by_id_wrappers },
        { "object_default_str_and_metadata_by_id_wrappers", test_object_default_str_and_metadata_by_id_wrappers },
        { "object_default_non_id_apis_match_by_id_apis", test_object_default_non_id_apis_match_by_id_apis },
        { "object_null_zero_and_small_buffer_policies", test_object_null_zero_and_small_buffer_policies },
        { "object_scalar_api_cross_type_errors_do_not_mutate", test_object_scalar_api_cross_type_errors_do_not_mutate },
        { "object_source_overlap_rejected_without_mutation", test_object_source_overlap_rejected_without_mutation },
        { "object_validation_and_default_reset", test_object_validation_and_default_reset },
        { "object_get_default_small_buffer_does_not_partial_overwrite", test_object_get_default_small_buffer_does_not_partial_overwrite },
        { "object_get_bytes_null_buffer_reports_len_without_copy", test_object_get_bytes_null_buffer_reports_len_without_copy },
        { "object_validation_rejects_without_payload_mutation", test_object_validation_rejects_without_payload_mutation },
        { "object_validation_rejects_without_changed_state", test_object_validation_rejects_without_changed_state },
        { "object_validation_reentrant_updates_other_object", test_object_validation_reentrant_updates_other_object },
    };

    return par_host_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
}
