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
#define FAL_SMOKE_SIZE (0x1200U)
/** @brief Disable the FAL short-I/O failpoint. */
#define FAL_SHORT_IO_DISABLED (0)

/** @brief Host FAL flash contents. */
static uint8_t g_fal_flash[FAL_SMOKE_SIZE];
/** @brief Host FAL device stub. */
static const struct fal_flash_dev g_fal_dev = { .len = FAL_SMOKE_SIZE, .blk_size = 64 };
/** @brief Host FAL partition stub. */
static struct fal_partition g_fal_part = {
    .name = "autogen_pm",
    .flash_name = "fal-flash",
    .offset = 0,
    .len = FAL_SMOKE_SIZE,
};
/** @brief FAL partition availability switch. */
static bool g_fal_partition_available;
/** @brief FAL flash-device availability switch. */
static bool g_fal_flash_available = true;
/** @brief Return one byte short from the next FAL read when non-zero. */
static int g_fal_short_read;
/** @brief Return one byte short from the next FAL write when non-zero. */
static int g_fal_short_write;
/** @brief Return one byte short from the next FAL erase when non-zero. */
static int g_fal_short_erase;

/** @brief Reset mutable host FAL stub state to the default partition geometry. */
static void fal_stub_reset(void)
{
    memset(g_fal_flash, 0xCC, sizeof(g_fal_flash));
    g_fal_part.offset = 0;
    g_fal_part.len = FAL_SMOKE_SIZE;
    g_fal_partition_available = false;
    g_fal_flash_available = true;
    g_fal_short_read = FAL_SHORT_IO_DISABLED;
    g_fal_short_write = FAL_SHORT_IO_DISABLED;
    g_fal_short_erase = FAL_SHORT_IO_DISABLED;
}

/**
 * @brief Check whether a partition-relative FAL access stays inside the stub.
 * @param part Partition handle.
 * @param addr Partition-relative offset.
 * @param size Number of bytes requested.
 * @param[out] p_abs Receives the absolute backing flash address.
 * @return true when the requested range is valid, otherwise false.
 */
static bool fal_stub_range_is_valid(const struct fal_partition *part,
                                    rt_uint32_t addr,
                                    rt_size_t size,
                                    size_t *p_abs)
{
    const size_t part_offset = (NULL != part) ? (size_t)part->offset : 0U;
    const size_t part_len = (NULL != part) ? (size_t)part->len : 0U;
    const size_t rel_addr = (size_t)addr;

    if ((NULL == part) || (NULL == p_abs) || (part->offset < 0) || (part->len < 0) ||
        (rel_addr > part_len) || (size > (part_len - rel_addr)) ||
        (part_offset > sizeof(g_fal_flash)) || (rel_addr > (sizeof(g_fal_flash) - part_offset)) ||
        (size > (sizeof(g_fal_flash) - part_offset - rel_addr)))
    {
        return false;
    }

    *p_abs = part_offset + rel_addr;
    return true;
}

const struct fal_partition *fal_partition_find(const char *name)
{
    return (g_fal_partition_available && (0 == strcmp(name, g_fal_part.name))) ? &g_fal_part : NULL;
}

const struct fal_flash_dev *fal_flash_device_find(const char *name)
{
    return (g_fal_flash_available && (0 == strcmp(name, g_fal_part.flash_name))) ? &g_fal_dev : NULL;
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
    size_t abs_addr = 0U;

    if ((NULL == buf) || (false == fal_stub_range_is_valid(part, addr, size, &abs_addr)))
    {
        return -1;
    }
    if (g_fal_short_read != FAL_SHORT_IO_DISABLED)
    {
        g_fal_short_read = FAL_SHORT_IO_DISABLED;
        return (size > 0U) ? (int)(size - 1U) : 0;
    }
    memcpy(buf, &g_fal_flash[abs_addr], size);
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
    size_t abs_addr = 0U;

    if ((NULL == buf) || (false == fal_stub_range_is_valid(part, addr, size, &abs_addr)))
    {
        return -1;
    }
    if (g_fal_short_write != FAL_SHORT_IO_DISABLED)
    {
        g_fal_short_write = FAL_SHORT_IO_DISABLED;
        return (size > 0U) ? (int)(size - 1U) : 0;
    }
    memcpy(&g_fal_flash[abs_addr], buf, size);
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
    size_t abs_addr = 0U;

    if (false == fal_stub_range_is_valid(part, addr, size, &abs_addr))
    {
        return -1;
    }
    if (g_fal_short_erase != FAL_SHORT_IO_DISABLED)
    {
        g_fal_short_erase = FAL_SHORT_IO_DISABLED;
        return (size > 0U) ? (int)(size - 1U) : 0;
    }
    memset(&g_fal_flash[abs_addr], 0xFF, size);
    return (int)size;
}

/** @brief Verify FAL adapter bind, missing partition, read/write, erase, and boundary paths. */
static bool test_flash_ee_fal_adapter_smoke(void)
{
    const par_store_backend_api_t *api;
    bool is_init = true;
    const uint8_t payload[4] = { 0x11U, 0x22U, 0x33U, 0x44U };
    uint8_t readback[sizeof(payload)] = { 0U };

    fal_stub_reset();
    TEST_ASSERT_OK(par_store_backend_bind());
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT((api->init() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
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

/** @brief Verify FAL partition offsets are honored as a hard storage boundary. */
static bool test_flash_ee_fal_partition_offset_semantics(void)
{
    const par_store_backend_api_t *api;
    const uint8_t payload[4] = { 0xA1U, 0xB2U, 0xC3U, 0xD4U };
    uint8_t readback[sizeof(payload)] = { 0U };

    fal_stub_reset();
    TEST_ASSERT_OK(par_store_backend_bind());
    g_fal_partition_available = true;
    g_fal_part.offset = 128;
    g_fal_part.len = 0x1000;
    g_fal_flash[127] = 0x5AU;

    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT_OK(api->init());
    g_fal_flash[132] = 0x6BU;
    TEST_ASSERT_OK(api->write(0U, (uint32_t)sizeof(payload), payload));
    TEST_ASSERT(g_fal_flash[127] == 0x5AU);
    TEST_ASSERT(g_fal_flash[132] == 0x6BU);
    TEST_ASSERT_OK(api->read(0U, (uint32_t)sizeof(readback), readback));
    TEST_ASSERT(0 == memcmp(readback, payload, sizeof(payload)));
    TEST_ASSERT_STATUS(api->write((uint32_t)g_fal_part.len - 2U, (uint32_t)sizeof(payload), payload), ePAR_ERROR_PARAM);
    TEST_ASSERT(g_fal_flash[132] == 0x6BU);
    TEST_ASSERT_OK(api->erase(0U, (uint32_t)sizeof(payload)));
    TEST_ASSERT(g_fal_flash[127] == 0x5AU);
    TEST_ASSERT(g_fal_flash[132] == 0x6BU);
    TEST_ASSERT_OK(api->deinit());
    return true;
}

/** @brief Verify short FAL format I/O returns propagate as initialization errors. */
static bool test_flash_ee_fal_short_io_propagates_error(void)
{
    const par_store_backend_api_t *api;

    fal_stub_reset();
    TEST_ASSERT_OK(par_store_backend_bind());
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);

    g_fal_partition_available = true;
    g_fal_short_read = 1;
    TEST_ASSERT((api->init() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);

    fal_stub_reset();
    g_fal_partition_available = true;
    g_fal_short_erase = 1;
    TEST_ASSERT((api->init() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);

    fal_stub_reset();
    g_fal_partition_available = true;
    g_fal_short_write = 1;
    TEST_ASSERT((api->init() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    return true;
}

/** @brief Verify adapter initialization rejects a missing underlying FAL flash device. */
static bool test_flash_ee_fal_missing_flash_device_fails_init(void)
{
    const par_store_backend_api_t *api;
    bool is_init = true;

    fal_stub_reset();
    TEST_ASSERT_OK(par_store_backend_bind());
    g_fal_partition_available = true;
    g_fal_flash_available = false;
    api = par_store_backend_get_api();
    TEST_ASSERT(NULL != api);
    TEST_ASSERT((api->init() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    api->is_init(&is_init);
    TEST_ASSERT(!is_init);
    return true;
}

/** @brief Entrypoint for the host FAL adapter smoke test. */
int main(void)
{
    static const par_host_test_case_t cases[] = {
        { "backend_flash_ee_fal_adapter_smoke", test_flash_ee_fal_adapter_smoke },
        { "backend_flash_ee_fal_partition_offset_semantics", test_flash_ee_fal_partition_offset_semantics },
        { "backend_flash_ee_fal_short_io_propagates_error", test_flash_ee_fal_short_io_propagates_error },
        { "backend_flash_ee_fal_missing_flash_device_fails_init", test_flash_ee_fal_missing_flash_device_fails_init },
    };

    return par_host_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
}
