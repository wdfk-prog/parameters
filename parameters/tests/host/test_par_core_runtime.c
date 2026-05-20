/**
 * @file test_par_core_runtime.c
 * @brief Exercise scalar parameter runtime behavior on a host build.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include "test_host_common.h"
#include "par_cfg_table_api.h"
#include "par_core_api.h"
#include "par_registration_api.h"

/** @brief Callback hit counter used by scalar on-change tests. */
static unsigned g_on_change_hits;
/** @brief Last parameter number observed by the scalar on-change callback. */
static par_num_t g_on_change_last_par;
/** @brief Validation callback acceptance switch. */
static bool g_validation_accept = true;
/** @brief Callback hit counter used by reentrant callback tests. */
static unsigned g_reentrant_hits;
/** @brief Validation callback readback result flag. */
static bool g_validation_read_ok;
/** @brief Callback registration mutation hit counter. */
static unsigned g_callback_register_hits;

/**
 * @brief Reset callback state shared by scalar callback test cases.
 */
static void reset_callback_state(void)
{
    g_on_change_hits = 0U;
    g_on_change_last_par = ePAR_NUM_OF;
    g_validation_accept = true;
    g_reentrant_hits = 0U;
    g_validation_read_ok = false;
    g_callback_register_hits = 0U;
}

/**
 * @brief Capture successful scalar value changes.
 * @param par_num Parameter number that changed.
 * @param new_val New scalar value.
 * @param old_val Previous scalar value.
 */
static void on_scalar_change(const par_num_t par_num,
                             const par_type_t new_val,
                             const par_type_t old_val)
{
    (void)new_val;
    (void)old_val;
    g_on_change_hits++;
    g_on_change_last_par = par_num;
}


/**
 * @brief Update another scalar from inside an on-change callback.
 * @param par_num Parameter number that changed.
 * @param new_val New scalar value.
 * @param old_val Previous scalar value.
 */
static void on_scalar_change_reentrant_update(const par_num_t par_num,
                                              const par_type_t new_val,
                                              const par_type_t old_val)
{
    (void)par_num;
    (void)new_val;
    (void)old_val;
    g_reentrant_hits++;
    (void)par_set_u16(ePAR_TEST_U16, 333U);
}

/**
 * @brief Re-enter the same scalar setter with the already committed value.
 * @param par_num Parameter number that changed.
 * @param new_val New scalar value.
 * @param old_val Previous scalar value.
 */
static void on_scalar_change_same_parameter_reentry(const par_num_t par_num,
                                                    const par_type_t new_val,
                                                    const par_type_t old_val)
{
    (void)old_val;
    g_reentrant_hits++;
    if (1U == g_reentrant_hits)
    {
        (void)par_set_u8(par_num, new_val.u8);
    }
}

/**
 * @brief Register a different callback while a callback is being dispatched.
 * @param par_num Parameter number that changed.
 * @param new_val New scalar value.
 * @param old_val Previous scalar value.
 */
static void on_scalar_change_register_new_callback(const par_num_t par_num,
                                                   const par_type_t new_val,
                                                   const par_type_t old_val)
{
    (void)new_val;
    (void)old_val;
    g_callback_register_hits++;
    par_register_on_change_cb(par_num, on_scalar_change);
}

/**
 * @brief Conditionally accept scalar writes for validation-path tests.
 * @param par_num Parameter number being written.
 * @param val Candidate scalar value.
 * @return true when the candidate is accepted.
 */
static bool scalar_validation(const par_num_t par_num, const par_type_t val)
{
    (void)par_num;
    (void)val;
    return g_validation_accept;
}

/**
 * @brief Validate a scalar while reading another parameter through the API.
 * @param par_num Parameter number being written.
 * @param val Candidate scalar value.
 * @return true when the validation read succeeds.
 */
static bool scalar_validation_reads_current_value(const par_num_t par_num,
                                                  const par_type_t val)
{
    uint8_t current = 0U;

    (void)par_num;
    (void)val;
    g_validation_read_ok = (ePAR_OK == par_get_u8(ePAR_TEST_MODE, &current));
    return g_validation_read_ok;
}

/**
 * @brief Update another scalar from inside scalar validation.
 * @param par_num Parameter number being written.
 * @param val Candidate scalar value.
 * @return true when the reentrant update succeeds.
 */
static bool scalar_validation_reentrant_update_other(const par_num_t par_num,
                                                     const par_type_t val)
{
    (void)par_num;
    (void)val;
    g_reentrant_hits++;
    return (ePAR_OK == par_set_u16(ePAR_TEST_U16, 555U));
}

/**
 * @brief Initialize the parameter module for one test case.
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

#include "core_runtime/par_core_runtime_basic_cases.inc"
#include "core_runtime/par_core_runtime_scalar_policy_cases.inc"
#include "core_runtime/par_core_runtime_callback_cases.inc"

/** @brief Entrypoint for scalar host runtime tests. */
int main(void)
{
    static const par_host_test_case_t cases[] = {
        { "core_init_deinit_and_use_before_init", test_core_init_deinit_and_use_before_init },
        { "scalar_set_get_all_widths", test_scalar_set_get_all_widths },
        { "scalar_error_policy", test_scalar_error_policy },
        { "scalar_generic_and_by_id_paths", test_scalar_generic_and_by_id_paths },
        { "scalar_validation_and_fast_policy", test_scalar_validation_and_fast_policy },
        { "scalar_change_callback_called_once", test_scalar_change_callback_called_once },
        { "scalar_change_callback_reentrant_updates_other_parameter", test_scalar_change_callback_reentrant_updates_other_parameter },
        { "core_cfg_table_api_returns_stable_table_and_size", test_core_cfg_table_api_returns_stable_table_and_size },
        { "scalar_defaults_and_set_by_id_public_paths", test_scalar_defaults_and_set_by_id_public_paths },
        { "scalar_fast_setters_cover_all_widths_and_bitwise", test_scalar_fast_setters_cover_all_widths_and_bitwise },
        { "scalar_fast_range_and_wrong_width_policies", test_scalar_fast_range_and_wrong_width_policies },
        { "scalar_callback_registration_edge_policies", test_scalar_callback_registration_edge_policies },
        { "scalar_validation_reentrant_updates_other_parameter", test_scalar_validation_reentrant_updates_other_parameter },
        { "scalar_callback_registers_new_callback_for_next_dispatch", test_scalar_callback_registers_new_callback_for_next_dispatch },
        { "scalar_validation_rejects_without_changed_state", test_scalar_validation_rejects_without_changed_state },
        { "core_manual_mutex_public_paths", test_core_manual_mutex_public_paths },
        { "scalar_metadata_and_role_policy_helpers", test_scalar_metadata_and_role_policy_helpers },
        { "scalar_reset_default_and_has_changed", test_scalar_reset_default_and_has_changed },
    };

    return par_host_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
}
