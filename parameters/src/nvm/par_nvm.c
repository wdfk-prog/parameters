/**
 * @file par_nvm.c
 * @brief Implement non-volatile storage support for parameters.
 * @author Ziga Miklosic
 * @version V3.0.1
 * @date 2026-01-29
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-03-30 1.2     wdfk-prog     simplify NVM flow and table-ID handling
 */
/**
 * @addtogroup PAR_NVM
 * @{ <!-- BEGIN GROUP -->
 *
 * Parameter storage to non-volatile memory handling.
 *
 *
 * @pre      NVM module shall have memory region called "Parameters".
 *
 * NVM module may already be initialized by the application, or it.
 * may be initialized on demand by this module when needed.
 *
 * @brief This module is responsible for parameter NVM object creation and.
 * storage manipulation. NVM parameter object consist of it's value.
 * and a CRC value for validation purposes.
 *
 * Parameter storage is reserved in "Parameters" region of NVM. Look.
 * at the nvm_cfg.h/c module for NVM region descriptions.
 *
 * Parameters stored into NVM in the native byte order of the target platform.
 *
 * For details how parameters are handled in NVM go look at the.
 * documentation.
 *
 * @note RULES OF "PAR_CFG_TABLE_ID_CHECK_EN" SETTINGS:
 *
 * During development it is normal that the parameter table and the persisted
 * compatibility model evolve. The serialized NVM header therefore stores the
 * current table-ID digest together with the persistent-object count, and the
 * header CRC protects both fields. Startup validates the live table-ID against
 * the stored digest when table-ID checking is enabled.
 *
 * A mismatch means the managed NVM image is no longer compatible with the live
 * firmware image. Typical causes are an intentional schema-version bump,
 * stored-prefix layout drift, or stored-image corruption. Self-describing
 * layouts treat persisted-parameter ID changes as incompatible. The fixed
 * payload-only layout intentionally allows pure external-ID renumbering and
 * compatible tail growth, while the grouped payload-only layout rebuilds on
 * any stored/live count mismatch.
 * The recovery path is centralized in par_nvm_init(): restore defaults first,
 * then rebuild the managed NVM image only for errors that require a rewrite.
 */
/**
 * @brief Include dependencies.
 */
#include "par_nvm.h"
#include "par_cfg.h"
#include "par_if.h"

#if (1 == PAR_CFG_NVM_EN)

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "par_store_backend.h"
#include "par_nvm_layout.h"
#include "par_nvm_scalar_store.h"
#include "par_nvm_table_id.h"
#include "par_nvm_object.h"
#include "par_nvm_object_store.h"
#include "par_object.h"
/**
 * @brief Compile-time definitions.
 */
/**
 * @brief Parameter signature value.
 */
#define PAR_NVM_SIGN (0xFF00AA55)

/**
 * @brief Parameter NVM header content address start.
 *
 * @note This is offset to reserved NVM region. For absolute address.
 * add that value to NVM start region.
 */
#define PAR_NVM_HEAD_ADDR            (0x00U)
#define PAR_NVM_HEAD_SIGN_ADDR       (PAR_NVM_HEAD_ADDR)
#define PAR_NVM_HEAD_SIGN_SIZE       ((uint32_t)sizeof(uint32_t))
#define PAR_NVM_HEAD_OBJ_NB_OFFSET   (PAR_NVM_HEAD_SIGN_SIZE)
#define PAR_NVM_HEAD_OBJ_NB_SIZE     ((uint32_t)sizeof(uint16_t))
#define PAR_NVM_HEAD_TABLE_ID_OFFSET (PAR_NVM_HEAD_OBJ_NB_OFFSET + PAR_NVM_HEAD_OBJ_NB_SIZE)
#define PAR_NVM_HEAD_TABLE_ID_SIZE   ((uint32_t)sizeof(uint32_t))
#define PAR_NVM_HEAD_CRC_OFFSET      (PAR_NVM_HEAD_TABLE_ID_OFFSET + PAR_NVM_HEAD_TABLE_ID_SIZE)
#define PAR_NVM_HEAD_CRC_SIZE        ((uint32_t)sizeof(uint16_t))
#define PAR_NVM_HEAD_SIZE            (PAR_NVM_HEAD_CRC_OFFSET + PAR_NVM_HEAD_CRC_SIZE)
#define PAR_NVM_FIRST_DATA_OBJ_ADDR  (PAR_NVM_HEAD_ADDR + PAR_NVM_HEAD_SIZE)

/**
 * @brief Runtime persistence-slot state.
 *
 * @details The common NVM flow uses this structure to track which compiled
 * persistent slots were reconstructed successfully from the serialized image.
 */
typedef struct
{
    bool loaded_slots[PAR_PERSIST_SLOT_MAP_CAPACITY]; /**< Runtime-loaded flag for each compiled persistent slot. */
    uint16_t loaded_count;                            /**< Number of runtime-loaded persistent slots. */
} par_nvm_slot_runtime_t;

#if (1 == PAR_CFG_ENABLE_NAME)
#define PAR_NVM_DBG_NAME_ARG(cfg_) (((const par_cfg_t *)(cfg_) != NULL) && (((const par_cfg_t *)(cfg_))->name != NULL) ? ((const par_cfg_t *)(cfg_))->name : "")
#else
#define PAR_NVM_DBG_NAME_ARG(cfg_) ""
#endif /* (1 == PAR_CFG_ENABLE_NAME) */

#if (1 == PAR_CFG_ENABLE_ID)
#define PAR_NVM_CFG_ID_VALUE(cfg_) (((const par_cfg_t *)(cfg_)) != NULL ? ((const par_cfg_t *)(cfg_))->id : 0U)
#else
#define PAR_NVM_CFG_ID_VALUE(cfg_) (0U)
#endif /* (1 == PAR_CFG_ENABLE_ID) */

#if (1 == PAR_CFG_NVM_SCALAR_EN)
/**
 * @brief Internal switch for scalar persisted-record support.
 */
#define PAR_NVM_SCALAR_STORE_ENABLED (1)
/**
 * @brief Number of scalar persisted records compiled into this build.
 */
#define PAR_NVM_SCALAR_RECORD_COUNT ((uint16_t)PAR_PERSISTENT_COMPILE_COUNT)
#else
/**
 * @brief Internal switch for scalar persisted-record support.
 */
#define PAR_NVM_SCALAR_STORE_ENABLED (0)
/**
 * @brief Number of scalar persisted records compiled into this build.
 */
#define PAR_NVM_SCALAR_RECORD_COUNT  (0U)
#endif /* (1 == PAR_CFG_NVM_SCALAR_EN) */

#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
/**
 * @brief Internal switch for object persistence block support.
 */
#define PAR_NVM_OBJECT_STORE_ENABLED (1)
#else
/**
 * @brief Internal switch for object persistence block support.
 */
#define PAR_NVM_OBJECT_STORE_ENABLED (0)
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

#define PAR_PERSIST_SLOT_ENTRY_SELECT(enum_, pers_)   PAR_PERSIST_SLOT_ENTRY_SELECT_I(enum_, pers_)
#define PAR_PERSIST_SLOT_ENTRY_SELECT_I(enum_, pers_) PAR_PERSIST_SLOT_ENTRY_SELECT_##pers_(enum_)
#define PAR_PERSIST_SLOT_ENTRY_SELECT_true(enum_)     [PAR_PERSIST_IDX_##enum_] = enum_,
#define PAR_PERSIST_SLOT_ENTRY_SELECT_false(enum_)
#define PAR_PERSIST_SLOT_ENTRY_SELECT_1(enum_) [PAR_PERSIST_IDX_##enum_] = enum_,
#define PAR_PERSIST_SLOT_ENTRY_SELECT_0(enum_)
#define PAR_ITEM_PERSIST_SLOT(...) PAR_PERSIST_SLOT_ENTRY_SELECT(PAR_XARG_ENUM(__VA_ARGS__), PAR_XARG_PERS(__VA_ARGS__))
#define PAR_ITEM_NOP(...)
/**
 * @brief Compile-time mapping from persistent slot to live parameter number.
 *
 * @details This table is derived directly from par_table.def. Slot order is the
 * single source of truth for the managed NVM payload layout used by par_nvm.c.
 */
static const par_num_t g_par_persist_slot_to_par_num[PAR_PERSIST_SLOT_MAP_CAPACITY] = {
#define PAR_ITEM_U8      PAR_ITEM_PERSIST_SLOT
#define PAR_ITEM_U16     PAR_ITEM_PERSIST_SLOT
#define PAR_ITEM_U32     PAR_ITEM_PERSIST_SLOT
#define PAR_ITEM_I8      PAR_ITEM_PERSIST_SLOT
#define PAR_ITEM_I16     PAR_ITEM_PERSIST_SLOT
#define PAR_ITEM_I32     PAR_ITEM_PERSIST_SLOT
#define PAR_ITEM_F32     PAR_ITEM_PERSIST_SLOT
#define PAR_ITEM_STR     PAR_ITEM_NOP
#define PAR_ITEM_BYTES   PAR_ITEM_NOP
#define PAR_ITEM_ARR_U8  PAR_ITEM_NOP
#define PAR_ITEM_ARR_U16 PAR_ITEM_NOP
#define PAR_ITEM_ARR_U32 PAR_ITEM_NOP
#include "par_table.def"
#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32
#undef PAR_ITEM_STR
#undef PAR_ITEM_BYTES
#undef PAR_ITEM_ARR_U8
#undef PAR_ITEM_ARR_U16
#undef PAR_ITEM_ARR_U32
};
#undef PAR_ITEM_PERSIST_SLOT
#undef PAR_ITEM_NOP
#undef PAR_PERSIST_SLOT_ENTRY_SELECT
#undef PAR_PERSIST_SLOT_ENTRY_SELECT_I
#undef PAR_PERSIST_SLOT_ENTRY_SELECT_true
#undef PAR_PERSIST_SLOT_ENTRY_SELECT_false
#undef PAR_PERSIST_SLOT_ENTRY_SELECT_1
#undef PAR_PERSIST_SLOT_ENTRY_SELECT_0


/**
 * @brief Module-scope variables.
 */
/**
 * @brief Initialization guard.
 */
static bool gb_is_init = false;
/**
 * @brief Ownership guard for the mounted storage backend.
 */
static bool gb_is_nvm_owner = false;
/**
 * @brief Selected parameter storage backend API.
 *
 * @details The backend is resolved once during initialization. The core NVM
 * logic then uses only this abstract interface and no longer depends on a
 * specific repository layout.
 */
static const par_store_backend_api_t *gp_store = NULL;
/**
 * @brief Selected persisted-record layout adapter.
 */
static const par_nvm_layout_api_t *gp_layout = NULL;
/**
 * @brief Runtime state of compiled persistent slots.
 */
static par_nvm_slot_runtime_t g_par_nvm_slot_runtime = { 0 };
/**
 * @brief Calculate serialized-header CRC-16.
 *
 * @details The header CRC is accumulated field-by-field over the serialized
 * native-order bytes of obj_nb and table_id. This avoids hashing compiler
 * padding bytes inside par_nvm_head_obj_t while intentionally keeping the
 * persisted format and table-ID comparison tied to the current target
 * architecture.
 *
 * @param p_head_obj Pointer to header object.
 * @return Calculated CRC-16 value.
 */
static uint16_t par_nvm_calc_head_crc(const par_nvm_head_obj_t * const p_head_obj)
{
    uint16_t crc = PAR_IF_CRC16_INIT;

    PAR_ASSERT(NULL != p_head_obj);

    crc = par_if_crc16_accumulate(crc, (const uint8_t * const)&p_head_obj->obj_nb, (uint32_t)sizeof(p_head_obj->obj_nb));
    crc = par_if_crc16_accumulate(crc, (const uint8_t * const)&p_head_obj->table_id, (uint32_t)sizeof(p_head_obj->table_id));

    return crc;
}
/**
 * @brief Read parameter NVM header.
 *
 * @param p_head_obj Pointer to parameter NVM header.
 * @return Status of operation.
 */
static par_status_t par_nvm_read_header(par_nvm_head_obj_t * const p_head_obj)
{
    par_status_t status = ePAR_OK;
    uint8_t head_buf[PAR_NVM_HEAD_SIZE] = { 0U };

    PAR_ASSERT(NULL != p_head_obj);

    const par_status_t store_status = gp_store->read(PAR_NVM_HEAD_ADDR, PAR_NVM_HEAD_SIZE, head_buf);
    if (ePAR_OK != store_status)
    {
        status = ePAR_ERROR_NVM;
        PAR_ERR_PRINT("PAR_NVM: header read failed, err=%u", (unsigned)store_status);
    }
    else
    {
        memcpy(&p_head_obj->sign, &head_buf[PAR_NVM_HEAD_SIGN_ADDR], sizeof(p_head_obj->sign));
        memcpy(&p_head_obj->obj_nb, &head_buf[PAR_NVM_HEAD_OBJ_NB_OFFSET], sizeof(p_head_obj->obj_nb));
        memcpy(&p_head_obj->table_id, &head_buf[PAR_NVM_HEAD_TABLE_ID_OFFSET], sizeof(p_head_obj->table_id));
        memcpy(&p_head_obj->crc, &head_buf[PAR_NVM_HEAD_CRC_OFFSET], sizeof(p_head_obj->crc));
    }

    return status;
}

/**
 * @brief Flush pending backend data and normalize sync failures.
 *
 * @param p_context Short log context for diagnostics.
 * @return Operation status.
 */
static par_status_t par_nvm_sync_backend(const char * const p_context)
{
    (void)p_context;
    const par_status_t sync_status = gp_store->sync();

    if (ePAR_OK != sync_status)
    {
        PAR_ERR_PRINT("PAR_NVM: sync failed%s%s err=%u",
                      (NULL != p_context) ? " " : "",
                      (NULL != p_context) ? p_context : "",
                      (unsigned)sync_status);
        return ePAR_ERROR_NVM;
    }

    return ePAR_OK;
}

#if (1 == PAR_CFG_NVM_WRITE_VERIFY_EN)
/**
 * @brief Verify a freshly written header by reading it back.
 *
 * @param p_expected Expected serialized header fields.
 * @return Operation status.
 */
static par_status_t par_nvm_verify_header_readback(const par_nvm_head_obj_t * const p_expected)
{
    par_nvm_head_obj_t readback = { 0 };
    par_status_t status = ePAR_OK;

    PAR_ASSERT(NULL != p_expected);

    status = par_nvm_read_header(&readback);
    if (ePAR_OK != status)
    {
        return status;
    }

    if ((readback.sign != p_expected->sign) ||
        (readback.obj_nb != p_expected->obj_nb) ||
        (readback.table_id != p_expected->table_id) ||
        (readback.crc != p_expected->crc))
    {
        PAR_ERR_PRINT("PAR_NVM: header readback mismatch exp(sign=0x%08lX,obj=%u,table=0x%08lX,crc=0x%04X) got(sign=0x%08lX,obj=%u,table=0x%08lX,crc=0x%04X)",
                      (unsigned long)p_expected->sign,
                      (unsigned)p_expected->obj_nb,
                      (unsigned long)p_expected->table_id,
                      (unsigned)p_expected->crc,
                      (unsigned long)readback.sign,
                      (unsigned)readback.obj_nb,
                      (unsigned long)readback.table_id,
                      (unsigned)readback.crc);
        return ePAR_ERROR_NVM;
    }

    if (par_nvm_calc_head_crc(&readback) != readback.crc)
    {
        PAR_ERR_PRINT("PAR_NVM: header readback CRC validation failed");
        return (par_status_t)(ePAR_ERROR_NVM | ePAR_ERROR_CRC);
    }

    return ePAR_OK;
}

/**
 * @brief Verify one freshly written persisted record by reading it back.
 *
 * @param addr Record start address.
 * @param par_num Live parameter number.
 * @param p_expected Expected canonical object.
 * @return Operation status.
 */
static par_status_t par_nvm_verify_record_readback(const uint32_t addr,
                                                   const par_num_t par_num,
                                                   const par_nvm_data_obj_t * const p_expected)
{
    par_nvm_data_obj_t readback = { 0 };
    par_status_t status = ePAR_OK;

    PAR_ASSERT(NULL != p_expected);

    status = gp_layout->read(gp_store, addr, par_num, &readback);
    if (ePAR_OK != status)
    {
        PAR_ERR_PRINT("PAR_NVM: record readback failed, par_num=%u addr=0x%08lX err=%u",
                      (unsigned)par_num,
                      (unsigned long)addr,
                      (unsigned)status);
        return (par_status_t)(ePAR_ERROR_NVM | (status & ePAR_ERROR_CRC));
    }

    if (false == gp_layout->data_obj_matches(par_num, p_expected, &readback))
    {
        PAR_ERR_PRINT("PAR_NVM: record readback mismatch, par_num=%u addr=0x%08lX",
                      (unsigned)par_num,
                      (unsigned long)addr);
        return ePAR_ERROR_NVM;
    }

    return ePAR_OK;
}
#endif /* (1 == PAR_CFG_NVM_WRITE_VERIFY_EN) */
/**
 * @brief Write parameter NVM header.
 *
 * @details The serialized header always stores the current table-ID digest
 * in the native byte order of the running target.
 * The compatibility comparison against the live digest still runs only when
 * table-ID checking is enabled.
 *
 * @param num_of_par Number of persistent parameters that are stored in NVM.
 * @return Status of operation.
 */
static par_status_t par_nvm_write_header(const uint16_t num_of_par)
{
    par_status_t status = ePAR_OK;
    par_nvm_head_obj_t head_obj = { 0 };
    uint8_t head_buf[PAR_NVM_HEAD_SIZE] = { 0U };

    head_obj.obj_nb = num_of_par;
    head_obj.table_id = par_nvm_table_id_calc_for_count(num_of_par);
    head_obj.crc = par_nvm_calc_head_crc(&head_obj);
    head_obj.sign = PAR_NVM_SIGN;

    memcpy(&head_buf[PAR_NVM_HEAD_SIGN_ADDR], &head_obj.sign, sizeof(head_obj.sign));
    memcpy(&head_buf[PAR_NVM_HEAD_OBJ_NB_OFFSET], &head_obj.obj_nb, sizeof(head_obj.obj_nb));
    memcpy(&head_buf[PAR_NVM_HEAD_TABLE_ID_OFFSET], &head_obj.table_id, sizeof(head_obj.table_id));
    memcpy(&head_buf[PAR_NVM_HEAD_CRC_OFFSET], &head_obj.crc, sizeof(head_obj.crc));

    const par_status_t store_status = gp_store->write(PAR_NVM_HEAD_ADDR, PAR_NVM_HEAD_SIZE, head_buf);
    if (ePAR_OK != store_status)
    {
        status = ePAR_ERROR_NVM;
        PAR_ERR_PRINT("PAR_NVM: header write failed, err=%u", (unsigned)store_status);
        return status;
    }

    status = par_nvm_sync_backend("after header write");
    if (ePAR_OK != status)
    {
        return status;
    }

#if (1 == PAR_CFG_NVM_WRITE_VERIFY_EN)
    status = par_nvm_verify_header_readback(&head_obj);
    if (ePAR_OK != status)
    {
        PAR_ERR_PRINT("PAR_NVM: header readback verification failed, err=%u", (unsigned)status);
        return status;
    }
#endif /* (1 == PAR_CFG_NVM_WRITE_VERIFY_EN) */

    PAR_DBG_PRINT("PAR_NVM: writing header with obj_count=%d", num_of_par);

    return status;
}
/**
 * @brief Validate parameter NVM header.
 *
 * @details The header CRC covers both the stored persistent-object count and
 * the stored table-ID digest bytes. This distinguishes header corruption from
 * a valid-but-incompatible table-ID mismatch.
 *
 * @param p_head_obj Pointer to validated header structure.
 * @return Status of operation.
 */
static par_status_t par_nvm_validate_header(par_nvm_head_obj_t * const p_head_obj)
{
    par_status_t status = ePAR_OK;
    uint16_t crc_calc = 0U;

    status = par_nvm_read_header(p_head_obj);
    if (ePAR_ERROR_NVM != status)
    {
        if (PAR_NVM_SIGN == p_head_obj->sign)
        {
            crc_calc = par_nvm_calc_head_crc(p_head_obj);
            if (crc_calc == p_head_obj->crc)
            {
                PAR_DBG_PRINT("PAR_NVM: header validated, stored_obj_count=%d", p_head_obj->obj_nb);
            }
            else
            {
                status = ePAR_ERROR_CRC;
                PAR_WARN_PRINT("PAR_NVM: header CRC corrupted");
            }
        }
        else
        {
            status = ePAR_ERROR;
            PAR_WARN_PRINT("PAR_NVM: header signature corrupted");
        }
    }

    return status;
}

/**
 * @brief Resolve a persistent slot index to a live parameter number.
 *
 * @param persist_idx Persistent slot index.
 * @param p_par_num Output live parameter number.
 * @return Operation status.
 */
static par_status_t par_nvm_get_num_by_persist_idx(const uint16_t persist_idx, par_num_t * const p_par_num)
{
    if ((persist_idx >= PAR_PERSISTENT_COMPILE_COUNT) || (NULL == p_par_num))
    {
        return ePAR_ERROR;
    }

    *p_par_num = g_par_persist_slot_to_par_num[persist_idx];
    return ePAR_OK;
}
/**
 * @brief Convert a compile-time persistent slot index into the managed NVM object address.
 *
 * @details The public NVM flow keeps this common entry point, while the active
 * layout implementation owns the actual address calculation.
 *
 * @param persist_idx Persistent slot index.
 * @return Start address of the slot inside the managed NVM payload area.
 */
static uint32_t par_nvm_addr_from_persist_idx(const uint16_t persist_idx)
{
    return gp_layout->addr_from_persist_idx(PAR_NVM_FIRST_DATA_OBJ_ADDR, persist_idx, g_par_persist_slot_to_par_num);
}


/**
 * @brief Print the compiled persistent-slot map and current runtime load state.
 *
 * This function is intended for debug use only. It prints, for each compiled
 * persistent slot, the slot index, parameter ID, computed NVM address,
 * serialized record size, runtime loaded flag, and parameter name when name
 * support is enabled.
 *
 * The slot-to-parameter relationship is derived from the compile-time
 * persistent mapping table, while the loaded flag reflects whether the slot
 * was loaded successfully during the current NVM initialization/load flow.
 *
 * @return ePAR_OK    Debug information was printed.
 * @return ePAR_ERROR Debug print is disabled at build time.
 */
par_status_t par_nvm_print_nvm_lut(void)
{
    par_status_t status = ePAR_OK;

#if (1 == PAR_CFG_DEBUG_EN)
    PAR_DBG_PRINT("PAR_NVM: Parameter NVM look-up table:");
    PAR_DBG_PRINT(" #	ID	Addr	Size	Loaded	name");
    PAR_DBG_PRINT("================================================");

    for (uint16_t persist_idx = 0U; persist_idx < PAR_PERSISTENT_COMPILE_COUNT; persist_idx++)
    {
        const par_num_t par_num = g_par_persist_slot_to_par_num[persist_idx];
        PAR_DBG_PRINT(
            " %u	%u	0x%08lX	%lu	%u	%s",
            (unsigned)persist_idx,
            (unsigned)PAR_NVM_CFG_ID_VALUE(par_get_config(par_num)),
            (unsigned long)par_nvm_addr_from_persist_idx(persist_idx),
            (unsigned long)gp_layout->record_size_from_par_num(par_num),
            (unsigned)g_par_nvm_slot_runtime.loaded_slots[persist_idx],
            PAR_NVM_DBG_NAME_ARG(par_get_config(par_num)));
    }
#else
    status = ePAR_ERROR;
#endif /* (1 == PAR_CFG_DEBUG_EN) */

    return status;
}

/**
 * @brief Clear the runtime-loaded persistent-slot view.
 *
 * @details This function only clears the in-RAM scalar load-status map. It is
 * safe and intentional to call it when no scalar persistent slot exists: the
 * map then remains empty, and debug/status APIs cannot report stale loaded
 * slots from an earlier init/deinit cycle or from a different build image.
 */
static void par_nvm_clear_lut(void)
{
    (void)memset(&g_par_nvm_slot_runtime, 0, sizeof(g_par_nvm_slot_runtime));
}
/**
 * @brief Mark every compiled persistent slot as available in the runtime view.
 *
 * @details This helper is used after a full rewrite path where the managed NVM
 * image is regenerated for the current compile-time persistent schema.
 */
static void par_nvm_build_new_nvm_lut(void)
{
    par_nvm_clear_lut();
    (void)memset(g_par_nvm_slot_runtime.loaded_slots, true, sizeof(g_par_nvm_slot_runtime.loaded_slots));
    g_par_nvm_slot_runtime.loaded_count = PAR_PERSISTENT_COMPILE_COUNT;
    par_nvm_print_nvm_lut();
}

/**
 * @brief Restore only scalar persistent parameters to their defaults.
 *
 * @return Operation status.
 */
static par_status_t par_nvm_set_scalar_persistent_defaults(void)
{
    par_status_t status = ePAR_OK;

    for (uint16_t persist_idx = 0U; persist_idx < PAR_PERSISTENT_COMPILE_COUNT; persist_idx++)
    {
        status |= par_set_to_default(g_par_persist_slot_to_par_num[persist_idx]);
    }

    return status;
}
/**
 * @brief Get parameter NVM object start address from the compile-time persistent slot.
 *
 * @param par_num Live parameter number.
 * @return NVM address of the persistent slot, or 0 when mapping is invalid.
 */
static uint32_t par_nvm_get_nvm_lut_addr(const par_num_t par_num)
{
    const par_cfg_t * const par_cfg = par_get_config(par_num);

    if ((NULL == par_cfg) || (false == par_cfg->persistent))
    {
        return 0U;
    }
#if (1 == PAR_NVM_OBJECT_STORE_ENABLED)
    if (true == par_object_type_is_object(par_cfg->type))
    {
        return 0U;
    }
#endif /* (1 == PAR_NVM_OBJECT_STORE_ENABLED) */
    if (par_cfg->persist_idx >= PAR_PERSISTENT_COMPILE_COUNT)
    {
        return 0U;
    }

    return par_nvm_addr_from_persist_idx(par_cfg->persist_idx);
}

/**
 * @brief Load all parameter values from NVM.
 *
 * @details Two stored-count asymmetries are handled explicitly:
 * - If the stored header count is greater than the compile-time persistent
 *   slot count, the image is treated as incompatible and the caller rebuilds
 *   the managed NVM area from current defaults.
 * - If the stored header count is smaller than the compile-time persistent
 *   slot count, compatible layouts restore the stored prefix first and append
 *   the missing tail slots from live defaults before rewriting the header
 *   count. The grouped payload-only layout is excluded from that repair path
 *   and rebuilds on any stored/live count mismatch.
 *
 * @param num_of_par Number of stored parameters inside NVM.
 * @return Status of operation.
 */
static par_status_t par_nvm_load_all(const uint16_t num_of_par)
{
    /* Hold context for one load-path validation failure. */
    typedef struct
    {
        const char *reason; /**< Short description of the validation failure. */
        uint16_t stored_id; /**< Stored ID reported by the selected layout, when available. */
    } par_nvm_load_error_ctx_t;

    par_status_t status = ePAR_OK;
    par_status_t op_status = ePAR_OK;
    par_num_t par_num = 0U;
    uint16_t i = 0U;
    par_nvm_data_obj_t obj_data = { 0 };
    uint16_t new_par_cnt = 0U;
    par_nvm_load_error_ctx_t err = { 0 };

    par_nvm_clear_lut();

    /* TODO: Revisit deployed-schema migration for persistent-slot shrink/reorder.
     * Consider introducing tombstone/delete-marker handling so removed slots can be represented
     * without forcing a destructive rebuild when relaxed compatibility rules are desired.
     */
    if (num_of_par > PAR_PERSISTENT_COMPILE_COUNT)
    {
        status = ePAR_ERROR;
        i = PAR_PERSISTENT_COMPILE_COUNT;
        err.reason = "stored-count-overflow";
        op_status = status;
        goto out;
    }

    /* Restore the stored prefix that still exists in the current compile-time schema. */
    for (i = 0U; i < num_of_par; i++)
    {
        const par_cfg_t *par_cfg = NULL;
        const uint32_t obj_addr = par_nvm_addr_from_persist_idx(i);

        op_status = par_nvm_get_num_by_persist_idx(i, &par_num);
        if (ePAR_OK != op_status)
        {
            status = ePAR_ERROR;
            err.reason = "persist-slot-invalid";
            goto out;
        }

        par_cfg = par_get_config(par_num);
        if ((NULL == par_cfg) || (false == par_cfg->persistent) || (par_cfg->persist_idx != i))
        {
            status = ePAR_ERROR;
            err.reason = "persist-map-invalid";
            op_status = status;
            goto out;
        }

        op_status = gp_layout->read(gp_store, obj_addr, par_num, &obj_data);
        if (ePAR_OK != op_status)
        {
            status = op_status;
            err.reason = (ePAR_ERROR_CRC == op_status) ? "crc-mismatch" : "read-failed";
            err.stored_id = gp_layout->get_error_stored_id(par_num, &obj_data);
            goto out;
        }

        op_status = gp_layout->validate_loaded_obj(par_num, &obj_data, &err.reason, &err.stored_id);
        if (ePAR_OK != op_status)
        {
            status = ePAR_ERROR;
            if (NULL == err.reason)
            {
                err.reason = "layout-validate-failed";
            }
            goto out;
        }

        op_status = par_set_scalar_fast(par_num, &obj_data.data);
        if (ePAR_OK != op_status)
        {
            status |= op_status;
            err.reason = "restore-failed";
            err.stored_id = gp_layout->get_error_stored_id(par_num, &obj_data);
            goto out;
        }

        g_par_nvm_slot_runtime.loaded_slots[i] = true;
    }

    /* Append any newly introduced persistent slots after restoring the stored prefix. */
    for (i = num_of_par; i < PAR_PERSISTENT_COMPILE_COUNT; i++)
    {
        const par_cfg_t *par_cfg = NULL;

        op_status = par_nvm_get_num_by_persist_idx(i, &par_num);
        if (ePAR_OK != op_status)
        {
            status = ePAR_ERROR;
            err.reason = "persist-slot-invalid";
            goto out;
        }

        par_cfg = par_get_config(par_num);
        if ((NULL == par_cfg) || (false == par_cfg->persistent) || (par_cfg->persist_idx != i))
        {
            status = ePAR_ERROR;
            err.reason = "persist-map-invalid";
            op_status = status;
            goto out;
        }

        status |= par_save(par_num);
        if (ePAR_OK != (status & ePAR_STATUS_ERROR_MASK))
        {
            err.reason = "append-save-failed";
            err.stored_id = (uint16_t)PAR_NVM_CFG_ID_VALUE(par_cfg);
            op_status = status;
            goto out;
        }

        g_par_nvm_slot_runtime.loaded_slots[i] = true;
        new_par_cnt++;
    }
    g_par_nvm_slot_runtime.loaded_count = i;

    if (new_par_cnt > 0U)
    {
        /* Missing stored slots were appended from live defaults; commit the new count. */
        status |= par_nvm_write_header(PAR_PERSISTENT_COMPILE_COUNT);
        if (ePAR_OK != (status & ePAR_STATUS_ERROR_MASK))
        {
            err.reason = "rewrite-header-failed";
            op_status = status;
            goto out;
        }

        PAR_INFO_PRINT("PAR_NVM: appended %u new persistent slots and rewrote header count to %u",
                       (unsigned)new_par_cnt,
                       (unsigned)PAR_PERSISTENT_COMPILE_COUNT);
    }

out:
#if (1 == PAR_CFG_DEBUG_EN)
    PAR_INFO_PRINT("PAR_NVM: load-all finished with status=%s", par_get_status_str(status));
    PAR_INFO_PRINT("PAR_NVM: stored parameter count in NVM=%u", (unsigned)num_of_par);
    PAR_INFO_PRINT("PAR_NVM: live persistent parameter count=%u", (unsigned)PAR_PERSISTENT_COMPILE_COUNT);

    if (NULL != err.reason)
    {
        PAR_DBG_PRINT(
            "PAR_NVM: load error slot=%u, addr=0x%08lX, stored_id=%u, par_num=%u, expected_id=%u, name=%s, reason=%s, op_status=%s, final_status=%s",
            (unsigned)i,
            (unsigned long)par_nvm_addr_from_persist_idx(i),
            (unsigned)err.stored_id,
            (unsigned)((i < PAR_PERSIST_SLOT_MAP_CAPACITY) ? g_par_persist_slot_to_par_num[i] : 0U),
            (unsigned)((i < PAR_PERSIST_SLOT_MAP_CAPACITY) ? PAR_NVM_CFG_ID_VALUE(par_get_config(g_par_persist_slot_to_par_num[i])) : 0U),
            PAR_NVM_DBG_NAME_ARG((i < PAR_PERSIST_SLOT_MAP_CAPACITY) ? par_get_config(g_par_persist_slot_to_par_num[i]) : NULL),
            err.reason,
            par_get_status_str(op_status),
            par_get_status_str(status));
    }
#endif /* (1 == PAR_CFG_DEBUG_EN) */
    return status;
}
/**
 * @brief Return whether scalar backend/layout initialization is required.
 *
 * @details Dedicated object-only persistence does not require the scalar
 * backend. Shared object placement still needs scalar storage because object
 * records are addressed through the scalar backend address space.
 *
 * @return true when scalar persistence or shared object persistence is active.
 */
static bool par_nvm_scalar_store_is_needed(void)
{
    if (0U < PAR_NVM_SCALAR_RECORD_COUNT)
    {
        return true;
    }

#if (1 == PAR_NVM_OBJECT_STORE_ENABLED)
    if ((par_nvm_object_get_count() > 0U) &&
        (true == par_nvm_object_store_uses_scalar_backend()))
    {
        return true;
    }
#endif /* (1 == PAR_NVM_OBJECT_STORE_ENABLED) */

    return false;
}

/**
 * @brief Return whether scalar persisted-record layout binding is required.
 *
 * @details Object-only dedicated persistence does not need a scalar layout.
 * Shared object placement can also skip the layout when no scalar persistent
 * slots exist because the object block starts at the first scalar-record
 * address without iterating any scalar record sizes.
 *
 * @return true when scalar persistent records are active.
 */
static bool par_nvm_scalar_layout_is_needed(void)
{
    return (0U < PAR_NVM_SCALAR_RECORD_COUNT);
}

/**
 * @brief Resolve, validate, and initialize required storage backends.
 *
 * @return Status of operation.
 */
static par_status_t par_nvm_init_nvm(void)
{
    par_status_t status = ePAR_OK;
    const bool need_scalar_store = par_nvm_scalar_store_is_needed();
    const bool need_scalar_layout = par_nvm_scalar_layout_is_needed();

#if (1 == PAR_NVM_OBJECT_STORE_ENABLED)
    par_nvm_object_invalidate_block_addr_cache();
#endif /* (1 == PAR_NVM_OBJECT_STORE_ENABLED) */

    gb_is_nvm_owner = false;
    gp_store = NULL;
    gp_layout = NULL;

    if (true == need_scalar_store)
    {
        status = par_nvm_scalar_store_init(&gp_store, &gb_is_nvm_owner);
    }
    else
    {
        PAR_DBG_PRINT("PAR_NVM: scalar storage backend not required");
    }

    if ((ePAR_OK == status) && (true == need_scalar_layout))
    {
        status = par_nvm_scalar_layout_bind(&gp_layout);
    }
    else if (ePAR_OK == status)
    {
        PAR_DBG_PRINT("PAR_NVM: scalar persisted-record layout not required");
    }

#if (1 == PAR_NVM_OBJECT_STORE_ENABLED)
    if ((ePAR_OK == status) && (par_nvm_object_get_count() > 0U))
    {
        status = par_nvm_object_store_init(gp_store);
    }
#endif /* (1 == PAR_NVM_OBJECT_STORE_ENABLED) */

    return status;
}
/**
 * @brief Clear NVM runtime state after deinitialization or rollback.
 *
 * @details This helper only clears the parameter NVM module state. Object
 * store ownership is released separately by par_nvm_object_store_deinit() so
 * callers can control whether the active object store remains intact after a
 * partial deinitialization failure.
 */
static void par_nvm_clear_runtime_state(void)
{
    gb_is_init = false;
    gb_is_nvm_owner = false;
    gp_store = NULL;
    gp_layout = NULL;
#if (1 == PAR_NVM_OBJECT_STORE_ENABLED)
    par_nvm_object_invalidate_block_addr_cache();
#endif /* (1 == PAR_NVM_OBJECT_STORE_ENABLED) */
}

/**
 * @brief Release NVM state after a late initialization failure.
 *
 * @details This is used after one or more backends may have been resolved and
 * initialized by the parameter module, but before initialization is allowed to
 * complete. It attempts to release module-owned resources and always clears the
 * runtime state so later API calls do not treat the failed initialization as usable.
 */
static void par_nvm_cleanup_after_init_failure(void)
{
#if (1 == PAR_NVM_OBJECT_STORE_ENABLED)
    const par_status_t object_store_status = par_nvm_object_store_deinit();

    if (ePAR_OK != object_store_status)
    {
        PAR_ERR_PRINT("PAR_NVM: object backend cleanup after init failure failed, err=%u", (unsigned)object_store_status);
    }
#endif /* (1 == PAR_NVM_OBJECT_STORE_ENABLED) */

    if (ePAR_OK != par_nvm_scalar_store_deinit(gp_store, gb_is_nvm_owner, "init failure"))
    {
        PAR_ERR_PRINT("PAR_NVM: scalar backend cleanup after init failure failed");
    }

    par_nvm_clear_runtime_state();
}

/**
 * @} <!-- END GROUP -->
 */

/**
 * @addtogroup API_PAR_NVM_FUNCTIONS
 * @{ <!-- BEGIN GROUP -->
 *
 * @brief Following functions are part of Device Parameter NVM manipulation API.
 */

/**
 * @brief Store all scalar persistent parameter values to the scalar NVM block.
 * @return Status of operation.
 */
static par_status_t par_nvm_write_scalar_all(void);

/**
 * @brief Initialize parameter NVM handling.
 * @details Initialization behavior depends on the settings in par_cfg.h.
 * Table-ID checking is enabled only when PAR_CFG_TABLE_ID_CHECK_EN is set.
 *
 * The recovery flow is centralized and cumulative:
 * - header validation runs first;
 * - table-ID validation runs only when the header is valid;
 * - header CRC validation covers both the stored object count and the stored
 *   table-ID digest bytes;
 * - payload loading runs only when both checks pass;
 * - then the collected error bits decide whether startup should restore live
 *   RAM values to defaults only, or restore defaults and also rebuild the
 *   managed NVM image.
 *
 * This keeps the recovery action aligned with the detected failure class
 * instead of hiding it inside a long if-else chain.
 *
 * @return Status of initialization.
 */
par_status_t par_nvm_init(void)
{
    par_status_t status = ePAR_OK;
    par_status_t detect_status = ePAR_OK;
    par_nvm_head_obj_t head_obj = { 0 };
    uint16_t per_par_nb = 0U;
    uint16_t object_par_nb = 0U;
    bool need_set_default = false;
    bool need_rewrite_nvm = false;

    PAR_DBG_PRINT("PAR_NVM: initialization started");
    status = par_nvm_init_nvm();
    if (ePAR_OK != status)
    {
        par_nvm_cleanup_after_init_failure();
        return status;
    }

    gb_is_init = true;
    per_par_nb = PAR_NVM_SCALAR_RECORD_COUNT;
#if (1 == PAR_NVM_OBJECT_STORE_ENABLED)
    object_par_nb = par_nvm_object_get_count();
#endif /* (1 == PAR_NVM_OBJECT_STORE_ENABLED) */

    if ((0U == per_par_nb) && (0U == object_par_nb))
    {
        PAR_INFO_PRINT("PAR_NVM: no persistent parameters configured");
        return (par_status_t)(status | ePAR_WAR_NO_PERSISTENT);
    }

#if (1 == PAR_NVM_OBJECT_STORE_ENABLED)
    if (object_par_nb > 0U)
    {
        const uint32_t object_block_addr =
            par_nvm_object_get_block_addr(PAR_NVM_FIRST_DATA_OBJ_ADDR, gp_layout, g_par_persist_slot_to_par_num);

        status = par_nvm_object_validate_storage_capacity(object_block_addr);
        if (ePAR_OK != status)
        {
            par_nvm_cleanup_after_init_failure();
            return status;
        }
    }
#endif /* (1 == PAR_NVM_OBJECT_STORE_ENABLED) */

    if (per_par_nb > 0U)
    {
        /* Step 1: validate scalar header */
        detect_status = par_nvm_validate_header(&head_obj);

#if (1 == PAR_CFG_TABLE_ID_CHECK_EN)
        /* Step 2: validate scalar-table compatibility only when header is valid */
        if (ePAR_OK == (detect_status & ePAR_STATUS_ERROR_MASK))
        {
            const par_nvm_compat_result_t compat = gp_layout->check_compat(&head_obj);

            if (ePAR_NVM_COMPAT_REBUILD == compat)
            {
                detect_status |= ePAR_ERROR_TABLE_ID;
            }
            else
            {
                if (ePAR_NVM_COMPAT_PREFIX_APPEND == compat)
                {
                    PAR_INFO_PRINT("PAR_NVM: stored scalar persistent prefix is compatible, restore=%u append=%u",
                                   (unsigned)head_obj.obj_nb,
                                   (unsigned)((uint16_t)PAR_PERSISTENT_COMPILE_COUNT - head_obj.obj_nb));
                }

                detect_status |= par_nvm_load_all(head_obj.obj_nb);
            }
        }
#else
        /* Step 2: load scalar payload only when header is valid */
        if (ePAR_OK == (detect_status & ePAR_STATUS_ERROR_MASK))
        {
            detect_status |= par_nvm_load_all(head_obj.obj_nb);
        }
#endif /* (1 == PAR_CFG_TABLE_ID_CHECK_EN) */

        /* Step 4: classify scalar recovery action from detected issues */
        if (0U != (detect_status & ePAR_ERROR_TABLE_ID))
        {
            PAR_WARN_PRINT("PAR_NVM: scalar table-ID mismatch detected, restoring scalar defaults and rebuilding scalar NVM image");
            need_set_default = true;
            need_rewrite_nvm = true;
        }

        if (0U != (detect_status & ePAR_ERROR_CRC))
        {
            PAR_WARN_PRINT("PAR_NVM: scalar CRC corruption detected, rebuilding scalar NVM from defaults");
            need_set_default = true;
            need_rewrite_nvm = true;
        }

        /*
         * ePAR_ERROR here represents generic header/signature validation failure
         * from par_nvm_validate_header().
         */
        if (0U != (detect_status & ePAR_ERROR))
        {
            PAR_WARN_PRINT("PAR_NVM: scalar header/signature mismatch detected, rebuilding scalar NVM from defaults");
            need_set_default = true;
            need_rewrite_nvm = true;
        }

        /*
         * NVM access failures are not considered fully recoverable.
         * Restore scalar live values to defaults, but keep ePAR_ERROR_NVM in final status.
         */
        if (0U != (detect_status & ePAR_ERROR_NVM))
        {
            PAR_WARN_PRINT("PAR_NVM: scalar NVM access error detected, restoring scalar values to defaults without forced rewrite");
            need_set_default = true;
        }

        /* Step 5: perform scalar recovery */
        if (true == need_set_default)
        {
            status |= par_nvm_set_scalar_persistent_defaults();
            if (ePAR_OK == (status & ePAR_STATUS_ERROR_MASK))
            {
                status |= ePAR_WAR_SET_TO_DEF;
            }
        }

        if ((true == need_rewrite_nvm) && (ePAR_OK == (status & ePAR_STATUS_ERROR_MASK)))
        {
            status |= par_nvm_write_scalar_all();
            if (ePAR_OK == (status & ePAR_STATUS_ERROR_MASK))
            {
                status |= ePAR_WAR_NVM_REWRITTEN;
            }
        }

        /*
         * Step 6: finalize scalar status. Recoverable scalar problems should not
         * remain as error bits once recovery succeeded. Backend access failures
         * remain reported.
         */
        if (0U != (detect_status & ePAR_ERROR_NVM))
        {
            status |= ePAR_ERROR_NVM;
        }
    }
    else
    {
        par_nvm_clear_lut();
    }

#if (1 == PAR_NVM_OBJECT_STORE_ENABLED)
    if ((ePAR_OK == (status & ePAR_STATUS_ERROR_MASK)) &&
        (par_nvm_object_get_count() > 0U))
    {
        const uint32_t object_block_addr =
            par_nvm_object_get_block_addr(PAR_NVM_FIRST_DATA_OBJ_ADDR, gp_layout, g_par_persist_slot_to_par_num);

        status |= par_nvm_object_init_from_active_store(object_block_addr);
        if (ePAR_OK != (status & ePAR_STATUS_ERROR_MASK))
        {
            par_nvm_cleanup_after_init_failure();
            return status;
        }
    }
#endif /* (1 == PAR_NVM_OBJECT_STORE_ENABLED) */

    PAR_INFO_PRINT("PAR_NVM: initialization finished with status=%s", par_get_status_str(status));
    return status;
}
/**
 * @brief De-Initialize parameter NVM handling.
 * @details Object backend deinitialization is attempted before scalar
 * backend deinitialization so object storage can still use any shared
 * lower-level storage dependency owned by the scalar backend. Runtime
 * state is cleared before returning so public APIs cannot keep using
 * partially released resources.
 *
 * @return Status of de-init.
 */
par_status_t par_nvm_deinit(void)
{
    par_status_t status = ePAR_OK;

    PAR_DBG_PRINT("PAR_NVM: deinitialization started");
    if (true == gb_is_init)
    {
#if (1 == PAR_NVM_OBJECT_STORE_ENABLED)
        {
            const par_status_t object_store_status = par_nvm_object_store_deinit();
            if (ePAR_OK != object_store_status)
            {
                status = ePAR_ERROR;
                PAR_ERR_PRINT("PAR_NVM: object backend deinit failed, err=%u", (unsigned)object_store_status);
            }
        }
#endif /* (1 == PAR_NVM_OBJECT_STORE_ENABLED) */

        if (ePAR_OK != par_nvm_scalar_store_deinit(gp_store, gb_is_nvm_owner, "deinit"))
        {
            status = ePAR_ERROR;
        }

        par_nvm_clear_runtime_state();
    }
    else
    {
        status = ePAR_ERROR;
    }

    PAR_INFO_PRINT("PAR_NVM: deinitialization finished with status=%s", par_get_status_str(status));
    return status;
}
/**
 * @brief Store parameter value to NVM.
 *
 * @note Sync has only effect when using EEPROM emulated NVM feature! When.
 * using Flash end memory device.
 *
 * @note In case of using Flash end memory for storing parameters take special.
 * care when enabling sync (nvm_sync=true). At each sync data from RAM.
 * is copied to FLASH.
 *
 * @param par_num Parameter enumeration number.
 * @param nvm_sync Request an explicit backend sync after parameter write. When
 *        PAR_CFG_NVM_WRITE_VERIFY_EN is enabled, write verification also
 *        forces a backend sync before the readback step even if this flag is
 *        false. Backend implementations may also persist data before this
 *        function returns when their write contract requires completed writes
 *        to be durable on success. Therefore false does not mean RAM-only
 *        staging, and a successful return means the backend-side work needed
 *        for this request has completed. This flag still does not guarantee
 *        transactional atomicity across any backend-specific internal
 *        chunking.
 * @return Status of operation.
 */
par_status_t par_nvm_write(const par_num_t par_num, const bool nvm_sync)
{
    if (true != gb_is_init)
    {
        return ePAR_ERROR_INIT;
    }

    if (par_num >= ePAR_NUM_OF)
    {
        return ePAR_ERROR;
    }

    const par_cfg_t * const par_cfg = par_get_config(par_num);
    if (true != par_cfg->persistent)
    {
        PAR_DBG_PRINT("PAR_NVM: skip write for non-persistent parameter, par_num=%u", (unsigned)par_num);
        return ePAR_ERROR;
    }

#if (1 == PAR_NVM_OBJECT_STORE_ENABLED)
    if (true == par_object_type_is_object(par_cfg->type))
    {
        const uint32_t object_block_addr =
            par_nvm_object_get_block_addr(PAR_NVM_FIRST_DATA_OBJ_ADDR, gp_layout, g_par_persist_slot_to_par_num);

        return par_nvm_object_write_to_active_store(object_block_addr, par_num, nvm_sync);
    }
#endif /* (1 == PAR_NVM_OBJECT_STORE_ENABLED) */

    par_status_t status = ePAR_OK;
    par_nvm_data_obj_t obj_data = { 0 };
    par_type_t live_data = { 0 };
    uint32_t par_addr = 0UL;
    par_status_t store_status = ePAR_OK;

    PAR_DBG_PRINT("PAR_NVM: writing persistent parameter, par_num=%u id=%u", (unsigned)par_num, (unsigned)PAR_NVM_CFG_ID_VALUE(par_cfg));
    status = par_get_scalar(par_num, &live_data);
    if (ePAR_OK != status)
    {
        PAR_ERR_PRINT("PAR_NVM: failed to read live value before write, par_num=%u id=%u err=%u",
                      (unsigned)par_num,
                      (unsigned)PAR_NVM_CFG_ID_VALUE(par_cfg),
                      (unsigned)status);
        return status;
    }
    gp_layout->populate_data_obj(par_num, &live_data, &obj_data);
    par_addr = par_nvm_get_nvm_lut_addr(par_num);
    store_status = gp_layout->write(gp_store, par_addr, par_num, &obj_data);
    if (ePAR_OK != store_status)
    {
        status |= ePAR_ERROR_NVM;
        PAR_ERR_PRINT("PAR_NVM: parameter write failed, par_num=%u id=%u addr=0x%08lX err=%u",
                      (unsigned)par_num,
                      (unsigned)PAR_NVM_CFG_ID_VALUE(par_cfg),
                      (unsigned long)par_addr,
                      (unsigned)store_status);
    }

    if (ePAR_OK == (status & ePAR_STATUS_ERROR_MASK))
    {
#if (1 == PAR_CFG_NVM_WRITE_VERIFY_EN)
        (void)nvm_sync;
        status |= par_nvm_sync_backend("before parameter readback verify");
        if (ePAR_OK == (status & ePAR_STATUS_ERROR_MASK))
        {
            status |= par_nvm_verify_record_readback(par_addr, par_num, &obj_data);
            if (ePAR_OK != (status & ePAR_STATUS_ERROR_MASK))
            {
                PAR_ERR_PRINT("PAR_NVM: parameter readback verification failed, par_num=%u id=%u addr=0x%08lX err=%u",
                              (unsigned)par_num,
                              (unsigned)PAR_NVM_CFG_ID_VALUE(par_cfg),
                              (unsigned long)par_addr,
                              (unsigned)status);
            }
        }
#else
        if (true == nvm_sync)
        {
            status |= par_nvm_sync_backend("after parameter write");
        }
#endif /* (1 == PAR_CFG_NVM_WRITE_VERIFY_EN) */
    }

    return status;
}

#if (1 == PAR_NVM_OBJECT_STORE_ENABLED)
/**
 * @brief Store one object parameter to NVM while its parameter mutex is held.
 *
 * @param par_num Parameter enumeration number.
 * @param nvm_sync Request an explicit backend sync after parameter write.
 * @return Status of operation.
 *
 * @pre Caller must hold the parameter mutex for @p par_num.
 */
par_status_t par_nvm_write_object_locked(const par_num_t par_num,
                                         const bool nvm_sync)
{
    const par_cfg_t *par_cfg = NULL;
    uint32_t object_block_addr = 0U;

    if (true != gb_is_init)
    {
        return ePAR_ERROR_INIT;
    }
    if (par_num >= ePAR_NUM_OF)
    {
        return ePAR_ERROR;
    }

    par_cfg = par_get_config(par_num);
    if ((NULL == par_cfg) || (true != par_cfg->persistent) ||
        (false == par_object_type_is_object(par_cfg->type)))
    {
        return ePAR_ERROR;
    }

    object_block_addr =
        par_nvm_object_get_block_addr(PAR_NVM_FIRST_DATA_OBJ_ADDR, gp_layout, g_par_persist_slot_to_par_num);

    return par_nvm_object_write_locked_to_active_store(object_block_addr, par_num, nvm_sync);
}
#endif /* (1 == PAR_NVM_OBJECT_STORE_ENABLED) */

/**
 * @brief Store all scalar persistent parameter values to the scalar NVM block.
 *
 * @return Status of operation.
 */
static par_status_t par_nvm_write_scalar_all(void)
{
    par_status_t status = ePAR_OK;

    if (0U == PAR_NVM_SCALAR_RECORD_COUNT)
    {
        /* Keep the runtime load-status map empty when no scalar slot exists. */
        par_nvm_clear_lut();
        return ePAR_OK;
    }

    /* Mark the scalar header invalid before bulk rewrite and commit that state. */
    const par_status_t store_status = gp_store->erase(PAR_NVM_HEAD_SIGN_ADDR, PAR_NVM_HEAD_SIGN_SIZE);
    if (ePAR_OK != store_status)
    {
        status |= ePAR_ERROR_NVM;
        PAR_ERR_PRINT("PAR_NVM: scalar signature erase failed, err=%u", (unsigned)store_status);
    }

    if (ePAR_OK == (status & ePAR_STATUS_ERROR_MASK))
    {
        status |= par_nvm_sync_backend("after scalar signature erase");
    }

    if (ePAR_OK == (status & ePAR_STATUS_ERROR_MASK))
    {
        for (uint16_t persist_idx = 0U; persist_idx < PAR_PERSISTENT_COMPILE_COUNT; persist_idx++)
        {
            const par_num_t par_num = g_par_persist_slot_to_par_num[persist_idx];
            status |= par_nvm_write(par_num, false);
            if (ePAR_OK != (status & ePAR_STATUS_ERROR_MASK))
            {
                PAR_ERR_PRINT("PAR_NVM: scalar bulk write aborted, par_num=%u id=%u addr=0x%08lX err=%u",
                              (unsigned)par_num,
                              (unsigned)PAR_NVM_CFG_ID_VALUE(par_get_config(par_num)),
                              (unsigned long)par_nvm_get_nvm_lut_addr(par_num),
                              (unsigned)status);
                break;
            }
        }
    }

    /* Restore a valid scalar header only after the scalar rewrite completes successfully. */
    if (ePAR_OK == (status & ePAR_STATUS_ERROR_MASK))
    {
        status |= par_nvm_write_header(PAR_PERSISTENT_COMPILE_COUNT);
    }

    if (ePAR_OK == (status & ePAR_STATUS_ERROR_MASK))
    {
        par_nvm_build_new_nvm_lut();
    }
    else
    {
        par_nvm_clear_lut();
    }

    return status;
}

/**
 * @brief Store all persistent parameters to NVM.
 *
 * @details The scalar block is rewritten first through the selected scalar
 * layout. When object persistence is enabled, configured, and scalar
 * persistence completed without errors, fixed-capacity object payloads are
 * then written to the dedicated object block at the configured object
 * persistence address.
 *
 * @return Status of operation.
 */
par_status_t par_nvm_write_all(void)
{
    if (true != gb_is_init)
    {
        return ePAR_ERROR_INIT;
    }

    par_status_t status = ePAR_OK;
    PAR_DBG_PRINT("PAR_NVM: storing all persistent parameters to NVM");

    status |= par_nvm_write_scalar_all();

#if (1 == PAR_NVM_OBJECT_STORE_ENABLED)
    if ((ePAR_OK == (status & ePAR_STATUS_ERROR_MASK)) &&
        (par_nvm_object_get_count() > 0U))
    {
        const uint32_t object_block_addr =
            par_nvm_object_get_block_addr(PAR_NVM_FIRST_DATA_OBJ_ADDR, gp_layout, g_par_persist_slot_to_par_num);

        status |= par_nvm_object_write_all_to_active_store(object_block_addr);
    }
#endif /* (1 == PAR_NVM_OBJECT_STORE_ENABLED) */

    PAR_INFO_PRINT("PAR_NVM: store-all finished with status=%s", par_get_status_str(status));

    return status;
}
/**
 * @brief Rewrite the whole parameter NVM section.
 * @details The signature is corrupted first to mark the image as being
 * rewritten, then the LUT and all persistent objects are rebuilt.
 * @return Status of operation.
 */
par_status_t par_nvm_reset_all(void)
{
    par_status_t status = ePAR_OK;

    PAR_ASSERT(true == gb_is_init);
    PAR_DBG_PRINT("PAR_NVM: rebuild-all requested");

    if (true != gb_is_init)
    {
        return ePAR_ERROR_INIT;
    }

    status |= par_nvm_write_all();
    if (ePAR_OK == (status & ePAR_STATUS_ERROR_MASK))
    {
        PAR_DBG_PRINT("PAR_NVM: rebuild-all finished successfully");
    }
    else
    {
        PAR_ERR_PRINT("PAR_NVM: rebuild-all failed, status=%s", par_get_status_str(status));
    }

    return status;
}
/**
 * @} <!-- END GROUP -->
 */

#endif /* (1 == PAR_CFG_NVM_EN) */
