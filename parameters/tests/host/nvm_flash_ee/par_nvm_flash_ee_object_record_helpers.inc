#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) && \
    (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_FIXED)
/**
 * @brief Flip one byte through the active object-store abstraction.
 * @param addr Backend byte address accepted by the object-store API.
 * @param mask XOR corruption mask.
 * @return true when the object store byte was modified and flushed.
 */
static bool host_object_store_xor_byte(const uint32_t addr, const uint8_t mask)
{
    const par_store_backend_api_t * const p_store = par_nvm_object_store_get_api();
    uint8_t value = 0U;

    TEST_ASSERT(NULL != p_store);
    TEST_ASSERT(NULL != p_store->read);
    TEST_ASSERT(NULL != p_store->write);
    TEST_ASSERT(NULL != p_store->sync);
    TEST_ASSERT_OK(p_store->read(addr, 1U, &value));
    value ^= mask;
    TEST_ASSERT_OK(p_store->write(addr, 1U, &value));
    TEST_ASSERT_OK(p_store->sync());
    return true;
}

/** @brief Initialize the module with a preserved fake flash image. */
static bool init_module(void);

/**
 * @brief Host mirror of serialized object persistence record metadata.
 * @details Production object NVM stores this metadata in native byte order
 * before the payload. The helper below validates every neighbor field before
 * returning the address of the serialized length field.
 */
typedef struct
{
    uint16_t id;        /**< External parameter ID. */
    uint8_t type;       /**< Serialized par_type_list_t value. */
    uint8_t flags;      /**< Reserved flags. */
    uint16_t elem_size; /**< Object element size in bytes. */
    uint16_t capacity;  /**< Fixed payload capacity in bytes. */
    uint16_t len;       /**< Valid payload length in bytes. */
    uint16_t crc;       /**< CRC-16 over metadata and payload bytes. */
} host_object_store_record_meta_t;

/** @brief Reserved flags value expected in host object NVM records. */
#define HOST_OBJECT_STORE_RECORD_FLAGS_NONE (0U)

/** @brief Serialized object block signature. */
#define HOST_OBJECT_STORE_SIGN (0x504A424FUL)
/** @brief Serialized object block version. */
#define HOST_OBJECT_STORE_VERSION (1U)

/**
 * @brief Host mirror of serialized object persistence block header.
 */
typedef struct
{
    uint32_t sign;      /**< Object block signature. */
    uint16_t version;   /**< Serialized-layout version. */
    uint16_t obj_nb;    /**< Stored persistent object count. */
    uint32_t table_id;  /**< Object persistent-schema digest. */
    uint32_t body_size; /**< Serialized object-record body size in bytes. */
    uint16_t crc;       /**< Header CRC-16. */
} host_object_store_head_t;

/** @brief Serialized object persistence header size. */
#define HOST_OBJECT_STORE_HEAD_SIZE \
    ((uint32_t)offsetof(host_object_store_head_t, crc) + (uint32_t)sizeof(uint16_t))

/**
 * @brief Read logical bytes through the active object-store abstraction.
 * @param addr Backend byte address accepted by the object-store API.
 * @param size Byte count to read.
 * @param p_buf Output buffer.
 * @return true when bytes were read.
 */
static bool host_object_store_read_bytes(const uint32_t addr,
                                         const uint32_t size,
                                         uint8_t * const p_buf)
{
    const par_store_backend_api_t * const p_store = par_nvm_object_store_get_api();

    TEST_ASSERT(NULL != p_store);
    TEST_ASSERT(NULL != p_store->read);
    TEST_ASSERT(NULL != p_buf);
    TEST_ASSERT_OK(p_store->read(addr, size, p_buf));
    return true;
}

/**
 * @brief Write logical bytes through the active object-store abstraction.
 * @param addr Backend byte address accepted by the object-store API.
 * @param size Byte count to write.
 * @param p_buf Source buffer.
 * @return true when bytes were written and flushed.
 */
static bool host_object_store_write_bytes(const uint32_t addr,
                                          const uint32_t size,
                                          const uint8_t * const p_buf)
{
    const par_store_backend_api_t * const p_store = par_nvm_object_store_get_api();

    TEST_ASSERT(NULL != p_store);
    TEST_ASSERT(NULL != p_store->write);
    TEST_ASSERT(NULL != p_store->sync);
    TEST_ASSERT(NULL != p_buf);
    TEST_ASSERT_OK(p_store->write(addr, size, p_buf));
    TEST_ASSERT_OK(p_store->sync());
    return true;
}

/**
 * @brief Calculate the serialized object-header CRC for host test mutations.
 * @param p_head Header fields to hash.
 * @return Header CRC-16 value.
 */
static uint16_t host_object_store_calc_head_crc(const host_object_store_head_t * const p_head)
{
    uint16_t crc = PAR_IF_CRC16_INIT;

    if (NULL == p_head)
    {
        return 0U;
    }

    crc = par_if_crc16_accumulate(crc, (const uint8_t *)&p_head->version, (uint32_t)sizeof(p_head->version));
    crc = par_if_crc16_accumulate(crc, (const uint8_t *)&p_head->obj_nb, (uint32_t)sizeof(p_head->obj_nb));
    crc = par_if_crc16_accumulate(crc, (const uint8_t *)&p_head->table_id, (uint32_t)sizeof(p_head->table_id));
    crc = par_if_crc16_accumulate(crc, (const uint8_t *)&p_head->body_size, (uint32_t)sizeof(p_head->body_size));
    return crc;
}

/**
 * @brief Read and unpack the object block header.
 * @param base_addr Backend address of the object block header.
 * @param p_head Output header mirror.
 * @return true when the header was read and unpacked.
 */
static bool host_object_store_read_head(const uint32_t base_addr,
                                        host_object_store_head_t * const p_head)
{
    uint8_t head_buf[HOST_OBJECT_STORE_HEAD_SIZE] = { 0U };

    TEST_ASSERT(NULL != p_head);
    TEST_ASSERT(HOST_OBJECT_STORE_HEAD_SIZE == 18U);
    TEST_ASSERT(host_object_store_read_bytes(base_addr,
                                             (uint32_t)sizeof(head_buf),
                                             head_buf));
    memcpy(&p_head->sign, &head_buf[offsetof(host_object_store_head_t, sign)], sizeof(p_head->sign));
    memcpy(&p_head->version, &head_buf[offsetof(host_object_store_head_t, version)], sizeof(p_head->version));
    memcpy(&p_head->obj_nb, &head_buf[offsetof(host_object_store_head_t, obj_nb)], sizeof(p_head->obj_nb));
    memcpy(&p_head->table_id, &head_buf[offsetof(host_object_store_head_t, table_id)], sizeof(p_head->table_id));
    memcpy(&p_head->body_size, &head_buf[offsetof(host_object_store_head_t, body_size)], sizeof(p_head->body_size));
    memcpy(&p_head->crc, &head_buf[offsetof(host_object_store_head_t, crc)], sizeof(p_head->crc));
    return true;
}

/**
 * @brief Pack and write the object block header.
 * @param base_addr Backend address of the object block header.
 * @param p_head Header mirror to serialize.
 * @return true when the header was written and flushed.
 */
static bool host_object_store_write_head(const uint32_t base_addr,
                                         const host_object_store_head_t * const p_head)
{
    uint8_t head_buf[HOST_OBJECT_STORE_HEAD_SIZE] = { 0U };

    TEST_ASSERT(NULL != p_head);
    memcpy(&head_buf[offsetof(host_object_store_head_t, sign)], &p_head->sign, sizeof(p_head->sign));
    memcpy(&head_buf[offsetof(host_object_store_head_t, version)], &p_head->version, sizeof(p_head->version));
    memcpy(&head_buf[offsetof(host_object_store_head_t, obj_nb)], &p_head->obj_nb, sizeof(p_head->obj_nb));
    memcpy(&head_buf[offsetof(host_object_store_head_t, table_id)], &p_head->table_id, sizeof(p_head->table_id));
    memcpy(&head_buf[offsetof(host_object_store_head_t, body_size)], &p_head->body_size, sizeof(p_head->body_size));
    memcpy(&head_buf[offsetof(host_object_store_head_t, crc)], &p_head->crc, sizeof(p_head->crc));
    TEST_ASSERT(host_object_store_write_bytes(base_addr,
                                              (uint32_t)sizeof(head_buf),
                                              head_buf));
    return true;
}

/**
 * @brief Read and unpack one object record metadata header.
 * @param record_addr Backend address of the object record metadata header.
 * @param p_meta Output metadata mirror.
 * @return true when metadata was read and unpacked.
 */
static bool host_object_store_read_record_meta(const uint32_t record_addr,
                                               host_object_store_record_meta_t * const p_meta)
{
    const par_store_backend_api_t * const p_store = par_nvm_object_store_get_api();
    uint8_t meta_buf[sizeof(host_object_store_record_meta_t)] = { 0U };

    TEST_ASSERT(NULL != p_store);
    TEST_ASSERT(NULL != p_store->read);
    TEST_ASSERT(NULL != p_meta);
    TEST_ASSERT(sizeof(host_object_store_record_meta_t) == 12U);
    TEST_ASSERT(offsetof(host_object_store_record_meta_t, id) == 0U);
    TEST_ASSERT(offsetof(host_object_store_record_meta_t, type) == 2U);
    TEST_ASSERT(offsetof(host_object_store_record_meta_t, flags) == 3U);
    TEST_ASSERT(offsetof(host_object_store_record_meta_t, elem_size) == 4U);
    TEST_ASSERT(offsetof(host_object_store_record_meta_t, capacity) == 6U);
    TEST_ASSERT(offsetof(host_object_store_record_meta_t, len) == 8U);
    TEST_ASSERT(offsetof(host_object_store_record_meta_t, crc) == 10U);
    TEST_ASSERT_OK(p_store->read(record_addr,
                                 (uint32_t)sizeof(meta_buf),
                                 meta_buf));

    memcpy(&p_meta->id,
           &meta_buf[offsetof(host_object_store_record_meta_t, id)],
           sizeof(p_meta->id));
    memcpy(&p_meta->type,
           &meta_buf[offsetof(host_object_store_record_meta_t, type)],
           sizeof(p_meta->type));
    memcpy(&p_meta->flags,
           &meta_buf[offsetof(host_object_store_record_meta_t, flags)],
           sizeof(p_meta->flags));
    memcpy(&p_meta->elem_size,
           &meta_buf[offsetof(host_object_store_record_meta_t, elem_size)],
           sizeof(p_meta->elem_size));
    memcpy(&p_meta->capacity,
           &meta_buf[offsetof(host_object_store_record_meta_t, capacity)],
           sizeof(p_meta->capacity));
    memcpy(&p_meta->len,
           &meta_buf[offsetof(host_object_store_record_meta_t, len)],
           sizeof(p_meta->len));
    memcpy(&p_meta->crc,
           &meta_buf[offsetof(host_object_store_record_meta_t, crc)],
           sizeof(p_meta->crc));
    return true;
}

/**
 * @brief Return the checked serialized length-field address for an object record.
 * @details The record base is still derived from the production object NVM
 * address helper. This function reads only the record metadata header and
 * validates id, type, flags, element size, capacity, and length before
 * returning the exact length-field address.
 * @param par_num Persistent object parameter number.
 * @param expected_len Expected committed payload length.
 * @param p_len_addr Output address of the serialized length field.
 * @return true when the metadata header matches the expected object record.
 */
static bool host_object_store_get_record_len_addr(const par_num_t par_num,
                                                  const uint16_t expected_len,
                                                  uint32_t * const p_len_addr)
{
    const par_cfg_t * const p_cfg = par_get_config(par_num);
    const uint32_t base_addr = PAR_CFG_NVM_OBJECT_FIXED_ADDR;
    const uint32_t block_size = par_nvm_object_get_block_size();
    const uint32_t record_addr = par_nvm_object_get_addr(base_addr, par_num);
    host_object_store_record_meta_t meta = { 0 };
    uint16_t expected_id = 0U;
    uint16_t expected_capacity = 0U;
    uint32_t record_offset = 0U;

    TEST_ASSERT(NULL != p_len_addr);
    TEST_ASSERT(NULL != p_cfg);
    TEST_ASSERT_OK(par_get_id_by_num(par_num, &expected_id));
    TEST_ASSERT_OK(par_get_obj_capacity(par_num, &expected_capacity));
    TEST_ASSERT(record_addr >= base_addr);
    record_offset = record_addr - base_addr;
    TEST_ASSERT(block_size >= record_offset);
    TEST_ASSERT(((uint32_t)sizeof(host_object_store_record_meta_t)) <=
                (block_size - record_offset));
    TEST_ASSERT(host_object_store_read_record_meta(record_addr, &meta));
    TEST_ASSERT(meta.id == expected_id);
    TEST_ASSERT(meta.type == (uint8_t)p_cfg->type);
    TEST_ASSERT(meta.flags == HOST_OBJECT_STORE_RECORD_FLAGS_NONE);
    TEST_ASSERT(meta.elem_size == p_cfg->value_cfg.object.elem_size);
    TEST_ASSERT(meta.capacity == expected_capacity);
    TEST_ASSERT(meta.len == expected_len);

    *p_len_addr = record_addr +
                  (uint32_t)offsetof(host_object_store_record_meta_t, len);
    return true;
}

/**
 * @brief Return a checked serialized field address for an object record.
 * @param par_num Persistent object parameter number.
 * @param expected_len Expected committed payload length.
 * @param field_offset Offset inside host_object_store_record_meta_t or payload.
 * @param p_field_addr Output serialized field address.
 * @return true when the metadata header matches the expected object record.
 */
static bool host_object_store_get_record_field_addr(const par_num_t par_num,
                                                    const uint16_t expected_len,
                                                    const uint32_t field_offset,
                                                    uint32_t * const p_field_addr)
{
    const uint32_t base_addr = PAR_CFG_NVM_OBJECT_FIXED_ADDR;
    const uint32_t record_addr = par_nvm_object_get_addr(base_addr, par_num);
    uint32_t len_addr = 0U;

    TEST_ASSERT(NULL != p_field_addr);
    TEST_ASSERT(host_object_store_get_record_len_addr(par_num, expected_len, &len_addr));
    (void)len_addr;
    *p_field_addr = record_addr + field_offset;
    return true;
}


/** @brief Verify BYTES payload CRC corruption restores the default payload. */
static bool test_nvm_object_bytes_payload_crc_corruption_restores_default(void)
{
    uint8_t bytes[4] = { 0U };
    uint16_t len = 0U;
    uint32_t crc_addr = 0U;
    const uint8_t payload[4] = { 4U, 3U, 2U, 1U };

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_bytes(ePAR_TEST_BYTES, payload, (uint16_t)sizeof(payload)));
    TEST_ASSERT_OK(par_save(ePAR_TEST_BYTES));
    TEST_ASSERT(host_object_store_get_record_field_addr(ePAR_TEST_BYTES,
                                                        (uint16_t)sizeof(payload),
                                                        (uint32_t)offsetof(host_object_store_record_meta_t, crc),
                                                        &crc_addr));
    TEST_ASSERT(host_object_store_xor_byte(crc_addr, 0x01U));
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_bytes(ePAR_TEST_BYTES, bytes, sizeof(bytes), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(bytes[0] == 0x01U);
    TEST_ASSERT(bytes[1] == 0x02U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}


#if defined(PAR_HOST_TEST_OBJECT_ARRAY_NVM)
/** @brief Verify ARR_U16 element-size corruption restores the default payload. */
static bool test_nvm_object_arr_u16_elem_size_corruption_restores_default(void)
{
    uint16_t arr[2] = { 0U };
    uint16_t count = 0U;
    uint32_t elem_size_addr = 0U;
    const uint16_t payload[2] = { 300U, 400U };

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_arr_u16(ePAR_TEST_ARR_U16, payload, 2U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_ARR_U16));
    TEST_ASSERT(host_object_store_get_record_field_addr(ePAR_TEST_ARR_U16,
                                                        (uint16_t)(2U * sizeof(uint16_t)),
                                                        (uint32_t)offsetof(host_object_store_record_meta_t, elem_size),
                                                        &elem_size_addr));
    TEST_ASSERT(host_object_store_xor_byte(elem_size_addr, 0x01U));
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_arr_u16(ePAR_TEST_ARR_U16, arr, 2U, &count));
    TEST_ASSERT(count == 2U);
    TEST_ASSERT(arr[0] == 100U);
    TEST_ASSERT(arr[1] == 200U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify ARR_U32 capacity corruption restores the default payload. */
static bool test_nvm_object_arr_u32_capacity_corruption_restores_default(void)
{
    uint32_t arr[2] = { 0UL };
    uint16_t count = 0U;
    uint32_t capacity_addr = 0U;
    const uint32_t payload[2] = { 3000UL, 4000UL };

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_arr_u32(ePAR_TEST_ARR_U32, payload, 2U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_ARR_U32));
    TEST_ASSERT(host_object_store_get_record_field_addr(ePAR_TEST_ARR_U32,
                                                        (uint16_t)(2U * sizeof(uint32_t)),
                                                        (uint32_t)offsetof(host_object_store_record_meta_t, capacity),
                                                        &capacity_addr));
    TEST_ASSERT(host_object_store_xor_byte(capacity_addr, 0x01U));
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_arr_u32(ePAR_TEST_ARR_U32, arr, 2U, &count));
    TEST_ASSERT(count == 2U);
    TEST_ASSERT(arr[0] == 1000UL);
    TEST_ASSERT(arr[1] == 2000UL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* defined(PAR_HOST_TEST_OBJECT_ARRAY_NVM) */

#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) && (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_FIXED) */
