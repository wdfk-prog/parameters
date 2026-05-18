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
