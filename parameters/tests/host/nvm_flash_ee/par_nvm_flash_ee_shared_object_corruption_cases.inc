#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) && \
    (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_FIXED)
/** @brief Corrupt committed object header and keep the scalar commit stable. */
static bool host_flash_child_corrupt_object_header_after_commit(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 6U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "object"));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    TEST_ASSERT(host_object_store_xor_byte(PAR_CFG_NVM_OBJECT_FIXED_ADDR, 0x01U));
    return true;
}

/** @brief Verify object header corruption restores object defaults without scalar loss. */
static bool test_nvm_object_header_corruption_restores_default_without_scalar_loss(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_corrupt_object_header_after_commit());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 6U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Corrupt the first object record length field after committing values. */
static bool host_flash_child_corrupt_object_record_len_after_commit(void)
{
    static const char committed[] = "object";
    uint32_t len_addr = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 7U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, committed));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    TEST_ASSERT(host_object_store_get_record_len_addr(ePAR_TEST_STR,
                                                       (uint16_t)strlen(committed),
                                                       &len_addr));
    TEST_ASSERT(host_object_store_xor_byte(len_addr, 0x7FU));
    return true;
}

/** @brief Verify invalid object record length rebuilds object defaults only. */
static bool test_nvm_object_record_len_corruption_restores_default_without_scalar_loss(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_corrupt_object_record_len_after_commit());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 7U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Corrupt the committed object payload after committing values. */
static bool host_flash_child_corrupt_object_payload_after_commit(void)
{
    static const char committed[] = "object";
    uint32_t payload_addr = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 8U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, committed));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    TEST_ASSERT(host_object_store_get_record_field_addr(ePAR_TEST_STR,
                                                         (uint16_t)strlen(committed),
                                                         (uint32_t)sizeof(host_object_store_record_meta_t),
                                                         &payload_addr));
    TEST_ASSERT(host_object_store_xor_byte(payload_addr, 0x01U));
    return true;
}

/** @brief Verify object payload CRC mismatch restores defaults without scalar loss. */
static bool test_nvm_object_payload_crc_corruption_restores_default_without_scalar_loss(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_corrupt_object_payload_after_commit());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 8U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Corrupt the committed object record ID after committing values. */
static bool host_flash_child_corrupt_object_record_id_after_commit(void)
{
    static const char committed[] = "object";
    uint32_t id_addr = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 9U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, committed));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    TEST_ASSERT(host_object_store_get_record_field_addr(ePAR_TEST_STR,
                                                         (uint16_t)strlen(committed),
                                                         (uint32_t)offsetof(host_object_store_record_meta_t, id),
                                                         &id_addr));
    TEST_ASSERT(host_object_store_xor_byte(id_addr, 0x01U));
    return true;
}

/** @brief Verify object record-ID mismatch restores defaults without scalar loss. */
static bool test_nvm_object_record_meta_id_corruption_restores_default_without_scalar_loss(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_corrupt_object_record_id_after_commit());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 9U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Corrupt object header body-size while keeping the header CRC valid. */
static bool host_flash_child_corrupt_object_header_body_size_after_commit(void)
{
    host_object_store_head_t head = { 0 };

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 10U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "object"));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    TEST_ASSERT(host_object_store_read_head(PAR_CFG_NVM_OBJECT_FIXED_ADDR, &head));
    TEST_ASSERT(head.sign == HOST_OBJECT_STORE_SIGN);
    TEST_ASSERT(head.version == HOST_OBJECT_STORE_VERSION);
    head.body_size ^= 1UL;
    head.crc = host_object_store_calc_head_crc(&head);
    TEST_ASSERT(host_object_store_write_head(PAR_CFG_NVM_OBJECT_FIXED_ADDR, &head));
    return true;
}

/** @brief Verify body-size mismatch rebuilds object defaults only. */
static bool test_nvm_object_header_body_size_mismatch_rebuilds_objects_only(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_corrupt_object_header_body_size_after_commit());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 10U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}


/** @brief Corrupt object header version while keeping its CRC valid. */
static bool host_flash_child_corrupt_object_header_version_after_commit(void)
{
    host_object_store_head_t head = { 0 };

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 6U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "object"));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    TEST_ASSERT(host_object_store_read_head(PAR_CFG_NVM_OBJECT_FIXED_ADDR, &head));
    head.version ^= 1U;
    head.crc = host_object_store_calc_head_crc(&head);
    TEST_ASSERT(host_object_store_write_head(PAR_CFG_NVM_OBJECT_FIXED_ADDR, &head));
    return true;
}

/** @brief Verify object header version mismatch rebuilds objects only. */
static bool test_nvm_object_header_version_mismatch_rebuilds_objects_only(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_corrupt_object_header_version_after_commit());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 6U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Corrupt one object record metadata field after committing values. */
static bool host_flash_child_corrupt_object_record_meta_field_after_commit(const uint32_t field_offset,
                                                                           const uint8_t mask,
                                                                           const uint8_t scalar_value)
{
    static const char committed[] = "object";
    uint32_t field_addr = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, scalar_value));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, committed));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    TEST_ASSERT(host_object_store_get_record_field_addr(ePAR_TEST_STR,
                                                         (uint16_t)strlen(committed),
                                                         field_offset,
                                                         &field_addr));
    TEST_ASSERT(host_object_store_xor_byte(field_addr, mask));
    return true;
}

/** @brief Verify object record type mismatch restores object defaults only. */
static bool test_nvm_object_record_type_mismatch_restores_default_without_scalar_loss(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_corrupt_object_record_meta_field_after_commit(
            (uint32_t)offsetof(host_object_store_record_meta_t, type), 0x01U, 7U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 7U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object record flags mismatch restores object defaults only. */
static bool test_nvm_object_record_flags_mismatch_restores_default_without_scalar_loss(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_corrupt_object_record_meta_field_after_commit(
            (uint32_t)offsetof(host_object_store_record_meta_t, flags), 0x01U, 8U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 8U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object record element-size mismatch restores defaults only. */
static bool test_nvm_object_record_elem_size_mismatch_restores_default_without_scalar_loss(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_corrupt_object_record_meta_field_after_commit(
            (uint32_t)offsetof(host_object_store_record_meta_t, elem_size), 0x01U, 9U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 9U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object record capacity mismatch restores defaults only. */
static bool test_nvm_object_record_capacity_mismatch_restores_default_without_scalar_loss(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_corrupt_object_record_meta_field_after_commit(
            (uint32_t)offsetof(host_object_store_record_meta_t, capacity), 0x01U, 10U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 10U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) && (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_FIXED) */
