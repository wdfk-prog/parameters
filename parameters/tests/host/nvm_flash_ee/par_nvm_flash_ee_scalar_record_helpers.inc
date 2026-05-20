/**
 * @brief Calculate the Flash-EE bank-header geometry CRC.
 * @param p_header Header fields to hash.
 * @return Header CRC value widened to 32 bits.
 */
static uint32_t host_flash_ee_calc_bank_header_crc(const host_flash_ee_bank_header_t * const p_header)
{
    uint16_t crc = PAR_IF_CRC16_INIT;

    TEST_ASSERT(NULL != p_header);
    crc = par_if_crc16_accumulate(crc,
                                  (const uint8_t *)p_header,
                                  (uint32_t)offsetof(host_flash_ee_bank_header_t, cfg_crc));
    return (uint32_t)crc;
}

/**
 * @brief Read a physical Flash-EE bank header from the fake image.
 * @param bank_base Physical bank base address.
 * @param p_header Output header mirror.
 * @return true when the header was copied from the fake image.
 */
static bool host_flash_ee_read_bank_header(const uint32_t bank_base,
                                           host_flash_ee_bank_header_t * const p_header)
{
    TEST_ASSERT(NULL != p_header);
    TEST_ASSERT((bank_base + (uint32_t)sizeof(*p_header)) <= HOST_FLASH_SIZE);
    memcpy(p_header, &g_flash[bank_base], sizeof(*p_header));
    return true;
}

/**
 * @brief Write one synthetic Flash-EE bank header into the fake image.
 * @param bank_base Physical bank base address.
 * @param seq Bank sequence number to serialize.
 * @param state Bank state word to serialize.
 * @return true when the header was written.
 */
static bool host_flash_ee_write_bank_header(const uint32_t bank_base,
                                            const uint32_t seq,
                                            const uint32_t state)
{
    host_flash_ee_bank_header_t header;

    TEST_ASSERT((bank_base + (uint32_t)sizeof(header)) <= HOST_FLASH_SIZE);
    memset(&header, 0xFF, sizeof(header));
    header.magic = HOST_FLASH_EE_HEADER_MAGIC;
    header.version = PAR_CFG_NVM_BACKEND_FLASH_EE_VERSION;
    header.logical_size = PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE;
    header.line_size = HOST_FLASH_EE_LINE_SIZE;
    header.record_size = HOST_FLASH_EE_RECORD_SIZE;
    header.bank_size = HOST_FLASH_EE_BANK_SIZE;
    header.seq = seq;
    header.state = state;
    header.cfg_crc = host_flash_ee_calc_bank_header_crc(&header);
    memcpy(&g_flash[bank_base], &header, sizeof(header));
    host_flash_write_image();
    return true;
}

/**
 * @brief Write a prepared or invalid higher-sequence inactive bank header.
 * @param state Bank state word to place in the inactive bank.
 * @param corrupt_crc true to corrupt the serialized geometry CRC.
 * @return true when the inactive bank header was written.
 */
static bool host_flash_ee_write_higher_seq_inactive_header(const uint32_t state,
                                                           const bool corrupt_crc)
{
    uint32_t active_base = 0U;
    uint32_t next_offset = 0U;
    uint32_t inactive_base = 0U;
    host_flash_ee_bank_header_t active_header = { 0 };

    TEST_ASSERT(host_flash_ee_find_active_bank(&active_base, &next_offset));
    TEST_ASSERT(host_flash_ee_read_bank_header(active_base, &active_header));
    inactive_base = (0U == active_base) ? HOST_FLASH_EE_BANK_SIZE : 0U;
    memset(&g_flash[inactive_base], 0xFF, HOST_FLASH_EE_BANK_SIZE);
    TEST_ASSERT(host_flash_ee_write_bank_header(inactive_base, active_header.seq + 1U, state));
    if (true == corrupt_crc)
    {
        g_flash[inactive_base + (uint32_t)offsetof(host_flash_ee_bank_header_t, cfg_crc)] ^= 0x01U;
        host_flash_write_image();
    }
    return true;
}


/**
 * @brief Append an intentionally uncommitted physical Flash-EE tail record.
 * @param line_index Logical EEPROM line index to encode in the partial record.
 * @return true when the partial record was written to the fake image.
 */
static bool host_flash_ee_append_uncommitted_tail_record(const uint32_t line_index)
{
    uint32_t bank_base = 0U;
    uint32_t next_offset = 0U;
    uint8_t record[HOST_FLASH_EE_RECORD_SIZE];
    host_flash_ee_record_meta_t meta = { 0 };

    TEST_ASSERT(host_flash_ee_find_active_bank(&bank_base, &next_offset));
    TEST_ASSERT((next_offset + HOST_FLASH_EE_RECORD_SIZE) <= HOST_FLASH_EE_BANK_SIZE);

    memset(record, 0xFF, sizeof(record));
    memset(record, 0x00, HOST_FLASH_EE_LINE_SIZE);
    meta.line_index = line_index;
    meta.payload_size = (uint16_t)HOST_FLASH_EE_LINE_SIZE;
    meta.record_crc = 0U;
    meta.reserved = 0UL;
    memcpy(&record[HOST_FLASH_EE_LINE_SIZE], &meta, sizeof(meta));
    record[HOST_FLASH_EE_RECORD_COMMIT_OFFSET] = 'R';
    memcpy(&g_flash[bank_base + next_offset], record, sizeof(record));
    host_flash_write_image();
    return true;
}

#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/**
 * @brief Append a committed but CRC-invalid tail record.
 * @details This fixture locks the Flash-EE replay current policy: a bad
 *          committed tail record makes the image untrusted and triggers
 *          default rebuild instead of falling back to older good records.
 *          Change this expectation only with an intentional recovery-policy
 *          change.
 * @param line_index Logical EEPROM line index to encode in the bad record.
 * @return true when the bad committed tail record was written.
 */
static bool host_flash_ee_append_bad_committed_tail_record(const uint32_t line_index)
{
    uint32_t bank_base = 0U;
    uint32_t next_offset = 0U;
    uint8_t record[HOST_FLASH_EE_RECORD_SIZE];
    host_flash_ee_record_meta_t meta = { 0 };

    TEST_ASSERT(host_flash_ee_find_active_bank(&bank_base, &next_offset));
    TEST_ASSERT((next_offset + HOST_FLASH_EE_RECORD_SIZE) <= HOST_FLASH_EE_BANK_SIZE);

    memset(record, 0xFF, sizeof(record));
    memset(record, 0x00, HOST_FLASH_EE_LINE_SIZE);
    meta.line_index = line_index;
    meta.payload_size = (uint16_t)HOST_FLASH_EE_LINE_SIZE;
    meta.record_crc = 0x1234U;
    meta.reserved = 0xFFFFFFFFUL;
    memcpy(&record[HOST_FLASH_EE_LINE_SIZE], &meta, sizeof(meta));
    memcpy(&record[HOST_FLASH_EE_RECORD_COMMIT_OFFSET],
           &(const uint32_t){ HOST_FLASH_EE_RECORD_MAGIC },
           sizeof(uint32_t));
    memcpy(&g_flash[bank_base + next_offset], record, sizeof(record));
    host_flash_write_image();
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

/**
 * @brief Calculate the scalar NVM header CRC for one count/table-ID pair.
 * @param count Stored persistent scalar count.
 * @param table_id Stored table-ID value.
 * @return Header CRC-16 value.
 */
static uint16_t host_scalar_header_crc(const uint16_t count,
                                       const uint32_t table_id)
{
    uint16_t crc = PAR_IF_CRC16_INIT;

    crc = par_if_crc16_accumulate(crc, (const uint8_t *)&count, (uint32_t)sizeof(count));
    crc = par_if_crc16_accumulate(crc, (const uint8_t *)&table_id, (uint32_t)sizeof(table_id));
    return crc;
}

/**
 * @brief Update a relaxed scalar table-ID digest with native-endian bytes.
 * @param p_hval Pointer to the rolling FNV-1a state.
 * @param p_serialized_size Pointer to the serialized byte counter.
 * @param p_value Pointer to bytes that should be hashed.
 * @param value_size Byte count to hash.
 */
static void host_scalar_table_id_hash_update(Fnv32_t * const p_hval,
                                             uint32_t * const p_serialized_size,
                                             const void * const p_value,
                                             const uint32_t value_size)
{
    *p_hval = fnv_32a_buf((void *)p_value, (size_t)value_size, *p_hval);
    *p_serialized_size += value_size;
}

/**
 * @brief Calculate a scalar table-ID digest without count-range assertions.
 * @details The production helper asserts when @p persistent_count is larger
 * than the current compile-time persistent scalar count. This host-only mirror
 * intentionally keeps the same serialized inputs while allowing oversized
 * counts, so count-overflow tests do not rely on a table-ID mismatch.
 * @param persistent_count Stored persistent scalar count to encode in digest.
 * @return Relaxed scalar table-ID digest for test image construction.
 */
static uint32_t host_scalar_table_id_calc_relaxed_count(const uint16_t persistent_count)
{
    Fnv32_t hval = FNV1_32A_INIT;
    uint32_t serialized_size = 0U;
    uint16_t hashed_count = 0U;
    const uint32_t schema_version = (uint32_t)PAR_CFG_TABLE_ID_SCHEMA_VER;
    const uint32_t record_layout = (uint32_t)PAR_CFG_NVM_RECORD_LAYOUT;

    host_scalar_table_id_hash_update(&hval,
                                     &serialized_size,
                                     &schema_version,
                                     (uint32_t)sizeof(schema_version));
    host_scalar_table_id_hash_update(&hval,
                                     &serialized_size,
                                     &record_layout,
                                     (uint32_t)sizeof(record_layout));
    host_scalar_table_id_hash_update(&hval,
                                     &serialized_size,
                                     &persistent_count,
                                     (uint32_t)sizeof(persistent_count));

    for (par_num_t par_num = 0U; par_num < ePAR_NUM_OF; par_num++)
    {
        const par_cfg_t * const p_cfg = par_get_config(par_num);
        const uint8_t type = (uint8_t)p_cfg->type;

        if (false == p_cfg->persistent)
        {
            continue;
        }

#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
        if (true == par_object_type_is_object(p_cfg->type))
        {
            continue;
        }
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

        if (hashed_count >= persistent_count)
        {
            break;
        }

        host_scalar_table_id_hash_update(&hval,
                                         &serialized_size,
                                         &type,
                                         (uint32_t)sizeof(type));

#if (1 == PAR_CFG_NVM_RECORD_LAYOUT_HAS_STORED_ID)
        {
            const uint16_t parameter_id = par_cfg_get_param_id_const(par_num);

            host_scalar_table_id_hash_update(&hval,
                                             &serialized_size,
                                             &parameter_id,
                                             (uint32_t)sizeof(parameter_id));
        }
#endif /* (1 == PAR_CFG_NVM_RECORD_LAYOUT_HAS_STORED_ID) */
        hashed_count++;
    }

    TEST_ASSERT(serialized_size > 0U);
    return (uint32_t)hval;
}

/**
 * @brief Rewrite the logical scalar NVM header through the active backend.
 * @param count Stored persistent scalar count to serialize.
 * @return true when the logical header was updated and synchronized.
 */
static bool host_scalar_store_write_header_count(const uint16_t count)
{
    const par_store_backend_api_t * const p_store = par_store_backend_flash_ee_get_api();
    const uint32_t table_id = par_nvm_table_id_calc_for_count(count);
    const uint16_t crc = host_scalar_header_crc(count, table_id);
    uint8_t header[HOST_NVM_HEAD_CRC_OFFSET + (uint32_t)sizeof(crc)] = { 0U };
    const uint32_t sign = HOST_NVM_SIGN;

    TEST_ASSERT(NULL != p_store);
    TEST_ASSERT(NULL != p_store->write);
    TEST_ASSERT(NULL != p_store->sync);
    memcpy(&header[0], &sign, sizeof(sign));
    memcpy(&header[HOST_NVM_HEAD_OBJ_NB_OFFSET], &count, sizeof(count));
    memcpy(&header[HOST_NVM_HEAD_TABLE_ID_OFFSET], &table_id, sizeof(table_id));
    memcpy(&header[HOST_NVM_HEAD_CRC_OFFSET], &crc, sizeof(crc));
    TEST_ASSERT_OK(p_store->write(0U, (uint32_t)sizeof(header), header));
    TEST_ASSERT_OK(p_store->sync());
    return true;
}


/**
 * @brief Rewrite the logical scalar NVM header with an arbitrary table ID.
 * @param count Stored persistent scalar count to serialize.
 * @param table_id Stored table-ID value to serialize.
 * @return true when the logical header was updated and synchronized.
 */
static bool host_scalar_store_write_raw_header(const uint16_t count,
                                               const uint32_t table_id)
{
    const par_store_backend_api_t * const p_store = par_store_backend_flash_ee_get_api();
    const uint16_t crc = host_scalar_header_crc(count, table_id);
    uint8_t header[HOST_NVM_HEAD_CRC_OFFSET + (uint32_t)sizeof(crc)] = { 0U };
    const uint32_t sign = HOST_NVM_SIGN;

    TEST_ASSERT(NULL != p_store);
    TEST_ASSERT(NULL != p_store->write);
    TEST_ASSERT(NULL != p_store->sync);
    memcpy(&header[0], &sign, sizeof(sign));
    memcpy(&header[HOST_NVM_HEAD_OBJ_NB_OFFSET], &count, sizeof(count));
    memcpy(&header[HOST_NVM_HEAD_TABLE_ID_OFFSET], &table_id, sizeof(table_id));
    memcpy(&header[HOST_NVM_HEAD_CRC_OFFSET], &crc, sizeof(crc));
    TEST_ASSERT_OK(p_store->write(0U, (uint32_t)sizeof(header), header));
    TEST_ASSERT_OK(p_store->sync());
    return true;
}
