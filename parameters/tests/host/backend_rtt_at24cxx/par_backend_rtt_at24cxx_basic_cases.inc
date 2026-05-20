/** @brief Verify AT24CXX backend missing-device and read/write paths. */
static bool test_rtt_at24cxx_adapter_smoke(void)
{
    const par_store_backend_api_t *api;
    uint8_t readback = 0U;
    const uint8_t value = 0x5AU;

    at24_stub_reset();
    TEST_ASSERT_OK(par_store_backend_bind());
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_STATUS(api->init(), ePAR_ERROR_INIT);

    g_at24_available = true;
    TEST_ASSERT_OK(api->init());
    TEST_ASSERT_OK(api->write(0U, 1U, &value));
    TEST_ASSERT_OK(api->read(0U, 1U, &readback));
    TEST_ASSERT(readback == value);
    TEST_ASSERT_OK(api->erase(0U, 1U));
    TEST_ASSERT_OK(api->read(0U, 1U, &readback));
    TEST_ASSERT(readback == 0xFFU);
    TEST_ASSERT_STATUS(api->read(UINT32_MAX, 1U, &readback), ePAR_ERROR_NVM);
    TEST_ASSERT_OK(api->deinit());
    return true;
}

/** @brief Verify writes larger than one page are split at AT24CXX page granularity. */
static bool test_rtt_at24cxx_page_split_write(void)
{
    const par_store_backend_api_t *api;
    uint8_t payload[AT24CXX_PAGE_BYTE + 3U];
    uint32_t expected_addr = (uint32_t)PAR_CFG_RTT_AT24_BASE_ADDR;
    uint32_t remaining = (uint32_t)sizeof(payload);
    uint8_t expected_count = 0U;

    for (uint8_t i = 0U; i < (uint8_t)sizeof(payload); i++)
    {
        payload[i] = (uint8_t)(0x10U + i);
    }

    at24_stub_reset();
    g_at24_available = true;
    TEST_ASSERT_OK(par_store_backend_bind());
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    TEST_ASSERT_OK(api->write(0U, (uint32_t)sizeof(payload), payload));
    while (remaining > 0U)
    {
        uint32_t xfer = (uint32_t)AT24CXX_PAGE_BYTE -
                        (expected_addr % (uint32_t)AT24CXX_PAGE_BYTE);

        if (xfer > remaining)
        {
            xfer = remaining;
        }

        TEST_ASSERT(expected_count < AT24_TRACE_MAX);
        TEST_ASSERT(g_at24_write_addr[expected_count] == expected_addr);
        TEST_ASSERT(g_at24_write_size[expected_count] == (uint16_t)xfer);
        expected_addr += xfer;
        remaining -= xfer;
        expected_count++;
    }
    TEST_ASSERT(g_at24_write_count == expected_count);
    TEST_ASSERT(0 == memcmp(&g_at24_dev.mem[PAR_CFG_RTT_AT24_BASE_ADDR], payload, sizeof(payload)));
    TEST_ASSERT_OK(api->deinit());
    return true;
}

/** @brief Verify non-page-aligned writes split at the remaining page window. */
static bool test_rtt_at24cxx_unaligned_page_split_write(void)
{
    const par_store_backend_api_t *api;
    const uint32_t page_size = (uint32_t)AT24CXX_PAGE_BYTE;
    const uint32_t base_page_offset = (uint32_t)PAR_CFG_RTT_AT24_BASE_ADDR %
                                      page_size;
    const uint32_t rel_addr = (page_size - 1U + page_size - base_page_offset) %
                              page_size;
    const uint32_t abs_addr = (uint32_t)PAR_CFG_RTT_AT24_BASE_ADDR + rel_addr;
    const uint32_t first_chunk = page_size - (abs_addr % page_size);
    uint8_t payload[2U];

    TEST_ASSERT(page_size > 1U);
    TEST_ASSERT((abs_addr % page_size) != 0U);
    TEST_ASSERT((uint32_t)PAR_CFG_RTT_AT24_SIZE >=
                (rel_addr + (uint32_t)sizeof(payload)));

    for (uint8_t i = 0U; i < (uint8_t)sizeof(payload); i++)
    {
        payload[i] = (uint8_t)(0x40U + i);
    }

    at24_stub_reset();
    g_at24_available = true;
    TEST_ASSERT_OK(par_store_backend_bind());
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    TEST_ASSERT_OK(api->write(rel_addr, (uint32_t)sizeof(payload), payload));
    TEST_ASSERT(g_at24_write_count == 2U);
    TEST_ASSERT(g_at24_write_addr[0] == abs_addr);
    TEST_ASSERT(g_at24_write_size[0] == first_chunk);
    TEST_ASSERT(g_at24_write_addr[1] == (abs_addr + first_chunk));
    TEST_ASSERT(g_at24_write_size[1] == ((uint32_t)sizeof(payload) - first_chunk));
    TEST_ASSERT(0 == memcmp(&g_at24_dev.mem[abs_addr], payload, sizeof(payload)));
    TEST_ASSERT_OK(api->deinit());
    return true;
}

/** @brief Verify configured AT24CXX base address is honored. */
static bool test_rtt_at24cxx_base_addr_offsets_window(void)
{
    const par_store_backend_api_t *api;
    const uint8_t payload[2] = { 0xA5U, 0x5AU };
    uint8_t readback[sizeof(payload)] = { 0U };

    at24_stub_reset();
    g_at24_available = true;
    if (PAR_CFG_RTT_AT24_BASE_ADDR > 0U)
    {
        g_at24_dev.mem[PAR_CFG_RTT_AT24_BASE_ADDR - 1U] = 0x33U;
    }
    TEST_ASSERT_OK(par_store_backend_bind());
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    TEST_ASSERT_OK(api->write(0U, (uint32_t)sizeof(payload), payload));
    TEST_ASSERT_OK(api->read(0U, (uint32_t)sizeof(readback), readback));
    TEST_ASSERT(0 == memcmp(readback, payload, sizeof(payload)));
    if (PAR_CFG_RTT_AT24_BASE_ADDR > 0U)
    {
        TEST_ASSERT(g_at24_dev.mem[PAR_CFG_RTT_AT24_BASE_ADDR - 1U] == 0x33U);
    }
    TEST_ASSERT(0 == memcmp(&g_at24_dev.mem[PAR_CFG_RTT_AT24_BASE_ADDR], payload, sizeof(payload)));
    TEST_ASSERT_OK(api->deinit());
    return true;
}

/** @brief Verify AT24CXX exact-end and zero-length access boundaries. */
static bool test_rtt_at24cxx_exact_end_and_zero_len_boundaries(void)
{
    const par_store_backend_api_t *api;
    const uint32_t end_addr = (uint32_t)PAR_CFG_RTT_AT24_SIZE;
    const uint8_t payload[2] = { 0x12U, 0x34U };
    uint8_t readback[sizeof(payload)] = { 0U };
    uint8_t zero = 0U;

    TEST_ASSERT(end_addr >= sizeof(payload));
    at24_stub_reset();
    g_at24_available = true;
    TEST_ASSERT_OK(par_store_backend_bind());
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    TEST_ASSERT_OK(api->write(end_addr - (uint32_t)sizeof(payload),
                              (uint32_t)sizeof(payload),
                              payload));
    TEST_ASSERT_OK(api->read(end_addr - (uint32_t)sizeof(readback),
                             (uint32_t)sizeof(readback),
                             readback));
    TEST_ASSERT(0 == memcmp(readback, payload, sizeof(payload)));
    TEST_ASSERT_STATUS(api->write(end_addr - 1U, (uint32_t)sizeof(payload), payload),
                       ePAR_ERROR_NVM);
    TEST_ASSERT_STATUS(api->read(end_addr - 1U, (uint32_t)sizeof(readback), readback),
                       ePAR_ERROR_NVM);
    TEST_ASSERT_STATUS(api->erase(end_addr - 1U, (uint32_t)sizeof(payload)), ePAR_ERROR_NVM);
    TEST_ASSERT_STATUS(api->read(end_addr, 0U, &zero), ePAR_ERROR_NVM);
    TEST_ASSERT_STATUS(api->write(end_addr, 0U, &zero), ePAR_ERROR_NVM);
    TEST_ASSERT_STATUS(api->erase(end_addr, 0U), ePAR_ERROR_NVM);
    TEST_ASSERT_OK(api->deinit());
    return true;
}

/** @brief Verify AT24CXX read/write driver errors propagate to the backend caller. */
static bool test_rtt_at24cxx_driver_error_propagates(void)
{
    const par_store_backend_api_t *api;
    const uint8_t payload[4] = { 1U, 2U, 3U, 4U };
    uint8_t readback[sizeof(payload)] = { 0U };

    at24_stub_reset();
    g_at24_available = true;
    TEST_ASSERT_OK(par_store_backend_bind());
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    g_at24_fail_write = 1;
    TEST_ASSERT_STATUS(api->write(0U, (uint32_t)sizeof(payload), payload), ePAR_ERROR_NVM);
    g_at24_fail_read = 1;
    TEST_ASSERT_STATUS(api->read(0U, (uint32_t)sizeof(readback), readback), ePAR_ERROR_NVM);
    g_at24_fail_write = 1;
    TEST_ASSERT_STATUS(api->erase(0U, (uint32_t)sizeof(payload)), ePAR_ERROR_NVM);
    TEST_ASSERT_OK(api->deinit());
    return true;
}
