/** @brief Verify FAL adapter bind, missing partition, read/write, erase, and boundary paths. */
static bool test_flash_ee_fal_adapter_smoke(void)
{
    const par_store_backend_api_t *api;
    bool is_init = true;
    const uint8_t payload[4] = { 0x11U, 0x22U, 0x33U, 0x44U };
    uint8_t readback[sizeof(payload)] = { 0U };

    fal_stub_reset();
    TEST_ASSERT_OK(par_store_backend_bind());
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT((api->init() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    api->is_init(&is_init);
    TEST_ASSERT(!is_init);

    g_fal_partition_available = true;
    TEST_ASSERT_OK(api->init());
    api->is_init(&is_init);
    TEST_ASSERT(is_init);
    TEST_ASSERT(api->name != NULL);
    TEST_ASSERT_OK(api->write(0U, (uint32_t)sizeof(payload), payload));
    TEST_ASSERT_OK(api->sync());
    TEST_ASSERT_OK(api->read(0U, (uint32_t)sizeof(readback), readback));
    TEST_ASSERT(0 == memcmp(readback, payload, sizeof(payload)));
    TEST_ASSERT_OK(api->erase(0U, (uint32_t)sizeof(payload)));
    TEST_ASSERT_OK(api->sync());
    memset(readback, 0U, sizeof(readback));
    TEST_ASSERT_OK(api->read(0U, (uint32_t)sizeof(readback), readback));
    TEST_ASSERT(0xFFU == readback[0]);
    TEST_ASSERT(0xFFU == readback[1]);
    TEST_ASSERT(0xFFU == readback[2]);
    TEST_ASSERT(0xFFU == readback[3]);
    TEST_ASSERT_STATUS(api->read(UINT32_MAX, 1U, readback), ePAR_ERROR_PARAM);
    TEST_ASSERT_STATUS(api->write(UINT32_MAX, 1U, payload), ePAR_ERROR_PARAM);
    TEST_ASSERT_STATUS(api->erase(UINT32_MAX, 1U), ePAR_ERROR_PARAM);
    TEST_ASSERT_OK(api->deinit());
    return true;
}

/** @brief Verify FAL partition offsets are honored as a hard storage boundary. */
static bool test_flash_ee_fal_partition_offset_semantics(void)
{
    const par_store_backend_api_t *api;
    const uint8_t payload[4] = { 0xA1U, 0xB2U, 0xC3U, 0xD4U };
    uint8_t readback[sizeof(payload)] = { 0U };

    fal_stub_reset();
    TEST_ASSERT_OK(par_store_backend_bind());
    g_fal_partition_available = true;
    g_fal_part.offset = 128;
    g_fal_part.len = 0x1000;
    g_fal_flash[127] = 0x5AU;

    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    g_fal_flash[132] = 0x6BU;
    TEST_ASSERT_OK(api->write(0U, (uint32_t)sizeof(payload), payload));
    TEST_ASSERT(g_fal_flash[127] == 0x5AU);
    TEST_ASSERT(g_fal_flash[132] == 0x6BU);
    TEST_ASSERT_OK(api->read(0U, (uint32_t)sizeof(readback), readback));
    TEST_ASSERT(0 == memcmp(readback, payload, sizeof(payload)));
    TEST_ASSERT_STATUS(api->write((uint32_t)g_fal_part.len - 2U, (uint32_t)sizeof(payload), payload), ePAR_ERROR_PARAM);
    TEST_ASSERT(g_fal_flash[132] == 0x6BU);
    TEST_ASSERT_OK(api->erase(0U, (uint32_t)sizeof(payload)));
    TEST_ASSERT(g_fal_flash[127] == 0x5AU);
    TEST_ASSERT(g_fal_flash[132] == 0x6BU);
    TEST_ASSERT_OK(api->deinit());
    return true;
}

/** @brief Verify FAL exact-end and zero-length access boundaries. */
static bool test_flash_ee_fal_exact_end_and_zero_len_boundaries(void)
{
    const par_store_backend_api_t *api;
    const uint8_t payload[2] = { 0x35U, 0x53U };
    uint8_t readback[sizeof(payload)] = { 0U };
    const uint32_t logical_size = PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE;
    uint8_t zero = 0U;

    TEST_ASSERT(logical_size >= (uint32_t)sizeof(payload));
    fal_stub_reset();
    TEST_ASSERT_OK(par_store_backend_bind());
    g_fal_partition_available = true;
    g_fal_part.offset = 128;
    g_fal_part.len = FAL_SMOKE_SIZE - g_fal_part.offset;
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    TEST_ASSERT_OK(api->write(logical_size - (uint32_t)sizeof(payload), (uint32_t)sizeof(payload), payload));
    TEST_ASSERT_OK(api->read(logical_size - (uint32_t)sizeof(readback), (uint32_t)sizeof(readback), readback));
    TEST_ASSERT(0 == memcmp(readback, payload, sizeof(payload)));
    TEST_ASSERT_STATUS(api->write(logical_size - 1U, (uint32_t)sizeof(payload), payload), ePAR_ERROR_PARAM);
    TEST_ASSERT_STATUS(api->read(logical_size - 1U, (uint32_t)sizeof(readback), readback), ePAR_ERROR_PARAM);
    TEST_ASSERT_STATUS(api->erase(logical_size - 1U, (uint32_t)sizeof(payload)), ePAR_ERROR_PARAM);
    TEST_ASSERT_STATUS(api->read(logical_size, 0U, &zero), ePAR_ERROR_PARAM);
    TEST_ASSERT_STATUS(api->write(logical_size, 0U, &zero), ePAR_ERROR_PARAM);
    TEST_ASSERT_STATUS(api->erase(logical_size, 0U), ePAR_ERROR_PARAM);
    TEST_ASSERT_OK(api->deinit());
    return true;
}

/** @brief Verify short FAL format I/O returns propagate as initialization errors. */
static bool test_flash_ee_fal_short_io_propagates_error(void)
{
    const par_store_backend_api_t *api;

    fal_stub_reset();
    TEST_ASSERT_OK(par_store_backend_bind());
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);

    g_fal_partition_available = true;
    g_fal_short_read = 1;
    TEST_ASSERT((api->init() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);

    fal_stub_reset();
    g_fal_partition_available = true;
    g_fal_short_erase = 1;
    TEST_ASSERT((api->init() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);

    fal_stub_reset();
    g_fal_partition_available = true;
    g_fal_short_write = 1;
    TEST_ASSERT((api->init() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    return true;
}
