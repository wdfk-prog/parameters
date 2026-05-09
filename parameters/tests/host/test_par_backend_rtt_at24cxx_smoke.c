/**
 * @file test_par_backend_rtt_at24cxx_smoke.c
 * @brief Smoke-test the RT-Thread AT24CXX backend with host stubs.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include "test_host_common.h"
#include "par_store_backend.h"

#include "at24cxx.h"

/** @brief Concrete host AT24CXX stub object. */
struct autogen_pm_ci_at24_device
{
    uint8_t mem[AT24CXX_MAX_MEM_ADDRESS]; /**< EEPROM bytes. */
};

/** @brief Host AT24CXX stub instance. */
static struct autogen_pm_ci_at24_device g_at24_dev;
/** @brief AT24CXX device availability switch. */
static bool g_at24_available;

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
    memcpy(&dev->mem[addr], buf, size);
    return RT_EOK;
}

/** @brief Verify AT24CXX backend missing-device and read/write paths. */
static bool test_rtt_at24cxx_adapter_smoke(void)
{
    const par_store_backend_api_t *api;
    uint8_t readback = 0U;
    const uint8_t value = 0x5AU;

    TEST_ASSERT_OK(par_store_backend_bind());
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_STATUS(api->init(), ePAR_ERROR_INIT);

    g_at24_available = true;
    memset(&g_at24_dev, 0xFF, sizeof(g_at24_dev));
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

/** @brief Entrypoint for the host AT24CXX backend smoke test. */
int main(void)
{
    static const par_host_test_case_t cases[] = {
        { "backend_rtt_at24cxx_adapter_smoke", test_rtt_at24cxx_adapter_smoke },
    };

    return par_host_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
}
