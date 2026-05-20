#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/** @brief Verify current partial-progress policy after the first AT24 page write. */
static bool test_rtt_at24cxx_second_page_write_fail_reports_partial_progress_current_policy(void)
{
    const par_store_backend_api_t *api;
    const uint32_t first_chunk = (uint32_t)AT24CXX_PAGE_BYTE -
                                 ((uint32_t)PAR_CFG_RTT_AT24_BASE_ADDR % (uint32_t)AT24CXX_PAGE_BYTE);
    uint8_t payload[AT24CXX_PAGE_BYTE + 2U];

    TEST_ASSERT(first_chunk > 0U);
    for (uint8_t i = 0U; i < (uint8_t)sizeof(payload); i++)
    {
        payload[i] = (uint8_t)(0x60U + i);
    }

    at24_stub_reset();
    g_at24_available = true;
    TEST_ASSERT_OK(par_store_backend_bind());
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    g_at24_fail_write = 2;
    TEST_ASSERT_STATUS(api->write(0U, (uint32_t)sizeof(payload), payload), ePAR_ERROR_NVM);
    TEST_ASSERT(g_at24_write_count == 1U);
    TEST_ASSERT(0 == memcmp(&g_at24_dev.mem[PAR_CFG_RTT_AT24_BASE_ADDR], payload, first_chunk));
    TEST_ASSERT(g_at24_dev.mem[PAR_CFG_RTT_AT24_BASE_ADDR + first_chunk] == 0xFFU);
    TEST_ASSERT_OK(api->deinit());
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

/** @brief Verify AT24CXX backend calls reject access after deinitialization. */
static bool test_rtt_at24cxx_after_deinit_rejects_io(void)
{
    const par_store_backend_api_t *api;
    uint8_t value = 0x5AU;
    uint8_t readback = 0U;

    at24_stub_reset();
    g_at24_available = true;
    TEST_ASSERT_OK(par_store_backend_bind());
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    TEST_ASSERT_OK(api->deinit());
    TEST_ASSERT_STATUS(api->read(0U, 1U, &readback), ePAR_ERROR_PARAM);
    TEST_ASSERT_STATUS(api->write(0U, 1U, &value), ePAR_ERROR_PARAM);
    TEST_ASSERT_STATUS(api->erase(0U, 1U), ePAR_ERROR_PARAM);
    return true;
}



/** @brief Verify a second-page AT24 read failure leaves the unread tail untouched. */
static bool test_rtt_at24cxx_second_page_read_fail_preserves_tail_buffer(void)
{
    const par_store_backend_api_t *api;
    const uint32_t first_chunk = (uint32_t)AT24CXX_PAGE_BYTE -
                                 ((uint32_t)PAR_CFG_RTT_AT24_BASE_ADDR %
                                  (uint32_t)AT24CXX_PAGE_BYTE);
    uint8_t readback[AT24CXX_PAGE_BYTE + 2U];

    at24_stub_reset();
    g_at24_available = true;
    for (uint8_t i = 0U; i < (uint8_t)sizeof(readback); i++)
    {
        g_at24_dev.mem[PAR_CFG_RTT_AT24_BASE_ADDR + i] = (uint8_t)(0x20U + i);
        readback[i] = 0xA5U;
    }

    TEST_ASSERT_OK(par_store_backend_bind());
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    g_at24_fail_read = 2;
    TEST_ASSERT_STATUS(api->read(0U, (uint32_t)sizeof(readback), readback), ePAR_ERROR_NVM);
    TEST_ASSERT(g_at24_read_count == 1U);
    TEST_ASSERT(0 == memcmp(readback, &g_at24_dev.mem[PAR_CFG_RTT_AT24_BASE_ADDR], first_chunk));
    TEST_ASSERT(readback[first_chunk] == 0xA5U);
    TEST_ASSERT(readback[first_chunk + 1U] == 0xA5U);
    TEST_ASSERT_OK(api->deinit());
    return true;
}

#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/** @brief Verify an erase chunk failure reports partial-progress current policy. */
static bool test_rtt_at24cxx_erase_chunk_fail_reports_partial_progress_current_policy(void)
{
    const par_store_backend_api_t *api;
    const uint32_t first_chunk = (uint32_t)AT24CXX_PAGE_BYTE -
                                 ((uint32_t)PAR_CFG_RTT_AT24_BASE_ADDR % (uint32_t)AT24CXX_PAGE_BYTE);
    const uint32_t size = (uint32_t)PAR_STORE_RTT_AT24_ERASE_CHUNK + 2U;

    at24_stub_reset();
    g_at24_available = true;
    memset(&g_at24_dev.mem[PAR_CFG_RTT_AT24_BASE_ADDR], 0x33, size);
    TEST_ASSERT_OK(par_store_backend_bind());
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    g_at24_fail_write = 2;
    TEST_ASSERT_STATUS(api->erase(0U, size), ePAR_ERROR_NVM);
    TEST_ASSERT(g_at24_write_count == 1U);
    for (uint32_t idx = 0U; idx < first_chunk; idx++)
    {
        TEST_ASSERT(g_at24_dev.mem[PAR_CFG_RTT_AT24_BASE_ADDR + idx] == 0xFFU);
    }
    TEST_ASSERT(g_at24_dev.mem[PAR_CFG_RTT_AT24_BASE_ADDR + first_chunk] == 0x33U);
    TEST_ASSERT_OK(api->deinit());
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

/** @brief Verify AT24CXX backend null-buffer and zero-length status policy. */
static bool test_rtt_at24cxx_null_buffer_and_zero_len_status_matrix(void)
{
    const par_store_backend_api_t *api;
    uint8_t value = 0x5AU;

    at24_stub_reset();
    g_at24_available = true;
    TEST_ASSERT_OK(par_store_backend_bind());
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    TEST_ASSERT_STATUS(api->read(0U, 1U, NULL), ePAR_ERROR_PARAM);
    TEST_ASSERT_STATUS(api->write(0U, 1U, NULL), ePAR_ERROR_PARAM);
    TEST_ASSERT_STATUS(api->read(0U, 0U, &value), ePAR_ERROR_NVM);
    TEST_ASSERT_STATUS(api->write(0U, 0U, &value), ePAR_ERROR_NVM);
    TEST_ASSERT_STATUS(api->erase(0U, 0U), ePAR_ERROR_NVM);
    TEST_ASSERT_OK(api->deinit());
    TEST_ASSERT_STATUS(api->read(0U, 1U, &value), ePAR_ERROR_PARAM);
    TEST_ASSERT_STATUS(api->write(0U, 1U, &value), ePAR_ERROR_PARAM);
    TEST_ASSERT_STATUS(api->erase(0U, 1U), ePAR_ERROR_PARAM);
    return true;
}

#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/** @brief Verify AT24CXX sync is a no-op under the current synchronous-write policy. */
static bool test_rtt_at24cxx_sync_noop_current_policy(void)
{
    const par_store_backend_api_t *api;

    at24_stub_reset();
    TEST_ASSERT_OK(par_store_backend_bind());
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->sync());
    g_at24_available = true;
    TEST_ASSERT_OK(api->init());
    TEST_ASSERT_OK(api->sync());
    TEST_ASSERT_OK(api->deinit());
    TEST_ASSERT_OK(api->sync());
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

/** @brief Verify repeated AT24CXX backend init/deinit cycles are idempotent. */
static bool test_rtt_at24cxx_repeated_init_deinit_is_idempotent(void)
{
    const par_store_backend_api_t *api;
    bool is_init = false;

    at24_stub_reset();
    g_at24_available = true;
    TEST_ASSERT_OK(par_store_backend_bind());
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    TEST_ASSERT_OK(api->init());
    api->is_init(&is_init);
    TEST_ASSERT(is_init);
    TEST_ASSERT_OK(api->sync());
    TEST_ASSERT_OK(api->deinit());
    TEST_ASSERT_OK(api->deinit());
    api->is_init(&is_init);
    TEST_ASSERT(!is_init);
    return true;
}

/** @brief Verify a failed AT24CXX probe leaves no half-open backend state. */
static bool test_rtt_at24cxx_init_check_fail_deinits_device_and_recovers(void)
{
    const par_store_backend_api_t *api;
    bool is_init = true;
    uint8_t readback = 0U;
    const uint8_t value = 0x6BU;

    at24_stub_reset();
    g_at24_available = true;
    g_at24_fail_check = 1;
    TEST_ASSERT_OK(par_store_backend_bind());
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_STATUS(api->init(), ePAR_ERROR_INIT);
    api->is_init(&is_init);
    TEST_ASSERT(!is_init);

    TEST_ASSERT_OK(api->init());
    api->is_init(&is_init);
    TEST_ASSERT(is_init);
    TEST_ASSERT_OK(api->write(0U, 1U, &value));
    TEST_ASSERT_OK(api->read(0U, 1U, &readback));
    TEST_ASSERT(readback == value);
    TEST_ASSERT_OK(api->deinit());
    return true;
}
