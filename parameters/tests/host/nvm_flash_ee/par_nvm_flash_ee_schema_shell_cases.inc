static bool test_nvm_table_id_mismatch_rebuilds_defaults(void)
{
    uint8_t value = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 9U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_deinit());

    /* Corrupt the serialized table-ID field in the scalar NVM header. */
    for (uint32_t addr = HOST_NVM_HEAD_TABLE_ID_OFFSET;
         addr < (HOST_NVM_HEAD_TABLE_ID_OFFSET + HOST_NVM_HEAD_TABLE_ID_SIZE);
         addr++)
    {
        host_flash_xor_byte(addr, 0xA5U);
    }

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 1U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}


#if defined(PAR_HOST_TEST_SCHEMA_EVOLUTION_WRITE)
/** @brief Create a persisted image with the pre-change scalar schema. */
static bool test_nvm_schema_rebuild_write_baseline_image(void)
{
    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 9U));
    TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, 444U));
    TEST_ASSERT_OK(par_save_all());
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* defined(PAR_HOST_TEST_SCHEMA_EVOLUTION_WRITE) */

#if defined(PAR_HOST_TEST_SCHEMA_EVOLUTION_READ)
/** @brief Verify type drift triggers full table rebuild with new defaults. */
static bool test_nvm_schema_type_change_triggers_table_rebuild_defaults(void)
{
    uint16_t mode = 0U;
    uint16_t u16 = 0U;

    host_flash_clear_failpoints();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_MODE, &mode));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT(mode == 8U);
    TEST_ASSERT(u16 == 100U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* defined(PAR_HOST_TEST_SCHEMA_EVOLUTION_READ) */

#if defined(PAR_HOST_TEST_SCHEMA_SLOT_REORDER_READ)
/** @brief Verify persistent slot reorder rebuilds defaults instead of cross-loading old values. */
static bool test_nvm_schema_slot_reorder_triggers_table_rebuild_defaults(void)
{
    uint8_t mode = 0U;
    uint16_t u16 = 0U;

    host_flash_clear_failpoints();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &mode));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT(mode == 1U);
    TEST_ASSERT(u16 == 100U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* defined(PAR_HOST_TEST_SCHEMA_SLOT_REORDER_READ) */

#if defined(PAR_HOST_TEST_SCHEMA_PERSISTENT_REMOVED_READ)
/** @brief Verify removing one persistent scalar slot rebuilds remaining defaults. */
static bool test_nvm_schema_persistent_removed_rebuilds_remaining_defaults(void)
{
    uint8_t mode = 0U;
    uint16_t u16 = 0U;
    uint32_t u32 = 0UL;

    host_flash_clear_failpoints();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &mode));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT_OK(par_get_u32(ePAR_TEST_U32, &u32));
    TEST_ASSERT(mode == 1U);
    TEST_ASSERT(u16 == 100U);
    TEST_ASSERT(u32 == 1000UL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* defined(PAR_HOST_TEST_SCHEMA_PERSISTENT_REMOVED_READ) */

#if defined(PAR_HOST_TEST_SCHEMA_SCALAR_TO_OBJECT_READ)
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/** @brief Verify scalar-to-object schema drift is rejected by the current policy. */
static bool test_nvm_schema_scalar_to_object_rejects_stale_image_current_policy(void)
{
    par_status_t status;

    host_flash_clear_failpoints();
    status = par_init();
    TEST_ASSERT((status & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    if (par_is_init())
    {
        (void)par_deinit();
    }
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
#endif /* defined(PAR_HOST_TEST_SCHEMA_SCALAR_TO_OBJECT_READ) */

#if defined(PAR_HOST_TEST_SCHEMA_OBJECT_TO_SCALAR_READ)
/** @brief Verify object-to-scalar schema drift rebuilds the scalar default. */
static bool test_nvm_schema_object_to_scalar_rebuilds_scalar_default(void)
{
    uint8_t value = 0U;

    host_flash_clear_failpoints();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_STR, &value));
    TEST_ASSERT(value == 3U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* defined(PAR_HOST_TEST_SCHEMA_OBJECT_TO_SCALAR_READ) */

#if defined(PAR_HOST_TEST_SCHEMA_OBJECT_CAPACITY_SHRINK_READ)
/** @brief Verify object capacity shrink rebuilds the shrunk object default. */
static bool test_nvm_schema_object_capacity_shrink_rebuilds_object_default(void)
{
    char str_buf[2] = { 0 };
    uint16_t len = 0U;
    uint8_t mode = 0U;

    host_flash_clear_failpoints();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &mode));
    TEST_ASSERT(mode == 9U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 1U);
    TEST_ASSERT(0 == strcmp(str_buf, "x"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* defined(PAR_HOST_TEST_SCHEMA_OBJECT_CAPACITY_SHRINK_READ) */

/** @brief Verify shell save persists live values across a restart. */
static bool test_msh_save_persists_live_scalar_after_restart(void)
{
    char *save_args[] = { "par", "save" };
    uint8_t value = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 8U));
    run_nvm_shell(2, save_args);
    TEST_ASSERT(nvm_shell_capture_contains("OK"));
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 8U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify shell save_clean rewrites the managed NVM area with live values. */
static bool test_msh_save_clean_rewrites_live_values(void)
{
    char *save_clean_args[] = { "par", "save_clean" };
    uint8_t value = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 6U));
    run_nvm_shell(2, save_clean_args);
    TEST_ASSERT(nvm_shell_capture_contains("OK"));
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 6U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify shell save reports backend write errors. */
static bool test_msh_save_reports_backend_error(void)
{
    char *save_args[] = { "par", "save" };

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 7U));
    g_fail_program_after = 0;
    run_nvm_shell(2, save_args);
    TEST_ASSERT(nvm_shell_capture_contains("ERR"));
    host_flash_clear_failpoints();
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify NVM wrapper APIs persist scalar and object live values. */
static bool test_nvm_save_by_id_save_all_and_n_save_wrappers(void)
{
    uint8_t u8 = 0U;
    uint16_t u16 = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;
    uint16_t mode_id = 0U;
    uint16_t str_id = 0U;
    par_type_t scalar_value = { 0 };
    const uint8_t str_payload[] = { 'i', 'd', 'n', 'v', 'm' };

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_id_by_num(ePAR_TEST_MODE, &mode_id));
    TEST_ASSERT_OK(par_get_id_by_num(ePAR_TEST_STR, &str_id));
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 3U));
    TEST_ASSERT_OK(par_save_by_id(mode_id));
    scalar_value.u16 = 777U;
    TEST_ASSERT_OK(par_set_scalar_n_save(ePAR_TEST_U16, &scalar_value.u16));
    TEST_ASSERT_OK(par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"nsave", 5U));
    TEST_ASSERT_STATUS(par_save_by_id(0xFFFFU), ePAR_ERROR);
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(u8 == 3U);
    TEST_ASSERT(u16 == 777U);
    TEST_ASSERT(len == 5U);
    TEST_ASSERT(strcmp(str_buf, "nsave") == 0);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save_all());
    TEST_ASSERT_OK(par_set_obj_n_save_by_id(str_id, str_payload, (uint16_t)sizeof(str_payload)));
    TEST_ASSERT_OK(par_deinit());

    memset(str_buf, 0, sizeof(str_buf));
    len = 0U;
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(u8 == 4U);
    TEST_ASSERT(u16 == 777U);
    TEST_ASSERT(len == sizeof(str_payload));
    TEST_ASSERT(strcmp(str_buf, "idnvm") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Create a stored scalar image with a shorter compatible prefix. */
static bool host_flash_child_write_short_scalar_prefix(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, 222U));
    TEST_ASSERT_OK(par_set_u32(ePAR_TEST_U32, 33333UL));
    TEST_ASSERT_OK(par_save_all());
    TEST_ASSERT(host_scalar_store_write_header_count(1U));
    return true;
}

/** @brief Verify a shorter scalar prefix follows the layout compatibility policy. */
static bool test_nvm_scalar_stored_count_smaller_applies_layout_policy(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    uint16_t u16 = 0U;
    uint32_t u32 = 0U;

    if (false == par_is_init())
    {
        TEST_ASSERT(init_module());
    }
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 1U));
    TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, 100U));
    TEST_ASSERT_OK(par_set_u32(ePAR_TEST_U32, 1000UL));
    TEST_ASSERT_OK(par_deinit());
    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_write_short_scalar_prefix());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT_OK(par_get_u32(ePAR_TEST_U32, &u32));
#if (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY)
    TEST_ASSERT(u8 == 1U);
#else
    TEST_ASSERT(u8 == 5U);
#endif /* (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY) */
    TEST_ASSERT(u16 == 100U);
    TEST_ASSERT(u32 == 1000UL);
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT_OK(par_get_u32(ePAR_TEST_U32, &u32));
#if (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY)
    TEST_ASSERT(u8 == 1U);
#else
    TEST_ASSERT(u8 == 5U);
#endif /* (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY) */
    TEST_ASSERT(u16 == 100U);
    TEST_ASSERT(u32 == 1000UL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}


/** @brief Create a scalar image whose stored count is larger than this build. */
static bool host_flash_child_write_oversized_scalar_count(void)
{
    const uint16_t compile_count = (uint16_t)PAR_PERSISTENT_COMPILE_COUNT;
    const uint16_t count = (uint16_t)(compile_count + 1U);
    const uint32_t table_id = host_scalar_table_id_calc_relaxed_count(count);

    TEST_ASSERT(host_scalar_table_id_calc_relaxed_count(compile_count) ==
                par_nvm_table_id_calc_for_count(compile_count));
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, 222U));
    TEST_ASSERT_OK(par_set_u32(ePAR_TEST_U32, 33333UL));
    TEST_ASSERT_OK(par_save_all());
    TEST_ASSERT(host_scalar_store_write_raw_header(count, table_id));
    return true;
}

/** @brief Verify stored-count overflow rebuilds scalar defaults once. */
static bool test_nvm_scalar_stored_count_larger_rebuilds_defaults_once(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    uint16_t u16 = 0U;
    uint32_t u32 = 0U;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_write_oversized_scalar_count());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT_OK(par_get_u32(ePAR_TEST_U32, &u32));
    TEST_ASSERT(u8 == 1U);
    TEST_ASSERT(u16 == 100U);
    TEST_ASSERT(u32 == 1000UL);
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT_OK(par_get_u32(ePAR_TEST_U32, &u32));
    TEST_ASSERT(u8 == 1U);
    TEST_ASSERT(u16 == 100U);
    TEST_ASSERT(u32 == 1000UL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}
