/** @brief Verify direct configuration-table accessors. */
static bool test_core_cfg_table_api_returns_stable_table_and_size(void)
{
    const par_cfg_t *table = NULL;
    const par_cfg_t *entry = NULL;

    if (par_is_init())
    {
        TEST_ASSERT_OK(par_deinit());
    }

    table = par_cfg_get_table();
    entry = par_cfg_get(ePAR_TEST_MODE);
    TEST_ASSERT(NULL != table);
    TEST_ASSERT(NULL != entry);
    TEST_ASSERT(entry == &table[ePAR_TEST_MODE]);
    TEST_ASSERT(par_cfg_get_table_size() == (uint32_t)(sizeof(par_cfg_t) * (size_t)ePAR_NUM_OF));
    return true;
}

/** @brief Verify scalar defaults and the generic set-by-ID public path. */
static bool test_scalar_defaults_and_set_by_id_public_paths(void)
{
    par_type_t def_value = { 0 };
    uint16_t value16 = 0U;
    int16_t def_i16 = 0;
    float32_t def_f32 = 0.0f;

    TEST_ASSERT_STATUS(par_get_scalar_default(ePAR_NUM_OF, &def_value), ePAR_ERROR_PAR_NUM);
    TEST_ASSERT_STATUS(par_get_scalar_default(ePAR_TEST_STR, &def_value), ePAR_ERROR_TYPE);
    TEST_ASSERT_OK(par_get_scalar_default(ePAR_TEST_U16, &def_value));
    TEST_ASSERT(def_value.u16 == 100U);
    TEST_ASSERT_OK(par_get_scalar_default(ePAR_TEST_I16, &def_i16));
    TEST_ASSERT(def_i16 == -10);
    TEST_ASSERT_OK(par_get_scalar_default(ePAR_TEST_F32, &def_f32));
    TEST_ASSERT(def_f32 == 1.5f);

    TEST_ASSERT(init_module());
    value16 = 654U;
    TEST_ASSERT_OK(par_set_scalar_by_id(3U, &value16));
    value16 = 0U;
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &value16));
    TEST_ASSERT(value16 == 654U);
    value16 = 1U;
    TEST_ASSERT_STATUS(par_set_scalar_by_id(9U, &value16), ePAR_ERROR_TYPE);
    TEST_ASSERT_STATUS(par_set_scalar_by_id(0xFFFFU, &value16), ePAR_ERROR);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify fast scalar setters and fast bitwise update helpers. */
static bool test_scalar_fast_setters_cover_all_widths_and_bitwise(void)
{
    uint8_t u8 = 0U;
    int8_t i8 = 0;
    uint16_t u16 = 0U;
    int16_t i16 = 0;
    uint32_t u32 = 0U;
    int32_t i32 = 0;
    float32_t f32 = 0.0f;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8_fast(ePAR_TEST_MODE, 8U));
    TEST_ASSERT_OK(par_set_i8_fast(ePAR_TEST_I8, -8));
    TEST_ASSERT_OK(par_set_u16_fast(ePAR_TEST_U16, 888U));
    TEST_ASSERT_OK(par_set_i16_fast(ePAR_TEST_I16, -88));
    TEST_ASSERT_OK(par_set_u32_fast(ePAR_TEST_U32, 88888UL));
    TEST_ASSERT_OK(par_set_i32_fast(ePAR_TEST_I32, -88888L));
    TEST_ASSERT_OK(par_set_f32_fast(ePAR_TEST_F32, 8.5f));
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT_OK(par_get_i8(ePAR_TEST_I8, &i8));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT_OK(par_get_i16(ePAR_TEST_I16, &i16));
    TEST_ASSERT_OK(par_get_u32(ePAR_TEST_U32, &u32));
    TEST_ASSERT_OK(par_get_i32(ePAR_TEST_I32, &i32));
    TEST_ASSERT_OK(par_get_f32(ePAR_TEST_F32, &f32));
    TEST_ASSERT(u8 == 8U);
    TEST_ASSERT(i8 == -8);
    TEST_ASSERT(u16 == 888U);
    TEST_ASSERT(i16 == -88);
    TEST_ASSERT(u32 == 88888UL);
    TEST_ASSERT(i32 == -88888L);
    TEST_ASSERT(f32 == 8.5f);

    TEST_ASSERT_OK(par_set_u8_fast(ePAR_TEST_MODE, 0x0AU));
    TEST_ASSERT_OK(par_bitand_set_u8_fast(ePAR_TEST_MODE, 0x06U));
    TEST_ASSERT_OK(par_bitor_set_u8_fast(ePAR_TEST_MODE, 0x05U));
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 0x07U);
    TEST_ASSERT_OK(par_set_u16_fast(ePAR_TEST_U16, 0x03C0U));
    TEST_ASSERT_OK(par_bitand_set_u16_fast(ePAR_TEST_U16, 0x00F0U));
    TEST_ASSERT_OK(par_bitor_set_u16_fast(ePAR_TEST_U16, 0x0003U));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT(u16 == 0x00C3U);
    TEST_ASSERT_OK(par_set_u32_fast(ePAR_TEST_U32, 0x0000F0F0UL));
    TEST_ASSERT_OK(par_bitand_set_u32_fast(ePAR_TEST_U32, 0x000000FFUL));
    TEST_ASSERT_OK(par_bitor_set_u32_fast(ePAR_TEST_U32, 0x00000100UL));
    TEST_ASSERT_OK(par_get_u32(ePAR_TEST_U32, &u32));
    TEST_ASSERT(u32 == 0x000001F0UL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}


/** @brief Verify fast setters clamp range and reject generic wrong-type paths. */
static bool test_scalar_fast_range_and_wrong_width_policies(void)
{
    uint8_t u8 = 0U;
    par_type_t scalar_value = { 0 };

    TEST_ASSERT(init_module());
    TEST_ASSERT_STATUS(par_set_u8_fast(ePAR_TEST_MODE, 99U), ePAR_WAR_LIMITED);
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 10U);

    scalar_value.u8 = 7U;
    TEST_ASSERT_STATUS(par_set_scalar(ePAR_TEST_STR, &scalar_value), ePAR_ERROR_TYPE);

    TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, 123U));
    reset_callback_state();
    par_register_on_change_cb(ePAR_TEST_U16, on_scalar_change);
    TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, 123U));
    TEST_ASSERT(g_on_change_hits == 0U);
    par_register_on_change_cb(ePAR_TEST_U16, NULL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify callback unregister, invalid registration, and reentry policies. */
static bool test_scalar_callback_registration_edge_policies(void)
{
    uint8_t value = 0U;

    TEST_ASSERT(init_module());
    reset_callback_state();
    par_register_on_change_cb(ePAR_TEST_MODE, on_scalar_change);
    par_register_on_change_cb(ePAR_TEST_MODE, NULL);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 2U));
    TEST_ASSERT(g_on_change_hits == 0U);

    reset_callback_state();
    par_register_on_change_cb(ePAR_NUM_OF, on_scalar_change);
    par_register_validation(ePAR_NUM_OF, scalar_validation);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 3U));
    TEST_ASSERT(g_on_change_hits == 0U);

    reset_callback_state();
    par_register_on_change_cb(ePAR_TEST_MODE, on_scalar_change_same_parameter_reentry);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT(g_reentrant_hits == 1U);
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 4U);
    par_register_on_change_cb(ePAR_TEST_MODE, NULL);

    reset_callback_state();
    par_register_validation(ePAR_TEST_U16, scalar_validation_reads_current_value);
    TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, 444U));
    TEST_ASSERT(g_validation_read_ok);
    par_register_validation(ePAR_TEST_U16, NULL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify current validation policy permits updating another scalar. */
static bool test_scalar_validation_reentrant_updates_other_parameter(void)
{
    uint16_t value16 = 0U;

    TEST_ASSERT(init_module());
    reset_callback_state();
    par_register_validation(ePAR_TEST_MODE, scalar_validation_reentrant_update_other);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 6U));
    TEST_ASSERT(g_reentrant_hits == 1U);
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &value16));
    TEST_ASSERT(value16 == 555U);
    par_register_validation(ePAR_TEST_MODE, NULL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}
