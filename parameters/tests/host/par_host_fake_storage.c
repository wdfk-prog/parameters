/**
 * @file par_host_fake_storage.c
 * @brief Implement file-backed fake EEPROM and flash storage for host CI.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#include "par_host_fake_storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Runtime state for the host fake storage image.
 */
typedef struct
{
    par_host_fake_storage_cfg_t cfg;     /**< Active medium configuration. */
    uint8_t *p_image;                    /**< In-memory image buffer. */
    char path[512];                      /**< Backing file path. */
    bool is_init;                        /**< Initialization flag. */
    par_host_fake_storage_op_t fault_op; /**< Operation type selected by the failpoint. */
    uint32_t fault_hit_index;            /**< One-based operation index to fail. */
    uint32_t fault_prefix_bytes;         /**< Prefix bytes to apply before failing. */
    uint32_t write_count;                /**< Observed EEPROM write count. */
    uint32_t program_count;              /**< Observed flash program count. */
    uint32_t erase_count;                /**< Observed erase count. */
    jmp_buf *p_powercut_jump;            /**< Optional jump target for immediate power-cut simulation. */
} par_host_fake_storage_state_t;

/**
 * @brief Singleton fake storage state.
 */
static par_host_fake_storage_state_t g_par_host_fake_storage = { 0 };

/**
 * @brief Resolve the backing image path.
 * @param cfg Medium configuration.
 * @param p_path Destination path buffer.
 * @param path_size Destination path buffer size.
 */
static void par_host_fake_storage_resolve_path(const par_host_fake_storage_cfg_t *cfg, char *p_path, size_t path_size)
{
    const char *p_env = getenv("PAR_HOST_NVM_IMAGE");
    const char *p_default = (NULL != cfg->default_path) ? cfg->default_path : "par_host_nvm.bin";

    if ((NULL == p_path) || (0U == path_size))
    {
        return;
    }

    if ((NULL != p_env) && ('\0' != p_env[0]))
    {
        (void)snprintf(p_path, path_size, "%s", p_env);
    }
    else
    {
        (void)snprintf(p_path, path_size, "%s", p_default);
    }
    p_path[path_size - 1U] = '\0';
}

/**
 * @brief Return true when the byte range is valid for the active image.
 * @param addr Range start.
 * @param size Range size.
 * @return true when the range is valid, otherwise false.
 */
static bool par_host_fake_storage_range_valid(uint32_t addr, uint32_t size)
{
    if ((false == g_par_host_fake_storage.is_init) || (NULL == g_par_host_fake_storage.p_image))
    {
        return false;
    }

    if (addr > g_par_host_fake_storage.cfg.size)
    {
        return false;
    }

    return (size <= (g_par_host_fake_storage.cfg.size - addr));
}

/**
 * @brief Load an existing image if it has the expected size.
 * @return true when an existing image was loaded.
 */
static bool par_host_fake_storage_try_load(void)
{
    FILE *p_file = fopen(g_par_host_fake_storage.path, "rb");
    long file_size;

    if (NULL == p_file)
    {
        return false;
    }

    if (0 != fseek(p_file, 0L, SEEK_END))
    {
        (void)fclose(p_file);
        return false;
    }

    file_size = ftell(p_file);
    if ((long)g_par_host_fake_storage.cfg.size != file_size)
    {
        (void)fclose(p_file);
        return false;
    }

    if (0 != fseek(p_file, 0L, SEEK_SET))
    {
        (void)fclose(p_file);
        return false;
    }

    if (g_par_host_fake_storage.cfg.size != fread(g_par_host_fake_storage.p_image, 1U, g_par_host_fake_storage.cfg.size, p_file))
    {
        (void)fclose(p_file);
        return false;
    }

    (void)fclose(p_file);
    return true;
}

/**
 * @brief Apply an EEPROM or flash write without failpoint bookkeeping.
 * @param addr Byte offset.
 * @param size Number of bytes to apply.
 * @param p_buf Source buffer.
 * @param flash_semantics true to enforce 1-to-0 flash programming.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_host_fake_storage_apply_write(uint32_t addr, uint32_t size, const uint8_t *p_buf, bool flash_semantics)
{
    uint32_t idx;

    if ((NULL == p_buf) || (false == par_host_fake_storage_range_valid(addr, size)))
    {
        return ePAR_ERROR_PARAM;
    }

    if ((true == flash_semantics) && (true == g_par_host_fake_storage.cfg.enforce_one_to_zero))
    {
        for (idx = 0U; idx < size; idx++)
        {
            const uint8_t old_value = g_par_host_fake_storage.p_image[addr + idx];
            const uint8_t new_value = p_buf[idx];

            if (0U != ((uint32_t)(~old_value) & (uint32_t)new_value))
            {
                return ePAR_ERROR_NVM;
            }
        }
    }

    for (idx = 0U; idx < size; idx++)
    {
        const uint8_t old_value = g_par_host_fake_storage.p_image[addr + idx];
        const uint8_t new_value = p_buf[idx];

        g_par_host_fake_storage.p_image[addr + idx] = (true == flash_semantics) ? (uint8_t)(old_value & new_value) : new_value;
    }

    return ePAR_OK;
}

/**
 * @brief Apply a partial erase without failpoint bookkeeping.
 * @param addr Byte offset.
 * @param size Number of bytes to erase.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_host_fake_storage_apply_erase(uint32_t addr, uint32_t size)
{
    if (false == par_host_fake_storage_range_valid(addr, size))
    {
        return ePAR_ERROR_PARAM;
    }

    (void)memset(&g_par_host_fake_storage.p_image[addr], g_par_host_fake_storage.cfg.erased_value, size);
    return ePAR_OK;
}

/**
 * @brief Return true when the current operation should trigger the failpoint.
 * @param op Operation type.
 * @param p_counter Operation counter to increment.
 * @return true when this operation is selected by the active failpoint.
 */
static bool par_host_fake_storage_fault_matches(par_host_fake_storage_op_t op, uint32_t *p_counter)
{
    if (NULL == p_counter)
    {
        return false;
    }

    (*p_counter)++;
    return ((g_par_host_fake_storage.fault_op == op) && (g_par_host_fake_storage.fault_hit_index == *p_counter));
}

/**
 * @brief Jump back to the active test case after a failpoint-created partial write.
 */
static void par_host_fake_storage_maybe_powercut_jump(void)
{
    if (NULL != g_par_host_fake_storage.p_powercut_jump)
    {
        longjmp(*g_par_host_fake_storage.p_powercut_jump, 1);
    }
}

/**
 * @brief Release allocated image memory and reset the fake-storage singleton state.
 */
static void par_host_fake_storage_cleanup_state(void)
{
    free(g_par_host_fake_storage.p_image);
    (void)memset(&g_par_host_fake_storage, 0, sizeof(g_par_host_fake_storage));
}

par_status_t par_host_fake_storage_init(const par_host_fake_storage_cfg_t *cfg)
{
    par_status_t status;

    if ((NULL == cfg) || (0U == cfg->size) || (0U == cfg->erase_size) || (0U == cfg->program_size))
    {
        return ePAR_ERROR_PARAM;
    }

    if (true == g_par_host_fake_storage.is_init)
    {
        return ePAR_OK;
    }

    (void)memset(&g_par_host_fake_storage, 0, sizeof(g_par_host_fake_storage));
    g_par_host_fake_storage.cfg = *cfg;
    par_host_fake_storage_resolve_path(cfg, g_par_host_fake_storage.path, sizeof(g_par_host_fake_storage.path));

    g_par_host_fake_storage.p_image = (uint8_t *)malloc(cfg->size);
    if (NULL == g_par_host_fake_storage.p_image)
    {
        par_host_fake_storage_cleanup_state();
        return ePAR_ERROR;
    }

    (void)memset(g_par_host_fake_storage.p_image, cfg->erased_value, cfg->size);
    g_par_host_fake_storage.is_init = true;

    if (false == par_host_fake_storage_try_load())
    {
        status = par_host_fake_storage_sync();
        if (ePAR_OK != status)
        {
            par_host_fake_storage_cleanup_state();
        }
        return status;
    }

    return ePAR_OK;
}

par_status_t par_host_fake_storage_deinit(void)
{
    par_status_t status = ePAR_OK;

    if (false == g_par_host_fake_storage.is_init)
    {
        return ePAR_OK;
    }

    status = par_host_fake_storage_sync();
    free(g_par_host_fake_storage.p_image);
    g_par_host_fake_storage.p_image = NULL;
    g_par_host_fake_storage.is_init = false;
    return status;
}

bool par_host_fake_storage_is_init(void)
{
    return g_par_host_fake_storage.is_init;
}

par_status_t par_host_fake_storage_reset_image(void)
{
    if (false == g_par_host_fake_storage.is_init)
    {
        return ePAR_ERROR_INIT;
    }

    (void)memset(g_par_host_fake_storage.p_image, g_par_host_fake_storage.cfg.erased_value, g_par_host_fake_storage.cfg.size);
    par_host_fake_storage_clear_failpoint();
    return par_host_fake_storage_sync();
}

par_status_t par_host_fake_storage_read(uint32_t addr, uint32_t size, uint8_t *p_buf)
{
    if ((NULL == p_buf) || (false == par_host_fake_storage_range_valid(addr, size)))
    {
        return ePAR_ERROR_PARAM;
    }

    (void)memcpy(p_buf, &g_par_host_fake_storage.p_image[addr], size);
    return ePAR_OK;
}

par_status_t par_host_fake_storage_write(uint32_t addr, uint32_t size, const uint8_t *p_buf)
{
    if ((false == g_par_host_fake_storage.is_init) || (NULL == g_par_host_fake_storage.p_image))
    {
        return ePAR_ERROR_INIT;
    }

    if (true == par_host_fake_storage_fault_matches(ePAR_HOST_FAKE_STORAGE_OP_WRITE, &g_par_host_fake_storage.write_count))
    {
        uint32_t prefix = g_par_host_fake_storage.fault_prefix_bytes;
        if (prefix > size)
        {
            prefix = size;
        }
        (void)par_host_fake_storage_apply_write(addr, prefix, p_buf, false);
        (void)par_host_fake_storage_sync();
        par_host_fake_storage_maybe_powercut_jump();
        return ePAR_ERROR_NVM;
    }

    return par_host_fake_storage_apply_write(addr, size, p_buf, false);
}

par_status_t par_host_fake_storage_program(uint32_t addr, uint32_t size, const uint8_t *p_buf)
{
    if ((false == g_par_host_fake_storage.is_init) || (NULL == g_par_host_fake_storage.p_image))
    {
        return ePAR_ERROR_INIT;
    }

    if ((0U == g_par_host_fake_storage.cfg.program_size) ||
        (0U != (addr % g_par_host_fake_storage.cfg.program_size)) || (0U != (size % g_par_host_fake_storage.cfg.program_size)))
    {
        return ePAR_ERROR_PARAM;
    }

    if (true == par_host_fake_storage_fault_matches(ePAR_HOST_FAKE_STORAGE_OP_PROGRAM, &g_par_host_fake_storage.program_count))
    {
        uint32_t prefix = g_par_host_fake_storage.fault_prefix_bytes;
        if (prefix > size)
        {
            prefix = size;
        }
        (void)par_host_fake_storage_apply_write(addr, prefix, p_buf, true);
        (void)par_host_fake_storage_sync();
        par_host_fake_storage_maybe_powercut_jump();
        return ePAR_ERROR_NVM;
    }

    return par_host_fake_storage_apply_write(addr, size, p_buf, true);
}

par_status_t par_host_fake_storage_erase(uint32_t addr, uint32_t size)
{
    if ((false == g_par_host_fake_storage.is_init) || (NULL == g_par_host_fake_storage.p_image))
    {
        return ePAR_ERROR_INIT;
    }

    if ((0U == g_par_host_fake_storage.cfg.erase_size) ||
        (0U != (addr % g_par_host_fake_storage.cfg.erase_size)) || (0U != (size % g_par_host_fake_storage.cfg.erase_size)))
    {
        return ePAR_ERROR_PARAM;
    }

    if (true == par_host_fake_storage_fault_matches(ePAR_HOST_FAKE_STORAGE_OP_ERASE, &g_par_host_fake_storage.erase_count))
    {
        uint32_t prefix = g_par_host_fake_storage.fault_prefix_bytes;
        if (prefix > size)
        {
            prefix = size;
        }
        (void)par_host_fake_storage_apply_erase(addr, prefix);
        (void)par_host_fake_storage_sync();
        par_host_fake_storage_maybe_powercut_jump();
        return ePAR_ERROR_NVM;
    }

    return par_host_fake_storage_apply_erase(addr, size);
}

par_status_t par_host_fake_storage_sync(void)
{
    FILE *p_file;

    if ((false == g_par_host_fake_storage.is_init) || (NULL == g_par_host_fake_storage.p_image))
    {
        return ePAR_ERROR_INIT;
    }

    p_file = fopen(g_par_host_fake_storage.path, "wb");
    if (NULL == p_file)
    {
        return ePAR_ERROR_NVM;
    }

    if (g_par_host_fake_storage.cfg.size != fwrite(g_par_host_fake_storage.p_image, 1U, g_par_host_fake_storage.cfg.size, p_file))
    {
        (void)fclose(p_file);
        return ePAR_ERROR_NVM;
    }

    if (0 != fclose(p_file))
    {
        return ePAR_ERROR_NVM;
    }

    return ePAR_OK;
}

void par_host_fake_storage_set_failpoint(par_host_fake_storage_op_t op, uint32_t hit_index, uint32_t prefix_bytes)
{
    g_par_host_fake_storage.fault_op = op;
    g_par_host_fake_storage.fault_hit_index = hit_index;
    g_par_host_fake_storage.fault_prefix_bytes = prefix_bytes;
    g_par_host_fake_storage.write_count = 0U;
    g_par_host_fake_storage.program_count = 0U;
    g_par_host_fake_storage.erase_count = 0U;
}

void par_host_fake_storage_set_powercut_jump(jmp_buf *p_jump)
{
    g_par_host_fake_storage.p_powercut_jump = p_jump;
}

void par_host_fake_storage_clear_powercut_jump(void)
{
    g_par_host_fake_storage.p_powercut_jump = NULL;
}

void par_host_fake_storage_clear_failpoint(void)
{
    g_par_host_fake_storage.fault_op = ePAR_HOST_FAKE_STORAGE_OP_NONE;
    g_par_host_fake_storage.fault_hit_index = 0U;
    g_par_host_fake_storage.fault_prefix_bytes = 0U;
    g_par_host_fake_storage.write_count = 0U;
    g_par_host_fake_storage.program_count = 0U;
    g_par_host_fake_storage.erase_count = 0U;
}

uint32_t par_host_fake_storage_size(void)
{
    return g_par_host_fake_storage.cfg.size;
}

uint32_t par_host_fake_storage_erase_size(void)
{
    return g_par_host_fake_storage.cfg.erase_size;
}

uint32_t par_host_fake_storage_program_size(void)
{
    return g_par_host_fake_storage.cfg.program_size;
}
