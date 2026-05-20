
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
/** @brief Address captured when the program failpoint rejects a write. */
static uint32_t g_fail_program_addr;
/** @brief Size captured when the program failpoint rejects a write. */
static uint32_t g_fail_program_size;
/** @brief true once the program failpoint has rejected one write. */
static bool g_fail_program_hit;
/** @brief Read failpoint countdown; negative disables it. */
static int g_fail_read_after = HOST_FLASH_FAIL_DISABLED;
/** @brief Erase failpoint countdown; negative disables it. */
static int g_fail_erase_after = HOST_FLASH_FAIL_DISABLED;
/** @brief Program corruption countdown; negative disables it. */
static int g_corrupt_program_after = HOST_FLASH_FAIL_DISABLED;
/** @brief Readback corruption countdown; negative disables it. */
static int g_corrupt_read_after = HOST_FLASH_FAIL_DISABLED;
/** @brief Dedicated object-store fake flash bytes. */
static uint8_t g_object_flash[HOST_FLASH_SIZE];
/** @brief Process-unique dedicated object fake flash image path. */
static char g_object_flash_image_path[HOST_FLASH_IMAGE_PATH_LEN + 16U];
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
/** @brief Dedicated object-store write corruption countdown; negative disables it. */
static int g_object_corrupt_write_after = HOST_FLASH_FAIL_DISABLED;
/** @brief Dedicated object-store readback corruption countdown; negative disables it. */
static int g_object_corrupt_read_after = HOST_FLASH_FAIL_DISABLED;
