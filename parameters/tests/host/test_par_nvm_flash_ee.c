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
#include "par_if.h"
#include "par_nvm_table_id.h"
#include "par_nvm_object_store.h"
#include "par_nvm_object.h"
#include "par_object.h"
#include "par_registration_api.h"
#include "fnv.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
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
/** @brief Offset of the serialized scalar NVM object-count field in host flash. */
#define HOST_NVM_HEAD_OBJ_NB_OFFSET ((uint32_t)sizeof(uint32_t))
/** @brief Offset of the serialized scalar NVM table-ID field in host flash. */
#define HOST_NVM_HEAD_TABLE_ID_OFFSET \
    ((uint32_t)sizeof(uint32_t) + (uint32_t)sizeof(uint16_t))
/** @brief Size of the serialized scalar NVM table-ID field in host flash. */
#define HOST_NVM_HEAD_TABLE_ID_SIZE   ((uint32_t)sizeof(uint32_t))

/** @brief Offset of the serialized scalar NVM header CRC field in host flash. */
#define HOST_NVM_HEAD_CRC_OFFSET \
    (HOST_NVM_HEAD_TABLE_ID_OFFSET + HOST_NVM_HEAD_TABLE_ID_SIZE)
/** @brief Parameter signature value stored in the scalar NVM header. */
#define HOST_NVM_SIGN                 (0xFF00AA55UL)
/** @brief Flash-EE bank count used by the native host backend. */
#define HOST_FLASH_EE_BANK_COUNT      (2U)
/** @brief Flash-EE bank header magic. */
#define HOST_FLASH_EE_HEADER_MAGIC    (0x50454548UL)
/** @brief Flash-EE active-bank state word. */
#define HOST_FLASH_EE_HEADER_ACTIVE   (0xFFFF0000UL)
/** @brief Flash-EE append-record commit-unit magic. */
#define HOST_FLASH_EE_RECORD_MAGIC    (0x50454552UL)
/** @brief Flash-EE prepared-bank state word. */
#define HOST_FLASH_EE_HEADER_PREPARE  (0xFFFFFF00UL)
/** @brief Flash-EE bank header size. */
#define HOST_FLASH_EE_HEADER_SIZE     (64U)
/** @brief Flash-EE record metadata size. */
#define HOST_FLASH_EE_RECORD_META_SIZE (12U)
/** @brief Flash-EE line size used by host NVM tests. */
#define HOST_FLASH_EE_LINE_SIZE       ((uint32_t)PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE)
/** @brief Flash-EE logical line count in the host test geometry. */
#define HOST_FLASH_EE_LINE_COUNT \
    ((uint32_t)(PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE / PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE))
/** @brief Flash-EE record commit-unit offset. */
#define HOST_FLASH_EE_RECORD_COMMIT_OFFSET \
    (((HOST_FLASH_EE_LINE_SIZE + HOST_FLASH_EE_RECORD_META_SIZE + HOST_FLASH_PROGRAM_SIZE - 1U) / \
      HOST_FLASH_PROGRAM_SIZE) * HOST_FLASH_PROGRAM_SIZE)
/** @brief Flash-EE append-record size used by host NVM tests. */
#define HOST_FLASH_EE_RECORD_SIZE \
    (HOST_FLASH_EE_RECORD_COMMIT_OFFSET + HOST_FLASH_PROGRAM_SIZE)
/** @brief Flash-EE physical bank size in the host fake flash. */
#define HOST_FLASH_EE_BANK_SIZE       (HOST_FLASH_SIZE / HOST_FLASH_EE_BANK_COUNT)
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

#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/** @brief Callback hit counter for NVM callback current-policy tests. */
static unsigned g_nvm_callback_hits;

/**
 * @brief Persist the changed scalar from inside an on-change callback.
 * @param par_num Parameter number that changed.
 * @param new_val New scalar value.
 * @param old_val Previous scalar value.
 */
static void on_nvm_scalar_change_save(const par_num_t par_num,
                                      const par_type_t new_val,
                                      const par_type_t old_val)
{
    (void)new_val;
    (void)old_val;
    g_nvm_callback_hits++;
    (void)par_save(par_num);
}

/**
 * @brief Persist all live values from inside an on-change callback.
 * @param par_num Parameter number that changed.
 * @param new_val New scalar value.
 * @param old_val Previous scalar value.
 */
static void on_nvm_scalar_change_save_all(const par_num_t par_num,
                                          const par_type_t new_val,
                                          const par_type_t old_val)
{
    (void)par_num;
    (void)new_val;
    (void)old_val;
    g_nvm_callback_hits++;
    (void)par_save_all();
}

/**
 * @brief Rewrite clean NVM state from inside an on-change callback.
 * @param par_num Parameter number that changed.
 * @param new_val New scalar value.
 * @param old_val Previous scalar value.
 */
static void on_nvm_scalar_change_save_clean(const par_num_t par_num,
                                            const par_type_t new_val,
                                            const par_type_t old_val)
{
    (void)par_num;
    (void)new_val;
    (void)old_val;
    g_nvm_callback_hits++;
    (void)par_save_clean();
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

/**
 * @brief Deinitialize the parameter module from inside an on-change callback.
 * @param par_num Parameter number that changed.
 * @param new_val New scalar value.
 * @param old_val Previous scalar value.
 */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
static void on_nvm_scalar_change_deinit(const par_num_t par_num,
                                        const par_type_t new_val,
                                        const par_type_t old_val)
{
    (void)par_num;
    (void)new_val;
    (void)old_val;
    g_nvm_callback_hits++;
    (void)par_deinit();
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */


#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (1 == PAR_CFG_ENABLE_TYPE_STR)
/** @brief Persist the current object from inside object validation. */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
static bool on_nvm_object_validation_save(const par_num_t par_num,
                                          const uint8_t *p_data,
                                          const uint16_t len)
{
    (void)p_data;
    (void)len;
    g_nvm_callback_hits++;
    return (ePAR_OK == par_save(par_num));
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

/** @brief Persist all current values from inside object validation. */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
static bool on_nvm_object_validation_save_all(const par_num_t par_num,
                                              const uint8_t *p_data,
                                              const uint16_t len)
{
    (void)par_num;
    (void)p_data;
    (void)len;
    g_nvm_callback_hits++;
    return (ePAR_OK == par_save_all());
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

/** @brief Rewrite clean current values from inside object validation. */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
static bool on_nvm_object_validation_save_clean(const par_num_t par_num,
                                                const uint8_t *p_data,
                                                const uint16_t len)
{
    (void)par_num;
    (void)p_data;
    (void)len;
    g_nvm_callback_hits++;
    return (ePAR_OK == par_save_clean());
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (1 == PAR_CFG_ENABLE_TYPE_STR) */

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


/**
 * @brief Calculate the Flash-EE bank-header geometry CRC.
 * @param p_header Header fields to hash.
 * @return Header CRC value widened to 32 bits.
 */
static uint32_t host_flash_ee_calc_bank_header_crc(const host_flash_ee_bank_header_t * const p_header)
{
    uint16_t crc = PAR_IF_CRC16_INIT;

    TEST_ASSERT(NULL != p_header);
    crc = par_if_crc16_accumulate(crc,
                                  (const uint8_t *)p_header,
                                  (uint32_t)offsetof(host_flash_ee_bank_header_t, cfg_crc));
    return (uint32_t)crc;
}

/**
 * @brief Read a physical Flash-EE bank header from the fake image.
 * @param bank_base Physical bank base address.
 * @param p_header Output header mirror.
 * @return true when the header was copied from the fake image.
 */
static bool host_flash_ee_read_bank_header(const uint32_t bank_base,
                                           host_flash_ee_bank_header_t * const p_header)
{
    TEST_ASSERT(NULL != p_header);
    TEST_ASSERT((bank_base + (uint32_t)sizeof(*p_header)) <= HOST_FLASH_SIZE);
    memcpy(p_header, &g_flash[bank_base], sizeof(*p_header));
    return true;
}

/**
 * @brief Write one synthetic Flash-EE bank header into the fake image.
 * @param bank_base Physical bank base address.
 * @param seq Bank sequence number to serialize.
 * @param state Bank state word to serialize.
 * @return true when the header was written.
 */
static bool host_flash_ee_write_bank_header(const uint32_t bank_base,
                                            const uint32_t seq,
                                            const uint32_t state)
{
    host_flash_ee_bank_header_t header;

    TEST_ASSERT((bank_base + (uint32_t)sizeof(header)) <= HOST_FLASH_SIZE);
    memset(&header, 0xFF, sizeof(header));
    header.magic = HOST_FLASH_EE_HEADER_MAGIC;
    header.version = PAR_CFG_NVM_BACKEND_FLASH_EE_VERSION;
    header.logical_size = PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE;
    header.line_size = HOST_FLASH_EE_LINE_SIZE;
    header.record_size = HOST_FLASH_EE_RECORD_SIZE;
    header.bank_size = HOST_FLASH_EE_BANK_SIZE;
    header.seq = seq;
    header.state = state;
    header.cfg_crc = host_flash_ee_calc_bank_header_crc(&header);
    memcpy(&g_flash[bank_base], &header, sizeof(header));
    host_flash_write_image();
    return true;
}

/**
 * @brief Write a prepared or invalid higher-sequence inactive bank header.
 * @param state Bank state word to place in the inactive bank.
 * @param corrupt_crc true to corrupt the serialized geometry CRC.
 * @return true when the inactive bank header was written.
 */
static bool host_flash_ee_write_higher_seq_inactive_header(const uint32_t state,
                                                           const bool corrupt_crc)
{
    uint32_t active_base = 0U;
    uint32_t next_offset = 0U;
    uint32_t inactive_base = 0U;
    host_flash_ee_bank_header_t active_header = { 0 };

    TEST_ASSERT(host_flash_ee_find_active_bank(&active_base, &next_offset));
    TEST_ASSERT(host_flash_ee_read_bank_header(active_base, &active_header));
    inactive_base = (0U == active_base) ? HOST_FLASH_EE_BANK_SIZE : 0U;
    memset(&g_flash[inactive_base], 0xFF, HOST_FLASH_EE_BANK_SIZE);
    TEST_ASSERT(host_flash_ee_write_bank_header(inactive_base, active_header.seq + 1U, state));
    if (true == corrupt_crc)
    {
        g_flash[inactive_base + (uint32_t)offsetof(host_flash_ee_bank_header_t, cfg_crc)] ^= 0x01U;
        host_flash_write_image();
    }
    return true;
}


/**
 * @brief Append an intentionally uncommitted physical Flash-EE tail record.
 * @param line_index Logical EEPROM line index to encode in the partial record.
 * @return true when the partial record was written to the fake image.
 */
static bool host_flash_ee_append_uncommitted_tail_record(const uint32_t line_index)
{
    uint32_t bank_base = 0U;
    uint32_t next_offset = 0U;
    uint8_t record[HOST_FLASH_EE_RECORD_SIZE];
    host_flash_ee_record_meta_t meta = { 0 };

    TEST_ASSERT(host_flash_ee_find_active_bank(&bank_base, &next_offset));
    TEST_ASSERT((next_offset + HOST_FLASH_EE_RECORD_SIZE) <= HOST_FLASH_EE_BANK_SIZE);

    memset(record, 0xFF, sizeof(record));
    memset(record, 0x00, HOST_FLASH_EE_LINE_SIZE);
    meta.line_index = line_index;
    meta.payload_size = (uint16_t)HOST_FLASH_EE_LINE_SIZE;
    meta.record_crc = 0U;
    meta.reserved = 0UL;
    memcpy(&record[HOST_FLASH_EE_LINE_SIZE], &meta, sizeof(meta));
    record[HOST_FLASH_EE_RECORD_COMMIT_OFFSET] = 'R';
    memcpy(&g_flash[bank_base + next_offset], record, sizeof(record));
    host_flash_write_image();
    return true;
}

/**
 * @brief Append a committed but CRC-invalid tail record.
 * @details This fixture locks the Flash-EE replay current policy: a bad
 *          committed tail record makes the image untrusted and triggers
 *          default rebuild instead of falling back to older good records.
 *          Change this expectation only with an intentional recovery-policy
 *          change.
 * @param line_index Logical EEPROM line index to encode in the bad record.
 * @return true when the bad committed tail record was written.
 */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
static bool host_flash_ee_append_bad_committed_tail_record(const uint32_t line_index)
{
    uint32_t bank_base = 0U;
    uint32_t next_offset = 0U;
    uint8_t record[HOST_FLASH_EE_RECORD_SIZE];
    host_flash_ee_record_meta_t meta = { 0 };

    TEST_ASSERT(host_flash_ee_find_active_bank(&bank_base, &next_offset));
    TEST_ASSERT((next_offset + HOST_FLASH_EE_RECORD_SIZE) <= HOST_FLASH_EE_BANK_SIZE);

    memset(record, 0xFF, sizeof(record));
    memset(record, 0x00, HOST_FLASH_EE_LINE_SIZE);
    meta.line_index = line_index;
    meta.payload_size = (uint16_t)HOST_FLASH_EE_LINE_SIZE;
    meta.record_crc = 0x1234U;
    meta.reserved = 0xFFFFFFFFUL;
    memcpy(&record[HOST_FLASH_EE_LINE_SIZE], &meta, sizeof(meta));
    memcpy(&record[HOST_FLASH_EE_RECORD_COMMIT_OFFSET],
           &(const uint32_t){ HOST_FLASH_EE_RECORD_MAGIC },
           sizeof(uint32_t));
    memcpy(&g_flash[bank_base + next_offset], record, sizeof(record));
    host_flash_write_image();
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

/**
 * @brief Calculate the scalar NVM header CRC for one count/table-ID pair.
 * @param count Stored persistent scalar count.
 * @param table_id Stored table-ID value.
 * @return Header CRC-16 value.
 */
static uint16_t host_scalar_header_crc(const uint16_t count,
                                       const uint32_t table_id)
{
    uint16_t crc = PAR_IF_CRC16_INIT;

    crc = par_if_crc16_accumulate(crc, (const uint8_t *)&count, (uint32_t)sizeof(count));
    crc = par_if_crc16_accumulate(crc, (const uint8_t *)&table_id, (uint32_t)sizeof(table_id));
    return crc;
}

/**
 * @brief Update a relaxed scalar table-ID digest with native-endian bytes.
 * @param p_hval Pointer to the rolling FNV-1a state.
 * @param p_serialized_size Pointer to the serialized byte counter.
 * @param p_value Pointer to bytes that should be hashed.
 * @param value_size Byte count to hash.
 */
static void host_scalar_table_id_hash_update(Fnv32_t * const p_hval,
                                             uint32_t * const p_serialized_size,
                                             const void * const p_value,
                                             const uint32_t value_size)
{
    *p_hval = fnv_32a_buf((void *)p_value, (size_t)value_size, *p_hval);
    *p_serialized_size += value_size;
}

/**
 * @brief Calculate a scalar table-ID digest without count-range assertions.
 * @details The production helper asserts when @p persistent_count is larger
 * than the current compile-time persistent scalar count. This host-only mirror
 * intentionally keeps the same serialized inputs while allowing oversized
 * counts, so count-overflow tests do not rely on a table-ID mismatch.
 * @param persistent_count Stored persistent scalar count to encode in digest.
 * @return Relaxed scalar table-ID digest for test image construction.
 */
static uint32_t host_scalar_table_id_calc_relaxed_count(const uint16_t persistent_count)
{
    Fnv32_t hval = FNV1_32A_INIT;
    uint32_t serialized_size = 0U;
    uint16_t hashed_count = 0U;
    const uint32_t schema_version = (uint32_t)PAR_CFG_TABLE_ID_SCHEMA_VER;
    const uint32_t record_layout = (uint32_t)PAR_CFG_NVM_RECORD_LAYOUT;

    host_scalar_table_id_hash_update(&hval,
                                     &serialized_size,
                                     &schema_version,
                                     (uint32_t)sizeof(schema_version));
    host_scalar_table_id_hash_update(&hval,
                                     &serialized_size,
                                     &record_layout,
                                     (uint32_t)sizeof(record_layout));
    host_scalar_table_id_hash_update(&hval,
                                     &serialized_size,
                                     &persistent_count,
                                     (uint32_t)sizeof(persistent_count));

    for (par_num_t par_num = 0U; par_num < ePAR_NUM_OF; par_num++)
    {
        const par_cfg_t * const p_cfg = par_get_config(par_num);
        const uint8_t type = (uint8_t)p_cfg->type;

        if (false == p_cfg->persistent)
        {
            continue;
        }

#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
        if (true == par_object_type_is_object(p_cfg->type))
        {
            continue;
        }
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

        if (hashed_count >= persistent_count)
        {
            break;
        }

        host_scalar_table_id_hash_update(&hval,
                                         &serialized_size,
                                         &type,
                                         (uint32_t)sizeof(type));

#if (1 == PAR_CFG_NVM_RECORD_LAYOUT_HAS_STORED_ID)
        {
            const uint16_t parameter_id = par_cfg_get_param_id_const(par_num);

            host_scalar_table_id_hash_update(&hval,
                                             &serialized_size,
                                             &parameter_id,
                                             (uint32_t)sizeof(parameter_id));
        }
#endif /* (1 == PAR_CFG_NVM_RECORD_LAYOUT_HAS_STORED_ID) */
        hashed_count++;
    }

    TEST_ASSERT(serialized_size > 0U);
    return (uint32_t)hval;
}

/**
 * @brief Rewrite the logical scalar NVM header through the active backend.
 * @param count Stored persistent scalar count to serialize.
 * @return true when the logical header was updated and synchronized.
 */
static bool host_scalar_store_write_header_count(const uint16_t count)
{
    const par_store_backend_api_t * const p_store = par_store_backend_flash_ee_get_api();
    const uint32_t table_id = par_nvm_table_id_calc_for_count(count);
    const uint16_t crc = host_scalar_header_crc(count, table_id);
    uint8_t header[HOST_NVM_HEAD_CRC_OFFSET + (uint32_t)sizeof(crc)] = { 0U };
    const uint32_t sign = HOST_NVM_SIGN;

    TEST_ASSERT(NULL != p_store);
    TEST_ASSERT(NULL != p_store->write);
    TEST_ASSERT(NULL != p_store->sync);
    memcpy(&header[0], &sign, sizeof(sign));
    memcpy(&header[HOST_NVM_HEAD_OBJ_NB_OFFSET], &count, sizeof(count));
    memcpy(&header[HOST_NVM_HEAD_TABLE_ID_OFFSET], &table_id, sizeof(table_id));
    memcpy(&header[HOST_NVM_HEAD_CRC_OFFSET], &crc, sizeof(crc));
    TEST_ASSERT_OK(p_store->write(0U, (uint32_t)sizeof(header), header));
    TEST_ASSERT_OK(p_store->sync());
    return true;
}


/**
 * @brief Rewrite the logical scalar NVM header with an arbitrary table ID.
 * @param count Stored persistent scalar count to serialize.
 * @param table_id Stored table-ID value to serialize.
 * @return true when the logical header was updated and synchronized.
 */
static bool host_scalar_store_write_raw_header(const uint16_t count,
                                               const uint32_t table_id)
{
    const par_store_backend_api_t * const p_store = par_store_backend_flash_ee_get_api();
    const uint16_t crc = host_scalar_header_crc(count, table_id);
    uint8_t header[HOST_NVM_HEAD_CRC_OFFSET + (uint32_t)sizeof(crc)] = { 0U };
    const uint32_t sign = HOST_NVM_SIGN;

    TEST_ASSERT(NULL != p_store);
    TEST_ASSERT(NULL != p_store->write);
    TEST_ASSERT(NULL != p_store->sync);
    memcpy(&header[0], &sign, sizeof(sign));
    memcpy(&header[HOST_NVM_HEAD_OBJ_NB_OFFSET], &count, sizeof(count));
    memcpy(&header[HOST_NVM_HEAD_TABLE_ID_OFFSET], &table_id, sizeof(table_id));
    memcpy(&header[HOST_NVM_HEAD_CRC_OFFSET], &crc, sizeof(crc));
    TEST_ASSERT_OK(p_store->write(0U, (uint32_t)sizeof(header), header));
    TEST_ASSERT_OK(p_store->sync());
    return true;
}


#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) && \
    (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_FIXED)
/**
 * @brief Flip one byte through the active object-store abstraction.
 * @param addr Backend byte address accepted by the object-store API.
 * @param mask XOR corruption mask.
 * @return true when the object store byte was modified and flushed.
 */
static bool host_object_store_xor_byte(const uint32_t addr, const uint8_t mask)
{
    const par_store_backend_api_t * const p_store = par_nvm_object_store_get_api();
    uint8_t value = 0U;

    TEST_ASSERT(NULL != p_store);
    TEST_ASSERT(NULL != p_store->read);
    TEST_ASSERT(NULL != p_store->write);
    TEST_ASSERT(NULL != p_store->sync);
    TEST_ASSERT_OK(p_store->read(addr, 1U, &value));
    value ^= mask;
    TEST_ASSERT_OK(p_store->write(addr, 1U, &value));
    TEST_ASSERT_OK(p_store->sync());
    return true;
}

/** @brief Initialize the module with a preserved fake flash image. */
static bool init_module(void);

/**
 * @brief Host mirror of serialized object persistence record metadata.
 * @details Production object NVM stores this metadata in native byte order
 * before the payload. The helper below validates every neighbor field before
 * returning the address of the serialized length field.
 */
typedef struct
{
    uint16_t id;        /**< External parameter ID. */
    uint8_t type;       /**< Serialized par_type_list_t value. */
    uint8_t flags;      /**< Reserved flags. */
    uint16_t elem_size; /**< Object element size in bytes. */
    uint16_t capacity;  /**< Fixed payload capacity in bytes. */
    uint16_t len;       /**< Valid payload length in bytes. */
    uint16_t crc;       /**< CRC-16 over metadata and payload bytes. */
} host_object_store_record_meta_t;

/** @brief Reserved flags value expected in host object NVM records. */
#define HOST_OBJECT_STORE_RECORD_FLAGS_NONE (0U)

/** @brief Serialized object block signature. */
#define HOST_OBJECT_STORE_SIGN (0x504A424FUL)
/** @brief Serialized object block version. */
#define HOST_OBJECT_STORE_VERSION (1U)

/**
 * @brief Host mirror of serialized object persistence block header.
 */
typedef struct
{
    uint32_t sign;      /**< Object block signature. */
    uint16_t version;   /**< Serialized-layout version. */
    uint16_t obj_nb;    /**< Stored persistent object count. */
    uint32_t table_id;  /**< Object persistent-schema digest. */
    uint32_t body_size; /**< Serialized object-record body size in bytes. */
    uint16_t crc;       /**< Header CRC-16. */
} host_object_store_head_t;

/** @brief Serialized object persistence header size. */
#define HOST_OBJECT_STORE_HEAD_SIZE \
    ((uint32_t)offsetof(host_object_store_head_t, crc) + (uint32_t)sizeof(uint16_t))

/**
 * @brief Read logical bytes through the active object-store abstraction.
 * @param addr Backend byte address accepted by the object-store API.
 * @param size Byte count to read.
 * @param p_buf Output buffer.
 * @return true when bytes were read.
 */
static bool host_object_store_read_bytes(const uint32_t addr,
                                         const uint32_t size,
                                         uint8_t * const p_buf)
{
    const par_store_backend_api_t * const p_store = par_nvm_object_store_get_api();

    TEST_ASSERT(NULL != p_store);
    TEST_ASSERT(NULL != p_store->read);
    TEST_ASSERT(NULL != p_buf);
    TEST_ASSERT_OK(p_store->read(addr, size, p_buf));
    return true;
}

/**
 * @brief Write logical bytes through the active object-store abstraction.
 * @param addr Backend byte address accepted by the object-store API.
 * @param size Byte count to write.
 * @param p_buf Source buffer.
 * @return true when bytes were written and flushed.
 */
static bool host_object_store_write_bytes(const uint32_t addr,
                                          const uint32_t size,
                                          const uint8_t * const p_buf)
{
    const par_store_backend_api_t * const p_store = par_nvm_object_store_get_api();

    TEST_ASSERT(NULL != p_store);
    TEST_ASSERT(NULL != p_store->write);
    TEST_ASSERT(NULL != p_store->sync);
    TEST_ASSERT(NULL != p_buf);
    TEST_ASSERT_OK(p_store->write(addr, size, p_buf));
    TEST_ASSERT_OK(p_store->sync());
    return true;
}

/**
 * @brief Calculate the serialized object-header CRC for host test mutations.
 * @param p_head Header fields to hash.
 * @return Header CRC-16 value.
 */
static uint16_t host_object_store_calc_head_crc(const host_object_store_head_t * const p_head)
{
    uint16_t crc = PAR_IF_CRC16_INIT;

    if (NULL == p_head)
    {
        return 0U;
    }

    crc = par_if_crc16_accumulate(crc, (const uint8_t *)&p_head->version, (uint32_t)sizeof(p_head->version));
    crc = par_if_crc16_accumulate(crc, (const uint8_t *)&p_head->obj_nb, (uint32_t)sizeof(p_head->obj_nb));
    crc = par_if_crc16_accumulate(crc, (const uint8_t *)&p_head->table_id, (uint32_t)sizeof(p_head->table_id));
    crc = par_if_crc16_accumulate(crc, (const uint8_t *)&p_head->body_size, (uint32_t)sizeof(p_head->body_size));
    return crc;
}

/**
 * @brief Read and unpack the object block header.
 * @param base_addr Backend address of the object block header.
 * @param p_head Output header mirror.
 * @return true when the header was read and unpacked.
 */
static bool host_object_store_read_head(const uint32_t base_addr,
                                        host_object_store_head_t * const p_head)
{
    uint8_t head_buf[HOST_OBJECT_STORE_HEAD_SIZE] = { 0U };

    TEST_ASSERT(NULL != p_head);
    TEST_ASSERT(HOST_OBJECT_STORE_HEAD_SIZE == 18U);
    TEST_ASSERT(host_object_store_read_bytes(base_addr,
                                             (uint32_t)sizeof(head_buf),
                                             head_buf));
    memcpy(&p_head->sign, &head_buf[offsetof(host_object_store_head_t, sign)], sizeof(p_head->sign));
    memcpy(&p_head->version, &head_buf[offsetof(host_object_store_head_t, version)], sizeof(p_head->version));
    memcpy(&p_head->obj_nb, &head_buf[offsetof(host_object_store_head_t, obj_nb)], sizeof(p_head->obj_nb));
    memcpy(&p_head->table_id, &head_buf[offsetof(host_object_store_head_t, table_id)], sizeof(p_head->table_id));
    memcpy(&p_head->body_size, &head_buf[offsetof(host_object_store_head_t, body_size)], sizeof(p_head->body_size));
    memcpy(&p_head->crc, &head_buf[offsetof(host_object_store_head_t, crc)], sizeof(p_head->crc));
    return true;
}

/**
 * @brief Pack and write the object block header.
 * @param base_addr Backend address of the object block header.
 * @param p_head Header mirror to serialize.
 * @return true when the header was written and flushed.
 */
static bool host_object_store_write_head(const uint32_t base_addr,
                                         const host_object_store_head_t * const p_head)
{
    uint8_t head_buf[HOST_OBJECT_STORE_HEAD_SIZE] = { 0U };

    TEST_ASSERT(NULL != p_head);
    memcpy(&head_buf[offsetof(host_object_store_head_t, sign)], &p_head->sign, sizeof(p_head->sign));
    memcpy(&head_buf[offsetof(host_object_store_head_t, version)], &p_head->version, sizeof(p_head->version));
    memcpy(&head_buf[offsetof(host_object_store_head_t, obj_nb)], &p_head->obj_nb, sizeof(p_head->obj_nb));
    memcpy(&head_buf[offsetof(host_object_store_head_t, table_id)], &p_head->table_id, sizeof(p_head->table_id));
    memcpy(&head_buf[offsetof(host_object_store_head_t, body_size)], &p_head->body_size, sizeof(p_head->body_size));
    memcpy(&head_buf[offsetof(host_object_store_head_t, crc)], &p_head->crc, sizeof(p_head->crc));
    TEST_ASSERT(host_object_store_write_bytes(base_addr,
                                              (uint32_t)sizeof(head_buf),
                                              head_buf));
    return true;
}

/**
 * @brief Read and unpack one object record metadata header.
 * @param record_addr Backend address of the object record metadata header.
 * @param p_meta Output metadata mirror.
 * @return true when metadata was read and unpacked.
 */
static bool host_object_store_read_record_meta(const uint32_t record_addr,
                                               host_object_store_record_meta_t * const p_meta)
{
    const par_store_backend_api_t * const p_store = par_nvm_object_store_get_api();
    uint8_t meta_buf[sizeof(host_object_store_record_meta_t)] = { 0U };

    TEST_ASSERT(NULL != p_store);
    TEST_ASSERT(NULL != p_store->read);
    TEST_ASSERT(NULL != p_meta);
    TEST_ASSERT(sizeof(host_object_store_record_meta_t) == 12U);
    TEST_ASSERT(offsetof(host_object_store_record_meta_t, id) == 0U);
    TEST_ASSERT(offsetof(host_object_store_record_meta_t, type) == 2U);
    TEST_ASSERT(offsetof(host_object_store_record_meta_t, flags) == 3U);
    TEST_ASSERT(offsetof(host_object_store_record_meta_t, elem_size) == 4U);
    TEST_ASSERT(offsetof(host_object_store_record_meta_t, capacity) == 6U);
    TEST_ASSERT(offsetof(host_object_store_record_meta_t, len) == 8U);
    TEST_ASSERT(offsetof(host_object_store_record_meta_t, crc) == 10U);
    TEST_ASSERT_OK(p_store->read(record_addr,
                                 (uint32_t)sizeof(meta_buf),
                                 meta_buf));

    memcpy(&p_meta->id,
           &meta_buf[offsetof(host_object_store_record_meta_t, id)],
           sizeof(p_meta->id));
    memcpy(&p_meta->type,
           &meta_buf[offsetof(host_object_store_record_meta_t, type)],
           sizeof(p_meta->type));
    memcpy(&p_meta->flags,
           &meta_buf[offsetof(host_object_store_record_meta_t, flags)],
           sizeof(p_meta->flags));
    memcpy(&p_meta->elem_size,
           &meta_buf[offsetof(host_object_store_record_meta_t, elem_size)],
           sizeof(p_meta->elem_size));
    memcpy(&p_meta->capacity,
           &meta_buf[offsetof(host_object_store_record_meta_t, capacity)],
           sizeof(p_meta->capacity));
    memcpy(&p_meta->len,
           &meta_buf[offsetof(host_object_store_record_meta_t, len)],
           sizeof(p_meta->len));
    memcpy(&p_meta->crc,
           &meta_buf[offsetof(host_object_store_record_meta_t, crc)],
           sizeof(p_meta->crc));
    return true;
}

/**
 * @brief Return the checked serialized length-field address for an object record.
 * @details The record base is still derived from the production object NVM
 * address helper. This function reads only the record metadata header and
 * validates id, type, flags, element size, capacity, and length before
 * returning the exact length-field address.
 * @param par_num Persistent object parameter number.
 * @param expected_len Expected committed payload length.
 * @param p_len_addr Output address of the serialized length field.
 * @return true when the metadata header matches the expected object record.
 */
static bool host_object_store_get_record_len_addr(const par_num_t par_num,
                                                  const uint16_t expected_len,
                                                  uint32_t * const p_len_addr)
{
    const par_cfg_t * const p_cfg = par_get_config(par_num);
    const uint32_t base_addr = PAR_CFG_NVM_OBJECT_FIXED_ADDR;
    const uint32_t block_size = par_nvm_object_get_block_size();
    const uint32_t record_addr = par_nvm_object_get_addr(base_addr, par_num);
    host_object_store_record_meta_t meta = { 0 };
    uint16_t expected_id = 0U;
    uint16_t expected_capacity = 0U;
    uint32_t record_offset = 0U;

    TEST_ASSERT(NULL != p_len_addr);
    TEST_ASSERT(NULL != p_cfg);
    TEST_ASSERT_OK(par_get_id_by_num(par_num, &expected_id));
    TEST_ASSERT_OK(par_get_obj_capacity(par_num, &expected_capacity));
    TEST_ASSERT(record_addr >= base_addr);
    record_offset = record_addr - base_addr;
    TEST_ASSERT(block_size >= record_offset);
    TEST_ASSERT(((uint32_t)sizeof(host_object_store_record_meta_t)) <=
                (block_size - record_offset));
    TEST_ASSERT(host_object_store_read_record_meta(record_addr, &meta));
    TEST_ASSERT(meta.id == expected_id);
    TEST_ASSERT(meta.type == (uint8_t)p_cfg->type);
    TEST_ASSERT(meta.flags == HOST_OBJECT_STORE_RECORD_FLAGS_NONE);
    TEST_ASSERT(meta.elem_size == p_cfg->value_cfg.object.elem_size);
    TEST_ASSERT(meta.capacity == expected_capacity);
    TEST_ASSERT(meta.len == expected_len);

    *p_len_addr = record_addr +
                  (uint32_t)offsetof(host_object_store_record_meta_t, len);
    return true;
}

/**
 * @brief Return a checked serialized field address for an object record.
 * @param par_num Persistent object parameter number.
 * @param expected_len Expected committed payload length.
 * @param field_offset Offset inside host_object_store_record_meta_t or payload.
 * @param p_field_addr Output serialized field address.
 * @return true when the metadata header matches the expected object record.
 */
static bool host_object_store_get_record_field_addr(const par_num_t par_num,
                                                    const uint16_t expected_len,
                                                    const uint32_t field_offset,
                                                    uint32_t * const p_field_addr)
{
    const uint32_t base_addr = PAR_CFG_NVM_OBJECT_FIXED_ADDR;
    const uint32_t record_addr = par_nvm_object_get_addr(base_addr, par_num);
    uint32_t len_addr = 0U;

    TEST_ASSERT(NULL != p_field_addr);
    TEST_ASSERT(host_object_store_get_record_len_addr(par_num, expected_len, &len_addr));
    (void)len_addr;
    *p_field_addr = record_addr + field_offset;
    return true;
}


/** @brief Verify BYTES payload CRC corruption restores the default payload. */
static bool test_nvm_object_bytes_payload_crc_corruption_restores_default(void)
{
    uint8_t bytes[4] = { 0U };
    uint16_t len = 0U;
    uint32_t crc_addr = 0U;
    const uint8_t payload[4] = { 4U, 3U, 2U, 1U };

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_bytes(ePAR_TEST_BYTES, payload, (uint16_t)sizeof(payload)));
    TEST_ASSERT_OK(par_save(ePAR_TEST_BYTES));
    TEST_ASSERT(host_object_store_get_record_field_addr(ePAR_TEST_BYTES,
                                                        (uint16_t)sizeof(payload),
                                                        (uint32_t)offsetof(host_object_store_record_meta_t, crc),
                                                        &crc_addr));
    TEST_ASSERT(host_object_store_xor_byte(crc_addr, 0x01U));
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_bytes(ePAR_TEST_BYTES, bytes, sizeof(bytes), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(bytes[0] == 0x01U);
    TEST_ASSERT(bytes[1] == 0x02U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}


#if defined(PAR_HOST_TEST_OBJECT_ARRAY_NVM)
/** @brief Verify ARR_U16 element-size corruption restores the default payload. */
static bool test_nvm_object_arr_u16_elem_size_corruption_restores_default(void)
{
    uint16_t arr[2] = { 0U };
    uint16_t count = 0U;
    uint32_t elem_size_addr = 0U;
    const uint16_t payload[2] = { 300U, 400U };

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_arr_u16(ePAR_TEST_ARR_U16, payload, 2U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_ARR_U16));
    TEST_ASSERT(host_object_store_get_record_field_addr(ePAR_TEST_ARR_U16,
                                                        (uint16_t)(2U * sizeof(uint16_t)),
                                                        (uint32_t)offsetof(host_object_store_record_meta_t, elem_size),
                                                        &elem_size_addr));
    TEST_ASSERT(host_object_store_xor_byte(elem_size_addr, 0x01U));
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_arr_u16(ePAR_TEST_ARR_U16, arr, 2U, &count));
    TEST_ASSERT(count == 2U);
    TEST_ASSERT(arr[0] == 100U);
    TEST_ASSERT(arr[1] == 200U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify ARR_U32 capacity corruption restores the default payload. */
static bool test_nvm_object_arr_u32_capacity_corruption_restores_default(void)
{
    uint32_t arr[2] = { 0UL };
    uint16_t count = 0U;
    uint32_t capacity_addr = 0U;
    const uint32_t payload[2] = { 3000UL, 4000UL };

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_arr_u32(ePAR_TEST_ARR_U32, payload, 2U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_ARR_U32));
    TEST_ASSERT(host_object_store_get_record_field_addr(ePAR_TEST_ARR_U32,
                                                        (uint16_t)(2U * sizeof(uint32_t)),
                                                        (uint32_t)offsetof(host_object_store_record_meta_t, capacity),
                                                        &capacity_addr));
    TEST_ASSERT(host_object_store_xor_byte(capacity_addr, 0x01U));
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_arr_u32(ePAR_TEST_ARR_U32, arr, 2U, &count));
    TEST_ASSERT(count == 2U);
    TEST_ASSERT(arr[0] == 1000UL);
    TEST_ASSERT(arr[1] == 2000UL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* defined(PAR_HOST_TEST_OBJECT_ARRAY_NVM) */

#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) && (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_FIXED) */

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

    if (NULL == p_geom)
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

/** @brief Create an uncommitted Flash-EE tail record after a valid scalar commit. */
static bool host_flash_child_append_uncommitted_tail_after_commit(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT(host_flash_ee_append_uncommitted_tail_record(0U));
    return true;
}

/** @brief Verify an uncommitted Flash-EE tail record is ignored on restart. */
static bool test_flash_ee_record_commit_marker_corruption_ignores_partial_record(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_append_uncommitted_tail_after_commit());
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

/** @brief Append a bad committed tail after a valid commit. */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
static bool host_flash_child_append_bad_committed_tail_after_commit(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT(host_flash_ee_append_bad_committed_tail_record(0U));
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

/** @brief Append a bad committed tail after two valid records for the same scalar. */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
static bool host_flash_child_append_newer_bad_record_after_two_commits(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT(host_flash_ee_append_bad_committed_tail_record(0U));
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

/**
 * @brief Verify a bad committed tail rebuilds defaults by current policy.
 * @details The current replay policy does not skip a bad committed tail and
 *          does not fall back to an older good record. Once the committed tail
 *          record fails validation, the image is treated as untrusted and the
 *          runtime is rebuilt from defaults.
 */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
static bool test_flash_ee_bad_committed_tail_rebuilds_default_current_policy(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_append_bad_committed_tail_after_commit());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_verify_mode_value(1U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

/**
 * @brief Verify a newer bad record rebuilds defaults by current policy.
 * @details This intentionally expects the default mode value after restart,
 *          even though older good records exist. Do not change this test to
 *          older-good fallback unless Flash-EE replay policy is changed and
 *          documented explicitly.
 */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
static bool test_flash_ee_newer_bad_record_rebuilds_default_current_policy(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_append_newer_bad_record_after_two_commits());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_verify_mode_value(1U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/** @brief Save a scalar through a change callback and exit without deinit. */
static bool host_flash_child_callback_save_during_dispatch(void)
{
    TEST_ASSERT(init_module());
    g_nvm_callback_hits = 0U;
    par_register_on_change_cb(ePAR_TEST_MODE, on_nvm_scalar_change_save);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT(g_nvm_callback_hits == 1U);
    return true;
}

/** @brief Verify current callback-time save policy survives a hard process restart. */
static bool test_flash_ee_callback_save_during_dispatch_current_policy(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_callback_save_during_dispatch());
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

/** @brief Save all live values from a callback and exit without deinit. */
static bool host_flash_child_callback_save_all_during_dispatch(void)
{
    TEST_ASSERT(init_module());
    g_nvm_callback_hits = 0U;
    par_register_on_change_cb(ePAR_TEST_MODE, on_nvm_scalar_change_save_all);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 7U));
    TEST_ASSERT(g_nvm_callback_hits == 1U);
    return true;
}

/** @brief Verify current callback-time save-all policy survives a hard process restart. */
static bool test_flash_ee_callback_save_all_during_dispatch_current_policy(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_callback_save_all_during_dispatch());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_verify_mode_value(7U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}

/** @brief Save-clean from a callback and exit without deinit. */
static bool host_flash_child_callback_save_clean_during_dispatch(void)
{
    TEST_ASSERT(init_module());
    g_nvm_callback_hits = 0U;
    par_register_on_change_cb(ePAR_TEST_MODE, on_nvm_scalar_change_save_clean);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 8U));
    TEST_ASSERT(g_nvm_callback_hits == 1U);
    return true;
}

/** @brief Verify current callback-time save-clean policy survives a hard process restart. */
static bool test_flash_ee_callback_save_clean_during_dispatch_current_policy(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_callback_save_clean_during_dispatch());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_verify_mode_value(8U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

/** @brief Deinitialize from a change callback and exit with the module down. */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
static bool host_flash_child_callback_deinit_during_dispatch(void)
{
    TEST_ASSERT(init_module());
    g_nvm_callback_hits = 0U;
    par_register_on_change_cb(ePAR_TEST_MODE, on_nvm_scalar_change_deinit);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 6U));
    TEST_ASSERT(g_nvm_callback_hits == 1U);
    TEST_ASSERT(!par_is_init());
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

/** @brief Verify callback-time deinit does not persist through the current policy. */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
static bool test_flash_ee_callback_deinit_during_dispatch_current_policy(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_callback_deinit_during_dispatch());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_verify_mode_value(1U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

/** @brief Fill the active bank, then fail the next checkpoint erase. */
static bool host_flash_child_fail_checkpoint_after_commit(void)
{
    uint32_t bank_base = 0U;
    uint32_t next_offset = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));

    for (uint16_t attempt = 0U; attempt < 300U; attempt++)
    {
        TEST_ASSERT(host_flash_ee_find_active_bank(&bank_base, &next_offset));
        if ((next_offset + HOST_FLASH_EE_RECORD_SIZE) > HOST_FLASH_EE_BANK_SIZE)
        {
            break;
        }
        TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, (uint16_t)(attempt % 1000U)));
        TEST_ASSERT_OK(par_save(ePAR_TEST_U16));
    }

    TEST_ASSERT(host_flash_ee_find_active_bank(&bank_base, &next_offset));
    TEST_ASSERT((next_offset + HOST_FLASH_EE_RECORD_SIZE) > HOST_FLASH_EE_BANK_SIZE);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    g_fail_erase_after = 0;
    TEST_ASSERT((par_save(ePAR_TEST_MODE) & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    return true;
}

/** @brief Verify checkpoint power-loss failure preserves the previous active bank. */
static bool test_flash_ee_checkpoint_power_loss_preserves_previous_active_bank(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_fail_checkpoint_after_commit());
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


/** @brief Create a higher-sequence prepared inactive bank after a valid commit. */
static bool host_flash_child_write_prepared_inactive_bank_after_commit(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT(host_flash_ee_write_higher_seq_inactive_header(HOST_FLASH_EE_HEADER_PREPARE, false));
    return true;
}

/** @brief Verify a prepared higher-sequence bank is ignored during restart. */
static bool test_flash_ee_checkpoint_prepare_header_power_loss_ignores_new_bank(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_write_prepared_inactive_bank_after_commit());
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

/** @brief Create a higher-sequence active bank with a corrupted geometry CRC. */
static bool host_flash_child_write_bad_crc_inactive_active_bank_after_commit(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT(host_flash_ee_write_higher_seq_inactive_header(HOST_FLASH_EE_HEADER_ACTIVE, true));
    return true;
}

/** @brief Verify an active bank with bad cfg CRC cannot replace the old bank. */
static bool test_flash_ee_newer_bank_with_bad_cfg_crc_is_ignored(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_write_bad_crc_inactive_active_bank_after_commit());
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


/**
 * @brief Reinitialize only the Flash-EE backend for raw logical tests.
 * @return true when the backend is bound and initialized.
 */
static bool host_flash_ee_backend_reinit_only(void)
{
    const par_store_backend_api_t * const p_store = par_store_backend_flash_ee_get_api();
    bool is_init = false;

    TEST_ASSERT(NULL != p_store);
    TEST_ASSERT(NULL != p_store->init);
    TEST_ASSERT(NULL != p_store->deinit);
    TEST_ASSERT(NULL != p_store->is_init);
    p_store->is_init(&is_init);
    if (is_init)
    {
        TEST_ASSERT_OK(p_store->deinit());
    }

    TEST_ASSERT_OK(par_store_backend_bind());
    TEST_ASSERT_OK(p_store->init());
    return true;
}

/**
 * @brief Build one deterministic logical Flash-EE line pattern.
 * @param p_line Output logical line buffer.
 * @param seed Pattern seed that identifies the expected line value.
 * @return true when @p p_line was populated.
 */
static bool host_flash_ee_build_line_pattern(uint8_t * const p_line,
                                             const uint8_t seed)
{
    TEST_ASSERT(NULL != p_line);
    for (uint32_t idx = 0U; idx < HOST_FLASH_EE_LINE_SIZE; idx++)
    {
        p_line[idx] = (uint8_t)(seed + idx);
    }
    return true;
}

/**
 * @brief Build the expected logical image for checkpoint-copy sweep tests.
 * @param p_expected Output logical image indexed by Flash-EE line.
 * @return true when @p p_expected was populated.
 */
static bool host_flash_ee_build_checkpoint_expected_image(
    uint8_t p_expected[HOST_FLASH_EE_LINE_COUNT][HOST_FLASH_EE_LINE_SIZE])
{
    TEST_ASSERT(NULL != p_expected);
    for (uint32_t line = 0U; line < HOST_FLASH_EE_LINE_COUNT; line++)
    {
        TEST_ASSERT(host_flash_ee_build_line_pattern(p_expected[line],
                                                     (uint8_t)(0x10U + line)));
    }

    TEST_ASSERT(host_flash_ee_build_line_pattern(p_expected[0], 0xA0U));
    return true;
}

/**
 * @brief Write one full logical Flash-EE line through the backend API.
 * @param p_store Active Flash-EE backend API.
 * @param line_index Logical line index to write.
 * @param p_line Source line buffer.
 * @return true when the logical line is persisted.
 */
static bool host_flash_ee_write_logical_line(const par_store_backend_api_t * const p_store,
                                             const uint32_t line_index,
                                             const uint8_t * const p_line)
{
    TEST_ASSERT(NULL != p_store);
    TEST_ASSERT(NULL != p_store->write);
    TEST_ASSERT(NULL != p_line);
    TEST_ASSERT(line_index < HOST_FLASH_EE_LINE_COUNT);
    TEST_ASSERT_OK(p_store->write(line_index * HOST_FLASH_EE_LINE_SIZE,
                                  HOST_FLASH_EE_LINE_SIZE,
                                  p_line));
    return true;
}

/**
 * @brief Prepare a full active bank with every logical line live.
 * @return true when the next logical write will force a checkpoint.
 */
static bool host_flash_ee_prepare_full_checkpoint_image(void)
{
    const par_store_backend_api_t * const p_store = par_store_backend_flash_ee_get_api();
    uint8_t line_buf[HOST_FLASH_EE_LINE_SIZE];
    uint32_t bank_base = 0U;
    uint32_t next_offset = 0U;

    TEST_ASSERT(host_flash_ee_backend_reinit_only());
    TEST_ASSERT(NULL != p_store);

    for (uint32_t line = 0U; line < HOST_FLASH_EE_LINE_COUNT; line++)
    {
        TEST_ASSERT(host_flash_ee_build_line_pattern(line_buf, (uint8_t)(0x10U + line)));
        TEST_ASSERT(host_flash_ee_write_logical_line(p_store, line, line_buf));
    }

    TEST_ASSERT(host_flash_ee_build_line_pattern(line_buf, 0xA0U));
    for (uint16_t attempt = 0U; attempt < 300U; attempt++)
    {
        TEST_ASSERT(host_flash_ee_find_active_bank(&bank_base, &next_offset));
        if ((next_offset + HOST_FLASH_EE_RECORD_SIZE) > HOST_FLASH_EE_BANK_SIZE)
        {
            break;
        }
        TEST_ASSERT(host_flash_ee_write_logical_line(p_store, 0U, line_buf));
    }

    TEST_ASSERT(host_flash_ee_find_active_bank(&bank_base, &next_offset));
    TEST_ASSERT((next_offset + HOST_FLASH_EE_RECORD_SIZE) > HOST_FLASH_EE_BANK_SIZE);
    return true;
}

/**
 * @brief Verify the logical Flash-EE image matches checkpoint-copy baseline.
 * @return true when all logical lines reload unchanged.
 */
static bool host_flash_ee_verify_checkpoint_expected_image(void)
{
    const par_store_backend_api_t * const p_store = par_store_backend_flash_ee_get_api();
    uint8_t expected[HOST_FLASH_EE_LINE_COUNT][HOST_FLASH_EE_LINE_SIZE];
    uint8_t readback[HOST_FLASH_EE_LINE_SIZE];

    host_flash_clear_failpoints();
    TEST_ASSERT(host_flash_ee_build_checkpoint_expected_image(expected));
    TEST_ASSERT(host_flash_ee_backend_reinit_only());
    TEST_ASSERT(NULL != p_store);
    TEST_ASSERT(NULL != p_store->read);

    for (uint32_t line = 0U; line < HOST_FLASH_EE_LINE_COUNT; line++)
    {
        TEST_ASSERT_OK(p_store->read(line * HOST_FLASH_EE_LINE_SIZE,
                                     HOST_FLASH_EE_LINE_SIZE,
                                     readback));
        TEST_ASSERT(0 == memcmp(readback, expected[line], sizeof(readback)));
    }

    TEST_ASSERT_OK(p_store->deinit());
    return true;
}


/** @brief Checkpoint-copy program phase selected for failpoint injection. */
typedef enum
{
    eHOST_FLASH_EE_CHECKPOINT_COPY_PAYLOAD_META = 0, /**< Payload and metadata program phase. */
    eHOST_FLASH_EE_CHECKPOINT_COPY_COMMIT_MARKER     /**< Commit-marker program phase. */
} host_flash_ee_checkpoint_copy_phase_t;

/**
 * @brief Return the checkpoint-copy program phase name used in repro logs.
 * @param phase Checkpoint-copy program phase.
 * @return Human-readable phase name.
 */
static const char *host_flash_ee_checkpoint_copy_phase_name(
    const host_flash_ee_checkpoint_copy_phase_t phase)
{
    return (eHOST_FLASH_EE_CHECKPOINT_COPY_PAYLOAD_META == phase) ?
           "payload_meta" : "commit_marker";
}

/**
 * @brief Return the failpoint countdown for one checkpoint-copy program phase.
 * @details Countdown zero is the prepared-bank header program. Record-copy
 *          payload/metadata and commit-marker writes start at countdown one.
 * @param line_index Logical line copied into the checkpoint target bank.
 * @param phase Record-copy program phase to fail.
 * @return Program failpoint countdown value for the selected phase.
 */
static int host_flash_ee_checkpoint_copy_fail_after(
    const uint32_t line_index,
    const host_flash_ee_checkpoint_copy_phase_t phase)
{
    return (int)(1U + (line_index * 2U) + (uint32_t)phase);
}

/**
 * @brief Calculate the expected failed program range for one copied record.
 * @param target_bank Physical base address of the checkpoint target bank.
 * @param line_index Logical line copied into the checkpoint target bank.
 * @param phase Record-copy program phase to fail.
 * @param p_addr Expected failed program address.
 * @param p_size Expected failed program size.
 * @return true when @p p_addr and @p p_size were populated.
 */
static bool host_flash_ee_checkpoint_copy_expected_program_range(
    const uint32_t target_bank,
    const uint32_t line_index,
    const host_flash_ee_checkpoint_copy_phase_t phase,
    uint32_t * const p_addr,
    uint32_t * const p_size)
{
    const uint32_t record_addr = target_bank + HOST_FLASH_EE_HEADER_SIZE +
                                 (line_index * HOST_FLASH_EE_RECORD_SIZE);

    TEST_ASSERT(line_index < HOST_FLASH_EE_LINE_COUNT);
    TEST_ASSERT(NULL != p_addr);
    TEST_ASSERT(NULL != p_size);
    if (eHOST_FLASH_EE_CHECKPOINT_COPY_PAYLOAD_META == phase)
    {
        *p_addr = record_addr;
        *p_size = HOST_FLASH_EE_RECORD_COMMIT_OFFSET;
    }
    else
    {
        *p_addr = record_addr + HOST_FLASH_EE_RECORD_COMMIT_OFFSET;
        *p_size = HOST_FLASH_PROGRAM_SIZE;
    }

    return true;
}

/**
 * @brief Fail one selected program operation during checkpoint record copy.
 * @param line_index Logical line whose copied record should fail.
 * @param phase Record-copy program phase to fail.
 * @return true when the selected checkpoint-copy failure is injected and
 *         lands on the expected physical record range.
 */
static bool host_flash_child_fail_checkpoint_record_copy_sweep(
    const uint32_t line_index,
    const host_flash_ee_checkpoint_copy_phase_t phase)
{
    const par_store_backend_api_t * const p_store = par_store_backend_flash_ee_get_api();
    uint8_t attempted[HOST_FLASH_EE_LINE_SIZE];
    uint32_t active_bank = 0U;
    uint32_t next_offset = 0U;
    uint32_t expected_addr = 0U;
    uint32_t expected_size = 0U;
    const int fail_after = host_flash_ee_checkpoint_copy_fail_after(line_index, phase);

    TEST_ASSERT(host_flash_ee_prepare_full_checkpoint_image());
    TEST_ASSERT(host_flash_ee_find_active_bank(&active_bank, &next_offset));
    TEST_ASSERT(host_flash_ee_checkpoint_copy_expected_program_range(
        (0U == active_bank) ? HOST_FLASH_EE_BANK_SIZE : 0U,
        line_index,
        phase,
        &expected_addr,
        &expected_size));
    TEST_ASSERT(host_flash_ee_build_line_pattern(attempted, 0xE0U));
    g_fail_program_after = fail_after;
    TEST_ASSERT((p_store->write(0U, HOST_FLASH_EE_LINE_SIZE, attempted) &
                 ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    TEST_ASSERT(g_fail_program_hit);
    TEST_ASSERT(g_fail_program_addr == expected_addr);
    TEST_ASSERT(g_fail_program_size == expected_size);
    return true;
}

/** @brief Verify every checkpoint-copied record tolerates program failpoints. */
static bool test_flash_ee_checkpoint_record_copy_failpoint_sweep_preserves_previous_bank(void)
{
    static const host_flash_ee_checkpoint_copy_phase_t phases[] = {
        eHOST_FLASH_EE_CHECKPOINT_COPY_PAYLOAD_META,
        eHOST_FLASH_EE_CHECKPOINT_COPY_COMMIT_MARKER,
    };

    if (0 != strcmp(PAR_HOST_TEST_PROFILE_NAME, "default"))
    {
        printf("PAR_HOST_CHECKPOINT_COPY_FAILPOINT_SKIP profile=%s\n",
               PAR_HOST_TEST_PROFILE_NAME);
        return true;
    }

    for (uint32_t line = 0U; line < HOST_FLASH_EE_LINE_COUNT; line++)
    {
        for (size_t phase_idx = 0U; phase_idx < (sizeof(phases) / sizeof(phases[0])); phase_idx++)
        {
            const host_flash_ee_checkpoint_copy_phase_t phase = phases[phase_idx];
            const int fail_after = host_flash_ee_checkpoint_copy_fail_after(line, phase);
            pid_t child_pid;

            printf("PAR_HOST_CHECKPOINT_COPY_FAILPOINT line=%lu phase=%s fail_after=%d\n",
                   (unsigned long)line,
                   host_flash_ee_checkpoint_copy_phase_name(phase),
                   fail_after);
            host_flash_reset_erased();
            child_pid = fork();
            TEST_ASSERT(child_pid >= 0);
            if (0 == child_pid)
            {
                host_child_exit_from_result(
                    host_flash_child_fail_checkpoint_record_copy_sweep(line, phase));
            }
            TEST_ASSERT(host_child_exit_is_success(child_pid));

            child_pid = fork();
            TEST_ASSERT(child_pid >= 0);
            if (0 == child_pid)
            {
                host_child_exit_from_result(host_flash_ee_verify_checkpoint_expected_image());
            }
            TEST_ASSERT(host_child_exit_is_success(child_pid));
        }
    }

    return true;
}

/** @brief Fill the active bank and fail during checkpoint record copy. */
static bool host_flash_child_fail_checkpoint_record_copy_after_commit(void)
{
    uint32_t bank_base = 0U;
    uint32_t next_offset = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));

    for (uint16_t attempt = 0U; attempt < 300U; attempt++)
    {
        TEST_ASSERT(host_flash_ee_find_active_bank(&bank_base, &next_offset));
        if ((next_offset + HOST_FLASH_EE_RECORD_SIZE) > HOST_FLASH_EE_BANK_SIZE)
        {
            break;
        }
        TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, (uint16_t)(attempt % 1000U)));
        TEST_ASSERT_OK(par_save(ePAR_TEST_U16));
    }

    TEST_ASSERT(host_flash_ee_find_active_bank(&bank_base, &next_offset));
    TEST_ASSERT((next_offset + HOST_FLASH_EE_RECORD_SIZE) > HOST_FLASH_EE_BANK_SIZE);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    g_fail_program_after = 1;
    TEST_ASSERT((par_save(ePAR_TEST_MODE) & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    return true;
}

/** @brief Verify record-copy failure keeps using the previous active bank. */
static bool test_flash_ee_checkpoint_record_copy_power_loss_preserves_previous_active_bank(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_fail_checkpoint_record_copy_after_commit());
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


/** @brief Verify a partial logical write preserves neighbor bytes in the line. */
static bool test_flash_ee_partial_line_write_preserves_neighbor_bytes(void)
{
    const par_store_backend_api_t * const p_store = par_store_backend_flash_ee_get_api();
    uint8_t initial[HOST_FLASH_EE_LINE_SIZE];
    uint8_t patch[2] = { 0xA5U, 0x5AU };
    uint8_t readback[HOST_FLASH_EE_LINE_SIZE];

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    for (uint32_t idx = 0U; idx < (uint32_t)sizeof(initial); idx++)
    {
        initial[idx] = (uint8_t)(idx + 1U);
    }
    TEST_ASSERT_OK(p_store->write(0U, (uint32_t)sizeof(initial), initial));
    TEST_ASSERT_OK(p_store->write(3U, (uint32_t)sizeof(patch), patch));
    TEST_ASSERT_OK(p_store->read(0U, (uint32_t)sizeof(readback), readback));
    initial[3] = patch[0];
    initial[4] = patch[1];
    TEST_ASSERT(0 == memcmp(readback, initial, sizeof(readback)));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify a logical write that crosses a cache window survives reload. */
static bool test_flash_ee_cross_cache_window_write_reload_roundtrip(void)
{
    const par_store_backend_api_t * const p_store = par_store_backend_flash_ee_get_api();
    const uint32_t addr = PAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE - 2U;
    const uint8_t payload[4] = { 0x11U, 0x22U, 0x33U, 0x44U };
    uint8_t readback[4] = { 0U };

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(p_store->write(addr, (uint32_t)sizeof(payload), payload));
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(p_store->read(addr, (uint32_t)sizeof(readback), readback));
    TEST_ASSERT(0 == memcmp(readback, payload, sizeof(readback)));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify a partial logical erase preserves bytes outside the range. */
static bool test_flash_ee_erase_partial_line_preserves_outside_range(void)
{
    const par_store_backend_api_t * const p_store = par_store_backend_flash_ee_get_api();
    uint8_t initial[HOST_FLASH_EE_LINE_SIZE];
    uint8_t expected[HOST_FLASH_EE_LINE_SIZE];
    uint8_t readback[HOST_FLASH_EE_LINE_SIZE];

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    for (uint32_t idx = 0U; idx < (uint32_t)sizeof(initial); idx++)
    {
        initial[idx] = (uint8_t)(0x80U + idx);
    }
    memcpy(expected, initial, sizeof(expected));
    expected[2] = 0xFFU;
    expected[3] = 0xFFU;
    expected[4] = 0xFFU;
    TEST_ASSERT_OK(p_store->write(0U, (uint32_t)sizeof(initial), initial));
    TEST_ASSERT_OK(p_store->erase(2U, 3U));
    TEST_ASSERT_OK(p_store->read(0U, (uint32_t)sizeof(readback), readback));
    TEST_ASSERT(0 == memcmp(readback, expected, sizeof(readback)));
    TEST_ASSERT_OK(par_deinit());
    return true;
}


/** @brief Verify raw fake-flash failpoint countdowns across read, program, and erase. */
static bool test_flash_ee_port_failpoint_countdown_matrix(void)
{
    const uint8_t payload[HOST_FLASH_PROGRAM_SIZE] = { 0x11U, 0x22U, 0x33U, 0x44U };
    const uint8_t second_payload[HOST_FLASH_PROGRAM_SIZE] = { 0x01U, 0x02U, 0x03U, 0x04U };
    uint8_t readback[HOST_FLASH_PROGRAM_SIZE] = { 0U };

    host_flash_reset_erased();
    TEST_ASSERT_OK(par_store_flash_ee_native_port_init());
    g_fail_program_after = 1;
    TEST_ASSERT_OK(par_store_flash_ee_native_port_program(0U,
                                                          (uint32_t)sizeof(payload),
                                                          payload));
    TEST_ASSERT_STATUS(par_store_flash_ee_native_port_program(HOST_FLASH_PROGRAM_SIZE,
                                                              (uint32_t)sizeof(second_payload),
                                                              second_payload),
                       ePAR_ERROR);
    host_flash_clear_failpoints();
    TEST_ASSERT_OK(par_store_flash_ee_native_port_read(0U,
                                                       (uint32_t)sizeof(readback),
                                                       readback));
    TEST_ASSERT(0 == memcmp(readback, payload, sizeof(readback)));

    g_fail_read_after = 1;
    TEST_ASSERT_OK(par_store_flash_ee_native_port_read(0U,
                                                       (uint32_t)sizeof(readback),
                                                       readback));
    TEST_ASSERT_STATUS(par_store_flash_ee_native_port_read(0U,
                                                           (uint32_t)sizeof(readback),
                                                           readback),
                       ePAR_ERROR);
    host_flash_clear_failpoints();

    g_fail_erase_after = 1;
    TEST_ASSERT_OK(par_store_flash_ee_native_port_erase(0U, HOST_FLASH_ERASE_SIZE));
    TEST_ASSERT_STATUS(par_store_flash_ee_native_port_erase(HOST_FLASH_ERASE_SIZE,
                                                            HOST_FLASH_ERASE_SIZE),
                       ePAR_ERROR);
    host_flash_clear_failpoints();
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


/** @brief Verify repeated failed saves converge to the last complete commit. */
static bool test_flash_ee_repeated_failed_saves_converge_to_last_commit(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_fail_save_after_commit(0));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_verify_mode_value(4U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 6U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_deinit());

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        TEST_ASSERT(init_module());
        TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 7U));
        g_fail_program_after = 1;
        TEST_ASSERT((par_save(ePAR_TEST_MODE) & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
        host_child_exit_from_result(true);
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_verify_mode_value(6U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}


/** @brief Verify deterministic scalar save/restart operations match a small model. */
static bool test_flash_ee_seeded_scalar_restarts_match_model(void)
{
    static const uint8_t values[] = { 2U, 7U, 3U, 10U, 1U, 8U };
    uint8_t value = 0U;

    host_flash_reset_erased();
    for (size_t i = 0U; i < (sizeof(values) / sizeof(values[0])); i++)
    {
        TEST_ASSERT(init_module());
        TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, values[i]));
        if (0U == (i % 2U))
        {
            TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
        }
        else
        {
            TEST_ASSERT_OK(par_save_all());
        }
        TEST_ASSERT_OK(par_deinit());

        TEST_ASSERT(init_module());
        TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &value));
        TEST_ASSERT(value == values[i]);
        TEST_ASSERT_OK(par_deinit());
    }
    return true;
}

#if (1 == PAR_CFG_NVM_WRITE_VERIFY_EN)
/** @brief Save a committed scalar, then corrupt the physical verify-readback candidate. */
static bool host_flash_child_write_verify_corrupt_scalar_save(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    g_corrupt_program_after = 0;
    TEST_ASSERT((par_save(ePAR_TEST_MODE) & ePAR_ERROR_NVM) != 0U);
    return true;
}

/** @brief Verify scalar write verification rejects physical readback mismatch. */
static bool test_nvm_scalar_write_verify_detects_readback_mismatch(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_write_verify_corrupt_scalar_save());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}


/** @brief Save a scalar while the verify-read path reports a backend error. */
static bool host_flash_child_write_verify_read_error_scalar_save(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    g_fail_read_after = 0;
    TEST_ASSERT((par_save(ePAR_TEST_MODE) & ePAR_ERROR_NVM) != 0U);
    return true;
}

/** @brief Verify scalar write verification reports readback transport errors but persists the completed write. */
static bool test_nvm_scalar_write_verify_read_error_is_reported(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_write_verify_read_error_scalar_save());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_verify_mode_value(5U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}
#endif /* (1 == PAR_CFG_NVM_WRITE_VERIFY_EN) */

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


#if defined(PAR_HOST_TEST_SCHEMA_EVOLUTION_WRITE)
/** @brief Create a persisted image with the pre-change scalar schema. */
static bool test_nvm_schema_rebuild_write_baseline_image(void)
{
    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 9U));
    TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, 444U));
    TEST_ASSERT_OK(par_save_all());
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* defined(PAR_HOST_TEST_SCHEMA_EVOLUTION_WRITE) */

#if defined(PAR_HOST_TEST_SCHEMA_EVOLUTION_READ)
/** @brief Verify type drift triggers full table rebuild with new defaults. */
static bool test_nvm_schema_type_change_triggers_table_rebuild_defaults(void)
{
    uint16_t mode = 0U;
    uint16_t u16 = 0U;

    host_flash_clear_failpoints();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_MODE, &mode));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT(mode == 8U);
    TEST_ASSERT(u16 == 100U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* defined(PAR_HOST_TEST_SCHEMA_EVOLUTION_READ) */

#if defined(PAR_HOST_TEST_SCHEMA_SLOT_REORDER_READ)
/** @brief Verify persistent slot reorder rebuilds defaults instead of cross-loading old values. */
static bool test_nvm_schema_slot_reorder_triggers_table_rebuild_defaults(void)
{
    uint8_t mode = 0U;
    uint16_t u16 = 0U;

    host_flash_clear_failpoints();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &mode));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT(mode == 1U);
    TEST_ASSERT(u16 == 100U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* defined(PAR_HOST_TEST_SCHEMA_SLOT_REORDER_READ) */

#if defined(PAR_HOST_TEST_SCHEMA_PERSISTENT_REMOVED_READ)
/** @brief Verify removing one persistent scalar slot rebuilds remaining defaults. */
static bool test_nvm_schema_persistent_removed_rebuilds_remaining_defaults(void)
{
    uint8_t mode = 0U;
    uint16_t u16 = 0U;
    uint32_t u32 = 0UL;

    host_flash_clear_failpoints();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &mode));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT_OK(par_get_u32(ePAR_TEST_U32, &u32));
    TEST_ASSERT(mode == 1U);
    TEST_ASSERT(u16 == 100U);
    TEST_ASSERT(u32 == 1000UL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* defined(PAR_HOST_TEST_SCHEMA_PERSISTENT_REMOVED_READ) */

#if defined(PAR_HOST_TEST_SCHEMA_SCALAR_TO_OBJECT_READ)
/** @brief Verify scalar-to-object schema drift is rejected by the current policy. */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
static bool test_nvm_schema_scalar_to_object_rejects_stale_image_current_policy(void)
{
    par_status_t status;

    host_flash_clear_failpoints();
    status = par_init();
    TEST_ASSERT((status & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    if (par_is_init())
    {
        (void)par_deinit();
    }
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
#endif /* defined(PAR_HOST_TEST_SCHEMA_SCALAR_TO_OBJECT_READ) */

#if defined(PAR_HOST_TEST_SCHEMA_OBJECT_TO_SCALAR_READ)
/** @brief Verify object-to-scalar schema drift rebuilds the scalar default. */
static bool test_nvm_schema_object_to_scalar_rebuilds_scalar_default(void)
{
    uint8_t value = 0U;

    host_flash_clear_failpoints();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_STR, &value));
    TEST_ASSERT(value == 3U);
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* defined(PAR_HOST_TEST_SCHEMA_OBJECT_TO_SCALAR_READ) */

#if defined(PAR_HOST_TEST_SCHEMA_OBJECT_CAPACITY_SHRINK_READ)
/** @brief Verify object capacity shrink rebuilds the shrunk object default. */
static bool test_nvm_schema_object_capacity_shrink_rebuilds_object_default(void)
{
    char str_buf[2] = { 0 };
    uint16_t len = 0U;
    uint8_t mode = 0U;

    host_flash_clear_failpoints();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &mode));
    TEST_ASSERT(mode == 9U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 1U);
    TEST_ASSERT(0 == strcmp(str_buf, "x"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* defined(PAR_HOST_TEST_SCHEMA_OBJECT_CAPACITY_SHRINK_READ) */

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

/** @brief Create a stored scalar image with a shorter compatible prefix. */
static bool host_flash_child_write_short_scalar_prefix(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, 222U));
    TEST_ASSERT_OK(par_set_u32(ePAR_TEST_U32, 33333UL));
    TEST_ASSERT_OK(par_save_all());
    TEST_ASSERT(host_scalar_store_write_header_count(1U));
    return true;
}

/** @brief Verify a shorter scalar prefix follows the layout compatibility policy. */
static bool test_nvm_scalar_stored_count_smaller_applies_layout_policy(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    uint16_t u16 = 0U;
    uint32_t u32 = 0U;

    if (false == par_is_init())
    {
        TEST_ASSERT(init_module());
    }
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 1U));
    TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, 100U));
    TEST_ASSERT_OK(par_set_u32(ePAR_TEST_U32, 1000UL));
    TEST_ASSERT_OK(par_deinit());
    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_write_short_scalar_prefix());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT_OK(par_get_u32(ePAR_TEST_U32, &u32));
#if (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY)
    TEST_ASSERT(u8 == 1U);
#else
    TEST_ASSERT(u8 == 5U);
#endif /* (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY) */
    TEST_ASSERT(u16 == 100U);
    TEST_ASSERT(u32 == 1000UL);
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT_OK(par_get_u32(ePAR_TEST_U32, &u32));
#if (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY)
    TEST_ASSERT(u8 == 1U);
#else
    TEST_ASSERT(u8 == 5U);
#endif /* (PAR_CFG_NVM_RECORD_LAYOUT == PAR_CFG_NVM_RECORD_LAYOUT_GROUPED_PAYLOAD_ONLY) */
    TEST_ASSERT(u16 == 100U);
    TEST_ASSERT(u32 == 1000UL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}


/** @brief Create a scalar image whose stored count is larger than this build. */
static bool host_flash_child_write_oversized_scalar_count(void)
{
    const uint16_t compile_count = (uint16_t)PAR_PERSISTENT_COMPILE_COUNT;
    const uint16_t count = (uint16_t)(compile_count + 1U);
    const uint32_t table_id = host_scalar_table_id_calc_relaxed_count(count);

    TEST_ASSERT(host_scalar_table_id_calc_relaxed_count(compile_count) ==
                par_nvm_table_id_calc_for_count(compile_count));
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, 222U));
    TEST_ASSERT_OK(par_set_u32(ePAR_TEST_U32, 33333UL));
    TEST_ASSERT_OK(par_save_all());
    TEST_ASSERT(host_scalar_store_write_raw_header(count, table_id));
    return true;
}

/** @brief Verify stored-count overflow rebuilds scalar defaults once. */
static bool test_nvm_scalar_stored_count_larger_rebuilds_defaults_once(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    uint16_t u16 = 0U;
    uint32_t u32 = 0U;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_write_oversized_scalar_count());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT_OK(par_get_u32(ePAR_TEST_U32, &u32));
    TEST_ASSERT(u8 == 1U);
    TEST_ASSERT(u16 == 100U);
    TEST_ASSERT(u32 == 1000UL);
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT_OK(par_get_u16(ePAR_TEST_U16, &u16));
    TEST_ASSERT_OK(par_get_u32(ePAR_TEST_U32, &u32));
    TEST_ASSERT(u8 == 1U);
    TEST_ASSERT(u16 == 100U);
    TEST_ASSERT(u32 == 1000UL);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/**
 * @brief Return the active object NVM image used by the current profile.
 * @return Pointer to the fake flash image that persists object writes.
 */
static const uint8_t *host_object_active_image(void)
{
#if (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED)
    return g_object_flash;
#else
    return g_flash;
#endif /* (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED) */
}

#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
/** @brief Verify object NVM block size stays within the active profile region. */
static bool test_nvm_object_region_profile_bounds(void)
{
    const uint32_t block_size = par_nvm_object_get_block_size();

    TEST_ASSERT(block_size > 0U);
#if (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) &&     (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_FIXED)
    TEST_ASSERT(block_size <= PAR_CFG_NVM_OBJECT_REGION_SIZE);
#endif /* (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) && (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_FIXED) */
    return true;
}
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

#if defined(PAR_HOST_TEST_OBJECT_ONLY) || \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED)
/** @brief Reinitialize the module and verify an object string value. */
static bool host_verify_reloaded_object_str_value(const char * const expected)
{
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    TEST_ASSERT(NULL != expected);
    if (par_is_init())
    {
        TEST_ASSERT_OK(par_deinit());
    }
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == (uint16_t)strlen(expected));
    TEST_ASSERT(0 == strcmp(str_buf, expected));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

#endif /* defined(PAR_HOST_TEST_OBJECT_ONLY) || (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED) */

#if defined(PAR_HOST_TEST_OBJECT_ONLY)
/** @brief Verify object-only persistence saves and reloads object rows. */
static bool test_nvm_object_only_save_reload_preserves_object(void)
{
    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"obj", 3U));
    TEST_ASSERT_OK(par_deinit());
    TEST_ASSERT(host_verify_reloaded_object_str_value("obj"));
    return true;
}
#endif /* defined(PAR_HOST_TEST_OBJECT_ONLY) */

#if defined(PAR_HOST_TEST_FIXED_OBJECT_INVALID)
/** @brief Verify invalid fixed object placement rejects NVM initialization. */
static bool test_nvm_fixed_object_invalid_rejects_init(void)
{
    par_status_t status;

    host_flash_reset_erased();
    status = par_init();
    TEST_ASSERT((status & ePAR_ERROR_NVM) != 0U);
    if (par_is_init())
    {
        (void)par_deinit();
    }
    return true;
}
#endif /* defined(PAR_HOST_TEST_FIXED_OBJECT_INVALID) */

#if (1 == PAR_CFG_NVM_OBJECT_WRITE_VERIFY_EN) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED)
/**
 * @brief Save one object, then corrupt the physical writeback candidate.
 * @details Dedicated object writes update the backend before write-verify
 * reports the mismatch. The failed save therefore leaves a corrupt persisted
 * record, and reload must reject it and restore the table default
 * instead of the previously saved value.
 */
static bool host_flash_child_write_verify_corrupt_object_save(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"old", 3U));
#if (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED)
    g_object_corrupt_write_after = 0;
#else
    g_corrupt_program_after = 0;
#endif /* (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED) */
    TEST_ASSERT((par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"new", 3U) & ePAR_ERROR_NVM) != 0U);
    host_flash_clear_failpoints();
    TEST_ASSERT(host_verify_reloaded_object_str_value("ap"));
    return true;
}

/** @brief Verify object write verification rejects physical readback mismatch. */
static bool test_nvm_object_write_verify_detects_payload_mismatch(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_write_verify_corrupt_object_save());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}


/** @brief Save one object while the verify-read path reports a backend error. */
static bool host_flash_child_write_verify_object_read_error_save(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"old", 3U));
    g_object_fail_read_after = 0;
    TEST_ASSERT((par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"new", 3U) & ePAR_ERROR_NVM) != 0U);
    host_flash_clear_failpoints();
    TEST_ASSERT(host_verify_reloaded_object_str_value("new"));
    return true;
}

/** @brief Verify object write verification reports readback transport errors. */
static bool test_nvm_object_write_verify_read_error_is_reported(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_write_verify_object_read_error_save());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}
#endif /* (1 == PAR_CFG_NVM_OBJECT_WRITE_VERIFY_EN) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED) */


#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED)
/**
 * @brief Verify a reloaded scalar/object pair matches expected committed values.
 * @param expected_mode Expected scalar mode value.
 * @param expected_str Expected string object value.
 * @return true when the reloaded values match.
 */
static bool host_flash_verify_mode_and_str_value(const uint8_t expected_mode,
                                                 const char * const expected_str);

/** @brief Save mixed dirty scalar/object values and fail a selected backend program. */
static bool host_flash_child_shared_save_all_fail_mixed_values(const int program_fail_after)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "old"));
    TEST_ASSERT_OK(par_save_all());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "new"));
    g_fail_program_after = program_fail_after;
    TEST_ASSERT((par_save_all() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    return true;
}

/** @brief Verify failed shared save-all does not load corrupt mixed values. */
static bool test_nvm_shared_save_all_failpoint_preserves_last_committed_values(void)
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
            host_child_exit_from_result(
                host_flash_child_shared_save_all_fail_mixed_values(fail_after_cases[i]));
        }
        TEST_ASSERT(host_child_exit_is_success(child_pid));

        child_pid = fork();
        TEST_ASSERT(child_pid >= 0);
        if (0 == child_pid)
        {
            host_child_exit_from_result(host_flash_verify_mode_and_str_value(4U, "old"));
        }
        TEST_ASSERT(host_child_exit_is_success(child_pid));
    }

    return true;
}
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) */



#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED)
/** @brief Verify a reloaded scalar/object pair matches expected committed values. */
static bool host_flash_verify_mode_and_str_value(const uint8_t expected_mode,
                                                 const char *const expected_str)
{
    uint8_t mode = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_clear_failpoints();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &mode));
    TEST_ASSERT(mode == expected_mode);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == (uint16_t)strlen(expected_str));
    TEST_ASSERT(0 == strcmp(str_buf, expected_str));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Save a shared object with a configurable program failpoint. */
static bool host_flash_child_shared_obj_n_save_fail_after_commit(const int program_fail_after)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "old"));
    TEST_ASSERT_OK(par_save_all());
    g_fail_program_after = program_fail_after;
    TEST_ASSERT((par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"new", 3U) &
                 ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    return true;
}

/** @brief Verify shared object n-save program failpoints preserve last commit. */
static bool test_nvm_shared_obj_n_save_program_failpoint_matrix_preserves_last_commit(void)
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
            host_child_exit_from_result(
                host_flash_child_shared_obj_n_save_fail_after_commit(fail_after_cases[i]));
        }
        TEST_ASSERT(host_child_exit_is_success(child_pid));

        child_pid = fork();
        TEST_ASSERT(child_pid >= 0);
        if (0 == child_pid)
        {
            host_child_exit_from_result(host_flash_verify_mode_and_str_value(4U, "old"));
        }
        TEST_ASSERT(host_child_exit_is_success(child_pid));
    }

    return true;
}

/** @brief Save mixed values, fail twice, and keep the last complete mixed commit. */
static bool test_nvm_shared_repeated_mixed_failures_converge_to_last_commit(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "old"));
    TEST_ASSERT_OK(par_save_all());
    TEST_ASSERT_OK(par_deinit());

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        TEST_ASSERT(init_module());
        TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
        TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "new"));
        g_fail_program_after = 0;
        TEST_ASSERT((par_save_all() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
        host_child_exit_from_result(true);
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        TEST_ASSERT(init_module());
        TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 6U));
        TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "bad"));
        g_fail_program_after = 1;
        TEST_ASSERT((par_save_all() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
        host_child_exit_from_result(true);
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_verify_mode_and_str_value(4U, "old"));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}

/** @brief Verify deterministic mixed scalar/object save-restart steps match a model. */
static bool test_nvm_shared_seeded_mixed_restarts_match_model(void)
{
    static const struct
    {
        uint8_t mode;       /**< Expected scalar mode after the step. */
        const char *str;    /**< Expected string object after the step. */
        bool save_all;      /**< Persist with save-all instead of per-item saves. */
    } steps[] = {
        { 2U, "aa", false },
        { 5U, "bbb", true },
        { 8U, "c", false },
        { 3U, "model", true },
        { 6U, "d", false },
        { 9U, "ee", true },
        { 4U, "fff", false },
        { 7U, "gggg", true },
        { 1U, "h", false },
        { 10U, "ii", true },
        { 2U, "jjj", false },
        { 5U, "final", true },
    };

    host_flash_reset_erased();
    for (size_t i = 0U; i < (sizeof(steps) / sizeof(steps[0])); i++)
    {
        TEST_ASSERT(init_module());
        TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, steps[i].mode));
        TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, steps[i].str));
        if (steps[i].save_all)
        {
            TEST_ASSERT_OK(par_save_all());
        }
        else
        {
            TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
            TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
        }
        TEST_ASSERT_OK(par_deinit());
        TEST_ASSERT(host_flash_verify_mode_and_str_value(steps[i].mode, steps[i].str));
    }
    return true;
}

/**
 * @brief Advance a deterministic host model pseudo-random seed.
 * @param p_seed Seed updated in place.
 * @return Next pseudo-random value.
 */
static uint32_t host_model_next_seed(uint32_t *const p_seed)
{
    *p_seed = ((*p_seed * 1103515245UL) + 12345UL);
    return *p_seed;
}

/**
 * @brief Generate a deterministic bounded object string for model tests.
 * @param seed Pseudo-random seed used to derive characters.
 * @param p_out Destination string buffer.
 * @param out_size Destination buffer size in bytes.
 */
static bool host_model_make_str(uint32_t seed, char *const p_out, const size_t out_size)
{
    const size_t len = (size_t)((seed % 6UL) + 1UL);

    TEST_ASSERT(out_size > len);
    for (size_t idx = 0U; idx < len; idx++)
    {
        seed = ((seed * 1664525UL) + 1013904223UL);
        p_out[idx] = (char)('a' + (char)((seed >> 24U) % 26UL));
    }
    p_out[len] = '\0';
    return true;
}

/** @brief Verify a larger deterministic mixed scalar/object random model. */
static bool test_nvm_shared_seeded_mixed_random_restarts_match_model(void)
{
    uint32_t seed = 0x13572468UL;

    host_flash_reset_erased();
    for (size_t step = 0U; step < 24U; step++)
    {
        char expected_str[9] = { 0 };
        const uint8_t expected_mode = (uint8_t)((host_model_next_seed(&seed) % 10UL) + 1UL);
        const bool save_all = ((host_model_next_seed(&seed) & 1UL) != 0UL);
        const bool object_first = ((host_model_next_seed(&seed) & 2UL) != 0UL);

        TEST_ASSERT(host_model_make_str(host_model_next_seed(&seed), expected_str, sizeof(expected_str)));
        TEST_ASSERT(init_module());
        TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, expected_mode));
        TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, expected_str));
        if (save_all)
        {
            TEST_ASSERT_OK(par_save_all());
        }
        else if (object_first)
        {
            TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
            TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
        }
        else
        {
            TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
            TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
        }
        TEST_ASSERT_OK(par_deinit());
        TEST_ASSERT(host_flash_verify_mode_and_str_value(expected_mode, expected_str));
    }
    return true;
}

#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (1 == PAR_CFG_ENABLE_TYPE_STR) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED)
/**
 * @brief Update an object while its validation callback persists current state.
 * @param validation Object validation callback to install.
 * @return true when the child scenario completes.
 */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
static bool host_flash_child_object_validation_save_policy(
    const pf_par_obj_validation_t validation)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "old"));
    TEST_ASSERT_OK(par_save_all());
    g_nvm_callback_hits = 0U;
    par_register_obj_validation(ePAR_TEST_STR, validation);
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "new"));
    TEST_ASSERT(g_nvm_callback_hits == 1U);
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

/** @brief Verify object validation-time save persists pre-validation state. */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
static bool test_nvm_object_validation_save_during_dispatch_current_policy(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_object_validation_save_policy(
            on_nvm_object_validation_save));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_verify_mode_and_str_value(4U, "old"));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

/** @brief Verify object validation-time save-all persists pre-validation state. */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
static bool test_nvm_object_validation_save_all_during_dispatch_current_policy(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_object_validation_save_policy(
            on_nvm_object_validation_save_all));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_verify_mode_and_str_value(4U, "old"));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

/** @brief Verify object validation-time save-clean persists pre-validation state. */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
static bool test_nvm_object_validation_save_clean_during_dispatch_current_policy(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_object_validation_save_policy(
            on_nvm_object_validation_save_clean));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_verify_mode_and_str_value(4U, "old"));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));
    return true;
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (1 == PAR_CFG_ENABLE_TYPE_STR) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) */


#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) */

/** @brief Verify unchanged object n-save calls do not mutate the flash image. */
static bool test_nvm_obj_n_save_unchanged_skips_backend_write(void)
{
    uint8_t before[HOST_FLASH_SIZE];
    uint8_t after[HOST_FLASH_SIZE];
    const uint8_t *store_image = NULL;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"stable", 6U));
    store_image = host_object_active_image();
    memcpy(before, store_image, sizeof(before));
    TEST_ASSERT_OK(par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"stable", 6U));
    memcpy(after, store_image, sizeof(after));
    TEST_ASSERT(0 == memcmp(before, after, sizeof(before)));
    TEST_ASSERT_OK(par_deinit());

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 6U);
    TEST_ASSERT(strcmp(str_buf, "stable") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify embedded-NUL string n-save rejects input without mutation. */
static bool test_nvm_str_embedded_nul_n_save_rejected_without_mutation(void)
{
    const uint8_t bad_payload[5] = { 'b', 'a', '\0', 'd', '!' };
    uint8_t before[HOST_FLASH_SIZE];
    uint8_t after[HOST_FLASH_SIZE];
    const uint8_t *store_image = NULL;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"stable", 6U));
    store_image = host_object_active_image();
    memcpy(before, store_image, sizeof(before));
    TEST_ASSERT_STATUS(par_set_obj_n_save(ePAR_TEST_STR,
                                           bad_payload,
                                           (uint16_t)sizeof(bad_payload)),
                       ePAR_ERROR_VALUE);
    memcpy(after, store_image, sizeof(after));
    TEST_ASSERT(0 == memcmp(before, after, sizeof(before)));
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 6U);
    TEST_ASSERT(strcmp(str_buf, "stable") == 0);
    TEST_ASSERT_OK(par_deinit());

    memset(str_buf, 0, sizeof(str_buf));
    len = 0U;
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 6U);
    TEST_ASSERT(strcmp(str_buf, "stable") == 0);
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



/** @brief Save a committed scalar, then fail save-clean with a dirty live value. */
static bool host_flash_child_fail_save_clean_after_commit(const int erase_fail_after,
                                                          const int program_fail_after)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save_all());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    g_fail_erase_after = erase_fail_after;
    g_fail_program_after = program_fail_after;
    TEST_ASSERT((par_save_clean() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    return true;
}

/**
 * @brief Execute a forked failpoint attempt and verify the old mode survives.
 * @param p_attempt Child callback that applies one failing save operation.
 * @param program_fail_after Program failpoint countdown passed to @p p_attempt.
 * @return true when the failed attempt preserves mode value 4 after restart.
 */
static bool host_flash_verify_mode_failpoint_case(
    bool (* const p_attempt)(const int),
    const int program_fail_after)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(p_attempt(program_fail_after));
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

/**
 * @brief Save a committed scalar, then fail save-clean at a selected program step.
 * @param program_fail_after Program failpoint countdown.
 * @return true when save-clean reports the injected backend error.
 */
static bool host_flash_child_fail_save_clean_program_after_commit(
    const int program_fail_after)
{
    return host_flash_child_fail_save_clean_after_commit(HOST_FLASH_FAIL_DISABLED,
                                                         program_fail_after);
}

/**
 * @brief Verify representative save APIs preserve the last full commit on
 *        early program failures.
 */
static bool test_flash_ee_representative_program_failpoint_sweep_preserves_last_commit(void)
{
    static bool (* const attempts[])(const int) = {
        host_flash_child_fail_save_after_commit,
        host_flash_child_fail_save_all_after_commit,
        host_flash_child_fail_save_clean_program_after_commit,
    };
    static const int program_fail_after_cases[] = { 0, 1 };

    for (size_t attempt = 0U;
         attempt < (sizeof(attempts) / sizeof(attempts[0]));
         attempt++)
    {
        for (size_t fail_idx = 0U;
             fail_idx < (sizeof(program_fail_after_cases) / sizeof(program_fail_after_cases[0]));
             fail_idx++)
        {
            TEST_ASSERT(host_flash_verify_mode_failpoint_case(
                attempts[attempt], program_fail_after_cases[fail_idx]));
        }
    }

    return true;
}

/** @brief Verify failed save-clean rewrites converge to the last full commit. */
static bool test_flash_ee_save_clean_failpoint_matrix_preserves_last_commit(void)
{
    static const struct
    {
        int erase_after;   /**< Erase failpoint countdown. */
        int program_after; /**< Program failpoint countdown. */
    } cases[] = {
        { HOST_FLASH_FAIL_DISABLED, 0 },
        { HOST_FLASH_FAIL_DISABLED, 1 },
    };

    for (size_t i = 0U; i < (sizeof(cases) / sizeof(cases[0])); i++)
    {
        pid_t child_pid;

        host_flash_reset_erased();
        child_pid = fork();
        TEST_ASSERT(child_pid >= 0);
        if (0 == child_pid)
        {
            host_child_exit_from_result(host_flash_child_fail_save_clean_after_commit(
                cases[i].erase_after, cases[i].program_after));
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

/** @brief Fill the active bank, then fail a save-clean checkpoint erase. */
static bool host_flash_child_fail_save_clean_checkpoint_erase_after_commit(void)
{
    uint32_t bank_base = 0U;
    uint32_t next_offset = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));

    for (uint16_t attempt = 0U; attempt < 300U; attempt++)
    {
        TEST_ASSERT(host_flash_ee_find_active_bank(&bank_base, &next_offset));
        if ((next_offset + HOST_FLASH_EE_RECORD_SIZE) > HOST_FLASH_EE_BANK_SIZE)
        {
            break;
        }
        TEST_ASSERT_OK(par_set_u16(ePAR_TEST_U16, (uint16_t)(attempt % 1000U)));
        TEST_ASSERT_OK(par_save(ePAR_TEST_U16));
    }

    TEST_ASSERT(host_flash_ee_find_active_bank(&bank_base, &next_offset));
    TEST_ASSERT((next_offset + HOST_FLASH_EE_RECORD_SIZE) > HOST_FLASH_EE_BANK_SIZE);
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    g_fail_erase_after = 0;
    TEST_ASSERT((par_save_clean() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    return true;
}

/** @brief Verify save-clean checkpoint erase failure keeps the last commit. */
static bool test_flash_ee_save_clean_checkpoint_erase_failure_preserves_last_commit(void)
{
    pid_t child_pid;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_fail_save_clean_checkpoint_erase_after_commit());
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


/** @brief Verify invalid Flash-EE geometry is rejected and later retry recovers. */
static bool test_flash_ee_init_rejects_invalid_geometry_and_recovers(void)
{
    const par_store_backend_api_t * const p_store = par_store_backend_flash_ee_get_api();
    host_flash_ee_geom_ctx_t geom;

    memset(&geom, 0, sizeof(geom));
    memset(geom.flash, 0xFF, sizeof(geom.flash));
    geom.region_size = HOST_FLASH_SIZE - 1U;
    geom.erase_size = HOST_FLASH_ERASE_SIZE;
    geom.program_size = HOST_FLASH_PROGRAM_SIZE;
    TEST_ASSERT_OK(par_store_backend_flash_ee_bind_port(&g_host_flash_ee_geom_port_api, &geom));
    TEST_ASSERT_STATUS(p_store->init(), ePAR_ERROR_INIT);
    TEST_ASSERT(par_store_backend_flash_ee_get_diag() == ePAR_STORE_FLASH_EE_DIAG_PORT_GEOMETRY);

    memset(geom.flash, 0xFF, sizeof(geom.flash));
    geom.region_size = HOST_FLASH_SIZE;
    geom.erase_size = HOST_FLASH_ERASE_SIZE;
    geom.program_size = HOST_FLASH_PROGRAM_SIZE;
    geom.is_init = false;
    TEST_ASSERT_OK(par_store_backend_flash_ee_bind_port(&g_host_flash_ee_geom_port_api, &geom));
    TEST_ASSERT_OK(p_store->init());
    TEST_ASSERT(par_store_backend_flash_ee_get_diag() == ePAR_STORE_FLASH_EE_DIAG_NONE);
    TEST_ASSERT_OK(p_store->deinit());
    TEST_ASSERT_OK(par_store_backend_bind());
    return true;
}

/** @brief Verify NVM wrapper APIs reject wrong type and before-init usage. */
static bool test_nvm_wrapper_negative_type_and_init_policies(void)
{
    uint16_t scalar_value = 123U;
    const uint8_t payload[3] = { 'b', 'a', 'd' };

    host_flash_reset_erased();
    if (par_is_init())
    {
        TEST_ASSERT_OK(par_deinit());
    }
    TEST_ASSERT_STATUS(par_save(ePAR_TEST_MODE), ePAR_ERROR_INIT);
    TEST_ASSERT_STATUS(par_save_all(), ePAR_ERROR_INIT);
    TEST_ASSERT_STATUS(par_save_clean(), ePAR_ERROR_INIT);

    TEST_ASSERT(init_module());
    TEST_ASSERT_STATUS(par_set_scalar_n_save(ePAR_TEST_STR, &scalar_value), ePAR_ERROR_TYPE);
    TEST_ASSERT_STATUS(par_set_obj_n_save(ePAR_TEST_MODE, payload, (uint16_t)sizeof(payload)), ePAR_ERROR_TYPE);
    TEST_ASSERT_STATUS(par_set_obj_n_save(ePAR_TEST_ARR_U16, payload, 3U), ePAR_ERROR_VALUE);
    TEST_ASSERT_STATUS(par_set_obj_n_save(ePAR_TEST_ARR_U32, payload, 6U), ePAR_ERROR_VALUE);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify saving a non-persistent row reports unsupported policy without image mutation. */
static bool test_nvm_save_non_persistent_is_noop_and_preserves_image(void)
{
    uint8_t before[HOST_FLASH_SIZE];
    uint8_t after[HOST_FLASH_SIZE];

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_i8(ePAR_TEST_I8, -5));
    memcpy(before, g_flash, sizeof(before));
    TEST_ASSERT_STATUS(par_save(ePAR_TEST_I8), ePAR_ERROR);
    memcpy(after, g_flash, sizeof(after));
    TEST_ASSERT(0 == memcmp(before, after, sizeof(before)));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) && \
    (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_FIXED)
/** @brief Corrupt committed object header and keep the scalar commit stable. */
static bool host_flash_child_corrupt_object_header_after_commit(void)
{
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 6U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "object"));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    TEST_ASSERT(host_object_store_xor_byte(PAR_CFG_NVM_OBJECT_FIXED_ADDR, 0x01U));
    return true;
}

/** @brief Verify object header corruption restores object defaults without scalar loss. */
static bool test_nvm_object_header_corruption_restores_default_without_scalar_loss(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_corrupt_object_header_after_commit());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 6U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Corrupt the first object record length field after committing values. */
static bool host_flash_child_corrupt_object_record_len_after_commit(void)
{
    static const char committed[] = "object";
    uint32_t len_addr = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 7U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, committed));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    TEST_ASSERT(host_object_store_get_record_len_addr(ePAR_TEST_STR,
                                                       (uint16_t)strlen(committed),
                                                       &len_addr));
    TEST_ASSERT(host_object_store_xor_byte(len_addr, 0x7FU));
    return true;
}

/** @brief Verify invalid object record length rebuilds object defaults only. */
static bool test_nvm_object_record_len_corruption_restores_default_without_scalar_loss(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_corrupt_object_record_len_after_commit());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 7U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Corrupt the committed object payload after committing values. */
static bool host_flash_child_corrupt_object_payload_after_commit(void)
{
    static const char committed[] = "object";
    uint32_t payload_addr = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 8U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, committed));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    TEST_ASSERT(host_object_store_get_record_field_addr(ePAR_TEST_STR,
                                                         (uint16_t)strlen(committed),
                                                         (uint32_t)sizeof(host_object_store_record_meta_t),
                                                         &payload_addr));
    TEST_ASSERT(host_object_store_xor_byte(payload_addr, 0x01U));
    return true;
}

/** @brief Verify object payload CRC mismatch restores defaults without scalar loss. */
static bool test_nvm_object_payload_crc_corruption_restores_default_without_scalar_loss(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_corrupt_object_payload_after_commit());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 8U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Corrupt the committed object record ID after committing values. */
static bool host_flash_child_corrupt_object_record_id_after_commit(void)
{
    static const char committed[] = "object";
    uint32_t id_addr = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 9U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, committed));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    TEST_ASSERT(host_object_store_get_record_field_addr(ePAR_TEST_STR,
                                                         (uint16_t)strlen(committed),
                                                         (uint32_t)offsetof(host_object_store_record_meta_t, id),
                                                         &id_addr));
    TEST_ASSERT(host_object_store_xor_byte(id_addr, 0x01U));
    return true;
}

/** @brief Verify object record-ID mismatch restores defaults without scalar loss. */
static bool test_nvm_object_record_meta_id_corruption_restores_default_without_scalar_loss(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_corrupt_object_record_id_after_commit());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 9U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Corrupt object header body-size while keeping the header CRC valid. */
static bool host_flash_child_corrupt_object_header_body_size_after_commit(void)
{
    host_object_store_head_t head = { 0 };

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 10U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "object"));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    TEST_ASSERT(host_object_store_read_head(PAR_CFG_NVM_OBJECT_FIXED_ADDR, &head));
    TEST_ASSERT(head.sign == HOST_OBJECT_STORE_SIGN);
    TEST_ASSERT(head.version == HOST_OBJECT_STORE_VERSION);
    head.body_size ^= 1UL;
    head.crc = host_object_store_calc_head_crc(&head);
    TEST_ASSERT(host_object_store_write_head(PAR_CFG_NVM_OBJECT_FIXED_ADDR, &head));
    return true;
}

/** @brief Verify body-size mismatch rebuilds object defaults only. */
static bool test_nvm_object_header_body_size_mismatch_rebuilds_objects_only(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_corrupt_object_header_body_size_after_commit());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 10U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}


/** @brief Corrupt object header version while keeping its CRC valid. */
static bool host_flash_child_corrupt_object_header_version_after_commit(void)
{
    host_object_store_head_t head = { 0 };

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 6U));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "object"));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    TEST_ASSERT(host_object_store_read_head(PAR_CFG_NVM_OBJECT_FIXED_ADDR, &head));
    head.version ^= 1U;
    head.crc = host_object_store_calc_head_crc(&head);
    TEST_ASSERT(host_object_store_write_head(PAR_CFG_NVM_OBJECT_FIXED_ADDR, &head));
    return true;
}

/** @brief Verify object header version mismatch rebuilds objects only. */
static bool test_nvm_object_header_version_mismatch_rebuilds_objects_only(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_corrupt_object_header_version_after_commit());
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 6U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Corrupt one object record metadata field after committing values. */
static bool host_flash_child_corrupt_object_record_meta_field_after_commit(const uint32_t field_offset,
                                                                           const uint8_t mask,
                                                                           const uint8_t scalar_value)
{
    static const char committed[] = "object";
    uint32_t field_addr = 0U;

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, scalar_value));
    TEST_ASSERT_OK(par_save(ePAR_TEST_MODE));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, committed));
    TEST_ASSERT_OK(par_save(ePAR_TEST_STR));
    TEST_ASSERT(host_object_store_get_record_field_addr(ePAR_TEST_STR,
                                                         (uint16_t)strlen(committed),
                                                         field_offset,
                                                         &field_addr));
    TEST_ASSERT(host_object_store_xor_byte(field_addr, mask));
    return true;
}

/** @brief Verify object record type mismatch restores object defaults only. */
static bool test_nvm_object_record_type_mismatch_restores_default_without_scalar_loss(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_corrupt_object_record_meta_field_after_commit(
            (uint32_t)offsetof(host_object_store_record_meta_t, type), 0x01U, 7U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 7U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object record flags mismatch restores object defaults only. */
static bool test_nvm_object_record_flags_mismatch_restores_default_without_scalar_loss(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_corrupt_object_record_meta_field_after_commit(
            (uint32_t)offsetof(host_object_store_record_meta_t, flags), 0x01U, 8U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 8U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object record element-size mismatch restores defaults only. */
static bool test_nvm_object_record_elem_size_mismatch_restores_default_without_scalar_loss(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_corrupt_object_record_meta_field_after_commit(
            (uint32_t)offsetof(host_object_store_record_meta_t, elem_size), 0x01U, 9U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 9U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify object record capacity mismatch restores defaults only. */
static bool test_nvm_object_record_capacity_mismatch_restores_default_without_scalar_loss(void)
{
    pid_t child_pid;
    uint8_t u8 = 0U;
    char str_buf[9] = { 0 };
    uint16_t len = 0U;

    host_flash_reset_erased();
    child_pid = fork();
    TEST_ASSERT(child_pid >= 0);
    if (0 == child_pid)
    {
        host_child_exit_from_result(host_flash_child_corrupt_object_record_meta_field_after_commit(
            (uint32_t)offsetof(host_object_store_record_meta_t, capacity), 0x01U, 10U));
    }
    TEST_ASSERT(host_child_exit_is_success(child_pid));

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 10U);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(strcmp(str_buf, "ap") == 0);
    TEST_ASSERT_OK(par_deinit());
    return true;
}

#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) && (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_FIXED) */

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


#if (PAR_CFG_NVM_OBJECT_DEDICATED_BASE_ADDR > 0U)
/** @brief Verify dedicated object persistence honors a non-zero backend base address. */
static bool test_object_dedicated_nonzero_base_save_reload_and_prefix_stable(void)
{
    uint8_t prefix[PAR_CFG_NVM_OBJECT_DEDICATED_BASE_ADDR];

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"base", 4U));
    memcpy(prefix, g_object_flash, sizeof(prefix));
    for (uint32_t idx = 0U; idx < (uint32_t)sizeof(prefix); idx++)
    {
        TEST_ASSERT(prefix[idx] == 0xFFU);
    }
    TEST_ASSERT(0xFFU != g_object_flash[PAR_CFG_NVM_OBJECT_DEDICATED_BASE_ADDR]);
    TEST_ASSERT(verify_reloaded_object_str("base"));
    TEST_ASSERT(0 == memcmp(prefix, g_object_flash, sizeof(prefix)));
    TEST_ASSERT_OK(par_deinit());
    return true;
}
#endif /* (PAR_CFG_NVM_OBJECT_DEDICATED_BASE_ADDR > 0U) */

/** @brief Verify object n-save write failures keep the persisted object stable. */
static bool test_object_dedicated_n_save_write_fail_preserves_persisted_object(void)
{
    uint8_t before[HOST_FLASH_SIZE];

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"old", 3U));
    memcpy(before, g_object_flash, sizeof(before));
    g_object_fail_write_after = 0;
    TEST_ASSERT((par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"new", 3U) & ePAR_ERROR_NVM) != 0U);
    TEST_ASSERT(0 == memcmp(before, g_object_flash, sizeof(before)));
    host_flash_clear_failpoints();
    TEST_ASSERT(verify_reloaded_object_str("old"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/** @brief Verify save-all object write failure documents scalar/default-object policy. */
static bool test_object_dedicated_save_all_object_write_fail_scalar_new_object_default(void)
{
    uint8_t u8 = 0U;

    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "old"));
    TEST_ASSERT_OK(par_save_all());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "new"));
    g_object_fail_write_after = 0;
    TEST_ASSERT((par_save_all() & ePAR_ERROR_NVM) != 0U);
    host_flash_clear_failpoints();
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &u8));
    TEST_ASSERT(u8 == 5U);
    TEST_ASSERT(verify_reloaded_object_str("ap"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}


/**
 * @brief Describe one dedicated object write-all failpoint phase.
 */
typedef struct
{
    int fail_after;          /**< Dedicated backend write countdown to fail. */
    const char *phase_name;  /**< Human-readable phase label for repro logs. */
} host_object_write_failpoint_case_t;

/**
 * @brief Verify scalar state and object defaults after failed object write-all.
 * @param expected_mode Scalar mode value expected after reload.
 * @return true when scalar persistence survived and object storage rebuilt defaults.
 */
static bool host_object_dedicated_verify_scalar_and_default_objects(
    const uint8_t expected_mode)
{
    static const uint8_t default_bytes[] = { 0x01U, 0x02U };
    uint8_t mode = 0U;
    char str_buf[9] = { 0 };
    uint8_t bytes_buf[4] = { 0U };
    uint16_t len = 0U;

    host_flash_clear_failpoints();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_get_u8(ePAR_TEST_MODE, &mode));
    TEST_ASSERT(mode == expected_mode);
    TEST_ASSERT_OK(par_get_str(ePAR_TEST_STR, str_buf, sizeof(str_buf), &len));
    TEST_ASSERT(len == 2U);
    TEST_ASSERT(0 == strcmp(str_buf, "ap"));
    TEST_ASSERT_OK(par_get_bytes(ePAR_TEST_BYTES, bytes_buf, sizeof(bytes_buf), &len));
    TEST_ASSERT(len == (uint16_t)sizeof(default_bytes));
    TEST_ASSERT(0 == memcmp(bytes_buf, default_bytes, sizeof(default_bytes)));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

/**
 * @brief Fail one selected dedicated object write-all phase in a child process.
 * @param fail_after Dedicated backend write countdown to fail.
 * @return true when the selected write-all phase reports an NVM error.
 */
static bool host_object_dedicated_child_save_all_fail_phase(const int fail_after)
{
    const uint8_t old_bytes[4] = { 1U, 2U, 3U, 4U };
    const uint8_t new_bytes[4] = { 9U, 8U, 7U, 6U };

    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 4U));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "old"));
    TEST_ASSERT_OK(par_set_bytes(ePAR_TEST_BYTES, old_bytes, (uint16_t)sizeof(old_bytes)));
    TEST_ASSERT_OK(par_save_all());

    TEST_ASSERT_OK(par_set_u8(ePAR_TEST_MODE, 5U));
    TEST_ASSERT_OK(par_set_str(ePAR_TEST_STR, "new"));
    TEST_ASSERT_OK(par_set_bytes(ePAR_TEST_BYTES, new_bytes, (uint16_t)sizeof(new_bytes)));
    g_object_fail_write_after = fail_after;
    TEST_ASSERT((par_save_all() & ePAR_STATUS_ERROR_MASK) != ePAR_OK);
    return true;
}

/** @brief Verify payload, metadata, and marker object write failpoints. */
static bool test_object_dedicated_save_all_payload_metadata_marker_failpoint_sweep(void)
{
    static const host_object_write_failpoint_case_t cases[] = {
        { 0, "str_payload" },
        { 1, "str_metadata" },
        { 2, "bytes_payload" },
        { 3, "bytes_metadata" },
        { 4, "header_marker" },
    };

    for (size_t i = 0U; i < (sizeof(cases) / sizeof(cases[0])); i++)
    {
        pid_t child_pid;

        printf("PAR_HOST_OBJECT_PHASE_FAILPOINT phase=%s fail_after=%d\n",
               cases[i].phase_name,
               cases[i].fail_after);
        host_flash_reset_erased();
        child_pid = fork();
        TEST_ASSERT(child_pid >= 0);
        if (0 == child_pid)
        {
            host_child_exit_from_result(
                host_object_dedicated_child_save_all_fail_phase(cases[i].fail_after));
        }
        TEST_ASSERT(host_child_exit_is_success(child_pid));

        child_pid = fork();
        TEST_ASSERT(child_pid >= 0);
        if (0 == child_pid)
        {
            host_child_exit_from_result(
                host_object_dedicated_verify_scalar_and_default_objects(5U));
        }
        TEST_ASSERT(host_child_exit_is_success(child_pid));
    }

    return true;
}


/** @brief Verify repeated dedicated object write failures converge to last object. */
static bool test_object_dedicated_repeated_write_failures_converge_to_last_object(void)
{
    host_flash_reset_erased();
    TEST_ASSERT(init_module());
    TEST_ASSERT_OK(par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"old", 3U));
    g_object_fail_write_after = 0;
    TEST_ASSERT((par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"new", 3U) &
                 ePAR_ERROR_NVM) != 0U);
    host_flash_clear_failpoints();
    TEST_ASSERT(verify_reloaded_object_str("old"));
    g_object_fail_write_after = 0;
    TEST_ASSERT((par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"bad", 3U) &
                 ePAR_ERROR_NVM) != 0U);
    host_flash_clear_failpoints();
    TEST_ASSERT(verify_reloaded_object_str("old"));
    TEST_ASSERT_OK(par_set_obj_n_save(ePAR_TEST_STR, (const uint8_t *)"mid", 3U));
    TEST_ASSERT(verify_reloaded_object_str("mid"));
    TEST_ASSERT_OK(par_deinit());
    return true;
}

#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED) */

/** @brief Entrypoint for NVM Flash EE host tests. */
int main(void)
{
    int result;
#if defined(PAR_HOST_TEST_SCHEMA_EVOLUTION_WRITE)
    static const par_host_test_case_t cases[] = {
        { "nvm_schema_rebuild_write_baseline_image", test_nvm_schema_rebuild_write_baseline_image },
    };
#elif defined(PAR_HOST_TEST_SCHEMA_EVOLUTION_READ)
    static const par_host_test_case_t cases[] = {
        { "nvm_schema_type_change_triggers_table_rebuild_defaults", test_nvm_schema_type_change_triggers_table_rebuild_defaults },
    };
#elif defined(PAR_HOST_TEST_SCHEMA_SLOT_REORDER_READ)
    static const par_host_test_case_t cases[] = {
        { "nvm_schema_slot_reorder_triggers_table_rebuild_defaults", test_nvm_schema_slot_reorder_triggers_table_rebuild_defaults },
    };
#elif defined(PAR_HOST_TEST_SCHEMA_PERSISTENT_REMOVED_READ)
    static const par_host_test_case_t cases[] = {
        { "nvm_schema_persistent_removed_rebuilds_remaining_defaults", test_nvm_schema_persistent_removed_rebuilds_remaining_defaults },
    };
#elif defined(PAR_HOST_TEST_SCHEMA_SCALAR_TO_OBJECT_READ)
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
    static const par_host_test_case_t cases[] = {
        { "nvm_schema_scalar_to_object_rejects_stale_image_current_policy", test_nvm_schema_scalar_to_object_rejects_stale_image_current_policy },
    };
#else
#error "PAR_HOST_TEST_SCHEMA_SCALAR_TO_OBJECT_READ requires PAR_HOST_ENABLE_CURRENT_POLICY_TESTS"
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
#elif defined(PAR_HOST_TEST_SCHEMA_OBJECT_TO_SCALAR_READ)
    static const par_host_test_case_t cases[] = {
        { "nvm_schema_object_to_scalar_rebuilds_scalar_default", test_nvm_schema_object_to_scalar_rebuilds_scalar_default },
    };
#elif defined(PAR_HOST_TEST_SCHEMA_OBJECT_CAPACITY_SHRINK_READ)
    static const par_host_test_case_t cases[] = {
        { "nvm_schema_object_capacity_shrink_rebuilds_object_default", test_nvm_schema_object_capacity_shrink_rebuilds_object_default },
    };
#elif defined(PAR_HOST_TEST_FIXED_OBJECT_INVALID)
    static const par_host_test_case_t cases[] = {
        { "nvm_fixed_object_invalid_rejects_init", test_nvm_fixed_object_invalid_rejects_init },
    };
#elif defined(PAR_HOST_TEST_OBJECT_ONLY)
    static const par_host_test_case_t cases[] = {
        { "nvm_object_only_save_reload_preserves_object", test_nvm_object_only_save_reload_preserves_object },
    };
#elif defined(PAR_HOST_TEST_OBJECT_ARRAY_NVM)
    static const par_host_test_case_t cases[] = {
        { "nvm_object_arr_u16_elem_size_corruption_restores_default", test_nvm_object_arr_u16_elem_size_corruption_restores_default },
        { "nvm_object_arr_u32_capacity_corruption_restores_default", test_nvm_object_arr_u32_capacity_corruption_restores_default },
    };
#else
    static const par_host_test_case_t cases[] = {
        { "nvm_first_boot_formats_and_restores_defaults", test_nvm_first_boot_formats_and_restores_defaults },
        { "nvm_save_reload_preserves_last_committed_scalar_values", test_nvm_save_reload_preserves_last_committed_scalar_values },
        { "nvm_object_save_reload_shared_fixed_addr", test_nvm_object_save_reload_shared_fixed_addr },
        { "nvm_object_updates_do_not_corrupt_scalar_values", test_nvm_object_updates_do_not_corrupt_scalar_values },
        { "nvm_scalar_updates_do_not_corrupt_object_values", test_nvm_scalar_updates_do_not_corrupt_object_values },
        { "flash_ee_failed_program_reload_preserves_last_committed_value", test_flash_ee_failed_program_reload_preserves_last_committed_value },
        { "flash_ee_program_failpoint_matrix_preserves_last_commit", test_flash_ee_program_failpoint_matrix_preserves_last_commit },
        { "flash_ee_repeated_failed_saves_converge_to_last_commit", test_flash_ee_repeated_failed_saves_converge_to_last_commit },
        { "flash_ee_seeded_scalar_restarts_match_model", test_flash_ee_seeded_scalar_restarts_match_model },
        { "flash_ee_representative_program_failpoint_sweep_preserves_last_commit", test_flash_ee_representative_program_failpoint_sweep_preserves_last_commit },
#if (1 == PAR_CFG_NVM_WRITE_VERIFY_EN)
        { "nvm_scalar_write_verify_detects_readback_mismatch", test_nvm_scalar_write_verify_detects_readback_mismatch },
        { "nvm_scalar_write_verify_read_error_is_reported", test_nvm_scalar_write_verify_read_error_is_reported },
#endif /* (1 == PAR_CFG_NVM_WRITE_VERIFY_EN) */
        { "flash_ee_save_all_failpoint_preserves_last_committed_scalar", test_flash_ee_save_all_failpoint_preserves_last_committed_scalar },
        { "flash_ee_save_clean_failpoint_matrix_preserves_last_commit", test_flash_ee_save_clean_failpoint_matrix_preserves_last_commit },
        { "flash_ee_save_clean_checkpoint_erase_failure_preserves_last_commit", test_flash_ee_save_clean_checkpoint_erase_failure_preserves_last_commit },
        { "flash_ee_failed_program_graceful_deinit_commits_live_value", test_flash_ee_failed_program_graceful_deinit_commits_live_value },
        { "flash_ee_corruption_rebuilds_default_value", test_flash_ee_corruption_rebuilds_default_value },
        { "nvm_table_id_mismatch_rebuilds_defaults", test_nvm_table_id_mismatch_rebuilds_defaults },
        { "flash_ee_failed_erase_preserves_existing_bytes", test_flash_ee_failed_erase_preserves_existing_bytes },
        { "flash_ee_many_updates_preserve_last_committed_value", test_flash_ee_many_updates_preserve_last_committed_value },
        { "flash_ee_record_commit_marker_corruption_ignores_partial_record", test_flash_ee_record_commit_marker_corruption_ignores_partial_record },
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
        { "flash_ee_bad_committed_tail_rebuilds_default_current_policy", test_flash_ee_bad_committed_tail_rebuilds_default_current_policy },
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
        { "flash_ee_newer_bad_record_rebuilds_default_current_policy", test_flash_ee_newer_bad_record_rebuilds_default_current_policy },
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
        { "flash_ee_callback_save_during_dispatch_current_policy", test_flash_ee_callback_save_during_dispatch_current_policy },
        { "flash_ee_callback_save_all_during_dispatch_current_policy", test_flash_ee_callback_save_all_during_dispatch_current_policy },
        { "flash_ee_callback_save_clean_during_dispatch_current_policy", test_flash_ee_callback_save_clean_during_dispatch_current_policy },
        { "flash_ee_callback_deinit_during_dispatch_current_policy", test_flash_ee_callback_deinit_during_dispatch_current_policy },
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
        { "flash_ee_checkpoint_power_loss_preserves_previous_active_bank", test_flash_ee_checkpoint_power_loss_preserves_previous_active_bank },
        { "flash_ee_checkpoint_prepare_header_power_loss_ignores_new_bank", test_flash_ee_checkpoint_prepare_header_power_loss_ignores_new_bank },
        { "flash_ee_newer_bank_with_bad_cfg_crc_is_ignored", test_flash_ee_newer_bank_with_bad_cfg_crc_is_ignored },
        { "flash_ee_checkpoint_record_copy_power_loss_preserves_previous_active_bank", test_flash_ee_checkpoint_record_copy_power_loss_preserves_previous_active_bank },
        { "flash_ee_checkpoint_record_copy_failpoint_sweep_preserves_previous_bank", test_flash_ee_checkpoint_record_copy_failpoint_sweep_preserves_previous_bank },
        { "flash_ee_port_rejects_wrapped_ranges", test_flash_ee_port_rejects_wrapped_ranges },
        { "flash_ee_program_one_to_zero_semantics", test_flash_ee_program_one_to_zero_semantics },
        { "flash_ee_partial_line_write_preserves_neighbor_bytes", test_flash_ee_partial_line_write_preserves_neighbor_bytes },
        { "flash_ee_cross_cache_window_write_reload_roundtrip", test_flash_ee_cross_cache_window_write_reload_roundtrip },
        { "flash_ee_erase_partial_line_preserves_outside_range", test_flash_ee_erase_partial_line_preserves_outside_range },
        { "flash_ee_port_failpoint_countdown_matrix", test_flash_ee_port_failpoint_countdown_matrix },
        { "flash_ee_port_read_failpoint_reports_error", test_flash_ee_port_read_failpoint_reports_error },
        { "flash_ee_init_rejects_invalid_geometry_and_recovers", test_flash_ee_init_rejects_invalid_geometry_and_recovers },
        { "nvm_save_by_id_save_all_and_n_save_wrappers", test_nvm_save_by_id_save_all_and_n_save_wrappers },
        { "nvm_scalar_stored_count_smaller_applies_layout_policy", test_nvm_scalar_stored_count_smaller_applies_layout_policy },
        { "nvm_scalar_stored_count_larger_rebuilds_defaults_once", test_nvm_scalar_stored_count_larger_rebuilds_defaults_once },
        { "nvm_obj_n_save_unchanged_skips_backend_write", test_nvm_obj_n_save_unchanged_skips_backend_write },
        { "nvm_str_embedded_nul_n_save_rejected_without_mutation", test_nvm_str_embedded_nul_n_save_rejected_without_mutation },
        { "nvm_wrapper_negative_type_and_init_policies", test_nvm_wrapper_negative_type_and_init_policies },
#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
        { "nvm_object_region_profile_bounds", test_nvm_object_region_profile_bounds },
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */
        { "nvm_save_non_persistent_is_noop_and_preserves_image", test_nvm_save_non_persistent_is_noop_and_preserves_image },
#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) && \
    (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_FIXED)
        { "nvm_object_header_corruption_restores_default_without_scalar_loss", test_nvm_object_header_corruption_restores_default_without_scalar_loss },
        { "nvm_object_record_len_corruption_restores_default_without_scalar_loss", test_nvm_object_record_len_corruption_restores_default_without_scalar_loss },
        { "nvm_object_payload_crc_corruption_restores_default_without_scalar_loss", test_nvm_object_payload_crc_corruption_restores_default_without_scalar_loss },
        { "nvm_object_record_meta_id_corruption_restores_default_without_scalar_loss", test_nvm_object_record_meta_id_corruption_restores_default_without_scalar_loss },
        { "nvm_object_header_body_size_mismatch_rebuilds_objects_only", test_nvm_object_header_body_size_mismatch_rebuilds_objects_only },
        { "nvm_object_header_version_mismatch_rebuilds_objects_only", test_nvm_object_header_version_mismatch_rebuilds_objects_only },
        { "nvm_object_record_type_mismatch_restores_default_without_scalar_loss", test_nvm_object_record_type_mismatch_restores_default_without_scalar_loss },
        { "nvm_object_record_flags_mismatch_restores_default_without_scalar_loss", test_nvm_object_record_flags_mismatch_restores_default_without_scalar_loss },
        { "nvm_object_record_elem_size_mismatch_restores_default_without_scalar_loss", test_nvm_object_record_elem_size_mismatch_restores_default_without_scalar_loss },
        { "nvm_object_record_capacity_mismatch_restores_default_without_scalar_loss", test_nvm_object_record_capacity_mismatch_restores_default_without_scalar_loss },
        { "nvm_object_bytes_payload_crc_corruption_restores_default", test_nvm_object_bytes_payload_crc_corruption_restores_default },
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) && (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_FIXED) */
#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED)
        { "nvm_shared_save_all_failpoint_preserves_last_committed_values", test_nvm_shared_save_all_failpoint_preserves_last_committed_values },
        { "nvm_shared_obj_n_save_program_failpoint_matrix_preserves_last_commit", test_nvm_shared_obj_n_save_program_failpoint_matrix_preserves_last_commit },
        { "nvm_shared_repeated_mixed_failures_converge_to_last_commit", test_nvm_shared_repeated_mixed_failures_converge_to_last_commit },
        { "nvm_shared_seeded_mixed_restarts_match_model", test_nvm_shared_seeded_mixed_restarts_match_model },
        { "nvm_shared_seeded_mixed_random_restarts_match_model", test_nvm_shared_seeded_mixed_random_restarts_match_model },
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
        { "nvm_object_validation_save_during_dispatch_current_policy", test_nvm_object_validation_save_during_dispatch_current_policy },
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
        { "nvm_object_validation_save_all_during_dispatch_current_policy", test_nvm_object_validation_save_all_during_dispatch_current_policy },
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
        { "nvm_object_validation_save_clean_during_dispatch_current_policy", test_nvm_object_validation_save_clean_during_dispatch_current_policy },
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) */
        { "msh_save_persists_live_scalar_after_restart", test_msh_save_persists_live_scalar_after_restart },
        { "msh_save_clean_rewrites_live_values", test_msh_save_clean_rewrites_live_values },
        { "msh_save_reports_backend_error", test_msh_save_reports_backend_error },
#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED)
        { "object_dedicated_write_fail_preserves_backend_image", test_object_dedicated_write_fail_preserves_backend_image },
        { "object_dedicated_read_fail_reports_error_and_recovers_object", test_object_dedicated_read_fail_reports_error_and_recovers_object },
        { "object_dedicated_sync_fail_reports_error_and_recovers_object", test_object_dedicated_sync_fail_reports_error_and_recovers_object },
#if (1 == PAR_CFG_NVM_OBJECT_WRITE_VERIFY_EN)
        { "nvm_object_write_verify_detects_payload_mismatch", test_nvm_object_write_verify_detects_payload_mismatch },
        { "nvm_object_write_verify_read_error_is_reported", test_nvm_object_write_verify_read_error_is_reported },
#endif /* (1 == PAR_CFG_NVM_OBJECT_WRITE_VERIFY_EN) */
#if (PAR_CFG_NVM_OBJECT_DEDICATED_BASE_ADDR > 0U)
        { "object_dedicated_nonzero_base_save_reload_and_prefix_stable", test_object_dedicated_nonzero_base_save_reload_and_prefix_stable },
#endif /* (PAR_CFG_NVM_OBJECT_DEDICATED_BASE_ADDR > 0U) */
        { "object_dedicated_erase_fail_aborts_save_all", test_object_dedicated_erase_fail_aborts_save_all },
        { "object_dedicated_n_save_write_fail_preserves_persisted_object", test_object_dedicated_n_save_write_fail_preserves_persisted_object },
        { "object_dedicated_save_all_object_write_fail_scalar_new_object_default", test_object_dedicated_save_all_object_write_fail_scalar_new_object_default },
        { "object_dedicated_save_all_payload_metadata_marker_failpoint_sweep", test_object_dedicated_save_all_payload_metadata_marker_failpoint_sweep },
        { "object_dedicated_repeated_write_failures_converge_to_last_object", test_object_dedicated_repeated_write_failures_converge_to_last_object },
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED) */
    };
#endif /* defined(PAR_HOST_TEST_SCHEMA_EVOLUTION_WRITE) */

    printf("PAR_HOST_NVM_PROFILE %s\n", PAR_HOST_TEST_PROFILE_NAME);
    result = par_host_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
    host_flash_remove_image();
    return result;
}
