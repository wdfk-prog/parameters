/**
 * @file test_par_backend_flash_ee_fal_smoke.c
 * @brief Smoke-test the FAL Flash EE adapter with host FAL stubs.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include "test_host_common.h"
#include "par_store_backend.h"

#include "fal.h"

/** @brief Host FAL smoke-test flash region size. */
#define FAL_SMOKE_SIZE (0x1000U)

/** @brief Host FAL flash contents. */
static uint8_t g_fal_flash[FAL_SMOKE_SIZE];
/** @brief Host FAL device stub. */
static const struct fal_flash_dev g_fal_dev = { .len = FAL_SMOKE_SIZE, .blk_size = 64 };
/** @brief Host FAL partition stub. */
static const struct fal_partition g_fal_part = {
    .name = "autogen_pm",
    .flash_name = "fal-flash",
    .offset = 0,
    .len = FAL_SMOKE_SIZE,
};
/** @brief FAL partition availability switch. */
static bool g_fal_partition_available;

const struct fal_partition *fal_partition_find(const char *name)
{
    return (g_fal_partition_available && (0 == strcmp(name, g_fal_part.name))) ? &g_fal_part : NULL;
}

const struct fal_flash_dev *fal_flash_device_find(const char *name)
{
    return (0 == strcmp(name, g_fal_part.flash_name)) ? &g_fal_dev : NULL;
}

/**
 * @brief Read from the host FAL partition stub.
 * @param part Partition handle.
 * @param addr Byte offset inside the partition.
 * @param buf Destination buffer.
 * @param size Number of bytes to read.
 * @return Number of bytes read on success, otherwise -1.
 */
int fal_partition_read(const struct fal_partition *part,
                       rt_uint32_t addr,
                       rt_uint8_t *buf,
                       rt_size_t size)
{
    if ((NULL == part) || (NULL == buf) ||
        (addr > sizeof(g_fal_flash)) || (size > (sizeof(g_fal_flash) - addr)))
    {
        return -1;
    }
    memcpy(buf, &g_fal_flash[addr], size);
    return (int)size;
}

/**
 * @brief Write to the host FAL partition stub.
 * @param part Partition handle.
 * @param addr Byte offset inside the partition.
 * @param buf Source buffer.
 * @param size Number of bytes to write.
 * @return Number of bytes written on success, otherwise -1.
 */
int fal_partition_write(const struct fal_partition *part,
                        rt_uint32_t addr,
                        const rt_uint8_t *buf,
                        rt_size_t size)
{
    if ((NULL == part) || (NULL == buf) ||
        (addr > sizeof(g_fal_flash)) || (size > (sizeof(g_fal_flash) - addr)))
    {
        return -1;
    }
    memcpy(&g_fal_flash[addr], buf, size);
    return (int)size;
}

/**
 * @brief Erase the host FAL partition stub.
 * @param part Partition handle.
 * @param addr Byte offset inside the partition.
 * @param size Number of bytes to erase.
 * @return Number of bytes erased on success, otherwise -1.
 */
int fal_partition_erase(const struct fal_partition *part,
                        rt_uint32_t addr,
                        rt_size_t size)
{
    if ((NULL == part) ||
        (addr > sizeof(g_fal_flash)) || (size > (sizeof(g_fal_flash) - addr)))
    {
        return -1;
    }
    memset(&g_fal_flash[addr], 0xFF, size);
    return (int)size;
}

/** @brief Verify FAL adapter bind, missing partition, read/write, erase, and boundary paths. */
static bool test_flash_ee_fal_adapter_smoke(void)
{
    const par_store_backend_api_t *api;
    bool is_init = true;
    const uint8_t payload[4] = { 0x11U, 0x22U, 0x33U, 0x44U };
    uint8_t readback[sizeof(payload)] = { 0U };

    TEST_ASSERT_OK(par_store_backend_bind());
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_STATUS(api->init(), ePAR_ERROR_INIT);
    api->is_init(&is_init);
    TEST_ASSERT(!is_init);

    g_fal_partition_available = true;
    TEST_ASSERT_OK(api->init());
    api->is_init(&is_init);
    TEST_ASSERT(is_init);
    TEST_ASSERT(api->name != NULL);
    TEST_ASSERT_OK(api->write(0U, (uint32_t)sizeof(payload), payload));
    TEST_ASSERT_OK(api->sync());
    TEST_ASSERT_OK(api->read(0U, (uint32_t)sizeof(readback), readback));
    TEST_ASSERT(0 == memcmp(readback, payload, sizeof(payload)));
    TEST_ASSERT_OK(api->erase(0U, (uint32_t)sizeof(payload)));
    TEST_ASSERT_OK(api->sync());
    memset(readback, 0U, sizeof(readback));
    TEST_ASSERT_OK(api->read(0U, (uint32_t)sizeof(readback), readback));
    TEST_ASSERT(0xFFU == readback[0]);
    TEST_ASSERT(0xFFU == readback[1]);
    TEST_ASSERT(0xFFU == readback[2]);
    TEST_ASSERT(0xFFU == readback[3]);
    TEST_ASSERT_STATUS(api->read(UINT32_MAX, 1U, readback), ePAR_ERROR_PARAM);
    TEST_ASSERT_STATUS(api->write(UINT32_MAX, 1U, payload), ePAR_ERROR_PARAM);
    TEST_ASSERT_STATUS(api->erase(UINT32_MAX, 1U), ePAR_ERROR_PARAM);
    TEST_ASSERT_OK(api->deinit());
    return true;
}

/** @brief Entrypoint for the host FAL adapter smoke test. */
int main(void)
{
    static const par_host_test_case_t cases[] = {
        { "backend_flash_ee_fal_adapter_smoke", test_flash_ee_fal_adapter_smoke },
    };

    return par_host_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
}
