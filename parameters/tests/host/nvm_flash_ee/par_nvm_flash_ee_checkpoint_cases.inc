static bool host_flash_child_fail_checkpoint_after_commit(void)
{
    uint32_t bank_base = 0U;
    uint32_t next_offset = 0U;
    const uint32_t max_attempts =
        (HOST_FLASH_EE_BANK_SIZE / HOST_FLASH_EE_RECORD_SIZE) + 2U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));

    for (uint32_t attempt = 0U; attempt < max_attempts; attempt++)
    {
        TEST_ASSERT(host_flash_ee_find_active_bank(&bank_base, &next_offset));
        if ((next_offset + HOST_FLASH_EE_RECORD_SIZE) > HOST_FLASH_EE_BANK_SIZE)
        {
            break;
        }
        TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, (uint16_t)(attempt % 1000U)));
        TEST_ASSERT_OK(par_save(ePAR_TEST_U16));
    }

    TEST_ASSERT(host_flash_ee_find_active_bank(&bank_base, &next_offset));
    TEST_ASSERT((next_offset + HOST_FLASH_EE_RECORD_SIZE) > HOST_FLASH_EE_BANK_SIZE);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    g_fail_erase_after = 0;
    TEST_ASSERT((par_save(ePAR_TEST_MODE) & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    return true;
}

/** @brief Verify checkpoint power-loss failure preserves the previous active bank. */
static bool test_flash_ee_checkpoint_power_loss_preserves_previous_active_bank(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_fail_checkpoint_after_commit());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_verify_mode_value(4U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}


/** @brief Create a higher-sequence prepared inactive bank after a valid commit. */
static bool host_flash_child_write_prepared_inactive_bank_after_commit(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT(host_flash_ee_write_higher_seq_inactive_header(HOST_FLASH_EE_HEADER_PREPARE, false));
    return true;
}

/** @brief Verify a prepared higher-sequence bank is ignored during restart. */
static bool test_flash_ee_checkpoint_prepare_header_power_loss_ignores_new_bank(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_write_prepared_inactive_bank_after_commit());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_verify_mode_value(4U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}

/** @brief Create a higher-sequence active bank with a corrupted geometry CRC. */
static bool host_flash_child_write_bad_crc_inactive_active_bank_after_commit(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT(host_flash_ee_write_higher_seq_inactive_header(HOST_FLASH_EE_HEADER_ACTIVE, true));
    return true;
}

/** @brief Verify an active bank with bad cfg CRC cannot replace the old bank. */
static bool test_flash_ee_newer_bank_with_bad_cfg_crc_is_ignored(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_write_bad_crc_inactive_active_bank_after_commit());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_verify_mode_value(4U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}


/**
 * @brief Reinitialize only the Flash-EE backend for raw logical tests.
 * @return true when the backend is bound and initialized.
 */
static bool host_flash_ee_backend_reinit_only(void)
{
    const par_store_backend_api_t * const p_store = par_store_backend_flash_ee_get_api();
    bool is_init = false;

    TEST_ASSERT(NULL != p_store);
    TEST_ASSERT(NULL != p_store->init);
    TEST_ASSERT(NULL != p_store->deinit);
    TEST_ASSERT(NULL != p_store->is_init);
    p_store->is_init(&is_init);
    if (is_init)
    {
        TEST_ASSERT_OK(p_store->deinit());
    }

    TEST_ASSERT_OK(par_store_backend_bind());
    TEST_ASSERT_OK(p_store->init());
    return true;
}

/**
 * @brief Build one deterministic logical Flash-EE line pattern.
 * @param p_line Output logical line buffer.
 * @param seed Pattern seed that identifies the expected line value.
 * @return true when @p p_line was populated.
 */
static bool host_flash_ee_build_line_pattern(uint8_t * const p_line,
                                             const uint8_t seed)
{
    TEST_ASSERT(NULL != p_line);
    for (uint32_t idx = 0U; idx < HOST_FLASH_EE_LINE_SIZE; idx++)
    {
        p_line[idx] = (uint8_t)(seed + idx);
    }
    return true;
}

/**
 * @brief Build the expected logical image for checkpoint-copy sweep tests.
 * @param p_expected Output logical image indexed by Flash-EE line.
 * @return true when @p p_expected was populated.
 */
static bool host_flash_ee_build_checkpoint_expected_image(
    uint8_t p_expected[HOST_FLASH_EE_LINE_COUNT][HOST_FLASH_EE_LINE_SIZE])
{
    TEST_ASSERT(NULL != p_expected);
    for (uint32_t line = 0U; line < HOST_FLASH_EE_LINE_COUNT; line++)
    {
        TEST_ASSERT(host_flash_ee_build_line_pattern(p_expected[line],
                                                     (uint8_t)(0x10U + line)));
    }

    TEST_ASSERT(host_flash_ee_build_line_pattern(p_expected[0], 0xA0U));
    return true;
}

/**
 * @brief Write one full logical Flash-EE line through the backend API.
 * @param p_store Active Flash-EE backend API.
 * @param line_index Logical line index to write.
 * @param p_line Source line buffer.
 * @return true when the logical line is persisted.
 */
static bool host_flash_ee_write_logical_line(const par_store_backend_api_t * const p_store,
                                             const uint32_t line_index,
                                             const uint8_t * const p_line)
{
    TEST_ASSERT(NULL != p_store);
    TEST_ASSERT(NULL != p_store->write);
    TEST_ASSERT(NULL != p_line);
    TEST_ASSERT(line_index < HOST_FLASH_EE_LINE_COUNT);
    TEST_ASSERT_OK(p_store->write(line_index * HOST_FLASH_EE_LINE_SIZE,
                                  HOST_FLASH_EE_LINE_SIZE,
                                  p_line));
    return true;
}

/**
 * @brief Prepare a full active bank with every logical line live.
 * @return true when the next logical write will force a checkpoint.
 */
static bool host_flash_ee_prepare_full_checkpoint_image(void)
{
    const par_store_backend_api_t * const p_store = par_store_backend_flash_ee_get_api();
    uint8_t line_buf[HOST_FLASH_EE_LINE_SIZE];
    uint32_t bank_base = 0U;
    uint32_t next_offset = 0U;
    const uint32_t max_attempts =
        (HOST_FLASH_EE_BANK_SIZE / HOST_FLASH_EE_RECORD_SIZE) + 2U;

    TEST_ASSERT(host_flash_ee_backend_reinit_only());
    TEST_ASSERT(NULL != p_store);

    for (uint32_t line = 0U; line < HOST_FLASH_EE_LINE_COUNT; line++)
    {
        TEST_ASSERT(host_flash_ee_build_line_pattern(line_buf, (uint8_t)(0x10U + line)));
        TEST_ASSERT(host_flash_ee_write_logical_line(p_store, line, line_buf));
    }

    TEST_ASSERT(host_flash_ee_build_line_pattern(line_buf, 0xA0U));
    for (uint32_t attempt = 0U; attempt < max_attempts; attempt++)
    {
        TEST_ASSERT(host_flash_ee_find_active_bank(&bank_base, &next_offset));
        if ((next_offset + HOST_FLASH_EE_RECORD_SIZE) > HOST_FLASH_EE_BANK_SIZE)
        {
            break;
        }
        TEST_ASSERT(host_flash_ee_write_logical_line(p_store, 0U, line_buf));
    }

    TEST_ASSERT(host_flash_ee_find_active_bank(&bank_base, &next_offset));
    TEST_ASSERT((next_offset + HOST_FLASH_EE_RECORD_SIZE) > HOST_FLASH_EE_BANK_SIZE);
    return true;
}

/**
 * @brief Verify the logical Flash-EE image matches checkpoint-copy baseline.
 * @return true when all logical lines reload unchanged.
 */
static bool host_flash_ee_verify_checkpoint_expected_image(void)
{
    const par_store_backend_api_t * const p_store = par_store_backend_flash_ee_get_api();
    uint8_t expected[HOST_FLASH_EE_LINE_COUNT][HOST_FLASH_EE_LINE_SIZE];
    uint8_t readback[HOST_FLASH_EE_LINE_SIZE];

    host_flash_clear_failpoints();
    TEST_ASSERT(host_flash_ee_build_checkpoint_expected_image(expected));
    TEST_ASSERT(host_flash_ee_backend_reinit_only());
    TEST_ASSERT(NULL != p_store);
    TEST_ASSERT(NULL != p_store->read);

    for (uint32_t line = 0U; line < HOST_FLASH_EE_LINE_COUNT; line++)
    {
        TEST_ASSERT_OK(p_store->read(line * HOST_FLASH_EE_LINE_SIZE,
                                     HOST_FLASH_EE_LINE_SIZE,
                                     readback));
        TEST_ASSERT(0 == memcmp(readback, expected[line], sizeof(readback)));
    }

    TEST_ASSERT_OK(p_store->deinit());
    return true;
}


/** @brief Checkpoint-copy program phase selected for failpoint injection. */
typedef enum
{
    eHOST_FLASH_EE_CHECKPOINT_COPY_PAYLOAD_META = 0, /**< Payload and metadata program phase. */
    eHOST_FLASH_EE_CHECKPOINT_COPY_COMMIT_MARKER     /**< Commit-marker program phase. */
} host_flash_ee_checkpoint_copy_phase_t;

/**
 * @brief Return the checkpoint-copy program phase name used in repro logs.
 * @param phase Checkpoint-copy program phase.
 * @return Human-readable phase name.
 */
static const char *host_flash_ee_checkpoint_copy_phase_name(
    const host_flash_ee_checkpoint_copy_phase_t phase)
{
    return (eHOST_FLASH_EE_CHECKPOINT_COPY_PAYLOAD_META == phase) ?
           "payload_meta" : "commit_marker";
}

/**
 * @brief Return the failpoint countdown for one checkpoint-copy program phase.
 * @details Countdown zero is the prepared-bank header program. Record-copy
 *          payload/metadata and commit-marker writes start at countdown one.
 * @param line_index Logical line copied into the checkpoint target bank.
 * @param phase Record-copy program phase to fail.
 * @return Program failpoint countdown value for the selected phase.
 */
static int host_flash_ee_checkpoint_copy_fail_after(
    const uint32_t line_index,
    const host_flash_ee_checkpoint_copy_phase_t phase)
{
    return (int)(1U + (line_index * 2U) + (uint32_t)phase);
}

/**
 * @brief Calculate the expected failed program range for one copied record.
 * @param target_bank Physical base address of the checkpoint target bank.
 * @param line_index Logical line copied into the checkpoint target bank.
 * @param phase Record-copy program phase to fail.
 * @param p_addr Expected failed program address.
 * @param p_size Expected failed program size.
 * @return true when @p p_addr and @p p_size were populated.
 */
static bool host_flash_ee_checkpoint_copy_expected_program_range(
    const uint32_t target_bank,
    const uint32_t line_index,
    const host_flash_ee_checkpoint_copy_phase_t phase,
    uint32_t * const p_addr,
    uint32_t * const p_size)
{
    const uint32_t record_addr = target_bank + HOST_FLASH_EE_HEADER_SIZE +
                                 (line_index * HOST_FLASH_EE_RECORD_SIZE);

    TEST_ASSERT(line_index < HOST_FLASH_EE_LINE_COUNT);
    TEST_ASSERT(NULL != p_addr);
    TEST_ASSERT(NULL != p_size);
    if (eHOST_FLASH_EE_CHECKPOINT_COPY_PAYLOAD_META == phase)
    {
        *p_addr = record_addr;
        *p_size = HOST_FLASH_EE_RECORD_COMMIT_OFFSET;
    }
    else
    {
        *p_addr = record_addr + HOST_FLASH_EE_RECORD_COMMIT_OFFSET;
        *p_size = HOST_FLASH_PROGRAM_SIZE;
    }

    return true;
}

/**
 * @brief Fail one selected program operation during checkpoint record copy.
 * @param line_index Logical line whose copied record should fail.
 * @param phase Record-copy program phase to fail.
 * @return true when the selected checkpoint-copy failure is injected and
 *         lands on the expected physical record range.
 */
static bool host_flash_child_fail_checkpoint_record_copy_sweep(
    const uint32_t line_index,
    const host_flash_ee_checkpoint_copy_phase_t phase)
{
    const par_store_backend_api_t * const p_store = par_store_backend_flash_ee_get_api();
    uint8_t attempted[HOST_FLASH_EE_LINE_SIZE];
    uint32_t active_bank = 0U;
    uint32_t next_offset = 0U;
    uint32_t expected_addr = 0U;
    uint32_t expected_size = 0U;
    const int fail_after = host_flash_ee_checkpoint_copy_fail_after(line_index, phase);

    TEST_ASSERT(host_flash_ee_prepare_full_checkpoint_image());
    TEST_ASSERT(host_flash_ee_find_active_bank(&active_bank, &next_offset));
    TEST_ASSERT(host_flash_ee_checkpoint_copy_expected_program_range(
        (0U == active_bank) ? HOST_FLASH_EE_BANK_SIZE : 0U,
        line_index,
        phase,
        &expected_addr,
        &expected_size));
    TEST_ASSERT(host_flash_ee_build_line_pattern(attempted, 0xE0U));
    g_fail_program_after = fail_after;
    TEST_ASSERT((p_store->write(0U, HOST_FLASH_EE_LINE_SIZE, attempted) &
                 ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    TEST_ASSERT(g_fail_program_hit);
    TEST_ASSERT(g_fail_program_addr == expected_addr);
    TEST_ASSERT(g_fail_program_size == expected_size);
    return true;
}

/** @brief Verify every checkpoint-copied record tolerates program failpoints. */
static bool test_flash_ee_checkpoint_record_copy_failpoint_sweep_preserves_previous_bank(void)
{
    static const host_flash_ee_checkpoint_copy_phase_t phases[] = {
        eHOST_FLASH_EE_CHECKPOINT_COPY_PAYLOAD_META,
        eHOST_FLASH_EE_CHECKPOINT_COPY_COMMIT_MARKER,
    };

    if (0 != strcmp(PAR_HOST_TEST_PROFILE_NAME, "default"))
    {
        printf("PAR_HOST_CHECKPOINT_COPY_FAILPOINT_SKIP profile=%s\n",
               PAR_HOST_TEST_PROFILE_NAME);
        return true;
    }

    for (uint32_t line = 0U; line < HOST_FLASH_EE_LINE_COUNT; line++)
    {
        for (size_t phase_idx = 0U; phase_idx < (sizeof(phases) / sizeof(phases[0])); phase_idx++)
        {
            const host_flash_ee_checkpoint_copy_phase_t phase = phases[phase_idx];
            const int fail_after = host_flash_ee_checkpoint_copy_fail_after(line, phase);
            pid_t child_pid;

            printf("PAR_HOST_CHECKPOINT_COPY_FAILPOINT line=%lu phase=%s fail_after=%d\n",
                   (unsigned long)line,
                   host_flash_ee_checkpoint_copy_phase_name(phase),
                   fail_after);
            host_flash_reset_erased();
            child_pid = fork();
            TEST_ASSERT(child_pid >= 0);
            if (0 == child_pid)
            {
                host_child_exit_from_result(
                    host_flash_child_fail_checkpoint_record_copy_sweep(line, phase));
            }
            TEST_ASSERT(host_child_exit_is_success(child_pid));

            child_pid = fork();
            TEST_ASSERT(child_pid >= 0);
            if (0 == child_pid)
            {
                host_child_exit_from_result(host_flash_ee_verify_checkpoint_expected_image());
            }
            TEST_ASSERT(host_child_exit_is_success(child_pid));
        }
    }

    return true;
}

/** @brief Fill the active bank and fail during checkpoint record copy. */
static bool host_flash_child_fail_checkpoint_record_copy_after_commit(void)
{
    uint32_t bank_base = 0U;
    uint32_t next_offset = 0U;
    const uint32_t max_attempts =
        (HOST_FLASH_EE_BANK_SIZE / HOST_FLASH_EE_RECORD_SIZE) + 2U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));

    for (uint32_t attempt = 0U; attempt < max_attempts; attempt++)
    {
        TEST_ASSERT(host_flash_ee_find_active_bank(&bank_base, &next_offset));
        if ((next_offset + HOST_FLASH_EE_RECORD_SIZE) > HOST_FLASH_EE_BANK_SIZE)
        {
            break;
        }
        TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, (uint16_t)(attempt % 1000U)));
        TEST_ASSERT_OK(par_save(ePAR_TEST_U16));
    }

    TEST_ASSERT(host_flash_ee_find_active_bank(&bank_base, &next_offset));
    TEST_ASSERT((next_offset + HOST_FLASH_EE_RECORD_SIZE) > HOST_FLASH_EE_BANK_SIZE);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    g_fail_program_after = 1;
    TEST_ASSERT((par_save(ePAR_TEST_MODE) & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    return true;
}

/** @brief Verify record-copy failure keeps using the previous active bank. */
static bool test_flash_ee_checkpoint_record_copy_power_loss_preserves_previous_active_bank(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_fail_checkpoint_record_copy_after_commit());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_verify_mode_value(4U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}
