/**
 * @brief Check whether a raw fake flash operation fits in the host region.
 * @param addr Byte address inside the host fake flash region.
 * @param size Number of bytes requested.
 * @return true when the requested range is valid, otherwise false.
 */
static bool host_flash_range_is_valid(const uint32_t addr, const uint32_t size)
{
    return (addr <= HOST_FLASH_SIZE) && (size <= (HOST_FLASH_SIZE - addr));
}


/** @brief Direct Flash-EE test port with mutable geometry. */
typedef struct
{
    uint8_t flash[HOST_FLASH_SIZE]; /**< Backing flash bytes. */
    uint32_t region_size;           /**< Reported region size. */
    uint32_t erase_size;            /**< Reported erase granularity. */
    uint32_t program_size;          /**< Reported program granularity. */
    bool is_init;                   /**< Port initialization state. */
} host_flash_ee_geom_ctx_t;

/** @brief Initialize a direct geometry-test flash port. */
static par_status_t host_flash_ee_geom_init(void *p_ctx)
{
    host_flash_ee_geom_ctx_t * const p_geom = (host_flash_ee_geom_ctx_t *)p_ctx;

    if ((NULL == p_geom) || (p_geom->region_size > HOST_FLASH_SIZE))
    {
        return ePAR_ERROR;
    }
    p_geom->is_init = true;
    return ePAR_OK;
}

/** @brief Deinitialize a direct geometry-test flash port. */
static par_status_t host_flash_ee_geom_deinit(void *p_ctx)
{
    host_flash_ee_geom_ctx_t * const p_geom = (host_flash_ee_geom_ctx_t *)p_ctx;

    if (NULL != p_geom)
    {
        p_geom->is_init = false;
    }
    return ePAR_OK;
}

/** @brief Report direct geometry-test port initialization state. */
static void host_flash_ee_geom_is_init(const void *p_ctx, bool *p_is_init)
{
    const host_flash_ee_geom_ctx_t * const p_geom = (const host_flash_ee_geom_ctx_t *)p_ctx;

    if (NULL != p_is_init)
    {
        *p_is_init = ((NULL != p_geom) && (true == p_geom->is_init));
    }
}

/** @brief Read bytes from a direct geometry-test flash port. */
static par_status_t host_flash_ee_geom_read(const void *p_ctx,
                                            uint32_t addr,
                                            uint32_t size,
                                            uint8_t *p_buf)
{
    const host_flash_ee_geom_ctx_t * const p_geom = (const host_flash_ee_geom_ctx_t *)p_ctx;

    if ((NULL == p_geom) || (NULL == p_buf) || (false == p_geom->is_init) ||
        (p_geom->region_size > HOST_FLASH_SIZE) ||
        (false == host_flash_range_is_valid(addr, size)) ||
        (addr > p_geom->region_size) || (size > (p_geom->region_size - addr)))
    {
        return ePAR_ERROR;
    }
    memcpy(p_buf, &p_geom->flash[addr], size);
    return ePAR_OK;
}

/** @brief Program bytes into a direct geometry-test flash port. */
static par_status_t host_flash_ee_geom_program(const void *p_ctx,
                                               uint32_t addr,
                                               uint32_t size,
                                               const uint8_t *p_buf)
{
    host_flash_ee_geom_ctx_t * const p_geom = (host_flash_ee_geom_ctx_t *)p_ctx;

    if ((NULL == p_geom) || (NULL == p_buf) || (false == p_geom->is_init) ||
        (p_geom->region_size > HOST_FLASH_SIZE) ||
        (false == host_flash_range_is_valid(addr, size)) ||
        (addr > p_geom->region_size) || (size > (p_geom->region_size - addr)))
    {
        return ePAR_ERROR;
    }
    for (uint32_t idx = 0U; idx < size; idx++)
    {
        if ((p_geom->flash[addr + idx] | p_buf[idx]) != p_geom->flash[addr + idx])
        {
            return ePAR_ERROR;
        }
    }
    for (uint32_t idx = 0U; idx < size; idx++)
    {
        p_geom->flash[addr + idx] &= p_buf[idx];
    }
    return ePAR_OK;
}

/** @brief Erase bytes in a direct geometry-test flash port. */
static par_status_t host_flash_ee_geom_erase(const void *p_ctx,
                                             uint32_t addr,
                                             uint32_t size)
{
    host_flash_ee_geom_ctx_t * const p_geom = (host_flash_ee_geom_ctx_t *)p_ctx;

    if ((NULL == p_geom) || (false == p_geom->is_init) ||
        (p_geom->region_size > HOST_FLASH_SIZE) ||
        (false == host_flash_range_is_valid(addr, size)) ||
        (addr > p_geom->region_size) || (size > (p_geom->region_size - addr)))
    {
        return ePAR_ERROR;
    }
    memset(&p_geom->flash[addr], 0xFF, size);
    return ePAR_OK;
}

/** @brief Return the direct geometry-test region size. */
static uint32_t host_flash_ee_geom_region_size(const void *p_ctx)
{
    return ((const host_flash_ee_geom_ctx_t *)p_ctx)->region_size;
}

/** @brief Return the direct geometry-test erase size. */
static uint32_t host_flash_ee_geom_erase_size(const void *p_ctx)
{
    return ((const host_flash_ee_geom_ctx_t *)p_ctx)->erase_size;
}

/** @brief Return the direct geometry-test program size. */
static uint32_t host_flash_ee_geom_program_size(const void *p_ctx)
{
    return ((const host_flash_ee_geom_ctx_t *)p_ctx)->program_size;
}

/** @brief Return the direct geometry-test port name. */
static const char *host_flash_ee_geom_name(const void *p_ctx)
{
    (void)p_ctx;
    return "host-geom";
}

/** @brief Direct geometry-test port API table. */
static const par_store_flash_ee_port_api_t g_host_flash_ee_geom_port_api = {
    .init = host_flash_ee_geom_init,
    .deinit = host_flash_ee_geom_deinit,
    .is_init = host_flash_ee_geom_is_init,
    .read = host_flash_ee_geom_read,
    .program = host_flash_ee_geom_program,
    .erase = host_flash_ee_geom_erase,
    .get_region_size = host_flash_ee_geom_region_size,
    .get_erase_size = host_flash_ee_geom_erase_size,
    .get_program_size = host_flash_ee_geom_program_size,
    .get_name = host_flash_ee_geom_name,
};

/**
 * @brief Return whether a forked host test child exited successfully.
 * @param child_pid PID returned by fork().
 * @return true when the child exits with status 0, otherwise false.
 */
static bool host_child_exit_is_success(const pid_t child_pid)
{
    int status = 0;

    if (child_pid <= 0)
    {
        return false;
    }
    if (child_pid != waitpid(child_pid, &status, 0))
    {
        return false;
    }

    return (WIFEXITED(status) && (0 == WEXITSTATUS(status)));
}

/**
 * @brief Exit a forked host test child from a boolean test result.
 * @param is_success Child test result.
 */
static void host_child_exit_from_result(const bool is_success)
{
    _exit(is_success ? 0 : 1);
}

/** @brief Initialize the native host fake flash port. */
par_status_t par_store_flash_ee_native_port_init(void)
{
    host_flash_load_image();
    g_flash_is_init = true;
    return ePAR_OK;
}

/** @brief Deinitialize the native host fake flash port. */
par_status_t par_store_flash_ee_native_port_deinit(void)
{
    g_flash_is_init = false;
    return ePAR_OK;
}

/** @brief Read raw bytes from the host fake flash. */
par_status_t par_store_flash_ee_native_port_read(uint32_t addr, uint32_t size, uint8_t *p_buf)
{
    if ((!g_flash_is_init) || (NULL == p_buf) || (false == host_flash_range_is_valid(addr, size)))
    {
        return ePAR_ERROR;
    }
    if (0 == g_fail_read_after)
    {
        return ePAR_ERROR;
    }
    if (g_fail_read_after > 0)
    {
        g_fail_read_after--;
    }

    memcpy(p_buf, &g_flash[addr], size);
    if ((size > 0U) && (0 == g_corrupt_read_after))
    {
        p_buf[0] ^= 0x01U;
    }
    if (g_corrupt_read_after > 0)
    {
        g_corrupt_read_after--;
    }
    return ePAR_OK;
}

/** @brief Program raw bytes into the host fake flash with one-to-zero semantics. */
par_status_t par_store_flash_ee_native_port_program(uint32_t addr, uint32_t size, const uint8_t *p_buf)
{
    if ((!g_flash_is_init) || (NULL == p_buf) || (false == host_flash_range_is_valid(addr, size)))
    {
        return ePAR_ERROR;
    }
    if (0 == g_fail_program_after)
    {
        g_fail_program_addr = addr;
        g_fail_program_size = size;
        g_fail_program_hit = true;
        return ePAR_ERROR;
    }
    if (g_fail_program_after > 0)
    {
        g_fail_program_after--;
    }

    for (uint32_t i = 0U; i < size; i++)
    {
        if ((uint8_t)(~g_flash[addr + i] & p_buf[i]) != 0U)
        {
            return ePAR_ERROR;
        }
    }
    for (uint32_t i = 0U; i < size; i++)
    {
        g_flash[addr + i] &= p_buf[i];
    }

    if ((size > 0U) && (0 == g_corrupt_program_after))
    {
        g_flash[addr] ^= 0x01U;
    }
    if (g_corrupt_program_after > 0)
    {
        g_corrupt_program_after--;
    }

    host_flash_write_image();
    return ePAR_OK;
}

/** @brief Erase raw bytes in the host fake flash. */
par_status_t par_store_flash_ee_native_port_erase(uint32_t addr, uint32_t size)
{
    if ((!g_flash_is_init) || (false == host_flash_range_is_valid(addr, size)) ||
        ((addr % HOST_FLASH_ERASE_SIZE) != 0U) || ((size % HOST_FLASH_ERASE_SIZE) != 0U))
    {
        return ePAR_ERROR;
    }
    if (0 == g_fail_erase_after)
    {
        return ePAR_ERROR;
    }
    if (g_fail_erase_after > 0)
    {
        g_fail_erase_after--;
    }

    memset(&g_flash[addr], 0xFF, size);
    host_flash_write_image();
    return ePAR_OK;
}

/** @brief Return the host fake flash region size. */
uint32_t par_store_flash_ee_native_port_region_size(void)
{
    return HOST_FLASH_SIZE;
}

/** @brief Return the host fake flash erase granularity. */
uint32_t par_store_flash_ee_native_port_erase_size(void)
{
    return HOST_FLASH_ERASE_SIZE;
}

/** @brief Return the host fake flash program granularity. */
uint32_t par_store_flash_ee_native_port_program_size(void)
{
    return HOST_FLASH_PROGRAM_SIZE;
}

/** @brief Return the host fake flash diagnostic name. */
const char *par_store_flash_ee_native_port_name(void)
{
    return "host-flash";
}

/** @brief Initialize the dedicated object-store fake backend. */
static par_status_t host_object_backend_init(void)
{
    host_object_flash_load_image();
    g_object_flash_is_init = true;
    return ePAR_OK;
}

/** @brief Deinitialize the dedicated object-store fake backend. */
static par_status_t host_object_backend_deinit(void)
{
    g_object_flash_is_init = false;
    return ePAR_OK;
}

/** @brief Query whether the dedicated object-store fake backend is initialized. */
static void host_object_backend_is_init(bool *const p_is_init)
{
    if (NULL != p_is_init)
    {
        *p_is_init = g_object_flash_is_init;
    }
}

/** @brief Read bytes from the dedicated object-store fake backend. */
static par_status_t host_object_backend_read(const uint32_t addr,
                                             const uint32_t size,
                                             uint8_t *const p_buf)
{
    if ((!g_object_flash_is_init) || (NULL == p_buf) ||
        (false == host_flash_range_is_valid(addr, size)))
    {
        return ePAR_ERROR;
    }

    if (0 == g_object_fail_read_after)
    {
        return ePAR_ERROR;
    }
    if (g_object_fail_read_after > 0)
    {
        g_object_fail_read_after--;
    }

    memcpy(p_buf, &g_object_flash[addr], size);
    if ((size > 0U) && (0 == g_object_corrupt_read_after))
    {
        p_buf[0] ^= 0x01U;
    }
    if (g_object_corrupt_read_after > 0)
    {
        g_object_corrupt_read_after--;
    }
    return ePAR_OK;
}

/** @brief Write bytes into the dedicated object-store fake backend. */
static par_status_t host_object_backend_write(const uint32_t addr,
                                              const uint32_t size,
                                              const uint8_t *const p_buf)
{
    if ((!g_object_flash_is_init) || (NULL == p_buf) ||
        (false == host_flash_range_is_valid(addr, size)))
    {
        return ePAR_ERROR;
    }

    if (0 == g_object_fail_write_after)
    {
        return ePAR_ERROR;
    }
    if (g_object_fail_write_after > 0)
    {
        g_object_fail_write_after--;
    }

    memcpy(&g_object_flash[addr], p_buf, size);
    if ((size > 0U) && (0 == g_object_corrupt_write_after))
    {
        g_object_flash[addr] ^= 0x01U;
    }
    if (g_object_corrupt_write_after > 0)
    {
        g_object_corrupt_write_after--;
    }
    host_object_flash_write_image();
    return ePAR_OK;
}

/** @brief Erase bytes in the dedicated object-store fake backend. */
static par_status_t host_object_backend_erase(const uint32_t addr, const uint32_t size)
{
    if ((!g_object_flash_is_init) || (false == host_flash_range_is_valid(addr, size)))
    {
        return ePAR_ERROR;
    }

    if (0 == g_object_fail_erase_after)
    {
        return ePAR_ERROR;
    }
    if (g_object_fail_erase_after > 0)
    {
        g_object_fail_erase_after--;
    }

    memset(&g_object_flash[addr], 0xFF, size);
    host_object_flash_write_image();
    return ePAR_OK;
}

/** @brief Flush the dedicated object-store fake backend. */
static par_status_t host_object_backend_sync(void)
{
    if (!g_object_flash_is_init)
    {
        return ePAR_ERROR;
    }

    if (0 == g_object_fail_sync_after)
    {
        return ePAR_ERROR;
    }
    if (g_object_fail_sync_after > 0)
    {
        g_object_fail_sync_after--;
    }

    host_object_flash_write_image();
    return ePAR_OK;
}

/** @brief Dedicated object-store backend API used by host matrix tests. */
static const par_store_backend_api_t g_host_object_backend_api = {
    .init = host_object_backend_init,
    .deinit = host_object_backend_deinit,
    .is_init = host_object_backend_is_init,
    .read = host_object_backend_read,
    .write = host_object_backend_write,
    .erase = host_object_backend_erase,
    .sync = host_object_backend_sync,
    .name = "host-object-flash",
};

/** @brief Bind the dedicated object-store fake backend. */
par_status_t par_object_store_backend_bind(void)
{
    return ePAR_OK;
}

/** @brief Return the dedicated object-store fake backend API. */
const par_store_backend_api_t *par_object_store_backend_get_api(void)
{
    return &g_host_object_backend_api;
}
