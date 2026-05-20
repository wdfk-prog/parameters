/** @brief Verify callback registration changes affect only later dispatches. */
static bool test_scalar_callback_registers_new_callback_for_next_dispatch(void)
{
    TEST_ASSERT(init_module());
    reset_callback_state();
    par_register_on_change_cb(ePAR_TEST_MODE, on_scalar_change_register_new_callback);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 2U));
    TEST_ASSERT(g_callback_register_hits == 1U);
    TEST_ASSERT(g_on_change_hits == 0U);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 3U));
    TEST_ASSERT(g_callback_register_hits == 1U);
    TEST_ASSERT(g_on_change_hits == 1U);
    TEST_ASSERT(g_on_change_last_par == ePAR_TEST_MODE);
    par_register_on_change_cb(ePAR_TEST_MODE, NULL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify rejected scalar validation leaves value and changed state untouched. */
static bool test_scalar_validation_rejects_without_changed_state(void)
{
    bool changed = true;
    uint8_t value = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_to_default(ePAR_TEST_MODE));
    reset_callback_state();
    par_register_validation(ePAR_TEST_MODE, scalar_validation);
    par_register_on_change_cb(ePAR_TEST_MODE, on_scalar_change);
    g_validation_accept = false;
    TEST_ASSERT_STATUS(par_set_u8(ePAR_TEST_MODE, 4U), ePAR_ERROR_VALUE);
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 1U);
    TEST_ASSERT_OK(par_has_changed(ePAR_TEST_MODE, &changed));
    TEST_ASSERT(!changed);
    TEST_ASSERT(g_on_change_hits == 0U);
    g_validation_accept = true;
    par_register_validation(ePAR_TEST_MODE, NULL);
    par_register_on_change_cb(ePAR_TEST_MODE, NULL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify manual mutex API wrappers remain balanced and callable. */
static bool test_core_manual_mutex_public_paths(void)
{
    TEST_ASSERT_OK(par_acquire_mutex(ePAR_TEST_MODE));
    par_release_mutex(ePAR_TEST_MODE);
    TEST_ASSERT_OK(par_acquire_mutex(ePAR_NUM_OF));
    par_release_mutex(ePAR_NUM_OF);
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
