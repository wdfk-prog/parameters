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
/** @brief Countdown for returning one byte short from FAL reads. */
static int g_fal_short_read;
/** @brief Countdown for returning one byte short from FAL writes. */
static int g_fal_short_write;
/** @brief Countdown for returning one byte short from FAL erases. */
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
    if (g_fal_short_read > FAL_SHORT_IO_DISABLED)
    {
        g_fal_short_read--;
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
    if (g_fal_short_write > FAL_SHORT_IO_DISABLED)
    {
        g_fal_short_write--;
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
    if (g_fal_short_erase > FAL_SHORT_IO_DISABLED)
    {
        g_fal_short_erase--;
        return (size > 0U) ? (int)(size - 1U) : 0;
    }
    memset(&g_fal_flash[abs_addr], 0xFF, size);
    return (int)size;
}

#include "backend_flash_ee_fal/par_backend_flash_ee_fal_basic_cases.inc"
#include "backend_flash_ee_fal/par_backend_flash_ee_fal_error_policy_cases.inc"

/** @brief Entrypoint for the host FAL adapter smoke test. */
int main(void)
{
    static const par_host_test_case_t cases[] = {
        { "backend_flash_ee_fal_adapter_smoke", test_flash_ee_fal_adapter_smoke },
        { "backend_flash_ee_fal_partition_offset_semantics", test_flash_ee_fal_partition_offset_semantics },
        { "backend_flash_ee_fal_exact_end_and_zero_len_boundaries", test_flash_ee_fal_exact_end_and_zero_len_boundaries },
        { "backend_flash_ee_fal_short_io_propagates_error", test_flash_ee_fal_short_io_propagates_error },
        { "backend_flash_ee_fal_missing_flash_device_fails_init", test_flash_ee_fal_missing_flash_device_fails_init },
        { "backend_flash_ee_fal_repeated_init_deinit_is_idempotent", test_flash_ee_fal_repeated_init_deinit_is_idempotent },
        { "backend_flash_ee_fal_after_deinit_rejects_io", test_flash_ee_fal_after_deinit_rejects_io },
        { "backend_flash_ee_fal_partition_smaller_than_geometry_fails_init", test_flash_ee_fal_partition_smaller_than_geometry_fails_init },
        { "backend_flash_ee_fal_short_read_write_preserves_buffers", test_flash_ee_fal_short_read_write_preserves_buffers },
        { "backend_flash_ee_fal_partition_offset_failed_io_preserves_neighbors", test_flash_ee_fal_partition_offset_failed_io_preserves_neighbors },
        { "backend_flash_ee_fal_runtime_short_io_reports_backend_errors", test_flash_ee_fal_runtime_short_io_reports_backend_errors },
        { "backend_flash_ee_fal_null_buffer_and_zero_len_status_matrix", test_flash_ee_fal_null_buffer_and_zero_len_status_matrix },
        { "backend_flash_ee_fal_short_erase_preserves_neighbors", test_flash_ee_fal_short_erase_preserves_neighbors },
    };

    return par_host_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
}
