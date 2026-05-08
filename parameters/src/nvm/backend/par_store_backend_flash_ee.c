/**
 * @file par_store_backend_flash_ee.c
 * @brief Implement the generic flash-emulated EEPROM backend core.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-04-14
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @details The backend exposes a byte-addressable persistence interface to
 * @ref par_nvm.c, while storing data on erase-before-write flash media.
 *
 * The core keeps a bounded write-back cache and appends fixed-size records to
 * the active flash bank during @ref sync. The flash region is split into two
 * banks. When the active bank becomes full, the backend performs a streaming
 * checkpoint into the inactive bank and then switches banks atomically through
 * a header state transition. The generic core does not depend on RT-Thread or
 * any flash SDK. Platform adapters provide the physical flash access through
 * @ref par_store_flash_ee_port_api_t.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-04-14 1.0     wdfk-prog     first version
 */
#include "par_store_backend_flash_ee.h"

#if (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN)

#include <string.h>
#include <stddef.h>
#include "par_if.h"


#define PAR_STORE_FLASH_EE_BANK_COUNT       (2u)
#define PAR_STORE_FLASH_EE_HEADER_MAGIC     (0x50454548u) /* PEEH */
#define PAR_STORE_FLASH_EE_RECORD_MAGIC     (0x50454552u) /* PEER */
#define PAR_STORE_FLASH_EE_HEADER_PREPARE   (0xFFFFFF00u)
#define PAR_STORE_FLASH_EE_HEADER_ACTIVE    (0xFFFF0000u)
#define PAR_STORE_FLASH_EE_ERASE_VALUE      (0xFFu)
#define PAR_STORE_FLASH_EE_HEADER_SIZE      (64u)
#define PAR_STORE_FLASH_EE_RECORD_META_SIZE (12u)
#define PAR_STORE_FLASH_EE_RECORD_BUF_SIZE  (PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE + PAR_STORE_FLASH_EE_RECORD_META_SIZE + (2u * PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE))
#define PAR_STORE_FLASH_EE_STATE_PATCH_BUF_SIZE \
    (((2u * PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE) >= 4u) ? (2u * PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE) : 4u)
#define PAR_STORE_FLASH_EE_CACHE_LINE_COUNT (PAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE / PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE)
#define PAR_STORE_FLASH_EE_DIRTY_BYTES      ((PAR_STORE_FLASH_EE_CACHE_LINE_COUNT + 7u) / 8u)

/**
 * @brief Describe one persistent flash-bank header.
 */
typedef struct
{
    uint32_t magic;        /**< Header magic used to identify a valid bank. */
    uint32_t version;      /**< On-flash backend format version. */
    uint32_t logical_size; /**< Exposed logical EEPROM size in bytes. */
    uint32_t line_size;    /**< Fixed payload size carried by each append record. */
    uint32_t record_size;  /**< Total append-record size including payload, metadata, padding, and commit unit. */
    uint32_t bank_size;    /**< Physical size of one bank inside the persistence region. */
    uint32_t seq;          /**< Monotonic bank sequence used to pick the newest bank. */
    uint32_t cfg_crc;      /**< CRC of the geometry fields stored in the header. */
    uint32_t state;        /**< Header state word, for example prepare or active. */
    uint32_t reserved[7];  /**< Reserved words kept erased for later format extension. */
} par_store_flash_ee_bank_header_t;

/**
 * @brief Describe the metadata stored before the final commit unit.
 *
 * @details Each record stores the payload first, then this metadata block,
 * optional erased padding, and finally one dedicated commit unit. The record
 * becomes visible only after the commit unit is programmed successfully.
 */
typedef struct
{
    uint32_t line_index;   /**< Logical line index updated by this record. */
    uint16_t payload_size; /**< Payload size in bytes. Always equals line_size. */
    uint16_t record_crc;   /**< CRC of the payload bytes plus semantic metadata fields. */
    uint32_t reserved;     /**< Reserved metadata word for future extension. */
} par_store_flash_ee_record_meta_t;

/**
 * @brief Hold the live runtime state of the backend core.
 *
 * @details The context tracks the selected active bank, the current append
 * tail, and the location of the in-RAM cache window. The cache window is only
 * a bounded staging area; committed data remains in flash append records.
 */
typedef struct
{
    bool is_bound;                                   /**< True once one physical port is bound. */
    bool is_init;                                    /**< True once the backend core finished initialization. */
    bool cache_valid;                                /**< True once the cache window contains reconstructed data. */
    const par_store_flash_ee_port_api_t *p_port_api; /**< Bound physical port API table. */
    void *p_port_ctx;                                /**< Opaque physical port context. */
    uint32_t region_size;                            /**< Full persistence-region size in bytes. */
    uint32_t bank_size;                              /**< Size of one physical bank in bytes. */
    uint32_t erase_size;                             /**< Physical erase granularity in bytes. */
    uint32_t program_size;                           /**< Physical program granularity in bytes. */
    uint32_t logical_size;                           /**< Exposed logical EEPROM size in bytes. */
    uint32_t line_size;                              /**< Fixed logical line size in bytes. */
    uint32_t line_count;                             /**< Number of logical lines in the address space. */
    uint32_t record_size;                            /**< Total append-record size in bytes. */
    uint32_t active_bank_base;                       /**< Base address of the currently active bank. */
    uint32_t inactive_bank_base;                     /**< Base address of the checkpoint target bank. */
    uint32_t active_seq;                             /**< Sequence number of the active bank. */
    uint32_t active_next_offset;                     /**< Next free append offset inside the active bank. */
    uint32_t cache_base;                             /**< Logical base address covered by the cache window. */
    uint32_t cache_size;                             /**< Actual cache-window size in bytes. */
    par_store_flash_ee_diag_t diag;                  /**< Latest diagnostic code. */
} par_store_flash_ee_ctx_t;

/**
 * @brief Hold the single backend instance state.
 */
static par_store_flash_ee_ctx_t g_par_store_flash_ee_ctx = {
    .is_bound = false,
    .is_init = false,
    .cache_valid = false,
    .p_port_api = NULL,
    .p_port_ctx = NULL,
    .region_size = 0u,
    .bank_size = 0u,
    .erase_size = 0u,
    .program_size = 0u,
    .logical_size = 0u,
    .line_size = PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE,
    .line_count = 0u,
    .record_size = 0u,
    .active_bank_base = 0u,
    .inactive_bank_base = 0u,
    .active_seq = 0u,
    .active_next_offset = 0u,
    .cache_base = 0u,
    .cache_size = 0u,
    .diag = ePAR_STORE_FLASH_EE_DIAG_NONE,
};

/**
 * @brief Store the bounded RAM cache window.
 *
 * @details The cache does not mirror the full logical EEPROM space. It only
 * keeps one aligned window so write, erase, and read operations can work on a
 * small hot region without allocating logical_size bytes of RAM.
 */
static uint8_t g_par_store_flash_ee_cache[PAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE];

/**
 * @brief Store one dirty bit per cached line.
 *
 * @details A dirty bit marks that the corresponding cache line differs from
 * the committed flash view and still needs to be appended during sync.
 */
static uint8_t g_par_store_flash_ee_dirty[PAR_STORE_FLASH_EE_DIRTY_BYTES];

/**
 * @brief Flush staged cache data into physical flash.
 *
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_sync(void);

/**
 * @brief Clear all cache dirty bits.
 */
static void par_store_flash_ee_clear_dirty(void);

/**
 * @brief Return true when one memory block is fully erased.
 *
 * @param[in] p_buf Buffer to inspect.
 * @param[in] size Buffer length in bytes.
 * @return true when the block contains only erased bytes; otherwise false.
 */
static bool par_store_flash_ee_is_erased_block(const uint8_t *p_buf, uint32_t size)
{
    uint32_t idx = 0u;

    for (idx = 0u; idx < size; ++idx)
    {
        if (PAR_STORE_FLASH_EE_ERASE_VALUE != p_buf[idx])
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Update the backend diagnostic code.
 *
 * @param[in] diag New diagnostic code.
 */
static void par_store_flash_ee_set_diag(par_store_flash_ee_diag_t diag)
{
    g_par_store_flash_ee_ctx.diag = diag;
}

/**
 * @brief Normalize a top-level backend return status.
 *
 * @details Successful top-level operations clear the last diagnostic so
 * @ref par_store_backend_flash_ee_get_diag reports the latest backend event
 * instead of keeping an older transient warning or failure forever.
 *
 * @param[in] status Operation result to normalize.
 * @return The original status value.
 */
static par_status_t par_store_flash_ee_complete_status(par_status_t status)
{
    if (ePAR_OK == status)
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_NONE);
    }

    return status;
}

/**
 * @brief Reset transient runtime state after a failed or completed session.
 *
 * @details Binding information stays intact so the same adapter can try
 * initialization again, but all derived geometry, bank-selection, and cache
 * state is cleared to avoid exposing a half-initialized backend view.
 */
static void par_store_flash_ee_reset_runtime_state(void)
{
    g_par_store_flash_ee_ctx.is_init = false;
    g_par_store_flash_ee_ctx.cache_valid = false;
    g_par_store_flash_ee_ctx.region_size = 0u;
    g_par_store_flash_ee_ctx.bank_size = 0u;
    g_par_store_flash_ee_ctx.erase_size = 0u;
    g_par_store_flash_ee_ctx.program_size = 0u;
    g_par_store_flash_ee_ctx.logical_size = 0u;
    g_par_store_flash_ee_ctx.line_size = PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE;
    g_par_store_flash_ee_ctx.line_count = 0u;
    g_par_store_flash_ee_ctx.record_size = 0u;
    g_par_store_flash_ee_ctx.active_bank_base = 0u;
    g_par_store_flash_ee_ctx.inactive_bank_base = 0u;
    g_par_store_flash_ee_ctx.active_seq = 0u;
    g_par_store_flash_ee_ctx.active_next_offset = 0u;
    g_par_store_flash_ee_ctx.cache_base = 0u;
    g_par_store_flash_ee_ctx.cache_size = 0u;
    par_store_flash_ee_clear_dirty();
}

/**
 * @brief Best-effort cleanup after backend init fails after the port opened.
 *
 * @details Once the physical port init succeeded, later failures must unwind
 * the opened port and clear transient runtime state so the next init attempt
 * does not inherit a partially initialized backend context.
 */
static void par_store_flash_ee_cleanup_failed_init(void)
{
    if ((NULL != g_par_store_flash_ee_ctx.p_port_api) &&
        (NULL != g_par_store_flash_ee_ctx.p_port_api->deinit))
    {
        const par_status_t cleanup_status =
            g_par_store_flash_ee_ctx.p_port_api->deinit(g_par_store_flash_ee_ctx.p_port_ctx);

        if (ePAR_OK != cleanup_status)
        {
            PAR_ERR_PRINT("PAR_FLASH_EE: port deinit after failed init failed, err=%u",
                          (unsigned)cleanup_status);
        }
    }

    par_store_flash_ee_reset_runtime_state();
}

/**
 * @brief Return true when a failed bank scan may be recovered by reformatting.
 *
 * @details Scan failures caused by structural incompatibility or corrupted
 * on-flash contents may be recovered by rebuilding the banks. Transport-level
 * flash read failures must not be treated as safe-to-format because they do
 * not prove that both banks are actually empty or unusable.
 *
 * @param[in] diag Diagnostic captured from the failed scan.
 * @return true when automatic reformat is allowed; otherwise false.
 */
static bool par_store_flash_ee_scan_diag_allows_reformat(par_store_flash_ee_diag_t diag)
{
    switch (diag)
    {
        case ePAR_STORE_FLASH_EE_DIAG_BANK_ERASED:
        case ePAR_STORE_FLASH_EE_DIAG_HEADER_MAGIC:
        case ePAR_STORE_FLASH_EE_DIAG_HEADER_VERSION:
        case ePAR_STORE_FLASH_EE_DIAG_HEADER_STATE:
        case ePAR_STORE_FLASH_EE_DIAG_HEADER_CRC:
        case ePAR_STORE_FLASH_EE_DIAG_HEADER_LAYOUT:
        case ePAR_STORE_FLASH_EE_DIAG_RECORD_MAGIC:
        case ePAR_STORE_FLASH_EE_DIAG_RECORD_RANGE:
        case ePAR_STORE_FLASH_EE_DIAG_RECORD_CRC:
            return true;

        default:
            return false;
    }
}

/**
 * @brief Read raw bytes from the bound physical flash port.
 *
 * @param[in] addr Byte offset inside the persistence region.
 * @param[in] size Number of bytes to read.
 * @param[out] p_buf Destination buffer.
 * @return ePAR_OK on success, otherwise @ref ePAR_ERROR_NVM.
 */
static par_status_t par_store_flash_ee_port_read(uint32_t addr, uint32_t size, uint8_t *p_buf)
{
    par_status_t status = ePAR_ERROR_NVM;

    status = g_par_store_flash_ee_ctx.p_port_api->read(g_par_store_flash_ee_ctx.p_port_ctx, addr, size, p_buf);
    if (ePAR_OK != status)
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_PORT_READ);
        PAR_ERR_PRINT("PAR_FLASH_EE: port read failed, addr=%lu, size=%lu", (unsigned long)addr, (unsigned long)size);
    }

    return status;
}

/**
 * @brief Program raw bytes through the bound physical flash port.
 *
 * @param[in] addr Byte offset inside the persistence region.
 * @param[in] size Number of bytes to program.
 * @param[in] p_buf Source buffer.
 * @return ePAR_OK on success, otherwise @ref ePAR_ERROR_NVM.
 */
static par_status_t par_store_flash_ee_port_program(uint32_t addr, uint32_t size, const uint8_t *p_buf)
{
    par_status_t status = ePAR_ERROR_NVM;

    status = g_par_store_flash_ee_ctx.p_port_api->program(g_par_store_flash_ee_ctx.p_port_ctx, addr, size, p_buf);
    if (ePAR_OK != status)
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_PORT_PROGRAM);
        PAR_ERR_PRINT("PAR_FLASH_EE: port program failed, addr=%lu, size=%lu", (unsigned long)addr, (unsigned long)size);
    }

    return status;
}

/**
 * @brief Erase raw bytes through the bound physical flash port.
 *
 * @param[in] addr Byte offset inside the persistence region.
 * @param[in] size Number of bytes to erase.
 * @return ePAR_OK on success, otherwise @ref ePAR_ERROR_NVM.
 */
static par_status_t par_store_flash_ee_port_erase(uint32_t addr, uint32_t size)
{
    par_status_t status = ePAR_ERROR_NVM;

    status = g_par_store_flash_ee_ctx.p_port_api->erase(g_par_store_flash_ee_ctx.p_port_ctx, addr, size);
    if (ePAR_OK != status)
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_PORT_ERASE);
        PAR_ERR_PRINT("PAR_FLASH_EE: port erase failed, addr=%lu, size=%lu", (unsigned long)addr, (unsigned long)size);
    }

    return status;
}

/**
 * @brief Calculate the persistent bank-header CRC.
 *
 * @param[in] p_header Header to hash.
 * @return CRC16 value widened to 32 bits.
 */
static uint32_t par_store_flash_ee_calc_header_crc(const par_store_flash_ee_bank_header_t *p_header)
{
    uint16_t crc = PAR_IF_CRC16_INIT;
    uint32_t size = (uint32_t)offsetof(par_store_flash_ee_bank_header_t, cfg_crc);

    crc = par_if_crc16_accumulate(crc, (const uint8_t *)p_header, size);
    return (uint32_t)crc;
}

/**
 * @brief Calculate one append-record integrity CRC.
 *
 * @details The CRC covers the payload bytes and the metadata fields that define
 * the record semantics during reconstruction. This prevents silently accepting
 * a committed payload under a corrupted logical line index or payload size.
 *
 * @param[in] p_payload Payload bytes.
 * @param[in] payload_size Payload length in bytes.
 * @param[in] line_index Logical line index stored in the record metadata.
 * @return CRC16 value.
 */
static uint16_t par_store_flash_ee_calc_record_crc(const uint8_t *p_payload, uint16_t payload_size, uint32_t line_index)
{
    uint16_t crc = PAR_IF_CRC16_INIT;

    crc = par_if_crc16_accumulate(crc, p_payload, (uint32_t)payload_size);
    crc = par_if_crc16_accumulate(crc, (const uint8_t *)&line_index, sizeof(line_index));
    crc = par_if_crc16_accumulate(crc, (const uint8_t *)&payload_size, sizeof(payload_size));
    return crc;
}

/**
 * @brief Return one bank base offset.
 *
 * @param[in] bank_index Bank index, either 0 or 1.
 * @return Bank base offset in bytes.
 */
static uint32_t par_store_flash_ee_bank_base(uint32_t bank_index)
{
    return (bank_index * g_par_store_flash_ee_ctx.bank_size);
}

/**
 * @brief Test whether one cache line is marked dirty.
 *
 * @param[in] cache_line Cache-line index inside the current cache window.
 * @return true when dirty; otherwise false.
 */
static bool par_store_flash_ee_is_dirty(uint32_t cache_line)
{
    return (0u != (g_par_store_flash_ee_dirty[cache_line / 8u] & (uint8_t)(1u << (cache_line % 8u))));
}

/**
 * @brief Mark one cache line as dirty.
 *
 * @param[in] cache_line Cache-line index inside the current cache window.
 */
static void par_store_flash_ee_mark_dirty(uint32_t cache_line)
{
    g_par_store_flash_ee_dirty[cache_line / 8u] |= (uint8_t)(1u << (cache_line % 8u));
}

/**
 * @brief Clear all cache dirty bits.
 */
static void par_store_flash_ee_clear_dirty(void)
{
    (void)memset(g_par_store_flash_ee_dirty, 0, sizeof(g_par_store_flash_ee_dirty));
}

/**
 * @brief Return true when the current cache contains dirty lines.
 *
 * @return true when dirty data is pending; otherwise false.
 */
static bool par_store_flash_ee_has_dirty(void)
{
    uint32_t idx = 0u;

    for (idx = 0u; idx < (uint32_t)sizeof(g_par_store_flash_ee_dirty); ++idx)
    {
        if (0u != g_par_store_flash_ee_dirty[idx])
        {
            return true;
        }
    }

    return false;
}


/**
 * @brief Count dirty cache lines inside the active cache window.
 *
 * @details The returned value is later converted into the number of append
 * records needed during sync. Each dirty cache line becomes exactly one record
 * append unless a checkpoint is triggered first.
 *
 * @return Number of dirty cache lines.
 */
static uint32_t par_store_flash_ee_count_dirty(void)
{
    uint32_t count = 0u;
    uint32_t cache_line = 0u;

    for (cache_line = 0u; cache_line < (g_par_store_flash_ee_ctx.cache_size / g_par_store_flash_ee_ctx.line_size); ++cache_line)
    {
        if (true == par_store_flash_ee_is_dirty(cache_line))
        {
            ++count;
        }
    }

    return count;
}

/**
 * @brief Align one value upward to the requested granularity.
 *
 * @param[in] value Input value to align.
 * @param[in] align Alignment granularity in bytes.
 * @return Aligned value.
 */
static uint32_t par_store_flash_ee_align_up(uint32_t value, uint32_t align)
{
    return ((value + align - 1u) / align) * align;
}

/**
 * @brief Return the metadata offset inside one append record.
 *
 * @return Metadata offset in bytes.
 */
static uint32_t par_store_flash_ee_record_meta_offset(void)
{
    return g_par_store_flash_ee_ctx.line_size;
}

/**
 * @brief Return the commit-unit offset inside one append record.
 *
 * @return Commit-unit offset in bytes.
 */
static uint32_t par_store_flash_ee_record_commit_offset(void)
{
    return par_store_flash_ee_align_up(par_store_flash_ee_record_meta_offset() + PAR_STORE_FLASH_EE_RECORD_META_SIZE,
                                       g_par_store_flash_ee_ctx.program_size);
}

/**
 * @brief Build the expected commit-unit bytes.
 *
 * @param[out] p_commit_buf Destination buffer with program-size bytes.
 */
static void par_store_flash_ee_build_commit_unit(uint8_t *p_commit_buf)
{
    uint32_t magic = PAR_STORE_FLASH_EE_RECORD_MAGIC;
    uint8_t magic_bytes[sizeof(uint32_t)];
    uint32_t idx = 0u;

    (void)memcpy(magic_bytes, &magic, sizeof(magic_bytes));
    for (idx = 0u; idx < g_par_store_flash_ee_ctx.program_size; ++idx)
    {
        p_commit_buf[idx] = magic_bytes[idx % (uint32_t)sizeof(magic_bytes)];
    }
}

/**
 * @brief Return true when one commit unit matches the committed pattern.
 *
 * @param[in] p_commit_buf Commit-unit bytes read from flash.
 * @return true when the record is committed; otherwise false.
 */
static bool par_store_flash_ee_is_valid_commit_unit(const uint8_t *p_commit_buf)
{
    uint8_t expected_commit[PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE];

    if (NULL == p_commit_buf)
    {
        return false;
    }

    par_store_flash_ee_build_commit_unit(expected_commit);
    return (0 == memcmp(p_commit_buf, expected_commit, g_par_store_flash_ee_ctx.program_size));
}

/**
 * @brief Return true when one commit unit still contains erased bytes.
 *
 * @param[in] p_commit_buf Commit-unit bytes read from flash.
 * @return true when the commit unit is still open; otherwise false.
 */
static bool par_store_flash_ee_commit_unit_has_erased_byte(const uint8_t *p_commit_buf)
{
    uint32_t idx = 0u;

    if (NULL == p_commit_buf)
    {
        return false;
    }

    for (idx = 0u; idx < g_par_store_flash_ee_ctx.program_size; ++idx)
    {
        if (PAR_STORE_FLASH_EE_ERASE_VALUE == p_commit_buf[idx])
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Parse one on-flash record metadata block.
 *
 * @param[in] p_record_buf Full record bytes.
 * @param[out] p_meta Receives the parsed metadata.
 */
static void par_store_flash_ee_parse_record_meta(const uint8_t *p_record_buf, par_store_flash_ee_record_meta_t *p_meta)
{
    (void)memcpy(p_meta,
                 &p_record_buf[par_store_flash_ee_record_meta_offset()],
                 sizeof(*p_meta));
}

/**
 * @brief Validate static and runtime backend geometry.
 *
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_validate_geometry(void)
{
    uint32_t max_records = 0u;

    if ((0u == g_par_store_flash_ee_ctx.region_size) ||
        (0u == g_par_store_flash_ee_ctx.erase_size) ||
        (0u == g_par_store_flash_ee_ctx.program_size))
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_PORT_GEOMETRY);
        return ePAR_ERROR_INIT;
    }

    if (g_par_store_flash_ee_ctx.program_size > PAR_CFG_NVM_BACKEND_FLASH_EE_PROGRAM_SIZE)
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_CONFIG);
        return ePAR_ERROR_INIT;
    }

    if ((0u == PAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE) ||
        (0u == PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE) ||
        (0u == PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE) ||
        (0u != (PAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE % PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE)) ||
        (0u != (PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE % PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE)))
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_CONFIG);
        return ePAR_ERROR_INIT;
    }

    g_par_store_flash_ee_ctx.bank_size = g_par_store_flash_ee_ctx.region_size / PAR_STORE_FLASH_EE_BANK_COUNT;
    g_par_store_flash_ee_ctx.logical_size = PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE;
    g_par_store_flash_ee_ctx.line_size = PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE;
    g_par_store_flash_ee_ctx.line_count = (g_par_store_flash_ee_ctx.logical_size + g_par_store_flash_ee_ctx.line_size - 1u) /
                                          g_par_store_flash_ee_ctx.line_size;
    g_par_store_flash_ee_ctx.record_size = par_store_flash_ee_record_commit_offset() + g_par_store_flash_ee_ctx.program_size;

    if ((0u != (PAR_STORE_FLASH_EE_HEADER_SIZE % g_par_store_flash_ee_ctx.program_size)) ||
        (0u != (g_par_store_flash_ee_ctx.record_size % g_par_store_flash_ee_ctx.program_size)))
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_CONFIG);
        return ePAR_ERROR_INIT;
    }

    if ((0u != (g_par_store_flash_ee_ctx.bank_size % g_par_store_flash_ee_ctx.erase_size)) ||
        (g_par_store_flash_ee_ctx.bank_size <= PAR_STORE_FLASH_EE_HEADER_SIZE))
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_PORT_GEOMETRY);
        return ePAR_ERROR_INIT;
    }

    max_records = (g_par_store_flash_ee_ctx.bank_size - PAR_STORE_FLASH_EE_HEADER_SIZE) /
                  g_par_store_flash_ee_ctx.record_size;
    if (max_records <= g_par_store_flash_ee_ctx.line_count)
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_CAPACITY);
        PAR_ERR_PRINT("PAR_FLASH_EE: bank capacity too small, max_records=%lu, line_count=%lu (need one extra append slot)",
                      (unsigned long)max_records,
                      (unsigned long)g_par_store_flash_ee_ctx.line_count);
        return ePAR_ERROR_INIT;
    }

    return ePAR_OK;
}

/**
 * @brief Probe one bank header and report its compatibility result.

 *
 * @param[in] p_header Bank header bytes.
 * @return Diagnostic code describing the probe result.
 */
static par_store_flash_ee_diag_t par_store_flash_ee_probe_header(const par_store_flash_ee_bank_header_t *p_header)
{
    if (true == par_store_flash_ee_is_erased_block((const uint8_t *)p_header, sizeof(*p_header)))
    {
        return ePAR_STORE_FLASH_EE_DIAG_BANK_ERASED;
    }

    if (PAR_STORE_FLASH_EE_HEADER_MAGIC != p_header->magic)
    {
        return ePAR_STORE_FLASH_EE_DIAG_HEADER_MAGIC;
    }

    if (PAR_CFG_NVM_BACKEND_FLASH_EE_VERSION != p_header->version)
    {
        return ePAR_STORE_FLASH_EE_DIAG_HEADER_VERSION;
    }

    if (PAR_STORE_FLASH_EE_HEADER_ACTIVE != p_header->state)
    {
        return ePAR_STORE_FLASH_EE_DIAG_HEADER_STATE;
    }

    if (par_store_flash_ee_calc_header_crc(p_header) != p_header->cfg_crc)
    {
        return ePAR_STORE_FLASH_EE_DIAG_HEADER_CRC;
    }

    if ((g_par_store_flash_ee_ctx.logical_size != p_header->logical_size) ||
        (g_par_store_flash_ee_ctx.line_size != p_header->line_size) ||
        (g_par_store_flash_ee_ctx.record_size != p_header->record_size) ||
        (g_par_store_flash_ee_ctx.bank_size != p_header->bank_size))
    {
        return ePAR_STORE_FLASH_EE_DIAG_HEADER_LAYOUT;
    }

    return ePAR_STORE_FLASH_EE_DIAG_NONE;
}

/**
 * @brief Return true when one programmed slot is still uncommitted.
 *
 * @details The flash-ee protocol writes payload and metadata first, then
 * programs one dedicated commit unit last. A slot is therefore treated as
 * uncommitted when it is not fully erased and its commit unit still contains at
 * least one erased byte.
 *
 * @param[in] p_record_buf Full record-sized buffer read from flash.
 * @return true when the slot is uncommitted; otherwise false.
 */
static bool par_store_flash_ee_is_uncommitted_record_slot(const uint8_t *p_record_buf)
{
    const uint32_t commit_offset = par_store_flash_ee_record_commit_offset();

    if (NULL == p_record_buf)
    {
        return false;
    }

    if (true == par_store_flash_ee_is_erased_block(p_record_buf, g_par_store_flash_ee_ctx.record_size))
    {
        return false;
    }

    return par_store_flash_ee_commit_unit_has_erased_byte(&p_record_buf[commit_offset]);
}

/**
 * @brief Scan one active bank and return the first free record offset.
 *
 * @param[in] bank_base Bank base offset.
 * @param[out] p_seq Receives the header sequence number.
 * @param[out] p_next_offset Receives the next append offset inside the bank.
 * @return ePAR_OK when the bank is valid enough to use; otherwise an error.
 *
 * @note Scan accepts one fully erased record slot as the end of the append
 * region. It also skips any programmed slot whose final commit unit still
 * contains erased bytes, because such a slot never became visible. Any slot
 * with a closed but invalid commit unit, or any committed record with invalid
 * metadata or CRC, makes the bank unusable.
 */
static par_status_t par_store_flash_ee_scan_bank(uint32_t bank_base, uint32_t *p_seq, uint32_t *p_next_offset)
{
    par_store_flash_ee_bank_header_t header;
    par_store_flash_ee_record_meta_t record_meta;
    uint8_t line_buf[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    uint8_t record_buf[PAR_STORE_FLASH_EE_RECORD_BUF_SIZE];
    const uint32_t commit_offset = par_store_flash_ee_record_commit_offset();
    par_store_flash_ee_diag_t probe = ePAR_STORE_FLASH_EE_DIAG_NONE;
    uint32_t offset = PAR_STORE_FLASH_EE_HEADER_SIZE;
    par_status_t status = ePAR_OK;

    status = par_store_flash_ee_port_read(bank_base, sizeof(header), (uint8_t *)&header);
    if (ePAR_OK != status)
    {
        return status;
    }

    probe = par_store_flash_ee_probe_header(&header);
    if (ePAR_STORE_FLASH_EE_DIAG_NONE != probe)
    {
        par_store_flash_ee_set_diag(probe);
        return ePAR_ERROR_NVM;
    }

    while ((offset + g_par_store_flash_ee_ctx.record_size) <= g_par_store_flash_ee_ctx.bank_size)
    {
        status = par_store_flash_ee_port_read(bank_base + offset,
                                              g_par_store_flash_ee_ctx.record_size,
                                              record_buf);
        if (ePAR_OK != status)
        {
            return status;
        }

        if (true == par_store_flash_ee_is_erased_block(record_buf, g_par_store_flash_ee_ctx.record_size))
        {
            break;
        }

        if (false == par_store_flash_ee_is_valid_commit_unit(&record_buf[commit_offset]))
        {
            if (true == par_store_flash_ee_is_uncommitted_record_slot(record_buf))
            {
                if (ePAR_STORE_FLASH_EE_DIAG_RECORD_TAIL != g_par_store_flash_ee_ctx.diag)
                {
                    par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_RECORD_TAIL);
                }
                offset += g_par_store_flash_ee_ctx.record_size;
                continue;
            }

            par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_RECORD_MAGIC);
            return ePAR_ERROR_NVM;
        }

        par_store_flash_ee_parse_record_meta(record_buf, &record_meta);
        if ((record_meta.line_index >= g_par_store_flash_ee_ctx.line_count) ||
            (record_meta.payload_size != g_par_store_flash_ee_ctx.line_size))
        {
            par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_RECORD_RANGE);
            return ePAR_ERROR_NVM;
        }

        (void)memcpy(line_buf, record_buf, g_par_store_flash_ee_ctx.line_size);
        if (par_store_flash_ee_calc_record_crc(line_buf,
                                               record_meta.payload_size,
                                               record_meta.line_index) != record_meta.record_crc)
        {
            par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_RECORD_CRC);
            return ePAR_ERROR_NVM;
        }

        offset += g_par_store_flash_ee_ctx.record_size;
    }

    *p_seq = header.seq;
    *p_next_offset = offset;
    return ePAR_OK;
}

/**
 * @brief Format one bank header in RAM.
 *
 * @param[out] p_header Destination header.
 * @param[in] seq Bank sequence number.
 * @param[in] state Header state value.
 */
static void par_store_flash_ee_build_header(par_store_flash_ee_bank_header_t *p_header, uint32_t seq, uint32_t state)
{
    (void)memset(p_header, 0xFF, sizeof(*p_header));
    p_header->magic = PAR_STORE_FLASH_EE_HEADER_MAGIC;
    p_header->version = PAR_CFG_NVM_BACKEND_FLASH_EE_VERSION;
    p_header->logical_size = g_par_store_flash_ee_ctx.logical_size;
    p_header->line_size = g_par_store_flash_ee_ctx.line_size;
    p_header->record_size = g_par_store_flash_ee_ctx.record_size;
    p_header->bank_size = g_par_store_flash_ee_ctx.bank_size;
    p_header->seq = seq;
    p_header->state = state;
    p_header->cfg_crc = par_store_flash_ee_calc_header_crc(p_header);
}

/**
 * @brief Prepare one empty bank for later record appends.
 *
 * @param[in] bank_base Bank base offset.
 * @param[in] seq Sequence number to place into the new header.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_prepare_bank(uint32_t bank_base, uint32_t seq)
{
    par_store_flash_ee_bank_header_t header;
    par_status_t status = ePAR_OK;

    status = par_store_flash_ee_port_erase(bank_base, g_par_store_flash_ee_ctx.bank_size);
    if (ePAR_OK != status)
    {
        return status;
    }

    par_store_flash_ee_build_header(&header, seq, PAR_STORE_FLASH_EE_HEADER_PREPARE);
    status = par_store_flash_ee_port_program(bank_base, sizeof(header), (const uint8_t *)&header);
    return status;
}

/**
 * @brief Activate one prepared bank by patching the state word.
 *
 * @param[in] bank_base Bank base offset.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_activate_bank(uint32_t bank_base)
{
    uint8_t state_patch[PAR_STORE_FLASH_EE_STATE_PATCH_BUF_SIZE];
    const uint32_t state_addr = bank_base + (uint32_t)offsetof(par_store_flash_ee_bank_header_t, state);
    const uint32_t patch_addr = state_addr - (state_addr % g_par_store_flash_ee_ctx.program_size);
    const uint32_t patch_offset = state_addr - patch_addr;
    const uint32_t patch_size = (((patch_offset + (uint32_t)sizeof(uint32_t)) + g_par_store_flash_ee_ctx.program_size - 1u) /
                                 g_par_store_flash_ee_ctx.program_size) *
                                g_par_store_flash_ee_ctx.program_size;
    const uint32_t active_state = PAR_STORE_FLASH_EE_HEADER_ACTIVE;
    par_status_t status = ePAR_OK;

    status = par_store_flash_ee_port_read(patch_addr, patch_size, state_patch);
    if (ePAR_OK != status)
    {
        return status;
    }

    (void)memcpy(&state_patch[patch_offset], &active_state, sizeof(active_state));

    return par_store_flash_ee_port_program(patch_addr,
                                           patch_size,
                                           state_patch);
}

/**
 * @brief Resolve one logical line from the active flash bank.
 *
 * @param[in] line_index Logical line index.
 * @param[out] p_line_buf Destination line buffer.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_resolve_line_from_bank(uint32_t line_index, uint8_t *p_line_buf)
{
    par_store_flash_ee_record_meta_t record_meta;
    uint8_t record_buf[PAR_STORE_FLASH_EE_RECORD_BUF_SIZE];
    const uint32_t commit_offset = par_store_flash_ee_record_commit_offset();
    par_status_t status = ePAR_OK;
    uint32_t offset = g_par_store_flash_ee_ctx.active_next_offset;

    (void)memset(p_line_buf, PAR_STORE_FLASH_EE_ERASE_VALUE, g_par_store_flash_ee_ctx.line_size);

    while (offset > PAR_STORE_FLASH_EE_HEADER_SIZE)
    {
        offset -= g_par_store_flash_ee_ctx.record_size;

        status = par_store_flash_ee_port_read(g_par_store_flash_ee_ctx.active_bank_base + offset,
                                              g_par_store_flash_ee_ctx.record_size,
                                              record_buf);
        if (ePAR_OK != status)
        {
            return status;
        }

        if (false == par_store_flash_ee_is_valid_commit_unit(&record_buf[commit_offset]))
        {
            continue;
        }

        par_store_flash_ee_parse_record_meta(record_buf, &record_meta);
        if ((record_meta.line_index != line_index) ||
            (record_meta.payload_size != g_par_store_flash_ee_ctx.line_size))
        {
            continue;
        }

        (void)memcpy(p_line_buf, record_buf, g_par_store_flash_ee_ctx.line_size);
        if (par_store_flash_ee_calc_record_crc(p_line_buf,
                                               record_meta.payload_size,
                                               record_meta.line_index) == record_meta.record_crc)
        {
            return ePAR_OK;
        }
    }

    return ePAR_OK;
}

/**
 * @brief Resolve one logical line from the current live backend view.

 *
 * @details The backend first checks whether the requested line belongs to the
 * currently loaded cache window. If yes, the cache line is returned directly
 * because it already contains any staged updates. Otherwise the function walks
 * the committed append log of the active bank backwards until it finds the
 * newest record for the line.
 *
 * @param[in] line_index Logical line index.
 * @param[out] p_line_buf Destination line buffer.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_resolve_live_line(uint32_t line_index, uint8_t *p_line_buf)
{
    uint32_t line_addr = line_index * g_par_store_flash_ee_ctx.line_size;

    if ((true == g_par_store_flash_ee_ctx.cache_valid) &&
        (line_addr >= g_par_store_flash_ee_ctx.cache_base) &&
        ((line_addr + g_par_store_flash_ee_ctx.line_size) <=
         (g_par_store_flash_ee_ctx.cache_base + g_par_store_flash_ee_ctx.cache_size)))
    {
        uint32_t cache_offset = line_addr - g_par_store_flash_ee_ctx.cache_base;
        (void)memcpy(p_line_buf, &g_par_store_flash_ee_cache[cache_offset], g_par_store_flash_ee_ctx.line_size);
        return ePAR_OK;
    }

    return par_store_flash_ee_resolve_line_from_bank(line_index, p_line_buf);
}

/**
 * @brief Read bytes directly from the current live backend view.
 *
 * @details This helper reconstructs bytes line by line without moving the
 * active cache window. It is used when a dirty cache window already exists and
 * the caller needs to read another logical window without committing pending
 * updates.
 *
 * @param[in] addr Start address inside the logical address space.
 * @param[in] size Number of bytes to read.
 * @param[out] p_buf Destination buffer.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_read_live_range(uint32_t addr, uint32_t size, uint8_t *p_buf)
{
    par_status_t status = ePAR_OK;
    uint8_t line_buf[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];
    uint32_t remaining = size;
    uint32_t cur_addr = addr;
    uint8_t *p_dst = p_buf;

    while (remaining > 0u)
    {
        const uint32_t line_index = cur_addr / g_par_store_flash_ee_ctx.line_size;
        const uint32_t line_offset = cur_addr % g_par_store_flash_ee_ctx.line_size;
        uint32_t chunk = g_par_store_flash_ee_ctx.line_size - line_offset;

        status = par_store_flash_ee_resolve_live_line(line_index, line_buf);
        if (ePAR_OK != status)
        {
            return status;
        }

        if (chunk > remaining)
        {
            chunk = remaining;
        }

        (void)memcpy(p_dst, &line_buf[line_offset], chunk);
        remaining -= chunk;
        cur_addr += chunk;
        p_dst += chunk;
    }

    return ePAR_OK;
}

/**
 * @brief Return the cache-window base that covers one logical address.
 *
 * @param[in] addr Logical address inside the emulated EEPROM space.
 * @return Cache-window base address.
 */
static uint32_t par_store_flash_ee_window_base(uint32_t addr)
{
    return (addr / PAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE) * PAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE;
}

/**
 * @brief Return true when one logical range stays inside the emulated EEPROM space.
 *
 * @param[in] addr Start address inside the logical address space.
 * @param[in] size Number of bytes in the request.
 * @return true when the full range is valid; otherwise false.
 */
static bool par_store_flash_ee_range_is_valid(uint32_t addr, uint32_t size)
{
    if ((0u == size) || (addr >= g_par_store_flash_ee_ctx.logical_size))
    {
        return false;
    }

    return (size <= (g_par_store_flash_ee_ctx.logical_size - addr));
}

/**
 * @brief Append one fixed-size line record into the target bank.
 *
 * @details The function writes the payload and metadata first, leaves the
 * dedicated commit unit erased, and then programs that commit unit last. A
 * record therefore becomes visible only after the final commit write succeeds.
 *
 * @param[in] bank_base Bank base offset.
 * @param[in,out] p_next_offset Current append offset. Advanced once the
 * slot payload and metadata have been programmed, even if the final commit
 * unit write later fails.
 * @param[in] line_index Logical line index.
 * @param[in] p_line_buf Payload bytes for this logical line.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_append_record(uint32_t bank_base,
                                                     uint32_t *p_next_offset,
                                                     uint32_t line_index,
                                                     const uint8_t *p_line_buf)
{
    uint8_t record_buf[PAR_STORE_FLASH_EE_RECORD_BUF_SIZE];
    par_store_flash_ee_record_meta_t record_meta;
    const uint32_t meta_offset = par_store_flash_ee_record_meta_offset();
    const uint32_t commit_offset = par_store_flash_ee_record_commit_offset();

    if ((*p_next_offset + g_par_store_flash_ee_ctx.record_size) > g_par_store_flash_ee_ctx.bank_size)
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_CAPACITY);
        return ePAR_ERROR_NVM;
    }

    (void)memset(record_buf, 0xFF, g_par_store_flash_ee_ctx.record_size);
    (void)memcpy(record_buf, p_line_buf, g_par_store_flash_ee_ctx.line_size);

    record_meta.line_index = line_index;
    record_meta.payload_size = (uint16_t)g_par_store_flash_ee_ctx.line_size;
    record_meta.record_crc = par_store_flash_ee_calc_record_crc(p_line_buf,
                                                                (uint16_t)g_par_store_flash_ee_ctx.line_size,
                                                                line_index);
    record_meta.reserved = 0xFFFFFFFFu;
    (void)memcpy(&record_buf[meta_offset], &record_meta, sizeof(record_meta));

    {
        const uint32_t slot_offset = *p_next_offset;

        if (ePAR_OK != par_store_flash_ee_port_program(bank_base + slot_offset,
                                                       commit_offset,
                                                       record_buf))
        {
            return ePAR_ERROR_NVM;
        }

        *p_next_offset += g_par_store_flash_ee_ctx.record_size;

        par_store_flash_ee_build_commit_unit(&record_buf[commit_offset]);
        if (ePAR_OK != par_store_flash_ee_port_program(bank_base + slot_offset + commit_offset,
                                                       g_par_store_flash_ee_ctx.program_size,
                                                       &record_buf[commit_offset]))
        {
            return ePAR_ERROR_NVM;
        }
    }

    return ePAR_OK;
}

/**
 * @brief Flush the current cache window by appending all dirty lines.
 *
 * @details The cache window is scanned line by line. Each dirty line is
 * converted into one append record and written to the current active bank in
 * ascending line order. The cache contents remain valid after the flush, but
 * the dirty bitmap is cleared because the committed flash state now matches
 * the cached state for this window.
 *
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_flush_cache(void)
{
    par_status_t status = ePAR_OK;
    uint32_t cache_line = 0u;

    if ((false == g_par_store_flash_ee_ctx.cache_valid) || (false == par_store_flash_ee_has_dirty()))
    {
        return ePAR_OK;
    }

    for (cache_line = 0u; cache_line < (g_par_store_flash_ee_ctx.cache_size / g_par_store_flash_ee_ctx.line_size); ++cache_line)
    {
        if (false == par_store_flash_ee_is_dirty(cache_line))
        {
            continue;
        }

        if ((g_par_store_flash_ee_ctx.active_next_offset + g_par_store_flash_ee_ctx.record_size) > g_par_store_flash_ee_ctx.bank_size)
        {
            return ePAR_ERROR_NVM;
        }

        status = par_store_flash_ee_append_record(g_par_store_flash_ee_ctx.active_bank_base,
                                                  &g_par_store_flash_ee_ctx.active_next_offset,
                                                  (g_par_store_flash_ee_ctx.cache_base / g_par_store_flash_ee_ctx.line_size) + cache_line,
                                                  &g_par_store_flash_ee_cache[cache_line * g_par_store_flash_ee_ctx.line_size]);
        if (ePAR_OK != status)
        {
            return status;
        }
    }

    par_store_flash_ee_clear_dirty();
    return ePAR_OK;
}

/**
 * @brief Load one aligned cache window from the live backend view.
 *
 * @details The loader only reconstructs the requested window in RAM. It never
 * flushes dirty data implicitly. Callers that try to move away from a dirty
 * cache window must either sync first or keep reading through the live view
 * helper so pending updates stay staged in RAM.
 *
 * @param[in] window_base Start address of the new cache window.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_load_cache(uint32_t window_base)
{
    par_status_t status = ePAR_OK;
    uint32_t line = 0u;

    if ((window_base >= g_par_store_flash_ee_ctx.logical_size) ||
        (0u != (window_base % g_par_store_flash_ee_ctx.line_size)))
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_CACHE_WINDOW);
        return ePAR_ERROR_PARAM;
    }

    if ((true == g_par_store_flash_ee_ctx.cache_valid) &&
        (g_par_store_flash_ee_ctx.cache_base == window_base))
    {
        return ePAR_OK;
    }

    if ((true == g_par_store_flash_ee_ctx.cache_valid) && (true == par_store_flash_ee_has_dirty()))
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_CACHE_DIRTY_WINDOW);
        return ePAR_ERROR_NVM;
    }

    g_par_store_flash_ee_ctx.cache_base = window_base;
    g_par_store_flash_ee_ctx.cache_size = g_par_store_flash_ee_ctx.logical_size - window_base;
    if (g_par_store_flash_ee_ctx.cache_size > PAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE)
    {
        g_par_store_flash_ee_ctx.cache_size = PAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE;
    }

    (void)memset(g_par_store_flash_ee_cache, PAR_STORE_FLASH_EE_ERASE_VALUE, g_par_store_flash_ee_ctx.cache_size);
    for (line = 0u; line < (g_par_store_flash_ee_ctx.cache_size / g_par_store_flash_ee_ctx.line_size); ++line)
    {
        status = par_store_flash_ee_resolve_line_from_bank((window_base / g_par_store_flash_ee_ctx.line_size) + line,
                                                           &g_par_store_flash_ee_cache[line * g_par_store_flash_ee_ctx.line_size]);
        if (ePAR_OK != status)
        {
            return status;
        }
    }

    g_par_store_flash_ee_ctx.cache_valid = true;
    par_store_flash_ee_clear_dirty();
    return ePAR_OK;
}

/**
 * @brief Ensure the requested cache window is ready for staging.
 *
 * @details The backend still keeps only one dirty cache window in RAM at a
 * time. When write or erase processing needs to move staging to another
 * window, this helper synchronizes the current dirty window first and then
 * loads the next window.
 *
 * @param[in] window_base Start address of the required cache window.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_prepare_cache_window(uint32_t window_base)
{
    par_status_t status = ePAR_OK;

    if ((true == g_par_store_flash_ee_ctx.cache_valid) &&
        (g_par_store_flash_ee_ctx.cache_base == window_base))
    {
        return ePAR_OK;
    }

    if ((true == g_par_store_flash_ee_ctx.cache_valid) &&
        (true == par_store_flash_ee_has_dirty()) &&
        (g_par_store_flash_ee_ctx.cache_base != window_base))
    {
        status = par_store_flash_ee_sync();
        if (ePAR_OK != status)
        {
            return status;
        }
    }

    return par_store_flash_ee_load_cache(window_base);
}

/**
 * @brief Commit the currently staged cache window when it is dirty.
 *
 * @details This helper is used at the end of logical write and erase requests
 * so a successful backend call never leaves the final modified cache window in
 * RAM only. The backend may still synchronize earlier windows while processing
 * a multi-window request, but returning @ref ePAR_OK guarantees that the last
 * staged window is also durable in flash. Multi-window requests are still not
 * transactional: an error reported after an earlier window was synchronized
 * does not roll that earlier committed window back.
 *
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_commit_staged_cache(void)
{
    if ((false == g_par_store_flash_ee_ctx.cache_valid) ||
        (false == par_store_flash_ee_has_dirty()))
    {
        return ePAR_OK;
    }

    return par_store_flash_ee_sync();
}

/**
 * @brief Build a compacted checkpoint inside the inactive bank.
 *
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_checkpoint(void)
{
    par_status_t status = ePAR_OK;
    uint32_t line_index = 0u;
    uint32_t next_offset = PAR_STORE_FLASH_EE_HEADER_SIZE;
    uint32_t target_bank = g_par_store_flash_ee_ctx.inactive_bank_base;
    uint8_t line_buf[PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE];

    status = par_store_flash_ee_prepare_bank(target_bank, g_par_store_flash_ee_ctx.active_seq + 1u);
    if (ePAR_OK != status)
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_CHECKPOINT);
        return status;
    }

    for (line_index = 0u; line_index < g_par_store_flash_ee_ctx.line_count; ++line_index)
    {
        status = par_store_flash_ee_resolve_live_line(line_index, line_buf);
        if (ePAR_OK != status)
        {
            par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_CHECKPOINT);
            return status;
        }

        if (true == par_store_flash_ee_is_erased_block(line_buf, g_par_store_flash_ee_ctx.line_size))
        {
            continue;
        }

        status = par_store_flash_ee_append_record(target_bank, &next_offset, line_index, line_buf);
        if (ePAR_OK != status)
        {
            par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_CHECKPOINT);
            return status;
        }
    }

    status = par_store_flash_ee_activate_bank(target_bank);
    if (ePAR_OK != status)
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_CHECKPOINT);
        return status;
    }

    g_par_store_flash_ee_ctx.active_bank_base = target_bank;
    g_par_store_flash_ee_ctx.inactive_bank_base = (0u == target_bank) ? g_par_store_flash_ee_ctx.bank_size : 0u;
    g_par_store_flash_ee_ctx.active_seq += 1u;
    g_par_store_flash_ee_ctx.active_next_offset = next_offset;
    par_store_flash_ee_clear_dirty();
    PAR_WARN_PRINT("PAR_FLASH_EE: checkpoint complete, seq=%lu, next_offset=%lu",
                   (unsigned long)g_par_store_flash_ee_ctx.active_seq,
                   (unsigned long)g_par_store_flash_ee_ctx.active_next_offset);
    return ePAR_OK;
}

/**
 * @brief Ensure the active bank has room for at least one more append record.
 *
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_ensure_capacity(uint32_t needed_records)
{
    if ((g_par_store_flash_ee_ctx.active_next_offset + (needed_records * g_par_store_flash_ee_ctx.record_size)) <=
        g_par_store_flash_ee_ctx.bank_size)
    {
        return ePAR_OK;
    }

    return par_store_flash_ee_checkpoint();
}

/**
 * @brief Initialize the generic flash-emulated EEPROM backend.
 *
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_init(void)
{
    par_status_t status = ePAR_OK;
    par_status_t scan_status0 = ePAR_ERROR_NVM;
    par_status_t scan_status1 = ePAR_ERROR_NVM;
    bool port_is_init = false;
    uint32_t seq0 = 0u;
    uint32_t seq1 = 0u;
    uint32_t next0 = 0u;
    uint32_t next1 = 0u;
    bool valid0 = false;
    bool valid1 = false;
    par_store_flash_ee_diag_t scan_diag0 = ePAR_STORE_FLASH_EE_DIAG_NONE;
    par_store_flash_ee_diag_t scan_diag1 = ePAR_STORE_FLASH_EE_DIAG_NONE;

    if ((false == g_par_store_flash_ee_ctx.is_bound) || (NULL == g_par_store_flash_ee_ctx.p_port_api))
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_PORT_NOT_BOUND);
        return ePAR_ERROR_INIT;
    }

    if (true == g_par_store_flash_ee_ctx.is_init)
    {
        return ePAR_OK;
    }

    status = g_par_store_flash_ee_ctx.p_port_api->init(g_par_store_flash_ee_ctx.p_port_ctx);
    if (ePAR_OK != status)
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_PORT_INIT);
        return ePAR_ERROR_INIT;
    }

    g_par_store_flash_ee_ctx.p_port_api->is_init(g_par_store_flash_ee_ctx.p_port_ctx, &port_is_init);
    if (false == port_is_init)
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_PORT_INIT);
        status = ePAR_ERROR_INIT;
        goto fail_after_port_init;
    }

    g_par_store_flash_ee_ctx.region_size = g_par_store_flash_ee_ctx.p_port_api->get_region_size(g_par_store_flash_ee_ctx.p_port_ctx);
    g_par_store_flash_ee_ctx.erase_size = g_par_store_flash_ee_ctx.p_port_api->get_erase_size(g_par_store_flash_ee_ctx.p_port_ctx);
    g_par_store_flash_ee_ctx.program_size = g_par_store_flash_ee_ctx.p_port_api->get_program_size(g_par_store_flash_ee_ctx.p_port_ctx);

    status = par_store_flash_ee_validate_geometry();
    if (ePAR_OK != status)
    {
        goto fail_after_port_init;
    }

    par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_NONE);
    scan_status0 = par_store_flash_ee_scan_bank(par_store_flash_ee_bank_base(0u), &seq0, &next0);
    scan_diag0 = g_par_store_flash_ee_ctx.diag;
    if (ePAR_OK == scan_status0)
    {
        valid0 = true;
    }

    par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_NONE);
    scan_status1 = par_store_flash_ee_scan_bank(par_store_flash_ee_bank_base(1u), &seq1, &next1);
    scan_diag1 = g_par_store_flash_ee_ctx.diag;
    if (ePAR_OK == scan_status1)
    {
        valid1 = true;
    }

    if ((false == valid0) && (false == valid1))
    {
        if ((false == par_store_flash_ee_scan_diag_allows_reformat(scan_diag0)) ||
            (false == par_store_flash_ee_scan_diag_allows_reformat(scan_diag1)))
        {
            par_store_flash_ee_set_diag((false == par_store_flash_ee_scan_diag_allows_reformat(scan_diag0)) ? scan_diag0 : scan_diag1);
            PAR_ERR_PRINT("PAR_FLASH_EE: refusing auto-format after fatal bank scan failure, bank0_diag=%u bank1_diag=%u",
                          (unsigned)scan_diag0,
                          (unsigned)scan_diag1);
            status = ePAR_ERROR_INIT;
            goto fail_after_port_init;
        }

        status = par_store_flash_ee_prepare_bank(par_store_flash_ee_bank_base(0u), 1u);
        if (ePAR_OK != status)
        {
            goto fail_after_port_init;
        }

        status = par_store_flash_ee_activate_bank(par_store_flash_ee_bank_base(0u));
        if (ePAR_OK != status)
        {
            goto fail_after_port_init;
        }

        g_par_store_flash_ee_ctx.active_bank_base = par_store_flash_ee_bank_base(0u);
        g_par_store_flash_ee_ctx.inactive_bank_base = par_store_flash_ee_bank_base(1u);
        g_par_store_flash_ee_ctx.active_seq = 1u;
        g_par_store_flash_ee_ctx.active_next_offset = PAR_STORE_FLASH_EE_HEADER_SIZE;
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_NONE);
        PAR_WARN_PRINT("PAR_FLASH_EE: formatted empty persistence banks");
    }
    else if ((true == valid0) && ((false == valid1) || (seq0 >= seq1)))
    {
        g_par_store_flash_ee_ctx.active_bank_base = par_store_flash_ee_bank_base(0u);
        g_par_store_flash_ee_ctx.inactive_bank_base = par_store_flash_ee_bank_base(1u);
        g_par_store_flash_ee_ctx.active_seq = seq0;
        g_par_store_flash_ee_ctx.active_next_offset = next0;
        par_store_flash_ee_set_diag(scan_diag0);
    }
    else
    {
        g_par_store_flash_ee_ctx.active_bank_base = par_store_flash_ee_bank_base(1u);
        g_par_store_flash_ee_ctx.inactive_bank_base = par_store_flash_ee_bank_base(0u);
        g_par_store_flash_ee_ctx.active_seq = seq1;
        g_par_store_flash_ee_ctx.active_next_offset = next1;
        par_store_flash_ee_set_diag(scan_diag1);
    }

    g_par_store_flash_ee_ctx.cache_valid = false;
    g_par_store_flash_ee_ctx.is_init = true;
    par_store_flash_ee_clear_dirty();
    PAR_DBG_PRINT("PAR_FLASH_EE: init ok, port=%s, bank_size=%lu, logical_size=%lu, line_size=%lu",
                  (NULL != g_par_store_flash_ee_ctx.p_port_api->get_name)
                      ? g_par_store_flash_ee_ctx.p_port_api->get_name(g_par_store_flash_ee_ctx.p_port_ctx)
                      : "unknown",
                  (unsigned long)g_par_store_flash_ee_ctx.bank_size,
                  (unsigned long)g_par_store_flash_ee_ctx.logical_size,
                  (unsigned long)g_par_store_flash_ee_ctx.line_size);
    return ePAR_OK;

fail_after_port_init:
    par_store_flash_ee_cleanup_failed_init();
    return status;
}

/**
 * @brief Deinitialize the generic flash-emulated EEPROM backend.
 *
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_deinit(void)
{
    par_status_t status = ePAR_OK;

    if ((false == g_par_store_flash_ee_ctx.is_init) || (NULL == g_par_store_flash_ee_ctx.p_port_api))
    {
        return ePAR_OK;
    }

    status = par_store_flash_ee_sync();
    if (ePAR_OK != status)
    {
        return status;
    }

    status = g_par_store_flash_ee_ctx.p_port_api->deinit(g_par_store_flash_ee_ctx.p_port_ctx);
    if (ePAR_OK != status)
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_PORT_INIT);
        return ePAR_ERROR;
    }

    g_par_store_flash_ee_ctx.is_init = false;
    g_par_store_flash_ee_ctx.cache_valid = false;
    return par_store_flash_ee_complete_status(ePAR_OK);
}

/**
 * @brief Report whether the generic backend is initialized.
 *
 * @param[out] p_is_init Receives the initialization state.
 */
static void par_store_flash_ee_is_init(bool *p_is_init)
{
    if (NULL == p_is_init)
    {
        return;
    }

    *p_is_init = g_par_store_flash_ee_ctx.is_init;
}

/**
 * @brief Read bytes from the flash-emulated EEPROM logical address space.
 *
 * @details The read path is cache-aware. The backend calculates the cache
 * window that covers the requested address range, loads that window on demand,
 * and then copies bytes from the RAM cache. If another dirty window is already
 * staged, reads fall back to the live-view helper so data can be reconstructed
 * without committing pending updates.
 *
 * @param[in] addr Start address inside the logical address space.
 * @param[in] size Number of bytes to read.
 * @param[out] p_buf Destination buffer.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_read(const uint32_t addr, const uint32_t size, uint8_t *p_buf)
{
    par_status_t status = ePAR_OK;
    uint32_t remaining = size;
    uint32_t cur_addr = addr;
    uint8_t *p_dst = p_buf;

    if ((NULL == p_buf) || (false == par_store_flash_ee_range_is_valid(addr, size)))
    {
        return ePAR_ERROR_PARAM;
    }

    if (false == g_par_store_flash_ee_ctx.is_init)
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_NOT_INIT);
        return ePAR_ERROR_INIT;
    }

    while (remaining > 0u)
    {
        uint32_t window_base = par_store_flash_ee_window_base(cur_addr);
        uint32_t window_offset = cur_addr - window_base;
        uint32_t chunk = 0u;

        if ((false == g_par_store_flash_ee_ctx.cache_valid) || (g_par_store_flash_ee_ctx.cache_base != window_base))
        {
            if ((true == g_par_store_flash_ee_ctx.cache_valid) &&
                (true == par_store_flash_ee_has_dirty()) &&
                (g_par_store_flash_ee_ctx.cache_base != window_base))
            {
                chunk = g_par_store_flash_ee_ctx.logical_size - window_base;
                if (chunk > PAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE)
                {
                    chunk = PAR_CFG_NVM_BACKEND_FLASH_EE_CACHE_SIZE;
                }
                chunk -= window_offset;
                if (chunk > remaining)
                {
                    chunk = remaining;
                }

                status = par_store_flash_ee_read_live_range(cur_addr, chunk, p_dst);
                if (ePAR_OK != status)
                {
                    return status;
                }

                remaining -= chunk;
                cur_addr += chunk;
                p_dst += chunk;
                continue;
            }

            status = par_store_flash_ee_load_cache(window_base);
            if (ePAR_OK != status)
            {
                return status;
            }
        }

        chunk = g_par_store_flash_ee_ctx.cache_size - window_offset;
        if (chunk > remaining)
        {
            chunk = remaining;
        }

        (void)memcpy(p_dst, &g_par_store_flash_ee_cache[window_offset], chunk);
        remaining -= chunk;
        cur_addr += chunk;
        p_dst += chunk;
    }

    return par_store_flash_ee_complete_status(ePAR_OK);
}

/**
 * @brief Write bytes into the flash-emulated EEPROM logical address space.
 *
 * @details The write path never programs flash directly. It loads the cache
 * window that covers the target logical address, updates the bytes in RAM, and
 * marks all overlapped cache lines dirty. The modified lines are later turned
 * into append records during sync. Requests may span multiple cache windows.
 * Before staging bytes in a new window, the backend synchronizes the current
 * dirty window and then continues with the next chunk. Before returning
 * success, the backend also synchronizes the final dirty window so the full
 * completed request is durable in flash. Requests that span multiple cache
 * windows are not transactional across those windows.
 *
 * @param[in] addr Start address inside the logical address space.
 * @param[in] size Number of bytes to write.
 * @param[in] p_buf Source buffer.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_write(const uint32_t addr, const uint32_t size, const uint8_t *p_buf)
{
    par_status_t status = ePAR_OK;
    uint32_t remaining = size;
    uint32_t cur_addr = addr;
    const uint8_t *p_src = p_buf;

    if ((NULL == p_buf) || (false == par_store_flash_ee_range_is_valid(addr, size)))
    {
        return ePAR_ERROR_PARAM;
    }

    if (false == g_par_store_flash_ee_ctx.is_init)
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_NOT_INIT);
        return ePAR_ERROR_INIT;
    }

    while (remaining > 0u)
    {
        uint32_t window_base = par_store_flash_ee_window_base(cur_addr);
        uint32_t window_offset = cur_addr - window_base;
        uint32_t chunk = 0u;
        uint32_t first_line = 0u;
        uint32_t last_line = 0u;
        uint32_t line = 0u;

        status = par_store_flash_ee_prepare_cache_window(window_base);
        if (ePAR_OK != status)
        {
            return status;
        }

        chunk = g_par_store_flash_ee_ctx.cache_size - window_offset;
        if (chunk > remaining)
        {
            chunk = remaining;
        }

        (void)memcpy(&g_par_store_flash_ee_cache[window_offset], p_src, chunk);

        first_line = window_offset / g_par_store_flash_ee_ctx.line_size;
        last_line = (window_offset + chunk - 1u) / g_par_store_flash_ee_ctx.line_size;
        for (line = first_line; line <= last_line; ++line)
        {
            par_store_flash_ee_mark_dirty(line);
        }

        remaining -= chunk;
        cur_addr += chunk;
        p_src += chunk;
    }

    status = par_store_flash_ee_commit_staged_cache();
    return par_store_flash_ee_complete_status(status);
}

/**
 * @brief Erase bytes inside the flash-emulated EEPROM logical address space.
 *
 * @details The logical erase path mirrors the write path. The target bytes in
 * the RAM cache are set to the erased value and all overlapped cache lines are
 * marked dirty so sync can append their new erased image later. Requests may
 * span multiple cache windows. Before staging bytes in a new window, the
 * backend synchronizes the current dirty window and then continues with the
 * next chunk. Before returning success, the backend also synchronizes the
 * final dirty window so the full completed request is durable in flash.
 * Requests that span multiple cache windows are not transactional across
 * those windows.
 *
 * @param[in] addr Start address inside the logical address space.
 * @param[in] size Number of bytes to erase.
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_erase(const uint32_t addr, const uint32_t size)
{
    par_status_t status = ePAR_OK;
    uint32_t remaining = size;
    uint32_t cur_addr = addr;

    if (false == par_store_flash_ee_range_is_valid(addr, size))
    {
        return ePAR_ERROR_PARAM;
    }

    if (false == g_par_store_flash_ee_ctx.is_init)
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_NOT_INIT);
        return ePAR_ERROR_INIT;
    }

    while (remaining > 0u)
    {
        uint32_t window_base = par_store_flash_ee_window_base(cur_addr);
        uint32_t window_offset = cur_addr - window_base;
        uint32_t chunk = 0u;
        uint32_t first_line = 0u;
        uint32_t last_line = 0u;
        uint32_t line = 0u;

        status = par_store_flash_ee_prepare_cache_window(window_base);
        if (ePAR_OK != status)
        {
            return status;
        }

        chunk = g_par_store_flash_ee_ctx.cache_size - window_offset;
        if (chunk > remaining)
        {
            chunk = remaining;
        }

        (void)memset(&g_par_store_flash_ee_cache[window_offset], PAR_STORE_FLASH_EE_ERASE_VALUE, chunk);

        first_line = window_offset / g_par_store_flash_ee_ctx.line_size;
        last_line = (window_offset + chunk - 1u) / g_par_store_flash_ee_ctx.line_size;
        for (line = first_line; line <= last_line; ++line)
        {
            par_store_flash_ee_mark_dirty(line);
        }

        remaining -= chunk;
        cur_addr += chunk;
    }

    status = par_store_flash_ee_commit_staged_cache();
    return par_store_flash_ee_complete_status(status);
}

/**
 * @brief Flush staged cache data into physical flash.
 *
 * @details Sync first estimates how many append records are needed for the
 * current dirty lines. If the active bank does not have enough free space, the
 * backend performs a checkpoint into the inactive bank. After capacity is
 * guaranteed, each dirty cache line is appended as one new record.
 *
 * @return ePAR_OK on success, otherwise an error code.
 */
static par_status_t par_store_flash_ee_sync(void)
{
    par_status_t status = ePAR_OK;

    if (false == g_par_store_flash_ee_ctx.is_init)
    {
        par_store_flash_ee_set_diag(ePAR_STORE_FLASH_EE_DIAG_NOT_INIT);
        return ePAR_ERROR_INIT;
    }

    status = par_store_flash_ee_ensure_capacity(par_store_flash_ee_count_dirty());
    if (ePAR_OK != status)
    {
        return status;
    }

    status = par_store_flash_ee_flush_cache();
    if (ePAR_OK != status)
    {
        if (ePAR_OK == par_store_flash_ee_checkpoint())
        {
            status = par_store_flash_ee_flush_cache();
        }
    }

    return par_store_flash_ee_complete_status(status);
}

/**
 * @brief Expose the backend operations to the persistence framework.
 */
static const par_store_backend_api_t g_par_store_flash_ee_api = {
    .init = par_store_flash_ee_init,
    .deinit = par_store_flash_ee_deinit,
    .is_init = par_store_flash_ee_is_init,
    .read = par_store_flash_ee_read,
    .write = par_store_flash_ee_write,
    .erase = par_store_flash_ee_erase,
    .sync = par_store_flash_ee_sync,
    .name = "flash_ee",
};

/**
 * @brief Bind one physical flash port to the backend core.
 *
 * @param[in] p_port_api Physical flash port API table.
 * @param[in,out] p_port_ctx Mutable port context passed back to the adapter.
 * @return ePAR_OK on success, otherwise an error code.
 */
par_status_t par_store_backend_flash_ee_bind_port(const par_store_flash_ee_port_api_t *p_port_api, void *p_port_ctx)
{
    if ((NULL == p_port_api) || (NULL == p_port_api->init) || (NULL == p_port_api->deinit) ||
        (NULL == p_port_api->is_init) || (NULL == p_port_api->read) || (NULL == p_port_api->program) ||
        (NULL == p_port_api->erase) || (NULL == p_port_api->get_region_size) ||
        (NULL == p_port_api->get_erase_size) || (NULL == p_port_api->get_program_size))
    {
        return ePAR_ERROR_PARAM;
    }

    if ((true == g_par_store_flash_ee_ctx.is_bound) &&
        (g_par_store_flash_ee_ctx.p_port_api == p_port_api) &&
        (g_par_store_flash_ee_ctx.p_port_ctx == p_port_ctx))
    {
        return ePAR_OK;
    }

    g_par_store_flash_ee_ctx.p_port_api = p_port_api;
    g_par_store_flash_ee_ctx.p_port_ctx = p_port_ctx;
    g_par_store_flash_ee_ctx.is_bound = true;
    g_par_store_flash_ee_ctx.is_init = false;
    g_par_store_flash_ee_ctx.cache_valid = false;
    par_store_flash_ee_clear_dirty();
    return ePAR_OK;
}

/**
 * @brief Return the generic backend API table.
 *
 * @return Pointer to the backend API table.
 */
const par_store_backend_api_t *par_store_backend_flash_ee_get_api(void)
{
    return &g_par_store_flash_ee_api;
}

/**
 * @brief Return the latest backend diagnostic code.
 *
 * @return Latest backend diagnostic code.
 */
par_store_flash_ee_diag_t par_store_backend_flash_ee_get_diag(void)
{
    return g_par_store_flash_ee_ctx.diag;
}

/**
 * @brief Return the diagnostic string for one backend diagnostic code.
 *
 * @param[in] diag Diagnostic code to translate.
 * @return Constant string representation.
 */
const char *par_store_backend_flash_ee_get_diag_str(par_store_flash_ee_diag_t diag)
{
    switch (diag)
    {
        case ePAR_STORE_FLASH_EE_DIAG_NONE: return "none";
        case ePAR_STORE_FLASH_EE_DIAG_PORT_NOT_BOUND: return "port_not_bound";
        case ePAR_STORE_FLASH_EE_DIAG_PORT_INIT: return "port_init";
        case ePAR_STORE_FLASH_EE_DIAG_PORT_READ: return "port_read";
        case ePAR_STORE_FLASH_EE_DIAG_PORT_PROGRAM: return "port_program";
        case ePAR_STORE_FLASH_EE_DIAG_PORT_ERASE: return "port_erase";
        case ePAR_STORE_FLASH_EE_DIAG_PORT_GEOMETRY: return "port_geometry";
        case ePAR_STORE_FLASH_EE_DIAG_CONFIG: return "config";
        case ePAR_STORE_FLASH_EE_DIAG_BANK_ERASED: return "bank_erased";
        case ePAR_STORE_FLASH_EE_DIAG_HEADER_MAGIC: return "header_magic";
        case ePAR_STORE_FLASH_EE_DIAG_HEADER_VERSION: return "header_version";
        case ePAR_STORE_FLASH_EE_DIAG_HEADER_STATE: return "header_state";
        case ePAR_STORE_FLASH_EE_DIAG_HEADER_CRC: return "header_crc";
        case ePAR_STORE_FLASH_EE_DIAG_HEADER_LAYOUT: return "header_layout";
        case ePAR_STORE_FLASH_EE_DIAG_RECORD_MAGIC: return "record_commit";
        case ePAR_STORE_FLASH_EE_DIAG_RECORD_RANGE: return "record_range";
        case ePAR_STORE_FLASH_EE_DIAG_RECORD_CRC: return "record_crc";
        case ePAR_STORE_FLASH_EE_DIAG_RECORD_TAIL: return "record_tail";
        case ePAR_STORE_FLASH_EE_DIAG_CACHE_WINDOW: return "cache_window";
        case ePAR_STORE_FLASH_EE_DIAG_CACHE_DIRTY_WINDOW: return "cache_dirty_window";
        case ePAR_STORE_FLASH_EE_DIAG_CAPACITY: return "capacity";
        case ePAR_STORE_FLASH_EE_DIAG_CHECKPOINT: return "checkpoint";
        case ePAR_STORE_FLASH_EE_DIAG_NOT_INIT: return "not_init";
        default: return "unknown";
    }
}

#endif /* (1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_NVM_BACKEND_FLASH_EE_EN) */
