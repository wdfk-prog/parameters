/** @brief Enable POSIX declarations used by fork-based reset tests. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif /* !defined(_POSIX_C_SOURCE) */

/**
 * @file test_par_nvm_flash_ee.c
 * @brief Exercise host Flash EE persistence and recovery paths.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include "test_host_common.h"
#include "rtthread.h"
#include "par_nvm_api.h"
#include "par_store_backend_flash_ee.h"
#include "par_store_backend.h"

#include <stdarg.h>
#include <sys/wait.h>
#include <unistd.h>

/** @brief Host fake flash size for native Flash EE tests. */
#define HOST_FLASH_SIZE (0x1000U)
/** @brief Host fake flash erase granularity. */
#define HOST_FLASH_ERASE_SIZE (64U)
/** @brief Host fake flash program granularity. */
#define HOST_FLASH_PROGRAM_SIZE (4U)
/** @brief Disable the failpoint counter. */
#define HOST_FLASH_FAIL_DISABLED (-1)

/** @brief Host fake flash image path buffer length. */
#define HOST_FLASH_IMAGE_PATH_LEN (128U)
/** @brief Offset of the serialized scalar NVM table-ID field in host flash. */
#define HOST_NVM_HEAD_TABLE_ID_OFFSET \
    ((uint32_t)sizeof(uint32_t) + (uint32_t)sizeof(uint16_t))
/** @brief Size of the serialized scalar NVM table-ID field in host flash. */
#define HOST_NVM_HEAD_TABLE_ID_SIZE   ((uint32_t)sizeof(uint32_t))

#ifndef PAR_HOST_TEST_PROFILE_NAME
/** @brief Human-readable host NVM profile name printed by matrix tests. */
#define PAR_HOST_TEST_PROFILE_NAME "default"
#endif /* !defined(PAR_HOST_TEST_PROFILE_NAME) */


/** @brief Captured shell output buffer size for NVM shell command tests. */
#define NVM_SHELL_CAPTURE_SIZE (8192U)

/** @brief Captured shell output for NVM shell command tests. */
static char g_nvm_shell_capture[NVM_SHELL_CAPTURE_SIZE];
/** @brief Number of used bytes in g_nvm_shell_capture. */
static size_t g_nvm_shell_capture_used;

/** @brief Reset the NVM shell output capture buffer. */
static void nvm_shell_capture_reset(void)
{
    g_nvm_shell_capture[0] = '\0';
    g_nvm_shell_capture_used = 0U;
}

/**
 * @brief Return whether captured NVM shell output contains a substring.
 * @param needle Null-terminated substring to find.
 * @return true when @p needle is present in the capture buffer.
 */
static bool nvm_shell_capture_contains(const char *needle)
{
    return (NULL != strstr(g_nvm_shell_capture, needle));
}

/** @brief Host rt_kprintf stub used by the included MSH command implementation. */
int rt_kprintf(const char *fmt, ...)
{
    int written;
    va_list args;
    size_t free_size = (g_nvm_shell_capture_used < NVM_SHELL_CAPTURE_SIZE) ?
                       (NVM_SHELL_CAPTURE_SIZE - g_nvm_shell_capture_used) : 0U;

    if (0U == free_size)
    {
        return 0;
    }

    va_start(args, fmt);
    written = vsnprintf(&g_nvm_shell_capture[g_nvm_shell_capture_used], free_size, fmt, args);
    va_end(args);

    if (written < 0)
    {
        return written;
    }
    if ((size_t)written >= free_size)
    {
        g_nvm_shell_capture_used = NVM_SHELL_CAPTURE_SIZE;
    }
    else
    {
        g_nvm_shell_capture_used += (size_t)written;
    }

    return written;
}

/** @brief Host rt_snprintf stub used by the included MSH command implementation. */
int rt_snprintf(char *buf, rt_size_t size, const char *fmt, ...)
{
    int written;
    va_list args;

    va_start(args, fmt);
    written = vsnprintf(buf, size, fmt, args);
    va_end(args);
    return written;
}

/** @brief Host heap allocation stub used by object-printing shell paths. */
void *rt_malloc(rt_size_t size)
{
    return malloc(size);
}

/** @brief Host heap free stub used by object-printing shell paths. */
void rt_free(void *ptr)
{
    free(ptr);
}

#include "../../../port/par_shell_tool.c"

/** @brief Raw fake flash bytes used by the native Flash EE port hooks. */
static uint8_t g_flash[HOST_FLASH_SIZE];
/** @brief Process-unique fake flash image path. */
static char g_flash_image_path[HOST_FLASH_IMAGE_PATH_LEN];
/** @brief Native fake flash initialization flag. */
static bool g_flash_is_init;
/** @brief Program failpoint countdown; negative disables it. */
static int g_fail_program_after = HOST_FLASH_FAIL_DISABLED;
/** @brief Read failpoint countdown; negative disables it. */
static int g_fail_read_after = HOST_FLASH_FAIL_DISABLED;
/** @brief Erase failpoint countdown; negative disables it. */
static int g_fail_erase_after = HOST_FLASH_FAIL_DISABLED;
/** @brief Dedicated object-store fake flash bytes. */
static uint8_t g_object_flash[HOST_FLASH_SIZE];
/** @brief Dedicated object-store initialization flag. */
static bool g_object_flash_is_init;
/** @brief Dedicated object-store write failpoint countdown; negative disables it. */
static int g_object_fail_write_after = HOST_FLASH_FAIL_DISABLED;
/** @brief Dedicated object-store read failpoint countdown; negative disables it. */
static int g_object_fail_read_after = HOST_FLASH_FAIL_DISABLED;
/** @brief Dedicated object-store erase failpoint countdown; negative disables it. */
static int g_object_fail_erase_after = HOST_FLASH_FAIL_DISABLED;
/** @brief Dedicated object-store sync failpoint countdown; negative disables it. */
static int g_object_fail_sync_after = HOST_FLASH_FAIL_DISABLED;

/**
 * @brief Return the process-unique fake flash image path.
 * @return Null-terminated host flash image path.
 */
static const char *host_flash_image_path(void)
{
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
 * @brief Remove the process-local fake flash image when host tests finish.
 */
static void host_flash_remove_image(void)
{
    if ('\0' != g_flash_image_path[0])
    {
        (void)remove(g_flash_image_path);
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
    g_fail_program_after = HOST_FLASH_FAIL_DISABLED;
    g_fail_read_after = HOST_FLASH_FAIL_DISABLED;
    g_fail_erase_after = HOST_FLASH_FAIL_DISABLED;
    g_object_fail_write_after = HOST_FLASH_FAIL_DISABLED;
    g_object_fail_read_after = HOST_FLASH_FAIL_DISABLED;
    g_object_fail_erase_after = HOST_FLASH_FAIL_DISABLED;
    g_object_fail_sync_after = HOST_FLASH_FAIL_DISABLED;
}

/** @brief Clear transient fake flash failpoints without erasing contents. */
static void host_flash_clear_failpoints(void)
{
    g_fail_program_after = HOST_FLASH_FAIL_DISABLED;
    g_fail_read_after = HOST_FLASH_FAIL_DISABLED;
    g_fail_erase_after = HOST_FLASH_FAIL_DISABLED;
    g_object_fail_write_after = HOST_FLASH_FAIL_DISABLED;
    g_object_fail_read_after = HOST_FLASH_FAIL_DISABLED;
    g_object_fail_erase_after = HOST_FLASH_FAIL_DISABLED;
    g_object_fail_sync_after = HOST_FLASH_FAIL_DISABLED;
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
 * @brief Check whether a raw fake flash operation fits in the host region.
 * @param addr Byte address inside the host fake flash region.
 * @param size Number of bytes requested.
 * @return true when the requested range is valid, otherwise false.
 */
static bool host_flash_range_is_valid(const uint32_t addr, const uint32_t size)
{
    return (addr <= HOST_FLASH_SIZE) && (size <= (HOST_FLASH_SIZE - addr));
}

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
    return ePAR_OK;
}

/** @brief Flush the dedicated object-store fake backend. */
static par_status_t host_object_backend_sync(void)
{
    if (0 == g_object_fail_sync_after)
    {
        return ePAR_ERROR;
    }
    if (g_object_fail_sync_after > 0)
    {
        g_object_fail_sync_after--;
    }

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

/** @brief Initialize the module with a preserved fake flash image. */
static bool init_module(void)
{
    if (par_is_init())
    {
        TEST_ASSERT_OK(par_deinit());
    }

    TEST_ASSERT_OK(par_init());
    return true;
}

/**
 * @brief Run the included shell command dispatcher with an argv vector.
 * @param argc Argument count.
 * @param argv Argument vector.
 */
static void run_nvm_shell(int argc, char **argv)
{
    nvm_shell_capture_reset();
    par_msh(argc, argv);
}

/** @brief Verify first boot on erased flash restores and writes defaults. */
static bool test_nvm_first_boot_formats_and_restores_defaults(void)
{
    uint8_t value = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 1U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify saved scalar values reload as their last committed values. */
static bool test_nvm_save_reload_preserves_last_committed_scalar_values(void)
{
    uint8_t u8 = 0U;
    uint16_t u16 = 0U;
    uint32_t u32 = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, 456U));
    TEST_ASSERT_OK(par_set_u32(ePAR_TEST_U32, 98765UL));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_save(ePAR_TEST_U16));
    TEST_ASSERT_OK(par_save(ePAR_TEST_U32));
    TEST_ASSERT_OK(par_deinit());

    host_flash_clear_failpoints();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT_OK(par_get_u32(ePAR_TEST_U32, &u32));
    TEST_ASSERT(u8 == 4U);
    TEST_ASSERT(u16 == 456U);
    TEST_ASSERT(u32 == 98765UL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object values survive deinit/init cycles. */
static bool test_nvm_object_save_reload_shared_fixed_addr(void)
{
    char str_buf[9] = { 0 };
    uint8_t byte_buf[4] = { 0U };
    uint16_t len = 0U;
    const uint8_t payload[4] = { 9U, 8U, 7U, 6U };

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "nvm"));
    TEST_ASSERT_OK(par_set_bytes(ePAR_TEST_BYTES, payload, (uint16_t)sizeof(payload)));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    TEST_ASSERT_OK(par_save(ePAR_TEST_BYTES));
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 3U);
    TEST_ASSERT(strcmp(str_buf, "nvm") == 0);
    TEST_ASSERT_OK(par_get_bytes(ePAR_TEST_BYTES, byte_buf, sizeof(byte_buf), &len));
    TEST_ASSERT(len == 4U);
    TEST_ASSERT(memcmp(byte_buf, payload, sizeof(payload)) == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Save one committed value, then leave a failed live value unflushed. */
static bool host_flash_child_failed_save_without_deinit(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    g_fail_program_after = 0;
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    TEST_ASSERT((par_save(ePAR_TEST_MODE) & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    return true;
}

/** @brief Verify one value after a process-local backend reset. */
static bool host_flash_child_verify_mode_value(const uint8_t expected_value)
{
    uint8_t value = 0U;

    host_flash_clear_failpoints();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == expected_value);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify failed programming keeps the last committed value after hard reset. */
static bool test_flash_ee_failed_program_reload_preserves_last_committed_value(void)
{
    pid_t child_pid;

    host_flash_reset_erased();

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_failed_save_without_deinit());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_verify_mode_value(4U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}

/** @brief Verify failed programming is retried by graceful deinit and commits the live value. */
static bool test_flash_ee_failed_program_graceful_deinit_commits_live_value(void)
{
    uint8_t value = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    g_fail_program_after = 0;
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    TEST_ASSERT((par_save(ePAR_TEST_MODE) & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    host_flash_clear_failpoints();
    TEST_ASSERT_OK(par_deinit());

    host_flash_clear_failpoints();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 5U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify physical flash corruption rebuilds the default value. */
static bool test_flash_ee_corruption_rebuilds_default_value(void)
{
    uint8_t value = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 6U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_deinit());

    for (uint32_t addr = 0U; addr < 128U; addr += 11U)
    {
        host_flash_xor_byte(addr, 0x5AU);
    }

    host_flash_clear_failpoints();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 1U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object saves cannot corrupt committed scalar records. */
static bool test_nvm_object_updates_do_not_corrupt_scalar_values(void)
{
    uint8_t u8 = 0U;
    uint16_t u16 = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 7U));
    TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, 222U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_save(ePAR_TEST_U16));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "object"));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT(u8 == 7U);
    TEST_ASSERT(u16 == 222U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify scalar saves cannot corrupt committed object payloads. */
static bool test_nvm_scalar_updates_do_not_corrupt_object_values(void)
{
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "stable"));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    for (uint8_t value = 2U; value < 9U; value++)
    {
        TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, value));
        TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    }
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 6U);
    TEST_ASSERT(strcmp(str_buf, "stable") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify erase failpoints reject erase without modifying existing bytes. */
static bool test_flash_ee_failed_erase_preserves_existing_bytes(void)
{
    uint8_t before[HOST_FLASH_ERASE_SIZE];
    uint8_t after[HOST_FLASH_ERASE_SIZE];
    const uint8_t zeros[HOST_FLASH_PROGRAM_SIZE] = { 0U };

    host_flash_reset_erased();
    TEST_ASSERT_OK(par_store_flash_ee_native_port_init());
    TEST_ASSERT_OK(par_store_flash_ee_native_port_program(0U,
                                                          (uint32_t)sizeof(zeros),
                                                          zeros));
    TEST_ASSERT_OK(par_store_flash_ee_native_port_read(0U, (uint32_t)sizeof(before), before));
    g_fail_erase_after = 0;
    TEST_ASSERT_STATUS(par_store_flash_ee_native_port_erase(0U, HOST_FLASH_ERASE_SIZE),
                       ePAR_ERROR);
    host_flash_clear_failpoints();
    TEST_ASSERT_OK(par_store_flash_ee_native_port_read(0U, (uint32_t)sizeof(after), after));
    TEST_ASSERT(0 == memcmp(before, after, sizeof(before)));
    TEST_ASSERT_OK(par_store_flash_ee_native_port_deinit());
    return true;
}

/** @brief Verify repeated scalar updates still reload the last committed value. */
static bool test_flash_ee_many_updates_preserve_last_committed_value(void)
{
    uint8_t value = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    for (uint8_t next = 1U; next <= 10U; next++)
    {
        TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, next));
        TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    }
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 10U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify fake flash port rejects address/size ranges that wrap uint32_t. */
static bool test_flash_ee_port_rejects_wrapped_ranges(void)
{
    uint8_t value = 0U;
    const uint8_t payload[4] = { 1U, 2U, 3U, 4U };

    host_flash_reset_erased();
    TEST_ASSERT_OK(par_store_flash_ee_native_port_init());
    TEST_ASSERT_STATUS(par_store_flash_ee_native_port_read(UINT32_MAX - 1U, 4U, &value),
                       ePAR_ERROR);
    TEST_ASSERT_STATUS(par_store_flash_ee_native_port_program(UINT32_MAX - 1U,
                                                              (uint32_t)sizeof(payload),
                                                              payload),
                       ePAR_ERROR);
    TEST_ASSERT_STATUS(par_store_flash_ee_native_port_erase(UINT32_MAX - 63U, 128U), ePAR_ERROR);
    TEST_ASSERT_OK(par_store_flash_ee_native_port_deinit());
    return true;
}

/** @brief Verify the fake flash port rejects invalid 0-to-1 programming. */
static bool test_flash_ee_program_one_to_zero_semantics(void)
{
    uint8_t readback[HOST_FLASH_PROGRAM_SIZE] = { 0U };
    uint8_t ones[HOST_FLASH_PROGRAM_SIZE];
    const uint8_t zeros[HOST_FLASH_PROGRAM_SIZE] = { 0U };

    memset(ones, 0xFF, sizeof(ones));
    host_flash_reset_erased();
    TEST_ASSERT_OK(par_store_flash_ee_native_port_init());
    TEST_ASSERT_OK(par_store_flash_ee_native_port_program(0U,
                                                          (uint32_t)sizeof(zeros),
                                                          zeros));
    TEST_ASSERT_STATUS(par_store_flash_ee_native_port_program(0U,
                                                              (uint32_t)sizeof(ones),
                                                              ones),
                       ePAR_ERROR);
    TEST_ASSERT_OK(par_store_flash_ee_native_port_read(0U,
                                                       (uint32_t)sizeof(readback),
                                                       readback));
    TEST_ASSERT(0 == memcmp(readback, zeros, sizeof(readback)));
    TEST_ASSERT_OK(par_store_flash_ee_native_port_deinit());
    return true;
}


/** @brief Save a committed value, then fail a later save without graceful deinit. */
static bool host_flash_child_fail_save_after_commit(const int program_fail_after)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    g_fail_program_after = program_fail_after;
    TEST_ASSERT((par_save(ePAR_TEST_MODE) & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    return true;
}

/** @brief Verify early and mid-write program failpoints preserve last committed values. */
static bool test_flash_ee_program_failpoint_matrix_preserves_last_commit(void)
{
    static const int fail_after_cases[] = { 0, 1 };

    for (size_t i = 0U; i < (sizeof(fail_after_cases) / sizeof(fail_after_cases[0])); i++)
    {
        pid_t child_pid;

        host_flash_reset_erased();
        child_pid = fork();
        TEST_ASSERT(child_pid >= 0);
        if (0 == child_pid)
        {
            host_child_exit_from_result(host_flash_child_fail_save_after_commit(fail_after_cases[i]));
        }
        TEST_ASSERT(host_child_exit_is_success(child_pid));

        child_pid = fork();
        TEST_ASSERT(child_pid >= 0);
        if (0 == child_pid)
        {
            host_child_exit_from_result(host_flash_child_verify_mode_value(4U));
        }
        TEST_ASSERT(host_child_exit_is_success(child_pid));
    }

    return true;
}

/** @brief Verify table-ID/header incompatibility rebuilds defaults instead of loading stale values. */
static bool test_nvm_table_id_mismatch_rebuilds_defaults(void)
{
    uint8_t value = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 9U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_deinit());

    /* Corrupt the serialized table-ID field in the scalar NVM header. */
    for (uint32_t addr = HOST_NVM_HEAD_TABLE_ID_OFFSET;
         addr < (HOST_NVM_HEAD_TABLE_ID_OFFSET + HOST_NVM_HEAD_TABLE_ID_SIZE);
         addr++)
    {
        host_flash_xor_byte(addr, 0xA5U);
    }

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 1U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify shell save persists live values across a restart. */
static bool test_msh_save_persists_live_scalar_after_restart(void)
{
    char *save_args[] = { "par", "save" };
    uint8_t value = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 8U));
    run_nvm_shell(2, save_args);
    TEST_ASSERT(nvm_shell_capture_contains("OK"));
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 8U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify shell save_clean rewrites the managed NVM area with live values. */
static bool test_msh_save_clean_rewrites_live_values(void)
{
    char *save_clean_args[] = { "par", "save_clean" };
    uint8_t value = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 6U));
    run_nvm_shell(2, save_clean_args);
    TEST_ASSERT(nvm_shell_capture_contains("OK"));
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
    TEST_ASSERT(value == 6U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify shell save reports backend write errors. */
static bool test_msh_save_reports_backend_error(void)
{
    char *save_args[] = { "par", "save" };

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 7U));
    g_fail_program_after = 0;
    run_nvm_shell(2, save_args);
    TEST_ASSERT(nvm_shell_capture_contains("ERR"));
    host_flash_clear_failpoints();
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify NVM wrapper APIs persist scalar and object live values. */
static bool test_nvm_save_by_id_save_all_and_n_save_wrappers(void)
{
    uint8_t u8 = 0U;
    uint16_t u16 = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;
    uint16_t mode_id = 0U;
    uint16_t str_id = 0U;
    par_type_t scalar_value = { 0 };
    const uint8_t str_payload[] = { 'i', 'd', 'n', 'v', 'm' };

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_id_by_num(ePAR_TEST_MODE, &mode_id));
    TEST_ASSERT_OK(par_get_id_by_num(ePAR_TEST_STR, &str_id));
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 3U));
    TEST_ASSERT_OK(par_save_by_id(mode_id));
    scalar_value.u16 = 777U;
    TEST_ASSERT_OK(par_set_scalar_n_save(ePAR_TEST_U16, &scalar_value.u16));
    TEST_ASSERT_OK(par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"nsave", 5U));
    TEST_ASSERT_STATUS(par_save_by_id(0xFFFFU), ePAR_ERROR);
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(u8 == 3U);
    TEST_ASSERT(u16 == 777U);
    TEST_ASSERT(len == 5U);
    TEST_ASSERT(strcmp(str_buf, "nsave") == 0);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save_all());
    TEST_ASSERT_OK(par_set_obj_n_save_by_id(str_id, str_payload, (uint16_t)sizeof(str_payload)));
    TEST_ASSERT_OK(par_deinit());

    memset(str_buf, 0, sizeof(str_buf));
    len = 0U;
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(u8 == 4U);
    TEST_ASSERT(u16 == 777U);
    TEST_ASSERT(len == sizeof(str_payload));
    TEST_ASSERT(strcmp(str_buf, "idnvm") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Save a committed scalar, then fail a save-all rewrite in a child. */
static bool host_flash_child_fail_save_all_after_commit(const int program_fail_after)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    g_fail_program_after = program_fail_after;
    TEST_ASSERT((par_save_all() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    return true;
}

/** @brief Verify failed bulk rewrites do not publish the partially written live value. */
static bool test_flash_ee_save_all_failpoint_preserves_last_committed_scalar(void)
{
    static const int fail_after_cases[] = { 0, 1 };

    for (size_t i = 0U; i < (sizeof(fail_after_cases) / sizeof(fail_after_cases[0])); i++)
    {
        pid_t child_pid;

        host_flash_reset_erased();
        child_pid = fork();
        TEST_ASSERT(child_pid >= 0);
        if (0 == child_pid)
        {
            host_child_exit_from_result(host_flash_child_fail_save_all_after_commit(fail_after_cases[i]));
        }
        TEST_ASSERT(host_child_exit_is_success(child_pid));

        child_pid = fork();
        TEST_ASSERT(child_pid >= 0);
        if (0 == child_pid)
        {
            host_child_exit_from_result(host_flash_child_verify_mode_value(4U));
        }
        TEST_ASSERT(host_child_exit_is_success(child_pid));
    }

    return true;
}

/** @brief Verify raw fake-flash read failpoints report errors without mutation. */
static bool test_flash_ee_port_read_failpoint_reports_error(void)
{
    uint8_t before[HOST_FLASH_PROGRAM_SIZE];
    uint8_t after[HOST_FLASH_PROGRAM_SIZE];
    const uint8_t payload[HOST_FLASH_PROGRAM_SIZE] = { 1U, 2U, 3U, 4U };

    host_flash_reset_erased();
    TEST_ASSERT_OK(par_store_flash_ee_native_port_init());
    TEST_ASSERT_OK(par_store_flash_ee_native_port_program(0U,
                                                          (uint32_t)sizeof(payload),
                                                          payload));
    TEST_ASSERT_OK(par_store_flash_ee_native_port_read(0U, (uint32_t)sizeof(before), before));
    g_fail_read_after = 0;
    TEST_ASSERT_STATUS(par_store_flash_ee_native_port_read(0U,
                                                           (uint32_t)sizeof(after),
                                                           after),
                       ePAR_ERROR);
    host_flash_clear_failpoints();
    TEST_ASSERT_OK(par_store_flash_ee_native_port_read(0U, (uint32_t)sizeof(after), after));
    TEST_ASSERT(0 == memcmp(before, after, sizeof(before)));
    TEST_ASSERT_OK(par_store_flash_ee_native_port_deinit());
    return true;
}

#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED)
/**
 * @brief Reinitialize the module and verify the persisted string object value.
 * @param expected Null-terminated string expected after reload.
 * @return true when the reloaded object matches @p expected.
 */
static bool verify_reloaded_object_str(const char *const expected)
{
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    TEST_ASSERT_OK(par_deinit());
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == (uint16_t)strlen(expected));
    TEST_ASSERT(strcmp(str_buf, expected) == 0);
    return true;
}

/** @brief Verify a dedicated object write failpoint does not mutate backend bytes. */
static bool test_object_dedicated_write_fail_preserves_backend_image(void)
{
    uint8_t before[HOST_FLASH_SIZE];

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "old"));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    memcpy(before, g_object_flash, sizeof(before));

    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "new"));
    g_object_fail_write_after = 0;
    TEST_ASSERT((par_save(ePAR_TEST_STR) & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    TEST_ASSERT(0 == memcmp(before, g_object_flash, sizeof(before)));
    host_flash_clear_failpoints();
    TEST_ASSERT(verify_reloaded_object_str("old"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify dedicated object readback failures report NVM errors. */
static bool test_object_dedicated_read_fail_reports_error_and_recovers_object(void)
{
    uint8_t u8 = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 6U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "old"));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "new"));
    g_object_fail_read_after = 0;
    TEST_ASSERT((par_save(ePAR_TEST_STR) & ePAR_ERROR_NVM) != 0U);
    host_flash_clear_failpoints();
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 6U);
    TEST_ASSERT(verify_reloaded_object_str("new"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify dedicated object sync failures report NVM errors. */
static bool test_object_dedicated_sync_fail_reports_error_and_recovers_object(void)
{
    uint8_t u8 = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 7U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "old"));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "new"));
    g_object_fail_sync_after = 0;
    TEST_ASSERT((par_save(ePAR_TEST_STR) & ePAR_ERROR_NVM) != 0U);
    host_flash_clear_failpoints();
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 7U);
    TEST_ASSERT(verify_reloaded_object_str("new"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify dedicated object bulk erase failures abort the object rewrite. */
static bool test_object_dedicated_erase_fail_aborts_save_all(void)
{
    uint8_t u8 = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 8U));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "old"));
    TEST_ASSERT_OK(par_save_all());
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "new"));
    g_object_fail_erase_after = 0;
    TEST_ASSERT((par_save_all() & ePAR_ERROR_NVM) != 0U);
    host_flash_clear_failpoints();
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 8U);
    TEST_ASSERT(verify_reloaded_object_str("old"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED) */

/** @brief Entrypoint for NVM Flash EE host tests. */
int main(void)
{
    int result;
    static const par_host_test_case_t cases[] = {
        { "nvm_first_boot_formats_and_restores_defaults", test_nvm_first_boot_formats_and_restores_defaults },
        { "nvm_save_reload_preserves_last_committed_scalar_values", test_nvm_save_reload_preserves_last_committed_scalar_values },
        { "nvm_object_save_reload_shared_fixed_addr", test_nvm_object_save_reload_shared_fixed_addr },
        { "nvm_object_updates_do_not_corrupt_scalar_values", test_nvm_object_updates_do_not_corrupt_scalar_values },
        { "nvm_scalar_updates_do_not_corrupt_object_values", test_nvm_scalar_updates_do_not_corrupt_object_values },
        { "flash_ee_failed_program_reload_preserves_last_committed_value", test_flash_ee_failed_program_reload_preserves_last_committed_value },
        { "flash_ee_program_failpoint_matrix_preserves_last_commit", test_flash_ee_program_failpoint_matrix_preserves_last_commit },
        { "flash_ee_save_all_failpoint_preserves_last_committed_scalar", test_flash_ee_save_all_failpoint_preserves_last_committed_scalar },
        { "flash_ee_failed_program_graceful_deinit_commits_live_value", test_flash_ee_failed_program_graceful_deinit_commits_live_value },
        { "flash_ee_corruption_rebuilds_default_value", test_flash_ee_corruption_rebuilds_default_value },
        { "nvm_table_id_mismatch_rebuilds_defaults", test_nvm_table_id_mismatch_rebuilds_defaults },
        { "flash_ee_failed_erase_preserves_existing_bytes", test_flash_ee_failed_erase_preserves_existing_bytes },
        { "flash_ee_many_updates_preserve_last_committed_value", test_flash_ee_many_updates_preserve_last_committed_value },
        { "flash_ee_port_rejects_wrapped_ranges", test_flash_ee_port_rejects_wrapped_ranges },
        { "flash_ee_program_one_to_zero_semantics", test_flash_ee_program_one_to_zero_semantics },
        { "flash_ee_port_read_failpoint_reports_error", test_flash_ee_port_read_failpoint_reports_error },
        { "nvm_save_by_id_save_all_and_n_save_wrappers", test_nvm_save_by_id_save_all_and_n_save_wrappers },
        { "msh_save_persists_live_scalar_after_restart", test_msh_save_persists_live_scalar_after_restart },
        { "msh_save_clean_rewrites_live_values", test_msh_save_clean_rewrites_live_values },
        { "msh_save_reports_backend_error", test_msh_save_reports_backend_error },
#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED)
        { "object_dedicated_write_fail_preserves_backend_image", test_object_dedicated_write_fail_preserves_backend_image },
        { "object_dedicated_read_fail_reports_error_and_recovers_object", test_object_dedicated_read_fail_reports_error_and_recovers_object },
        { "object_dedicated_sync_fail_reports_error_and_recovers_object", test_object_dedicated_sync_fail_reports_error_and_recovers_object },
        { "object_dedicated_erase_fail_aborts_save_all", test_object_dedicated_erase_fail_aborts_save_all },
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED) */
    };

    printf("PAR_HOST_NVM_PROFILE %s\n", PAR_HOST_TEST_PROFILE_NAME);
    result = par_host_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
    host_flash_remove_image();
    return result;
}
