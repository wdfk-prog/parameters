#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED)
/**
 * @brief Reinitialize the module and verify the persisted string object value.
 * @param expected Null-terminated string expected after reload.
 * @return true when the reloaded object matches @p expected.
 */
static bool verify_reloaded_object_str(const char *const expected)
{
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    TEST_ASSERT_OK(par_deinit());
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == (uint16_t)strlen(expected));
    TEST_ASSERT(strcmp(str_buf, expected) == 0);
    return true;
}

/** @brief Verify a dedicated object write failpoint does not mutate backend bytes. */
static bool test_object_dedicated_write_fail_preserves_backend_image(void)
{
    uint8_t before[HOST_FLASH_SIZE];

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "old"));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    memcpy(before, g_object_flash, sizeof(before));

    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "new"));
    g_object_fail_write_after = 0;
    TEST_ASSERT((par_save(ePAR_TEST_STR) & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    TEST_ASSERT(0 == memcmp(before, g_object_flash, sizeof(before)));
    host_flash_clear_failpoints();
    TEST_ASSERT(verify_reloaded_object_str("old"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify dedicated object readback failures report NVM errors. */
static bool test_object_dedicated_read_fail_reports_error_and_recovers_object(void)
{
    uint8_t u8 = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 6U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "old"));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "new"));
    g_object_fail_read_after = 0;
    TEST_ASSERT((par_save(ePAR_TEST_STR) & ePAR_ERROR_NVM) != 0U);
    host_flash_clear_failpoints();
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 6U);
    TEST_ASSERT(verify_reloaded_object_str("new"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify dedicated object sync failures report NVM errors. */
static bool test_object_dedicated_sync_fail_reports_error_and_recovers_object(void)
{
    uint8_t u8 = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 7U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "old"));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "new"));
    g_object_fail_sync_after = 0;
    TEST_ASSERT((par_save(ePAR_TEST_STR) & ePAR_ERROR_NVM) != 0U);
    host_flash_clear_failpoints();
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 7U);
    TEST_ASSERT(verify_reloaded_object_str("new"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify dedicated object bulk erase failures abort the object rewrite. */
static bool test_object_dedicated_erase_fail_aborts_save_all(void)
{
    uint8_t u8 = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 8U));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "old"));
    TEST_ASSERT_OK(par_save_all());
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "new"));
    g_object_fail_erase_after = 0;
    TEST_ASSERT((par_save_all() & ePAR_ERROR_NVM) != 0U);
    host_flash_clear_failpoints();
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 8U);
    TEST_ASSERT(verify_reloaded_object_str("old"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}


#if (PAR_CFG_NVM_OBJECT_DEDICATED_BASE_ADDR > 0U)
/** @brief Verify dedicated object persistence honors a non-zero backend base address. */
static bool test_object_dedicated_nonzero_base_save_reload_and_prefix_stable(void)
{
    uint8_t prefix[PAR_CFG_NVM_OBJECT_DEDICATED_BASE_ADDR];

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"base", 4U));
    memcpy(prefix, g_object_flash, sizeof(prefix));
    for (uint32_t idx = 0U; idx < (uint32_t)sizeof(prefix); idx++)
    {
        TEST_ASSERT(prefix[idx] == 0xFFU);
    }
    TEST_ASSERT(0xFFU != g_object_flash[PAR_CFG_NVM_OBJECT_DEDICATED_BASE_ADDR]);
    TEST_ASSERT(verify_reloaded_object_str("base"));
    TEST_ASSERT(0 == memcmp(prefix, g_object_flash, sizeof(prefix)));
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* (PAR_CFG_NVM_OBJECT_DEDICATED_BASE_ADDR > 0U) */

/** @brief Verify object n-save write failures keep the persisted object stable. */
static bool test_object_dedicated_n_save_write_fail_preserves_persisted_object(void)
{
    uint8_t before[HOST_FLASH_SIZE];

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"old", 3U));
    memcpy(before, g_object_flash, sizeof(before));
    g_object_fail_write_after = 0;
    TEST_ASSERT((par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"new", 3U) & ePAR_ERROR_NVM) != 0U);
    TEST_ASSERT(0 == memcmp(before, g_object_flash, sizeof(before)));
    host_flash_clear_failpoints();
    TEST_ASSERT(verify_reloaded_object_str("old"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify save-all object write failure documents scalar/default-object policy. */
static bool test_object_dedicated_save_all_object_write_fail_scalar_new_object_default(void)
{
    uint8_t u8 = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "old"));
    TEST_ASSERT_OK(par_save_all());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "new"));
    g_object_fail_write_after = 0;
    TEST_ASSERT((par_save_all() & ePAR_ERROR_NVM) != 0U);
    host_flash_clear_failpoints();
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 5U);
    TEST_ASSERT(verify_reloaded_object_str("ap"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}


/**
 * @brief Describe one dedicated object write-all failpoint phase.
 */
typedef struct
{
    int fail_after;          /**< Dedicated backend write countdown to fail. */
    const char *phase_name;  /**< Human-readable phase label for repro logs. */
} host_object_write_failpoint_case_t;

/**
 * @brief Verify scalar state and object defaults after failed object write-all.
 * @param expected_mode Scalar mode value expected after reload.
 * @return true when scalar persistence survived and object storage rebuilt defaults.
 */
static bool host_object_dedicated_verify_scalar_and_default_objects(
    const uint8_t expected_mode)
{
    static const uint8_t default_bytes[] = { 0x01U, 0x02U };
    uint8_t mode = 0U;
    char str_buf[9] = { 0 };
    uint8_t bytes_buf[4] = { 0U };
    uint16_t len = 0U;

    host_flash_clear_failpoints();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &mode));
    TEST_ASSERT(mode == expected_mode);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(0 == strcmp(str_buf, "ap"));
    TEST_ASSERT_OK(par_get_bytes(ePAR_TEST_BYTES, bytes_buf, sizeof(bytes_buf), &len));
    TEST_ASSERT(len == (uint16_t)sizeof(default_bytes));
    TEST_ASSERT(0 == memcmp(bytes_buf, default_bytes, sizeof(default_bytes)));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/**
 * @brief Fail one selected dedicated object write-all phase in a child process.
 * @param fail_after Dedicated backend write countdown to fail.
 * @return true when the selected write-all phase reports an NVM error.
 */
static bool host_object_dedicated_child_save_all_fail_phase(const int fail_after)
{
    const uint8_t old_bytes[4] = { 1U, 2U, 3U, 4U };
    const uint8_t new_bytes[4] = { 9U, 8U, 7U, 6U };

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "old"));
    TEST_ASSERT_OK(par_set_bytes(ePAR_TEST_BYTES, old_bytes, (uint16_t)sizeof(old_bytes)));
    TEST_ASSERT_OK(par_save_all());

    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "new"));
    TEST_ASSERT_OK(par_set_bytes(ePAR_TEST_BYTES, new_bytes, (uint16_t)sizeof(new_bytes)));
    g_object_fail_write_after = fail_after;
    TEST_ASSERT((par_save_all() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    return true;
}

/** @brief Verify payload, metadata, and marker object write failpoints. */
static bool test_object_dedicated_save_all_payload_metadata_marker_failpoint_sweep(void)
{
    static const host_object_write_failpoint_case_t cases[] = {
        { 0, "str_payload" },
        { 1, "str_metadata" },
        { 2, "bytes_payload" },
        { 3, "bytes_metadata" },
        { 4, "header_marker" },
    };

    for (size_t i = 0U; i < (sizeof(cases) / sizeof(cases[0])); i++)
    {
        pid_t child_pid;

        printf("PAR_HOST_OBJECT_PHASE_FAILPOINT phase=%s fail_after=%d\n",
               cases[i].phase_name,
               cases[i].fail_after);
        host_flash_reset_erased();
        child_pid = fork();
        TEST_ASSERT(child_pid >= 0);
        if (0 == child_pid)
        {
            host_child_exit_from_result(
                host_object_dedicated_child_save_all_fail_phase(cases[i].fail_after));
        }
        TEST_ASSERT(host_child_exit_is_success(child_pid));

        child_pid = fork();
        TEST_ASSERT(child_pid >= 0);
        if (0 == child_pid)
        {
            host_child_exit_from_result(
                host_object_dedicated_verify_scalar_and_default_objects(5U));
        }
        TEST_ASSERT(host_child_exit_is_success(child_pid));
    }

    return true;
}


/** @brief Verify repeated dedicated object write failures converge to last object. */
static bool test_object_dedicated_repeated_write_failures_converge_to_last_object(void)
{
    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"old", 3U));
    g_object_fail_write_after = 0;
    TEST_ASSERT((par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"new", 3U) &
                 ePAR_ERROR_NVM) != 0U);
    host_flash_clear_failpoints();
    TEST_ASSERT(verify_reloaded_object_str("old"));
    g_object_fail_write_after = 0;
    TEST_ASSERT((par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"bad", 3U) &
                 ePAR_ERROR_NVM) != 0U);
    host_flash_clear_failpoints();
    TEST_ASSERT(verify_reloaded_object_str("old"));
    TEST_ASSERT_OK(par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"mid", 3U));
    TEST_ASSERT(verify_reloaded_object_str("mid"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED) */
