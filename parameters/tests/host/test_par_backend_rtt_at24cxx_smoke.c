/**
 * @file test_par_backend_rtt_at24cxx_smoke.c
 * @brief Smoke-test the RT-Thread AT24CXX backend with host stubs.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include "test_host_common.h"
#include "par_store_backend.h"

#include "at24cxx.h"

/** @brief Disable countdown AT24CXX transfer failpoints. */
#define AT24_FAIL_DISABLED (0)
/** @brief Maximum AT24CXX transfer trace entries kept by the host stub. */
#define AT24_TRACE_MAX (8U)

/** @brief Concrete host AT24CXX stub object. */
struct autogen_pm_ci_at24_device
{
    uint8_t mem[AT24CXX_MAX_MEM_ADDRESS]; /**< EEPROM bytes. */
};

/** @brief Host AT24CXX stub instance. */
static struct autogen_pm_ci_at24_device g_at24_dev;
/** @brief AT24CXX device availability switch. */
static bool g_at24_available;
/** @brief Read failure countdown; zero disables it. */
static int g_at24_fail_read;
/** @brief Write failure countdown; zero disables it. */
static int g_at24_fail_write;
/** @brief Number of traced AT24CXX read operations. */
static uint8_t g_at24_read_count;
/** @brief Number of traced AT24CXX write operations. */
static uint8_t g_at24_write_count;
/** @brief Absolute addresses used by traced AT24CXX write operations. */
static uint32_t g_at24_write_addr[AT24_TRACE_MAX];
/** @brief Sizes used by traced AT24CXX write operations. */
static uint16_t g_at24_write_size[AT24_TRACE_MAX];

/** @brief Reset the mutable AT24CXX host stub state. */
static void at24_stub_reset(void)
{
    memset(&g_at24_dev, 0xFF, sizeof(g_at24_dev));
    g_at24_available = false;
    g_at24_fail_read = AT24_FAIL_DISABLED;
    g_at24_fail_write = AT24_FAIL_DISABLED;
    g_at24_read_count = 0U;
    g_at24_write_count = 0U;
    memset(g_at24_write_addr, 0, sizeof(g_at24_write_addr));
    memset(g_at24_write_size, 0, sizeof(g_at24_write_size));
}

at24cxx_device_t at24cxx_init(const char *i2c_bus_name, uint8_t addr_input)
{
    (void)i2c_bus_name;
    (void)addr_input;
    return g_at24_available ? &g_at24_dev : RT_NULL;
}

void at24cxx_deinit(at24cxx_device_t dev)
{
    (void)dev;
}

int at24cxx_check(at24cxx_device_t dev)
{
    return (RT_NULL != dev) ? RT_EOK : -1;
}

int at24cxx_page_read(at24cxx_device_t dev, uint32_t addr, uint8_t *buf, uint16_t size)
{
    if ((RT_NULL == dev) || (NULL == buf) ||
        ((addr > AT24CXX_MAX_MEM_ADDRESS) || (size > (AT24CXX_MAX_MEM_ADDRESS - addr))))
    {
        return -1;
    }
    if (g_at24_fail_read > AT24_FAIL_DISABLED)
    {
        g_at24_fail_read--;
        if (AT24_FAIL_DISABLED == g_at24_fail_read)
        {
            return -1;
        }
    }
    g_at24_read_count++;
    memcpy(buf, &dev->mem[addr], size);
    return RT_EOK;
}

int at24cxx_page_write(at24cxx_device_t dev, uint32_t addr, const uint8_t *buf, uint16_t size)
{
    if ((RT_NULL == dev) || (NULL == buf) ||
        ((addr > AT24CXX_MAX_MEM_ADDRESS) || (size > (AT24CXX_MAX_MEM_ADDRESS - addr))))
    {
        return -1;
    }
    if (g_at24_fail_write > AT24_FAIL_DISABLED)
    {
        g_at24_fail_write--;
        if (AT24_FAIL_DISABLED == g_at24_fail_write)
        {
            return -1;
        }
    }
    if (g_at24_write_count < AT24_TRACE_MAX)
    {
        g_at24_write_addr[g_at24_write_count] = addr;
        g_at24_write_size[g_at24_write_count] = size;
    }
    g_at24_write_count++;
    memcpy(&dev->mem[addr], buf, size);
    return RT_EOK;
}

/** @brief Verify AT24CXX backend missing-device and read/write paths. */
static bool test_rtt_at24cxx_adapter_smoke(void)
{
    const par_store_backend_api_t *api;
    uint8_t readback = 0U;
    const uint8_t value = 0x5AU;

    at24_stub_reset();
    TEST_ASSERT_OK(par_store_backend_bind());
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_STATUS(api->init(), ePAR_ERROR_INIT);

    g_at24_available = true;
    TEST_ASSERT_OK(api->init());
    TEST_ASSERT_OK(api->write(0U, 1U, &value));
    TEST_ASSERT_OK(api->read(0U, 1U, &readback));
    TEST_ASSERT(readback == value);
    TEST_ASSERT_OK(api->erase(0U, 1U));
    TEST_ASSERT_OK(api->read(0U, 1U, &readback));
    TEST_ASSERT(readback == 0xFFU);
    TEST_ASSERT_STATUS(api->read(UINT32_MAX, 1U, &readback), ePAR_ERROR_NVM);
    TEST_ASSERT_OK(api->deinit());
    return true;
}

/** @brief Verify writes larger than one page are split at AT24CXX page granularity. */
static bool test_rtt_at24cxx_page_split_write(void)
{
    const par_store_backend_api_t *api;
    uint8_t payload[AT24CXX_PAGE_BYTE + 3U];
    uint32_t expected_addr = (uint32_t)PAR_CFG_RTT_AT24_BASE_ADDR;
    uint32_t remaining = (uint32_t)sizeof(payload);
    uint8_t expected_count = 0U;

    for (uint8_t i = 0U; i < (uint8_t)sizeof(payload); i++)
    {
        payload[i] = (uint8_t)(0x10U + i);
    }

    at24_stub_reset();
    g_at24_available = true;
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    TEST_ASSERT_OK(api->write(0U, (uint32_t)sizeof(payload), payload));
    while (remaining > 0U)
    {
        uint32_t xfer = (uint32_t)AT24CXX_PAGE_BYTE -
                        (expected_addr % (uint32_t)AT24CXX_PAGE_BYTE);

        if (xfer > remaining)
        {
            xfer = remaining;
        }

        TEST_ASSERT(expected_count < AT24_TRACE_MAX);
        TEST_ASSERT(g_at24_write_addr[expected_count] == expected_addr);
        TEST_ASSERT(g_at24_write_size[expected_count] == (uint16_t)xfer);
        expected_addr += xfer;
        remaining -= xfer;
        expected_count++;
    }
    TEST_ASSERT(g_at24_write_count == expected_count);
    TEST_ASSERT(0 == memcmp(&g_at24_dev.mem[PAR_CFG_RTT_AT24_BASE_ADDR], payload, sizeof(payload)));
    TEST_ASSERT_OK(api->deinit());
    return true;
}

/** @brief Verify non-page-aligned writes split at the remaining page window. */
static bool test_rtt_at24cxx_unaligned_page_split_write(void)
{
    const par_store_backend_api_t *api;
    const uint32_t page_size = (uint32_t)AT24CXX_PAGE_BYTE;
    const uint32_t base_page_offset = (uint32_t)PAR_CFG_RTT_AT24_BASE_ADDR %
                                      page_size;
    const uint32_t rel_addr = (page_size - 1U + page_size - base_page_offset) %
                              page_size;
    const uint32_t abs_addr = (uint32_t)PAR_CFG_RTT_AT24_BASE_ADDR + rel_addr;
    const uint32_t first_chunk = page_size - (abs_addr % page_size);
    uint8_t payload[2U];

    TEST_ASSERT(page_size > 1U);
    TEST_ASSERT((abs_addr % page_size) != 0U);
    TEST_ASSERT((uint32_t)PAR_CFG_RTT_AT24_SIZE >=
                (rel_addr + (uint32_t)sizeof(payload)));

    for (uint8_t i = 0U; i < (uint8_t)sizeof(payload); i++)
    {
        payload[i] = (uint8_t)(0x40U + i);
    }

    at24_stub_reset();
    g_at24_available = true;
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    TEST_ASSERT_OK(api->write(rel_addr, (uint32_t)sizeof(payload), payload));
    TEST_ASSERT(g_at24_write_count == 2U);
    TEST_ASSERT(g_at24_write_addr[0] == abs_addr);
    TEST_ASSERT(g_at24_write_size[0] == first_chunk);
    TEST_ASSERT(g_at24_write_addr[1] == (abs_addr + first_chunk));
    TEST_ASSERT(g_at24_write_size[1] == ((uint32_t)sizeof(payload) - first_chunk));
    TEST_ASSERT(0 == memcmp(&g_at24_dev.mem[abs_addr], payload, sizeof(payload)));
    TEST_ASSERT_OK(api->deinit());
    return true;
}

/** @brief Verify configured AT24CXX base address is honored. */
static bool test_rtt_at24cxx_base_addr_offsets_window(void)
{
    const par_store_backend_api_t *api;
    const uint8_t payload[2] = { 0xA5U, 0x5AU };
    uint8_t readback[sizeof(payload)] = { 0U };

    at24_stub_reset();
    g_at24_available = true;
    if (PAR_CFG_RTT_AT24_BASE_ADDR > 0U)
    {
        g_at24_dev.mem[PAR_CFG_RTT_AT24_BASE_ADDR - 1U] = 0x33U;
    }
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    TEST_ASSERT_OK(api->write(0U, (uint32_t)sizeof(payload), payload));
    TEST_ASSERT_OK(api->read(0U, (uint32_t)sizeof(readback), readback));
    TEST_ASSERT(0 == memcmp(readback, payload, sizeof(payload)));
    if (PAR_CFG_RTT_AT24_BASE_ADDR > 0U)
    {
        TEST_ASSERT(g_at24_dev.mem[PAR_CFG_RTT_AT24_BASE_ADDR - 1U] == 0x33U);
    }
    TEST_ASSERT(0 == memcmp(&g_at24_dev.mem[PAR_CFG_RTT_AT24_BASE_ADDR], payload, sizeof(payload)));
    TEST_ASSERT_OK(api->deinit());
    return true;
}

/** @brief Verify AT24CXX exact-end and zero-length access boundaries. */
static bool test_rtt_at24cxx_exact_end_and_zero_len_boundaries(void)
{
    const par_store_backend_api_t *api;
    const uint32_t end_addr = (uint32_t)PAR_CFG_RTT_AT24_SIZE;
    const uint8_t payload[2] = { 0x12U, 0x34U };
    uint8_t readback[sizeof(payload)] = { 0U };
    uint8_t zero = 0U;

    TEST_ASSERT(end_addr >= sizeof(payload));
    at24_stub_reset();
    g_at24_available = true;
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    TEST_ASSERT_OK(api->write(end_addr - (uint32_t)sizeof(payload),
                              (uint32_t)sizeof(payload),
                              payload));
    TEST_ASSERT_OK(api->read(end_addr - (uint32_t)sizeof(readback),
                             (uint32_t)sizeof(readback),
                             readback));
    TEST_ASSERT(0 == memcmp(readback, payload, sizeof(payload)));
    TEST_ASSERT_STATUS(api->write(end_addr - 1U, (uint32_t)sizeof(payload), payload),
                       ePAR_ERROR_NVM);
    TEST_ASSERT_STATUS(api->read(end_addr - 1U, (uint32_t)sizeof(readback), readback),
                       ePAR_ERROR_NVM);
    TEST_ASSERT_STATUS(api->erase(end_addr - 1U, (uint32_t)sizeof(payload)), ePAR_ERROR_NVM);
    TEST_ASSERT_STATUS(api->read(end_addr, 0U, &zero), ePAR_ERROR_NVM);
    TEST_ASSERT_STATUS(api->write(end_addr, 0U, &zero), ePAR_ERROR_NVM);
    TEST_ASSERT_STATUS(api->erase(end_addr, 0U), ePAR_ERROR_NVM);
    TEST_ASSERT_OK(api->deinit());
    return true;
}

/** @brief Verify AT24CXX read/write driver errors propagate to the backend caller. */
static bool test_rtt_at24cxx_driver_error_propagates(void)
{
    const par_store_backend_api_t *api;
    const uint8_t payload[4] = { 1U, 2U, 3U, 4U };
    uint8_t readback[sizeof(payload)] = { 0U };

    at24_stub_reset();
    g_at24_available = true;
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    g_at24_fail_write = 1;
    TEST_ASSERT_STATUS(api->write(0U, (uint32_t)sizeof(payload), payload), ePAR_ERROR_NVM);
    g_at24_fail_read = 1;
    TEST_ASSERT_STATUS(api->read(0U, (uint32_t)sizeof(readback), readback), ePAR_ERROR_NVM);
    g_at24_fail_write = 1;
    TEST_ASSERT_STATUS(api->erase(0U, (uint32_t)sizeof(payload)), ePAR_ERROR_NVM);
    TEST_ASSERT_OK(api->deinit());
    return true;
}


/** @brief Verify a failure after the first AT24 page write preserves untouched tail bytes. */
static bool test_rtt_at24cxx_second_page_write_fail_reports_partial_progress_policy(void)
{
    const par_store_backend_api_t *api;
    const uint32_t first_chunk = (uint32_t)AT24CXX_PAGE_BYTE -
                                 ((uint32_t)PAR_CFG_RTT_AT24_BASE_ADDR % (uint32_t)AT24CXX_PAGE_BYTE);
    uint8_t payload[AT24CXX_PAGE_BYTE + 2U];

    TEST_ASSERT(first_chunk > 0U);
    for (uint8_t i = 0U; i < (uint8_t)sizeof(payload); i++)
    {
        payload[i] = (uint8_t)(0x60U + i);
    }

    at24_stub_reset();
    g_at24_available = true;
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    g_at24_fail_write = 2;
    TEST_ASSERT_STATUS(api->write(0U, (uint32_t)sizeof(payload), payload), ePAR_ERROR_NVM);
    TEST_ASSERT(g_at24_write_count == 1U);
    TEST_ASSERT(0 == memcmp(&g_at24_dev.mem[PAR_CFG_RTT_AT24_BASE_ADDR], payload, first_chunk));
    TEST_ASSERT(g_at24_dev.mem[PAR_CFG_RTT_AT24_BASE_ADDR + first_chunk] == 0xFFU);
    TEST_ASSERT_OK(api->deinit());
    return true;
}

/** @brief Verify AT24CXX backend calls reject access after deinitialization. */
static bool test_rtt_at24cxx_after_deinit_rejects_io(void)
{
    const par_store_backend_api_t *api;
    uint8_t value = 0x5AU;
    uint8_t readback = 0U;

    at24_stub_reset();
    g_at24_available = true;
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    TEST_ASSERT_OK(api->deinit());
    TEST_ASSERT_STATUS(api->read(0U, 1U, &readback), ePAR_ERROR_PARAM);
    TEST_ASSERT_STATUS(api->write(0U, 1U, &value), ePAR_ERROR_PARAM);
    TEST_ASSERT_STATUS(api->erase(0U, 1U), ePAR_ERROR_PARAM);
    return true;
}



/** @brief Verify a second-page AT24 read failure leaves the unread tail untouched. */
static bool test_rtt_at24cxx_second_page_read_fail_preserves_tail_buffer(void)
{
    const par_store_backend_api_t *api;
    const uint32_t first_chunk = (uint32_t)AT24CXX_PAGE_BYTE -
                                 ((uint32_t)PAR_CFG_RTT_AT24_BASE_ADDR %
                                  (uint32_t)AT24CXX_PAGE_BYTE);
    uint8_t readback[AT24CXX_PAGE_BYTE + 2U];

    at24_stub_reset();
    g_at24_available = true;
    for (uint8_t i = 0U; i < (uint8_t)sizeof(readback); i++)
    {
        g_at24_dev.mem[PAR_CFG_RTT_AT24_BASE_ADDR + i] = (uint8_t)(0x20U + i);
        readback[i] = 0xA5U;
    }

    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    g_at24_fail_read = 2;
    TEST_ASSERT_STATUS(api->read(0U, (uint32_t)sizeof(readback), readback), ePAR_ERROR_NVM);
    TEST_ASSERT(g_at24_read_count == 1U);
    TEST_ASSERT(0 == memcmp(readback, &g_at24_dev.mem[PAR_CFG_RTT_AT24_BASE_ADDR], first_chunk));
    TEST_ASSERT(readback[first_chunk] == 0xA5U);
    TEST_ASSERT(readback[first_chunk + 1U] == 0xA5U);
    TEST_ASSERT_OK(api->deinit());
    return true;
}

/** @brief Verify an erase chunk failure reports partial-progress current policy. */
static bool test_rtt_at24cxx_erase_chunk_fail_reports_partial_progress_policy(void)
{
    const par_store_backend_api_t *api;
    const uint32_t first_chunk = (uint32_t)AT24CXX_PAGE_BYTE -
                                 ((uint32_t)PAR_CFG_RTT_AT24_BASE_ADDR % (uint32_t)AT24CXX_PAGE_BYTE);
    const uint32_t size = (uint32_t)PAR_STORE_RTT_AT24_ERASE_CHUNK + 2U;

    at24_stub_reset();
    g_at24_available = true;
    memset(&g_at24_dev.mem[PAR_CFG_RTT_AT24_BASE_ADDR], 0x33, size);
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    g_at24_fail_write = 2;
    TEST_ASSERT_STATUS(api->erase(0U, size), ePAR_ERROR_NVM);
    TEST_ASSERT(g_at24_write_count == 1U);
    for (uint32_t idx = 0U; idx < first_chunk; idx++)
    {
        TEST_ASSERT(g_at24_dev.mem[PAR_CFG_RTT_AT24_BASE_ADDR + idx] == 0xFFU);
    }
    TEST_ASSERT(g_at24_dev.mem[PAR_CFG_RTT_AT24_BASE_ADDR + first_chunk] == 0x33U);
    TEST_ASSERT_OK(api->deinit());
    return true;
}

/** @brief Verify AT24CXX sync is a no-op under the current synchronous-write policy. */
static bool test_rtt_at24cxx_sync_noop_current_policy(void)
{
    const par_store_backend_api_t *api;

    at24_stub_reset();
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->sync());
    g_at24_available = true;
    TEST_ASSERT_OK(api->init());
    TEST_ASSERT_OK(api->sync());
    TEST_ASSERT_OK(api->deinit());
    TEST_ASSERT_OK(api->sync());
    return true;
}

/** @brief Verify repeated AT24CXX backend init/deinit cycles are idempotent. */
static bool test_rtt_at24cxx_repeated_init_deinit_is_idempotent(void)
{
    const par_store_backend_api_t *api;
    bool is_init = false;

    at24_stub_reset();
    g_at24_available = true;
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    TEST_ASSERT_OK(api->init());
    api->is_init(&is_init);
    TEST_ASSERT(is_init);
    TEST_ASSERT_OK(api->sync());
    TEST_ASSERT_OK(api->deinit());
    TEST_ASSERT_OK(api->deinit());
    api->is_init(&is_init);
    TEST_ASSERT(!is_init);
    return true;
}

/** @brief Entrypoint for the host AT24CXX backend smoke test. */
int main(void)
{
    static const par_host_test_case_t cases[] = {
        { "backend_rtt_at24cxx_adapter_smoke", test_rtt_at24cxx_adapter_smoke },
        { "backend_rtt_at24cxx_page_split_write", test_rtt_at24cxx_page_split_write },
        { "backend_rtt_at24cxx_unaligned_page_split_write", test_rtt_at24cxx_unaligned_page_split_write },
        { "backend_rtt_at24cxx_base_addr_offsets_window", test_rtt_at24cxx_base_addr_offsets_window },
        { "backend_rtt_at24cxx_exact_end_and_zero_len_boundaries", test_rtt_at24cxx_exact_end_and_zero_len_boundaries },
        { "backend_rtt_at24cxx_driver_error_propagates", test_rtt_at24cxx_driver_error_propagates },
        { "backend_rtt_at24cxx_second_page_write_fail_reports_partial_progress_policy", test_rtt_at24cxx_second_page_write_fail_reports_partial_progress_policy },
        { "backend_rtt_at24cxx_second_page_read_fail_preserves_tail_buffer", test_rtt_at24cxx_second_page_read_fail_preserves_tail_buffer },
        { "backend_rtt_at24cxx_erase_chunk_fail_reports_partial_progress_policy", test_rtt_at24cxx_erase_chunk_fail_reports_partial_progress_policy },
        { "backend_rtt_at24cxx_sync_noop_current_policy", test_rtt_at24cxx_sync_noop_current_policy },
        { "backend_rtt_at24cxx_repeated_init_deinit_is_idempotent", test_rtt_at24cxx_repeated_init_deinit_is_idempotent },
        { "backend_rtt_at24cxx_after_deinit_rejects_io", test_rtt_at24cxx_after_deinit_rejects_io },
    };

    return par_host_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
}
