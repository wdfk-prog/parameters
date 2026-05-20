/** @brief Verify adapter initialization rejects a missing underlying FAL flash device. */
static bool test_flash_ee_fal_missing_flash_device_fails_init(void)
{
    const par_store_backend_api_t *api;
    bool is_init = true;

    fal_stub_reset();
    TEST_ASSERT_OK(par_store_backend_bind());
    g_fal_partition_available = true;
    g_fal_flash_available = false;
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT((api->init() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    api->is_init(&is_init);
    TEST_ASSERT(!is_init);
    return true;
}


/** @brief Verify repeated FAL adapter init/deinit cycles leave a clean state. */
static bool test_flash_ee_fal_repeated_init_deinit_is_idempotent(void)
{
    const par_store_backend_api_t *api;
    bool is_init = false;

    fal_stub_reset();
    TEST_ASSERT_OK(par_store_backend_bind());
    g_fal_partition_available = true;
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    TEST_ASSERT_OK(api->init());
    api->is_init(&is_init);
    TEST_ASSERT(is_init);
    TEST_ASSERT_OK(api->deinit());
    TEST_ASSERT_OK(api->deinit());
    api->is_init(&is_init);
    TEST_ASSERT(!is_init);
    return true;
}

/** @brief Verify FAL adapter I/O calls reject access after deinitialization. */
static bool test_flash_ee_fal_after_deinit_rejects_io(void)
{
    const par_store_backend_api_t *api;
    const uint8_t payload[4] = { 0x71U, 0x72U, 0x73U, 0x74U };
    uint8_t readback[sizeof(payload)] = { 0U };

    fal_stub_reset();
    TEST_ASSERT_OK(par_store_backend_bind());
    g_fal_partition_available = true;
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    TEST_ASSERT_OK(api->deinit());
    TEST_ASSERT_STATUS(api->read(0U, (uint32_t)sizeof(readback), readback), ePAR_ERROR_INIT);
    TEST_ASSERT_STATUS(api->write(0U, (uint32_t)sizeof(payload), payload), ePAR_ERROR_INIT);
    TEST_ASSERT_STATUS(api->erase(0U, (uint32_t)sizeof(payload)), ePAR_ERROR_INIT);
    return true;
}

/** @brief Verify too-small FAL partitions fail during Flash-EE initialization. */
static bool test_flash_ee_fal_partition_smaller_than_geometry_fails_init(void)
{
    const par_store_backend_api_t *api;
    bool is_init = true;

    fal_stub_reset();
    TEST_ASSERT_OK(par_store_backend_bind());
    g_fal_partition_available = true;
    g_fal_part.len = (long)(PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE / 2U);
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT((api->init() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    api->is_init(&is_init);
    TEST_ASSERT(!is_init);
    return true;
}



/** @brief Verify short FAL read/write errors do not corrupt destination or flash tail. */
static bool test_flash_ee_fal_short_read_write_preserves_buffers(void)
{
    const uint8_t payload[4] = { 0x21U, 0x22U, 0x23U, 0x24U };
    uint8_t readback[sizeof(payload)] = { 0xA5U, 0xA5U, 0xA5U, 0xA5U };

    fal_stub_reset();
    g_fal_partition_available = true;
    g_fal_short_write = 1;
    TEST_ASSERT(fal_partition_write(&g_fal_part, 0U, payload, sizeof(payload)) ==
                ((int)sizeof(payload) - 1));
    for (uint32_t idx = 0U; idx < (uint32_t)sizeof(payload); idx++)
    {
        TEST_ASSERT(g_fal_flash[idx] == 0xCCU);
    }
    g_fal_short_read = 1;
    TEST_ASSERT(fal_partition_read(&g_fal_part, 0U, readback, sizeof(readback)) ==
                ((int)sizeof(readback) - 1));
    TEST_ASSERT(readback[0] == 0xA5U);
    TEST_ASSERT(readback[sizeof(readback) - 1U] == 0xA5U);
    return true;
}

/** @brief Verify partition offsets remain protected when FAL I/O fails short. */
static bool test_flash_ee_fal_partition_offset_failed_io_preserves_neighbors(void)
{
    const uint8_t payload[4] = { 0x91U, 0x92U, 0x93U, 0x94U };
    uint8_t readback[sizeof(payload)] = { 0U };

    fal_stub_reset();
    g_fal_partition_available = true;
    g_fal_part.offset = 128;
    g_fal_part.len = FAL_SMOKE_SIZE - g_fal_part.offset;
    g_fal_flash[g_fal_part.offset - 1] = 0x5AU;
    g_fal_short_write = 1;
    TEST_ASSERT(fal_partition_write(&g_fal_part, 0U, payload, sizeof(payload)) ==
                ((int)sizeof(payload) - 1));
    TEST_ASSERT(g_fal_flash[g_fal_part.offset - 1] == 0x5AU);
    TEST_ASSERT(g_fal_flash[g_fal_part.offset] == 0xCCU);
    TEST_ASSERT(fal_partition_write(&g_fal_part, 0U, payload, sizeof(payload)) == (int)sizeof(payload));
    g_fal_short_read = 1;
    TEST_ASSERT(fal_partition_read(&g_fal_part, 0U, readback, sizeof(readback)) ==
                ((int)sizeof(readback) - 1));
    TEST_ASSERT(g_fal_flash[g_fal_part.offset - 1] == 0x5AU);
    return true;
}

/** @brief Verify initialized FAL backend API reports runtime short I/O errors. */
static bool test_flash_ee_fal_runtime_short_io_reports_backend_errors(void)
{
    const par_store_backend_api_t *api;
    const uint8_t payload[4] = { 0x31U, 0x32U, 0x33U, 0x34U };
    const uint8_t next_payload[4] = { 0x41U, 0x42U, 0x43U, 0x44U };
    uint8_t readback[sizeof(payload)] = { 0xA5U, 0xA5U, 0xA5U, 0xA5U };

    fal_stub_reset();
    TEST_ASSERT_OK(par_store_backend_bind());
    g_fal_partition_available = true;
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    TEST_ASSERT_OK(api->write(0U, (uint32_t)sizeof(payload), payload));

    g_fal_short_read = 1;
    TEST_ASSERT_STATUS(api->read(0U, (uint32_t)sizeof(readback), readback), ePAR_ERROR_NVM);
    TEST_ASSERT(readback[0] == 0xA5U);
    TEST_ASSERT(readback[sizeof(readback) - 1U] == 0xA5U);

    g_fal_short_write = 2;
    TEST_ASSERT_STATUS(api->write(0U, (uint32_t)sizeof(next_payload), next_payload), ePAR_ERROR_NVM);
    TEST_ASSERT_OK(api->sync());
    TEST_ASSERT_OK(api->deinit());
    return true;
}

/** @brief Verify FAL backend reports stable NULL-buffer and zero-length statuses. */
static bool test_flash_ee_fal_null_buffer_and_zero_len_status_matrix(void)
{
    const par_store_backend_api_t *api;
    const uint8_t payload[2] = { 0x12U, 0x34U };
    uint8_t readback[2] = { 0xA5U, 0x5AU };

    fal_stub_reset();
    TEST_ASSERT_OK(par_store_backend_bind());
    g_fal_partition_available = true;
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    TEST_ASSERT_STATUS(api->read(0U, 1U, NULL), ePAR_ERROR_PARAM);
    TEST_ASSERT_STATUS(api->write(0U, 1U, NULL), ePAR_ERROR_PARAM);
    TEST_ASSERT_STATUS(api->read(0U, 0U, readback), ePAR_ERROR_PARAM);
    TEST_ASSERT_STATUS(api->write(0U, 0U, payload), ePAR_ERROR_PARAM);
    TEST_ASSERT_STATUS(api->erase(0U, 0U), ePAR_ERROR_PARAM);
    TEST_ASSERT(readback[0] == 0xA5U);
    TEST_ASSERT(readback[1] == 0x5AU);
    TEST_ASSERT_OK(api->deinit());
    return true;
}

/** @brief Verify a short FAL erase leaves partition neighbors and body unchanged. */
static bool test_flash_ee_fal_short_erase_preserves_neighbors(void)
{
    const size_t part_offset = 128U;

    fal_stub_reset();
    g_fal_partition_available = true;
    g_fal_part.offset = (long)part_offset;
    g_fal_part.len = FAL_SMOKE_SIZE - (long)part_offset;
    g_fal_flash[part_offset - 1U] = 0x5AU;
    g_fal_flash[part_offset] = 0x33U;
    g_fal_flash[part_offset + 3U] = 0x44U;
    g_fal_short_erase = 1;
    TEST_ASSERT(fal_partition_erase(&g_fal_part, 0U, 4U) == 3);
    TEST_ASSERT(g_fal_flash[part_offset - 1U] == 0x5AU);
    TEST_ASSERT(g_fal_flash[part_offset] == 0x33U);
    TEST_ASSERT(g_fal_flash[part_offset + 3U] == 0x44U);
    return true;
}
