/** @brief Verify string default, empty, full-capacity, and overflow paths. */
static bool test_object_str_boundaries(void)
{
    char buf[9] = { 0 };
    uint16_t len = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, buf, sizeof(buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(buf, "ap") == 0);
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, ""));
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, buf, sizeof(buf), &len));
    TEST_ASSERT(len == 0U);
    TEST_ASSERT(strcmp(buf, "") == 0);
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "12345678"));
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, buf, sizeof(buf), &len));
    TEST_ASSERT(len == 8U);
    TEST_ASSERT(strcmp(buf, "12345678") == 0);
    TEST_ASSERT_STATUS(par_set_str(ePAR_TEST_STR, "123456789"), ePAR_ERROR_VALUE);
    TEST_ASSERT_STATUS(par_get_str(ePAR_TEST_STR, buf, 4U, &len), ePAR_ERROR_PARAM);
    TEST_ASSERT(len == 8U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify raw byte object default, exact capacity, and overflow paths. */
static bool test_object_bytes_boundaries(void)
{
    uint8_t buf[4] = { 0U };
    uint16_t len = 0U;
    const uint8_t payload[4] = { 4U, 3U, 2U, 1U };
    const uint8_t overflow[5] = { 1U, 2U, 3U, 4U, 5U };

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_bytes(ePAR_TEST_BYTES, buf, sizeof(buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(buf[0] == 1U && buf[1] == 2U);
    TEST_ASSERT_OK(par_set_bytes(ePAR_TEST_BYTES, payload, (uint16_t)sizeof(payload)));
    memset(buf, 0, sizeof(buf));
    TEST_ASSERT_OK(par_get_bytes(ePAR_TEST_BYTES, buf, sizeof(buf), &len));
    TEST_ASSERT(len == 4U);
    TEST_ASSERT(memcmp(buf, payload, sizeof(payload)) == 0);
    TEST_ASSERT_STATUS(par_set_bytes(ePAR_TEST_BYTES, overflow, (uint16_t)sizeof(overflow)), ePAR_ERROR_VALUE);
    TEST_ASSERT_STATUS(par_get_bytes(ePAR_TEST_BYTES, buf, 2U, &len), ePAR_ERROR_PARAM);
    TEST_ASSERT(len == 4U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify ARR_U8 setter/getter and default payload behavior. */
static bool test_object_arr_u8_roundtrip(void)
{
    uint8_t buf[3] = { 0U };
    uint16_t count = 0U;
    const uint8_t payload[3] = { 7U, 8U, 9U };

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_default_arr_u8(ePAR_TEST_ARR_U8, buf, 3U, &count));
    TEST_ASSERT(count == 3U);
    TEST_ASSERT(buf[0] == 1U && buf[1] == 2U && buf[2] == 3U);
    TEST_ASSERT_OK(par_set_arr_u8(ePAR_TEST_ARR_U8, payload, 3U));
    TEST_ASSERT_OK(par_get_arr_u8(ePAR_TEST_ARR_U8, buf, 3U, &count));
    TEST_ASSERT(count == 3U);
    TEST_ASSERT(memcmp(buf, payload, sizeof(payload)) == 0);
    TEST_ASSERT_STATUS(par_set_arr_u8(ePAR_TEST_ARR_U8, payload, 2U), ePAR_ERROR_VALUE);
    TEST_ASSERT_STATUS(par_get_arr_u8(ePAR_TEST_ARR_U8, buf, 2U, &count), ePAR_ERROR_PARAM);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify ARR_U16 count-to-byte conversion behavior. */
static bool test_object_arr_u16_count_to_byte_length(void)
{
    uint16_t buf[2] = { 0U };
    uint16_t count = 0U;
    const uint16_t payload[2] = { 300U, 400U };

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_default_arr_u16(ePAR_TEST_ARR_U16, buf, 2U, &count));
    TEST_ASSERT(count == 2U);
    TEST_ASSERT(buf[0] == 100U && buf[1] == 200U);
    TEST_ASSERT_OK(par_set_arr_u16(ePAR_TEST_ARR_U16, payload, 2U));
    TEST_ASSERT_OK(par_get_arr_u16(ePAR_TEST_ARR_U16, buf, 2U, &count));
    TEST_ASSERT(count == 2U);
    TEST_ASSERT(buf[0] == 300U && buf[1] == 400U);
    TEST_ASSERT_STATUS(par_set_arr_u16(ePAR_TEST_ARR_U16, payload, 1U), ePAR_ERROR_VALUE);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify ARR_U32 count-to-byte conversion behavior. */
static bool test_object_arr_u32_count_to_byte_length(void)
{
    uint32_t buf[2] = { 0U };
    uint16_t count = 0U;
    const uint32_t payload[2] = { 3000UL, 4000UL };

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_default_arr_u32(ePAR_TEST_ARR_U32, buf, 2U, &count));
    TEST_ASSERT(count == 2U);
    TEST_ASSERT(buf[0] == 1000UL && buf[1] == 2000UL);
    TEST_ASSERT_OK(par_set_arr_u32(ePAR_TEST_ARR_U32, payload, 2U));
    TEST_ASSERT_OK(par_get_arr_u32(ePAR_TEST_ARR_U32, buf, 2U, &count));
    TEST_ASSERT(count == 2U);
    TEST_ASSERT(buf[0] == 3000UL && buf[1] == 4000UL);
    TEST_ASSERT_STATUS(par_set_arr_u32(ePAR_TEST_ARR_U32, payload, 1U), ePAR_ERROR_VALUE);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object ID wrappers and capacity metadata. */
static bool test_object_by_id_and_capacity(void)
{
    char str_buf[9] = { 0 };
    uint16_t len = 0U;
    uint16_t capacity = 0U;
    uint16_t str_id = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_id_by_num(ePAR_TEST_STR, &str_id));
    TEST_ASSERT_OK(par_set_str_by_id(str_id, "idpath"));
    TEST_ASSERT_OK(par_get_str_by_id(str_id, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 6U);
    TEST_ASSERT(strcmp(str_buf, "idpath") == 0);
    TEST_ASSERT_OK(par_get_obj_capacity_by_id(str_id, &capacity));
    TEST_ASSERT(capacity == 8U);
    TEST_ASSERT_OK(par_get_obj_len_by_id(str_id, &len));
    TEST_ASSERT(len == 6U);
    TEST_ASSERT_STATUS(par_get_str_by_id(0xFFFFU, str_buf, sizeof(str_buf), &len), ePAR_ERROR);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object byte and array ID wrappers plus default-by-ID paths. */
static bool test_object_bytes_and_arrays_by_id_wrappers(void)
{
    uint8_t bytes_buf[4] = { 0U };
    uint8_t arr_u8_buf[3] = { 0U };
    uint16_t arr_u16_buf[2] = { 0U };
    uint32_t arr_u32_buf[2] = { 0U };
    uint16_t out_count = 0U;
    uint16_t bytes_id = 0U;
    uint16_t arr_u8_id = 0U;
    uint16_t arr_u16_id = 0U;
    uint16_t arr_u32_id = 0U;
    const uint8_t bytes_payload[4] = { 5U, 6U, 7U, 8U };
    const uint8_t arr_u8_payload[3] = { 9U, 8U, 7U };
    const uint16_t arr_u16_payload[2] = { 500U, 600U };
    const uint32_t arr_u32_payload[2] = { 5000UL, 6000UL };

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_id_by_num(ePAR_TEST_BYTES, &bytes_id));
    TEST_ASSERT_OK(par_get_id_by_num(ePAR_TEST_ARR_U8, &arr_u8_id));
    TEST_ASSERT_OK(par_get_id_by_num(ePAR_TEST_ARR_U16, &arr_u16_id));
    TEST_ASSERT_OK(par_get_id_by_num(ePAR_TEST_ARR_U32, &arr_u32_id));
    TEST_ASSERT_OK(par_set_bytes_by_id(bytes_id, bytes_payload, (uint16_t)sizeof(bytes_payload)));
    TEST_ASSERT_OK(par_get_bytes_by_id(bytes_id, bytes_buf, sizeof(bytes_buf), &out_count));
    TEST_ASSERT(out_count == (uint16_t)sizeof(bytes_payload));
    TEST_ASSERT(0 == memcmp(bytes_buf, bytes_payload, sizeof(bytes_payload)));
    memset(bytes_buf, 0, sizeof(bytes_buf));
    TEST_ASSERT_OK(par_get_default_bytes_by_id(bytes_id, bytes_buf, sizeof(bytes_buf), &out_count));
    TEST_ASSERT(out_count == 2U);
    TEST_ASSERT(bytes_buf[0] == 1U && bytes_buf[1] == 2U);

    TEST_ASSERT_OK(par_set_arr_u8_by_id(arr_u8_id, arr_u8_payload, 3U));
    TEST_ASSERT_OK(par_get_arr_u8_by_id(arr_u8_id, arr_u8_buf, 3U, &out_count));
    TEST_ASSERT(out_count == 3U);
    TEST_ASSERT(0 == memcmp(arr_u8_buf, arr_u8_payload, sizeof(arr_u8_payload)));
    memset(arr_u8_buf, 0, sizeof(arr_u8_buf));
    TEST_ASSERT_OK(par_get_default_arr_u8_by_id(arr_u8_id, arr_u8_buf, 3U, &out_count));
    TEST_ASSERT(out_count == 3U);
    TEST_ASSERT(arr_u8_buf[0] == 1U && arr_u8_buf[1] == 2U && arr_u8_buf[2] == 3U);

    TEST_ASSERT_OK(par_set_arr_u16_by_id(arr_u16_id, arr_u16_payload, 2U));
    TEST_ASSERT_OK(par_get_arr_u16_by_id(arr_u16_id, arr_u16_buf, 2U, &out_count));
    TEST_ASSERT(out_count == 2U);
    TEST_ASSERT(arr_u16_buf[0] == 500U && arr_u16_buf[1] == 600U);
    memset(arr_u16_buf, 0, sizeof(arr_u16_buf));
    TEST_ASSERT_OK(par_get_default_arr_u16_by_id(arr_u16_id, arr_u16_buf, 2U, &out_count));
    TEST_ASSERT(out_count == 2U);
    TEST_ASSERT(arr_u16_buf[0] == 100U && arr_u16_buf[1] == 200U);

    TEST_ASSERT_OK(par_set_arr_u32_by_id(arr_u32_id, arr_u32_payload, 2U));
    TEST_ASSERT_OK(par_get_arr_u32_by_id(arr_u32_id, arr_u32_buf, 2U, &out_count));
    TEST_ASSERT(out_count == 2U);
    TEST_ASSERT(arr_u32_buf[0] == 5000UL && arr_u32_buf[1] == 6000UL);
    memset(arr_u32_buf, 0, sizeof(arr_u32_buf));
    TEST_ASSERT_OK(par_get_default_arr_u32_by_id(arr_u32_id, arr_u32_buf, 2U, &out_count));
    TEST_ASSERT(out_count == 2U);
    TEST_ASSERT(arr_u32_buf[0] == 1000UL && arr_u32_buf[1] == 2000UL);

    TEST_ASSERT_STATUS(par_set_bytes_by_id(0xFFFFU, bytes_payload, 1U), ePAR_ERROR);
    TEST_ASSERT_STATUS(par_get_arr_u8_by_id(0xFFFFU, arr_u8_buf, 3U, &out_count), ePAR_ERROR);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object default string and metadata helper ID wrappers. */
static bool test_object_default_str_and_metadata_by_id_wrappers(void)
{
    char str_buf[9] = { 0 };
    uint16_t out_len = 0U;
    uint16_t capacity = 0U;
    uint16_t str_id = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_id_by_num(ePAR_TEST_STR, &str_id));
    TEST_ASSERT_OK(par_set_str_by_id(str_id, "runtime"));
    TEST_ASSERT_OK(par_get_default_str_by_id(str_id, str_buf, sizeof(str_buf), &out_len));
    TEST_ASSERT(out_len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_get_obj_len_by_id(str_id, &out_len));
    TEST_ASSERT(out_len == 7U);
    TEST_ASSERT_OK(par_get_obj_capacity_by_id(str_id, &capacity));
    TEST_ASSERT(capacity == 8U);
    TEST_ASSERT_STATUS(par_get_obj_len_by_id(0xFFFFU, &out_len), ePAR_ERROR);
    TEST_ASSERT_STATUS(par_get_obj_capacity_by_id(0xFFFFU, &capacity), ePAR_ERROR);
    TEST_ASSERT_OK(par_deinit());
    return true;
}


/** @brief Verify non-ID object default APIs match by-ID wrappers and metadata. */
static bool test_object_default_non_id_apis_match_by_id_apis(void)
{
    uint8_t bytes_buf[4] = { 0U };
    char str_buf[9] = { 0 };
    uint16_t out_len = 0U;
    uint16_t capacity = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_default_bytes(ePAR_TEST_BYTES, bytes_buf, sizeof(bytes_buf), &out_len));
    TEST_ASSERT(out_len == 2U);
    TEST_ASSERT(bytes_buf[0] == 1U && bytes_buf[1] == 2U);
    TEST_ASSERT_OK(par_get_default_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &out_len));
    TEST_ASSERT(out_len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "length"));
    TEST_ASSERT_OK(par_get_obj_len(ePAR_TEST_STR, &out_len));
    TEST_ASSERT(out_len == 6U);
    TEST_ASSERT_OK(par_get_obj_capacity(ePAR_TEST_STR, &capacity));
    TEST_ASSERT(capacity == 8U);
    TEST_ASSERT_STATUS(par_get_obj_len(ePAR_TEST_U16, &out_len), ePAR_ERROR_TYPE);
    TEST_ASSERT_STATUS(par_get_obj_capacity(ePAR_TEST_U16, &capacity), ePAR_ERROR_TYPE);
    TEST_ASSERT_OK(par_deinit());
    return true;
}
