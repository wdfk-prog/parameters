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
/** @brief Device-check failure countdown; zero disables it. */
static int g_at24_fail_check;
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
    g_at24_fail_check = AT24_FAIL_DISABLED;
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
    if (g_at24_fail_check > AT24_FAIL_DISABLED)
    {
        g_at24_fail_check--;
        if (AT24_FAIL_DISABLED == g_at24_fail_check)
        {
            return -1;
        }
    }
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

#include "backend_rtt_at24cxx/par_backend_rtt_at24cxx_basic_cases.inc"
#include "backend_rtt_at24cxx/par_backend_rtt_at24cxx_error_policy_cases.inc"

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
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
        { "backend_rtt_at24cxx_second_page_write_fail_reports_partial_progress_current_policy", test_rtt_at24cxx_second_page_write_fail_reports_partial_progress_current_policy },
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
        { "backend_rtt_at24cxx_second_page_read_fail_preserves_tail_buffer", test_rtt_at24cxx_second_page_read_fail_preserves_tail_buffer },
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
        { "backend_rtt_at24cxx_erase_chunk_fail_reports_partial_progress_current_policy", test_rtt_at24cxx_erase_chunk_fail_reports_partial_progress_current_policy },
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
        { "backend_rtt_at24cxx_sync_noop_current_policy", test_rtt_at24cxx_sync_noop_current_policy },
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
        { "backend_rtt_at24cxx_repeated_init_deinit_is_idempotent", test_rtt_at24cxx_repeated_init_deinit_is_idempotent },
        { "backend_rtt_at24cxx_init_check_fail_deinits_device_and_recovers", test_rtt_at24cxx_init_check_fail_deinits_device_and_recovers },
        { "backend_rtt_at24cxx_null_buffer_and_zero_len_status_matrix", test_rtt_at24cxx_null_buffer_and_zero_len_status_matrix },
        { "backend_rtt_at24cxx_after_deinit_rejects_io", test_rtt_at24cxx_after_deinit_rejects_io },
    };

    return par_host_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
}
