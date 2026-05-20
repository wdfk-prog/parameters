/**
 * @brief Return the process-unique fake flash image path.
 * @return Null-terminated host flash image path.
 */
static const char *host_flash_image_path(void)
{
    const char * const env_path = getenv("PAR_HOST_FLASH_IMAGE_PATH");

    if ((NULL != env_path) && ('\0' != env_path[0]))
    {
        return env_path;
    }

    if ('\0' == g_flash_image_path[0])
    {
        const int written = snprintf(g_flash_image_path,
                                     sizeof(g_flash_image_path),
                                     "/tmp/autogen_pm_flash_ee_host_%ld.bin",
                                     (long)getpid());

        if ((written < 0) || ((size_t)written >= sizeof(g_flash_image_path)))
        {
            fprintf(stderr, "host flash image path is too long\n");
            abort();
        }
    }

    return g_flash_image_path;
}

/**
 * @brief Return the process-unique dedicated object fake flash image path.
 * @return Null-terminated host object flash image path.
 */
static const char *host_object_flash_image_path(void)
{
    const char * const env_path = getenv("PAR_HOST_OBJECT_FLASH_IMAGE_PATH");

    if ((NULL != env_path) && ('\0' != env_path[0]))
    {
        return env_path;
    }

    if ('\0' == g_object_flash_image_path[0])
    {
        const int written = snprintf(g_object_flash_image_path,
                                     sizeof(g_object_flash_image_path),
                                     "%s.object",
                                     host_flash_image_path());

        if ((written < 0) || ((size_t)written >= sizeof(g_object_flash_image_path)))
        {
            fprintf(stderr, "host object flash image path is too long\n");
            abort();
        }
    }

    return g_object_flash_image_path;
}

/**
 * @brief Write the current dedicated object fake flash image to disk.
 */
static void host_object_flash_write_image(void)
{
    FILE *fp = fopen(host_object_flash_image_path(), "wb");

    if (NULL == fp)
    {
        perror("fopen host object flash image");
        abort();
    }
    if (HOST_FLASH_SIZE != fwrite(g_object_flash, 1U, HOST_FLASH_SIZE, fp))
    {
        perror("fwrite host object flash image");
        abort();
    }
    if (0 != fclose(fp))
    {
        perror("fclose host object flash image");
        abort();
    }
}

/**
 * @brief Load the dedicated object fake flash image from disk.
 */
static void host_object_flash_load_image(void)
{
    FILE *fp = fopen(host_object_flash_image_path(), "rb");

    if (NULL == fp)
    {
        memset(g_object_flash, 0xFF, sizeof(g_object_flash));
        host_object_flash_write_image();
        return;
    }

    if (HOST_FLASH_SIZE != fread(g_object_flash, 1U, HOST_FLASH_SIZE, fp))
    {
        perror("fread host object flash image");
        abort();
    }
    if (0 != fclose(fp))
    {
        perror("fclose host object flash image");
        abort();
    }
}

/**
 * @brief Remove the process-local fake flash image when host tests finish.
 */
static void host_flash_remove_image(void)
{
    const char * const env_path = getenv("PAR_HOST_FLASH_IMAGE_PATH");
    const char * const object_env_path = getenv("PAR_HOST_OBJECT_FLASH_IMAGE_PATH");

    if ((NULL == env_path) || ('\0' == env_path[0]))
    {
        if ('\0' != g_flash_image_path[0])
        {
            (void)remove(g_flash_image_path);
        }
    }
    if ((NULL == object_env_path) || ('\0' == object_env_path[0]))
    {
        if ('\0' != g_object_flash_image_path[0])
        {
            (void)remove(g_object_flash_image_path);
        }
    }
}

/**
 * @brief Write the current fake flash image to the host persistence file.
 */
static void host_flash_write_image(void)
{
    FILE *fp = fopen(host_flash_image_path(), "wb");

    if (NULL == fp)
    {
        perror("fopen host flash image");
        abort();
    }
    if (HOST_FLASH_SIZE != fwrite(g_flash, 1U, HOST_FLASH_SIZE, fp))
    {
        perror("fwrite host flash image");
        abort();
    }
    if (0 != fclose(fp))
    {
        perror("fclose host flash image");
        abort();
    }
}

/**
 * @brief Load the fake flash image from the host persistence file.
 */
static void host_flash_load_image(void)
{
    FILE *fp = fopen(host_flash_image_path(), "rb");

    if (NULL == fp)
    {
        memset(g_flash, 0xFF, sizeof(g_flash));
        host_flash_write_image();
        return;
    }

    if (HOST_FLASH_SIZE != fread(g_flash, 1U, HOST_FLASH_SIZE, fp))
    {
        perror("fread host flash image");
        abort();
    }
    if (0 != fclose(fp))
    {
        perror("fclose host flash image");
        abort();
    }
}

/** @brief Reset fake flash content to the erased state. */
static void host_flash_reset_erased(void)
{
    memset(g_flash, 0xFF, sizeof(g_flash));
    host_flash_write_image();
    g_flash_is_init = false;
    g_object_flash_is_init = false;
    memset(g_object_flash, 0xFF, sizeof(g_object_flash));
    host_object_flash_write_image();
    g_fail_program_after = HOST_FLASH_FAIL_DISABLED;
    g_fail_program_addr = 0U;
    g_fail_program_size = 0U;
    g_fail_program_hit = false;
    g_fail_read_after = HOST_FLASH_FAIL_DISABLED;
    g_fail_erase_after = HOST_FLASH_FAIL_DISABLED;
    g_corrupt_program_after = HOST_FLASH_FAIL_DISABLED;
    g_corrupt_read_after = HOST_FLASH_FAIL_DISABLED;
    g_object_fail_write_after = HOST_FLASH_FAIL_DISABLED;
    g_object_fail_read_after = HOST_FLASH_FAIL_DISABLED;
    g_object_fail_erase_after = HOST_FLASH_FAIL_DISABLED;
    g_object_fail_sync_after = HOST_FLASH_FAIL_DISABLED;
    g_object_corrupt_write_after = HOST_FLASH_FAIL_DISABLED;
    g_object_corrupt_read_after = HOST_FLASH_FAIL_DISABLED;
}

/** @brief Clear transient fake flash failpoints without erasing contents. */
static void host_flash_clear_failpoints(void)
{
    g_fail_program_after = HOST_FLASH_FAIL_DISABLED;
    g_fail_program_addr = 0U;
    g_fail_program_size = 0U;
    g_fail_program_hit = false;
    g_fail_read_after = HOST_FLASH_FAIL_DISABLED;
    g_fail_erase_after = HOST_FLASH_FAIL_DISABLED;
    g_corrupt_program_after = HOST_FLASH_FAIL_DISABLED;
    g_corrupt_read_after = HOST_FLASH_FAIL_DISABLED;
    g_object_fail_write_after = HOST_FLASH_FAIL_DISABLED;
    g_object_fail_read_after = HOST_FLASH_FAIL_DISABLED;
    g_object_fail_erase_after = HOST_FLASH_FAIL_DISABLED;
    g_object_fail_sync_after = HOST_FLASH_FAIL_DISABLED;
    g_object_corrupt_write_after = HOST_FLASH_FAIL_DISABLED;
    g_object_corrupt_read_after = HOST_FLASH_FAIL_DISABLED;
}

/** @brief Flip one byte in fake flash to emulate physical corruption. */
static void host_flash_xor_byte(const uint32_t addr, const uint8_t mask)
{
    if (addr < HOST_FLASH_SIZE)
    {
        g_flash[addr] ^= mask;
        host_flash_write_image();
    }
}

/**
 * @brief Host mirror of the Flash-EE bank header fields used by tests.
 */
typedef struct
{
    uint32_t magic;        /**< Header magic used to identify a bank. */
    uint32_t version;      /**< Serialized backend format version. */
    uint32_t logical_size; /**< Exposed logical EEPROM size in bytes. */
    uint32_t line_size;    /**< Fixed payload size carried by each record. */
    uint32_t record_size;  /**< Total append-record size. */
    uint32_t bank_size;    /**< Physical size of one bank. */
    uint32_t seq;          /**< Monotonic bank sequence. */
    uint32_t cfg_crc;      /**< Geometry CRC. */
    uint32_t state;        /**< Bank state word. */
    uint32_t reserved[7];  /**< Reserved erased words. */
} host_flash_ee_bank_header_t;

/**
 * @brief Host mirror of Flash-EE append-record metadata.
 */
typedef struct
{
    uint32_t line_index;   /**< Logical line index updated by the record. */
    uint16_t payload_size; /**< Payload byte count. */
    uint16_t record_crc;   /**< Payload and semantic metadata CRC. */
    uint32_t reserved;     /**< Reserved metadata word. */
} host_flash_ee_record_meta_t;

/**
 * @brief Return whether a physical Flash-EE record slot is fully erased.
 * @param p_buf Record bytes to inspect.
 * @param size Record byte count.
 * @return true when all bytes are erased.
 */
static bool host_flash_ee_is_erased(const uint8_t * const p_buf,
                                    const uint32_t size)
{
    for (uint32_t idx = 0U; idx < size; idx++)
    {
        if (0xFFU != p_buf[idx])
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Locate the active Flash-EE bank and first free record slot.
 * @param p_bank_base Output physical bank base address.
 * @param p_next_offset Output next record offset inside the bank.
 * @return true when an active bank was found.
 */
static bool host_flash_ee_find_active_bank(uint32_t * const p_bank_base,
                                           uint32_t * const p_next_offset)
{
    uint32_t best_seq = 0U;
    bool found = false;

    TEST_ASSERT(NULL != p_bank_base);
    TEST_ASSERT(NULL != p_next_offset);

    for (uint32_t bank = 0U; bank < HOST_FLASH_EE_BANK_COUNT; bank++)
    {
        const uint32_t bank_base = bank * HOST_FLASH_EE_BANK_SIZE;
        host_flash_ee_bank_header_t header = { 0 };
        uint32_t offset = HOST_FLASH_EE_HEADER_SIZE;

        memcpy(&header, &g_flash[bank_base], sizeof(header));
        if ((HOST_FLASH_EE_HEADER_MAGIC != header.magic) ||
            (HOST_FLASH_EE_HEADER_ACTIVE != header.state))
        {
            continue;
        }

        while ((offset + HOST_FLASH_EE_RECORD_SIZE) <= HOST_FLASH_EE_BANK_SIZE)
        {
            if (host_flash_ee_is_erased(&g_flash[bank_base + offset],
                                        HOST_FLASH_EE_RECORD_SIZE))
            {
                break;
            }
            offset += HOST_FLASH_EE_RECORD_SIZE;
        }

        if ((false == found) || (header.seq >= best_seq))
        {
            best_seq = header.seq;
            *p_bank_base = bank_base;
            *p_next_offset = offset;
            found = true;
        }
    }

    return found;
}
