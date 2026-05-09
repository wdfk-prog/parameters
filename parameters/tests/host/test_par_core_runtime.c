/**
 * @file test_par_core_runtime.c
 * @brief Exercise scalar parameter runtime behavior on a host build.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include "test_host_common.h"
#include "par_registration_api.h"

/** @brief Callback hit counter used by scalar on-change tests. */
static unsigned g_on_change_hits;
/** @brief Last parameter number observed by the scalar on-change callback. */
static par_num_t g_on_change_last_par;
/** @brief Validation callback acceptance switch. */
static bool g_validation_accept = true;
/** @brief Callback hit counter used by reentrant callback tests. */
static unsigned g_reentrant_hits;

/**
 * @brief Reset callback state shared by scalar callback test cases.
 */
static void reset_callback_state(void)
{
    g_on_change_hits = 0U;
    g_on_change_last_par = ePAR_NUM_OF;
    g_validation_accept = true;
    g_reentrant_hits = 0U;
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

/** @brief Verify lifecycle calls and use-before-init errors. */
static bool test_core_init_deinit_and_use_before_init(void)
{
    uint8_t value = 0U;

    if (par_is_init())
    {
        TEST_ASSERT_OK(par_deinit());
    }

    TEST_ASSERT_STATUS(par_get_u8(ePAR_TEST_MODE, &value), ePAR_ERROR_INIT);
    TEST_ASSERT_OK(par_init());
    TEST_ASSERT(par_is_init());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 1U);
    TEST_ASSERT_OK(par_deinit());
    TEST_ASSERT(!par_is_init());
    return true;
}

/** @brief Verify typed scalar setter/getter behavior for all scalar widths. */
static bool test_scalar_set_get_all_widths(void)
{
    uint8_t u8 = 0U;
    int8_t i8 = 0;
    uint16_t u16 = 0U;
    int16_t i16 = 0;
    uint32_t u32 = 0U;
    int32_t i32 = 0;
    float32_t f32 = 0.0f;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 7U));
    TEST_ASSERT_OK(par_set_i8(ePAR_TEST_I8, -7));
    TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, 321U));
    TEST_ASSERT_OK(par_set_i16(ePAR_TEST_I16, -21));
    TEST_ASSERT_OK(par_set_u32(ePAR_TEST_U32, 12345UL));
    TEST_ASSERT_OK(par_set_i32(ePAR_TEST_I32, -12345L));
    TEST_ASSERT_OK(par_set_f32(ePAR_TEST_F32, 2.5f));

    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT_OK(par_get_i8(ePAR_TEST_I8, &i8));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT_OK(par_get_i16(ePAR_TEST_I16, &i16));
    TEST_ASSERT_OK(par_get_u32(ePAR_TEST_U32, &u32));
    TEST_ASSERT_OK(par_get_i32(ePAR_TEST_I32, &i32));
    TEST_ASSERT_OK(par_get_f32(ePAR_TEST_F32, &f32));

    TEST_ASSERT(u8 == 7U);
    TEST_ASSERT(i8 == -7);
    TEST_ASSERT(u16 == 321U);
    TEST_ASSERT(i16 == -21);
    TEST_ASSERT(u32 == 12345UL);
    TEST_ASSERT(i32 == -12345L);
    TEST_ASSERT(f32 == 2.5f);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify range, access, and invalid-argument error policies. */
static bool test_scalar_error_policy(void)
{
    uint8_t value = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_STATUS(par_set_u8(ePAR_TEST_MODE, 11U), ePAR_WAR_LIMITED);
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 10U);
    TEST_ASSERT_STATUS(par_set_u8(ePAR_TEST_RO, 3U), ePAR_ERROR_ACCESS);
    TEST_ASSERT_STATUS(par_get_u8(ePAR_NUM_OF, &value), ePAR_ERROR_PAR_NUM);
    TEST_ASSERT_STATUS(par_get_u8(ePAR_TEST_MODE, NULL), ePAR_ERROR_PARAM);
    TEST_ASSERT_STATUS(par_set_str(ePAR_TEST_MODE, "x"), ePAR_ERROR_TYPE);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify generic and ID-based scalar APIs. */
static bool test_scalar_generic_and_by_id_paths(void)
{
    par_type_t value = { 0 };
    par_num_t par_num = ePAR_NUM_OF;
    uint16_t id = 0U;

    TEST_ASSERT(init_module());
    value.u16 = 444U;
    TEST_ASSERT_OK(par_set_scalar(ePAR_TEST_U16, &value));
    value.u16 = 0U;
    TEST_ASSERT_OK(par_get_scalar_by_id(3U, &value));
    TEST_ASSERT(value.u16 == 444U);
    TEST_ASSERT_OK(par_get_num_by_id(3U, &par_num));
    TEST_ASSERT(par_num == ePAR_TEST_U16);
    TEST_ASSERT_OK(par_get_id_by_num(ePAR_TEST_U16, &id));
    TEST_ASSERT(id == 3U);
    TEST_ASSERT_STATUS(par_get_num_by_id(0xFFFFU, &par_num), ePAR_ERROR);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify validation callback and fast-setter bypass behavior. */
static bool test_scalar_validation_and_fast_policy(void)
{
    uint8_t value = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8_fast(ePAR_TEST_MODE, 1U));
    reset_callback_state();
    par_register_validation(ePAR_TEST_MODE, scalar_validation);
    g_validation_accept = false;
    TEST_ASSERT_STATUS(par_set_u8(ePAR_TEST_MODE, 3U), ePAR_ERROR_VALUE);
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 1U);
    TEST_ASSERT_OK(par_set_u8_fast(ePAR_TEST_MODE, 3U));
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 3U);
    par_register_validation(ePAR_TEST_MODE, NULL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify scalar on-change callback only fires for real changes. */
static bool test_scalar_change_callback_called_once(void)
{
    TEST_ASSERT(init_module());
    reset_callback_state();
    par_register_on_change_cb(ePAR_TEST_MODE, on_scalar_change);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 2U));
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 2U));
    TEST_ASSERT(g_on_change_hits == 1U);
    TEST_ASSERT(g_on_change_last_par == ePAR_TEST_MODE);
    par_register_on_change_cb(ePAR_TEST_MODE, NULL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}


/** @brief Verify current callback policy permits updating another scalar. */
static bool test_scalar_change_callback_reentrant_updates_other_parameter(void)
{
    uint16_t value16 = 0U;

    TEST_ASSERT(init_module());
    reset_callback_state();
    TEST_ASSERT_OK(par_set_u8_fast(ePAR_TEST_MODE, 1U));
    par_register_on_change_cb(ePAR_TEST_MODE, on_scalar_change_reentrant_update);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 2U));
    TEST_ASSERT(g_reentrant_hits == 1U);
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &value16));
    TEST_ASSERT(value16 == 333U);
    par_register_on_change_cb(ePAR_TEST_MODE, NULL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify metadata and role-policy helper APIs without module init. */
static bool test_scalar_metadata_and_role_policy_helpers(void)
{
    const par_cfg_t *cfg = NULL;
    par_range_t range = { 0 };

    if (par_is_init())
    {
        TEST_ASSERT_OK(par_deinit());
    }

    cfg = par_get_config(ePAR_TEST_MODE);
    TEST_ASSERT(NULL != cfg);
    TEST_ASSERT(NULL == par_get_config(ePAR_NUM_OF));
    TEST_ASSERT(strcmp(par_get_name(ePAR_TEST_MODE), "Mode") == 0);
    TEST_ASSERT(NULL == par_get_name(ePAR_NUM_OF));
    TEST_ASSERT(strcmp(par_get_unit(ePAR_TEST_F32), "V") == 0);
    TEST_ASSERT(NULL == par_get_unit(ePAR_NUM_OF));
    TEST_ASSERT(strcmp(par_get_desc(ePAR_TEST_MODE), "Persistent U8 test value") == 0);
    TEST_ASSERT(NULL == par_get_desc(ePAR_NUM_OF));
    TEST_ASSERT(par_get_type(ePAR_TEST_U16) == ePAR_TYPE_U16);
    TEST_ASSERT(par_get_type(ePAR_NUM_OF) == ePAR_TYPE_NUM_OF);

    range = par_get_range(ePAR_TEST_MODE);
    TEST_ASSERT(range.min.u8 == 0U);
    TEST_ASSERT(range.max.u8 == 10U);
    range = par_get_range(ePAR_TEST_STR);
    TEST_ASSERT(range.min.u32 == 0U && range.max.u32 == 0U);

    TEST_ASSERT(par_get_access(ePAR_TEST_RO) == ePAR_ACCESS_RO);
    TEST_ASSERT(par_get_access(ePAR_NUM_OF) == ePAR_ACCESS_NONE);
    TEST_ASSERT(par_get_read_roles(ePAR_TEST_RO) == ePAR_ROLE_ALL);
    TEST_ASSERT(par_get_write_roles(ePAR_TEST_RO) == ePAR_ROLE_NONE);
    TEST_ASSERT(par_can_read(ePAR_TEST_RO, ePAR_ROLE_PUBLIC));
    TEST_ASSERT(!par_can_write(ePAR_TEST_RO, ePAR_ROLE_PUBLIC));
    TEST_ASSERT(par_can_write(ePAR_TEST_MODE, ePAR_ROLE_DEVELOPER));
    TEST_ASSERT(!par_can_read(ePAR_NUM_OF, ePAR_ROLE_PUBLIC));
    TEST_ASSERT(!par_can_read(ePAR_TEST_MODE, (par_role_t)(1UL << 8U)));
    TEST_ASSERT(!par_can_write(ePAR_TEST_MODE, ePAR_ROLE_NONE));
    return true;
}

/** @brief Verify default reset and has-changed tracking. */
static bool test_scalar_reset_default_and_has_changed(void)
{
    bool changed = false;
    uint8_t value = 0U;
    uint16_t value16 = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_to_default(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_to_default(ePAR_TEST_U16));
    TEST_ASSERT_OK(par_deinit());
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_has_changed(ePAR_TEST_MODE, &changed));
    TEST_ASSERT(!changed);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    TEST_ASSERT_OK(par_has_changed(ePAR_TEST_MODE, &changed));
    TEST_ASSERT(changed);
    TEST_ASSERT_OK(par_set_to_default(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 1U);
    TEST_ASSERT_OK(par_has_changed(ePAR_TEST_MODE, &changed));
    TEST_ASSERT(!changed);
    TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, 222U));
    TEST_ASSERT_OK(par_reset_all_to_default_raw());
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &value16));
    TEST_ASSERT(value16 == 100U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

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
        { "scalar_metadata_and_role_policy_helpers", test_scalar_metadata_and_role_policy_helpers },
        { "scalar_reset_default_and_has_changed", test_scalar_reset_default_and_has_changed },
    };

    return par_host_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
}
