/** @brief Verify NULL, zero-length, and small-buffer object edge policies. */
static bool test_object_null_zero_and_small_buffer_policies(void)
{
    uint8_t bytes_buf[4] = { 0U };
    uint16_t arr16_buf[2] = { 0U };
    uint32_t arr32_buf[2] = { 0U };
    char str_buf[9] = { 0 };
    uint16_t out_count = 0U;
    uint16_t out_len = 0U;
    const uint8_t bytes_payload[2] = { 0x11U, 0x22U };

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, NULL));
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &out_len));
    TEST_ASSERT(out_len == 0U);
    TEST_ASSERT(strcmp(str_buf, "") == 0);

    TEST_ASSERT_OK(par_set_bytes(ePAR_TEST_BYTES, NULL, 0U));
    TEST_ASSERT_OK(par_get_bytes(ePAR_TEST_BYTES, bytes_buf, sizeof(bytes_buf), &out_len));
    TEST_ASSERT(out_len == 0U);
    TEST_ASSERT_STATUS(par_set_bytes(ePAR_TEST_BYTES, NULL, 1U), ePAR_ERROR_PARAM);
    TEST_ASSERT_OK(par_set_bytes(ePAR_TEST_BYTES, bytes_payload, (uint16_t)sizeof(bytes_payload)));
    TEST_ASSERT_STATUS(par_get_arr_u16(ePAR_TEST_ARR_U16, arr16_buf, 1U, &out_count), ePAR_ERROR_PARAM);
    TEST_ASSERT(out_count == 2U);
    out_count = 0xA5A5U;
    TEST_ASSERT_STATUS(par_get_arr_u32(ePAR_TEST_ARR_U32, arr32_buf, 1U, &out_count), ePAR_ERROR_PARAM);
    TEST_ASSERT(out_count == 2U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify scalar/object API cross-type errors leave values unchanged. */
static bool test_object_scalar_api_cross_type_errors_do_not_mutate(void)
{
    uint8_t u8 = 0U;
    char str_buf[9] = { 0 };
    uint16_t out_len = 0U;
    par_type_t scalar_value = { 0 };
    uint8_t payload[1] = { 0x55U };

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "stable"));
    scalar_value.u8 = 6U;
    TEST_ASSERT_STATUS(par_set_scalar(ePAR_TEST_STR, &scalar_value.u8), ePAR_ERROR_TYPE);
    TEST_ASSERT_STATUS(par_get_scalar(ePAR_TEST_STR, &scalar_value), ePAR_ERROR_TYPE);
    TEST_ASSERT_STATUS(par_set_bytes(ePAR_TEST_MODE, payload, (uint16_t)sizeof(payload)), ePAR_ERROR_TYPE);
    TEST_ASSERT_STATUS(par_get_bytes(ePAR_TEST_MODE, payload, (uint16_t)sizeof(payload), &out_len), ePAR_ERROR_TYPE);
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 5U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &out_len));
    TEST_ASSERT(out_len == 6U);
    TEST_ASSERT(strcmp(str_buf, "stable") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object pool overlap input is rejected without mutation. */
static bool test_object_source_overlap_rejected_without_mutation(void)
{
    const uint8_t *p_payload = NULL;
    uint16_t len = 0U;
    uint16_t capacity = 0U;
    uint8_t bytes_buf[4] = { 0U };
    uint16_t out_len = 0U;
    const uint8_t original[4] = { 9U, 8U, 7U, 6U };

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_bytes(ePAR_TEST_BYTES, original, (uint16_t)sizeof(original)));
    TEST_ASSERT_OK(par_object_get_view(ePAR_TEST_BYTES, &p_payload, &len, &capacity));
    TEST_ASSERT(NULL != p_payload);
    TEST_ASSERT(len == (uint16_t)sizeof(original));
    TEST_ASSERT(capacity == (uint16_t)sizeof(bytes_buf));
    TEST_ASSERT_STATUS(par_set_bytes(ePAR_TEST_BYTES, p_payload, len), ePAR_ERROR_PARAM);
    TEST_ASSERT_OK(par_get_bytes(ePAR_TEST_BYTES, bytes_buf, sizeof(bytes_buf), &out_len));
    TEST_ASSERT(out_len == (uint16_t)sizeof(original));
    TEST_ASSERT(0 == memcmp(bytes_buf, original, sizeof(original)));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object validation callback and reset-to-default behavior. */
static bool test_object_validation_and_default_reset(void)
{
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    TEST_ASSERT(init_module());
    g_obj_validation_accept = false;
    par_register_obj_validation(ePAR_TEST_STR, object_validation);
    TEST_ASSERT_STATUS(par_set_str(ePAR_TEST_STR, "reject"), ePAR_ERROR_VALUE);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    g_obj_validation_accept = true;
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "accept"));
    TEST_ASSERT_OK(par_set_to_default(ePAR_TEST_STR));
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    par_register_obj_validation(ePAR_TEST_STR, NULL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}


/** @brief Verify small default-object buffers do not partially overwrite output. */
static bool test_object_get_default_small_buffer_does_not_partial_overwrite(void)
{
    uint16_t arr16_buf[2] = { 0xAAAAU, 0xBBBBU };
    uint16_t out_count = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_STATUS(par_get_default_arr_u16(ePAR_TEST_ARR_U16, arr16_buf, 1U, &out_count),
                       ePAR_ERROR_PARAM);
    TEST_ASSERT(out_count == 2U);
    TEST_ASSERT(arr16_buf[0] == 0xAAAAU);
    TEST_ASSERT(arr16_buf[1] == 0xBBBBU);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify NULL output buffer acts as a length query without copying. */
static bool test_object_get_bytes_null_buffer_reports_len_without_copy(void)
{
    uint16_t out_len = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_bytes(ePAR_TEST_BYTES, NULL, 4U, &out_len));
    TEST_ASSERT(out_len == 2U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object validation sees candidate bytes and rejection is atomic. */
static bool test_object_validation_rejects_without_payload_mutation(void)
{
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "old"));
    memset(g_obj_validation_seen, 0, sizeof(g_obj_validation_seen));
    g_obj_validation_seen_len = 0U;
    g_obj_validation_accept = false;
    par_register_obj_validation(ePAR_TEST_STR, object_validation);
    TEST_ASSERT_STATUS(par_set_str(ePAR_TEST_STR, "new"), ePAR_ERROR_VALUE);
    TEST_ASSERT(g_obj_validation_seen_len == 3U);
    TEST_ASSERT(0 == memcmp(g_obj_validation_seen, "new", 3U));
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 3U);
    TEST_ASSERT(strcmp(str_buf, "old") == 0);
    g_obj_validation_accept = true;
    par_register_obj_validation(ePAR_TEST_STR, NULL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}


/** @brief Verify rejected object validation leaves default changed-state untouched. */
static bool test_object_validation_rejects_without_changed_state(void)
{
    bool changed = true;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_to_default(ePAR_TEST_STR));
    g_obj_validation_accept = false;
    par_register_obj_validation(ePAR_TEST_STR, object_validation);
    TEST_ASSERT_STATUS(par_set_str(ePAR_TEST_STR, "new"), ePAR_ERROR_VALUE);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(0 == strcmp(str_buf, "ap"));
    TEST_ASSERT_OK(par_has_changed(ePAR_TEST_STR, &changed));
    TEST_ASSERT(!changed);
    g_obj_validation_accept = true;
    par_register_obj_validation(ePAR_TEST_STR, NULL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify current object validation policy permits updating another object. */
static bool test_object_validation_reentrant_updates_other_object(void)
{
    uint8_t bytes_buf[4] = { 0U };
    uint16_t out_len = 0U;

    TEST_ASSERT(init_module());
    par_register_obj_validation(ePAR_TEST_STR, object_validation_reentrant_update_bytes);
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "new"));
    TEST_ASSERT_OK(par_get_bytes(ePAR_TEST_BYTES, bytes_buf, sizeof(bytes_buf), &out_len));
    TEST_ASSERT(out_len == (uint16_t)sizeof(bytes_buf));
    TEST_ASSERT(bytes_buf[0] == 7U);
    TEST_ASSERT(bytes_buf[1] == 8U);
    TEST_ASSERT(bytes_buf[2] == 9U);
    TEST_ASSERT(bytes_buf[3] == 10U);
    par_register_obj_validation(ePAR_TEST_STR, NULL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object APIs reject use before initialization without mutating outputs. */
static bool test_object_api_use_before_init_returns_init_error(void)
{
    char str_buf[9] = { 'x', 'x', 'x', 'x', 'x', 'x', 'x', 'x', '\0' };
    uint8_t bytes_buf[4] = { 0xAAU, 0xBBU, 0xCCU, 0xDDU };
    uint16_t arr16_buf[2] = { 0xAAAAU, 0xBBBBU };
    uint16_t out_len = 0x5A5AU;

    if (par_is_init())
    {
        TEST_ASSERT_OK(par_deinit());
    }

    TEST_ASSERT_STATUS(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &out_len),
                       ePAR_ERROR_INIT);
    TEST_ASSERT(0 == memcmp(str_buf, "xxxxxxxx", 8U));
    TEST_ASSERT(out_len == 0x5A5AU);
    TEST_ASSERT_STATUS(par_set_str(ePAR_TEST_STR, "late"), ePAR_ERROR_INIT);
    TEST_ASSERT_STATUS(par_get_bytes(ePAR_TEST_BYTES, bytes_buf, sizeof(bytes_buf), &out_len),
                       ePAR_ERROR_INIT);
    TEST_ASSERT(bytes_buf[0] == 0xAAU);
    TEST_ASSERT_STATUS(par_set_bytes(ePAR_TEST_BYTES, bytes_buf, 2U), ePAR_ERROR_INIT);
    TEST_ASSERT_STATUS(par_get_arr_u16(ePAR_TEST_ARR_U16, arr16_buf, 2U, &out_len),
                       ePAR_ERROR_INIT);
    TEST_ASSERT(arr16_buf[0] == 0xAAAAU);
    TEST_ASSERT_STATUS(par_set_arr_u16(ePAR_TEST_ARR_U16, arr16_buf, 2U),
                       ePAR_ERROR_INIT);
    TEST_ASSERT_STATUS(par_get_obj_len(ePAR_TEST_STR, &out_len), ePAR_ERROR_INIT);
    return true;
}

/** @brief Verify object APIs reject use after deinitialization. */
static bool test_object_api_after_deinit_returns_init_error(void)
{
    char str_buf[9] = { 0 };
    uint16_t out_len = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "live"));
    TEST_ASSERT_OK(par_deinit());
    TEST_ASSERT_STATUS(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &out_len),
                       ePAR_ERROR_INIT);
    TEST_ASSERT_STATUS(par_set_str(ePAR_TEST_STR, "after"), ePAR_ERROR_INIT);
    return true;
}

/** @brief Verify default object metadata APIs remain available before init. */
static bool test_object_default_api_use_before_init_reports_metadata(void)
{
    char str_buf[9] = { 0 };
    uint16_t arr16_buf[2] = { 0U };
    uint16_t out_len = 0U;
    uint16_t capacity = 0U;

    if (par_is_init())
    {
        TEST_ASSERT_OK(par_deinit());
    }

    TEST_ASSERT_OK(par_get_default_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &out_len));
    TEST_ASSERT(out_len == 2U);
    TEST_ASSERT(0 == strcmp(str_buf, "ap"));
    TEST_ASSERT_OK(par_get_default_arr_u16(ePAR_TEST_ARR_U16, arr16_buf, 2U, &out_len));
    TEST_ASSERT(out_len == 2U);
    TEST_ASSERT(arr16_buf[0] == 100U);
    TEST_ASSERT(arr16_buf[1] == 200U);
    TEST_ASSERT_OK(par_get_obj_capacity(ePAR_TEST_STR, &capacity));
    TEST_ASSERT(capacity == 8U);
    return true;
}
