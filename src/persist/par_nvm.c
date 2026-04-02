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
 * 2026-01-29 V3.0.1  Ziga Miklosic
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
 * firmware image. Typical causes are an intentional schema-version bump, a
 * change in persisted-parameter order/type/ID, or stored-image corruption.
 * The recovery path is centralized in par_nvm_init(): restore defaults first,
 * then rebuild the managed NVM image only for errors that require a rewrite.
 */
/**
 * @brief Include dependencies.
 */
#include "persist/par_nvm.h"
#include "par_cfg.h"
#include "port/par_if.h"

#if (1 == PAR_CFG_NVM_EN)

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "persist/backend/par_store_backend.h"
#include "persist/par_nvm_table_id.h"
/**
 * @brief Parameter NVM header object.
 */
typedef struct
{
    uint32_t sign;      /**< Signature in host order. */
    uint16_t obj_nb;    /**< Stored data object number in host order. */
    uint32_t table_id;  /**< Stored parameter-table ID in platform-native order. */
    uint16_t crc;       /**< Header CRC-16 over serialized obj_nb and table_id. */
} par_nvm_head_obj_t;
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
 * @brief Persisted parameter object layout (current implementation).
 *
 * @details Each persistent parameter is serialized as one fixed-size 8-byte
 * object in the managed NVM area:
 *
 * Byte offset:   0       1       2       3       4       5       6       7
 *              +-------+-------+-------+-------+-------+-------+-------+-------+
 * Field:       |        id        | size  | crc8  |        data slot       |
 *              +-------+-------+-------+-------+-------+-------+-------+-------+
 * Width:       |<---- 2 bytes ---->| 1 B   | 1 B   |<------ 4 bytes ------->|
 *
 * Meaning:
 * - id   : external parameter ID of this persisted entry. Its main value is
 *          that persisted records stay self-describing instead of depending
 *          only on fixed slot positions.
 * - size : payload-width descriptor. The current implementation always writes
 *          the full 4-byte payload-slot width, so this field does not save
 *          NVM space yet; it is kept as a descriptor and integrity helper.
 * - crc  : per-record CRC-8 calculated over the serialized id/size/data bytes.
 * - data : 32-bit payload slot used to store the parameter value. Even U8/U16
 *          parameters still occupy the same 4-byte payload slot in NVM.
 *
 * @note Live RAM layout and persisted NVM layout are intentionally different.
 * RAM storage is grouped by value width, while the NVM persistence area is a
 * linear list of fixed 8-byte objects.
 */
typedef struct
{
    uint16_t id;        /**< Parameter ID. */
    uint8_t size;       /**< Payload-width descriptor. */
    uint8_t crc;        /**< CRC-8 over id, size, and 4-byte payload slot. */
    par_type_t data;    /**< Fixed 4-byte payload slot for the parameter value. */
} par_nvm_data_obj_t;

#define PAR_NVM_DATA_OBJ_STRIDE ((uint32_t)sizeof(par_nvm_data_obj_t))
#define PAR_NVM_DATA_SLOT_SIZE  ((uint8_t)sizeof(((par_nvm_data_obj_t *)0)->data))
/**
 * @brief Runtime persistence-slot state.
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
#endif

#define PAR_PERSIST_SLOT_ENTRY_SELECT(enum_, pers_)   PAR_PERSIST_SLOT_ENTRY_SELECT_I(enum_, pers_)
#define PAR_PERSIST_SLOT_ENTRY_SELECT_I(enum_, pers_) PAR_PERSIST_SLOT_ENTRY_SELECT_##pers_(enum_)
#define PAR_PERSIST_SLOT_ENTRY_SELECT_true(enum_)     [PAR_PERSIST_IDX_##enum_] = enum_,
#define PAR_PERSIST_SLOT_ENTRY_SELECT_false(enum_)
#define PAR_PERSIST_SLOT_ENTRY_SELECT_1(enum_) [PAR_PERSIST_IDX_##enum_] = enum_,
#define PAR_PERSIST_SLOT_ENTRY_SELECT_0(enum_)
#define PAR_ITEM_PERSIST_SLOT(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_) PAR_PERSIST_SLOT_ENTRY_SELECT(enum_, pers_)
/**
 * @brief Compile-time mapping from persistent slot to live parameter number.
 *
 * @details This table is derived directly from par_table.def. Slot order is the
 * single source of truth for the managed NVM payload layout used by par_nvm.c.
 */
static const par_num_t g_par_persist_slot_to_par_num[PAR_PERSIST_SLOT_MAP_CAPACITY] = {
#define PAR_ITEM_U8  PAR_ITEM_PERSIST_SLOT
#define PAR_ITEM_U16 PAR_ITEM_PERSIST_SLOT
#define PAR_ITEM_U32 PAR_ITEM_PERSIST_SLOT
#define PAR_ITEM_I8  PAR_ITEM_PERSIST_SLOT
#define PAR_ITEM_I16 PAR_ITEM_PERSIST_SLOT
#define PAR_ITEM_I32 PAR_ITEM_PERSIST_SLOT
#define PAR_ITEM_F32 PAR_ITEM_PERSIST_SLOT
#include "../../par_table.def"
#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32
};
#undef PAR_ITEM_PERSIST_SLOT
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
 * @brief Calculate per-record CRC-8 over serialized id/size/data bytes.
 *
 * @details The record CRC is accumulated field-by-field over the serialized
 * native-order bytes of id, size, and the fixed 4-byte payload slot.
 *
 * @param p_obj Pointer to data object.
 * @return Calculated CRC-8 value.
 */
static uint8_t par_nvm_calc_obj_crc(const par_nvm_data_obj_t * const p_obj)
{
    uint32_t data_raw = 0U;
    uint8_t crc = PAR_IF_CRC8_INIT;

    PAR_ASSERT(NULL != p_obj);

    memcpy(&data_raw, &p_obj->data, sizeof(data_raw));

    crc = par_if_crc8_accumulate(crc, (const uint8_t * const)&p_obj->id, (uint32_t)sizeof(p_obj->id));
    crc = par_if_crc8_accumulate(crc, (const uint8_t * const)&p_obj->size, (uint32_t)sizeof(p_obj->size));
    crc = par_if_crc8_accumulate(crc, (const uint8_t * const)&data_raw, (uint32_t)sizeof(data_raw));

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
        PAR_DBG_PRINT("PAR_NVM: header read failed, %u", (unsigned)store_status);
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
    head_obj.table_id = par_nvm_table_id_calc();
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
        PAR_DBG_PRINT("PAR_NVM: header write failed, %u", (unsigned)store_status);
        return status;
    }

    PAR_DBG_PRINT("PAR_NVM: Write NVM header with %d nb. of object", num_of_par);

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
                PAR_DBG_PRINT("PAR_NVM: NVM header OK! Nb. of stored obj: %d", p_head_obj->obj_nb);
            }
            else
            {
                status = ePAR_ERROR_CRC;
                PAR_DBG_PRINT("PAR_NVM: Header CRC corrupted!");
            }
        }
        else
        {
            status = ePAR_ERROR;
            PAR_DBG_PRINT("PAR_NVM: Signature corrupted!");
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
 * @details The managed NVM payload area is a dense linear array of fixed-size
 * par_nvm_data_obj_t records. Therefore the slot address is derived directly as:
 * first-data-object-address + slot-index * object-stride.
 *
 * @param persist_idx Persistent slot index.
 * @return Start address of the slot inside the managed NVM payload area.
 */
static uint32_t par_nvm_addr_from_persist_idx(const uint16_t persist_idx)
{
    return (PAR_NVM_FIRST_DATA_OBJ_ADDR + (PAR_NVM_DATA_OBJ_STRIDE * persist_idx));
}

/**
 * @brief Print the compiled persistent-slot map and current runtime load state.
 *
 * This function is intended for debug use only. It prints, for each compiled
 * persistent slot, the slot index, parameter ID, computed NVM address, runtime
 * loaded flag, and parameter name when name support is enabled.
 *
 * The slot-to-parameter relationship is derived from the compile-time persistent
 * mapping table, while the loaded flag reflects whether the slot was loaded
 * successfully during the current NVM initialization/load flow.
 *
 * @return ePAR_OK    Debug information was printed.
 * @return ePAR_ERROR Debug print is disabled at build time.
 */
par_status_t par_nvm_print_nvm_lut(void)
{
    par_status_t status = ePAR_OK;

#if (1 == PAR_CFG_DEBUG_EN)
    PAR_DBG_PRINT("PAR_NVM: Parameter NVM look-up table:");
    PAR_DBG_PRINT(" #\tID\tAddr\tLoaded\tname");
    PAR_DBG_PRINT("================================");

    for (uint16_t persist_idx = 0U; persist_idx < PAR_PERSISTENT_COMPILE_COUNT; persist_idx++)
    {
        PAR_DBG_PRINT(
            " %u\t%u\t0x%08lX\t%u\t%s",
            (unsigned)persist_idx,
            (unsigned)par_get_config(g_par_persist_slot_to_par_num[persist_idx])->id,
            (unsigned long)par_nvm_addr_from_persist_idx(persist_idx),
            (unsigned)g_par_nvm_slot_runtime.loaded_slots[persist_idx],
            PAR_NVM_DBG_NAME_ARG(par_get_config(g_par_persist_slot_to_par_num[persist_idx])));
    }
#else
    status = ePAR_ERROR;
#endif

    return status;
}
/**
 * @brief Clear the runtime-loaded persistent-slot view.
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
 * @brief Get parameter NVM object start address based on its ID.
 *
 * @note In case ID is not found there is a problem with building the.
 * NVM lut!!!
 *
 * @param id Parameter ID.
 * @return NVM address of object with ID.
 */
static uint32_t par_nvm_get_nvm_lut_addr(const uint16_t id)
{
    par_num_t par_num = 0U;
    const par_cfg_t *par_cfg = NULL;

    if (ePAR_OK != par_get_num_by_id(id, &par_num))
    {
        return 0U;
    }

    par_cfg = par_get_config(par_num);
    if ((NULL == par_cfg) || (false == par_cfg->persistent) || (par_cfg->persist_idx >= PAR_PERSISTENT_COMPILE_COUNT))
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
 *   slot count, the stored prefix is restored first and the missing tail slots
 *   are appended from live defaults before the header count is rewritten.
 *
 * @param num_of_par Number of stored parameters inside NVM.
 * @return Status of operation.
 */
static par_status_t par_nvm_load_all(const uint16_t num_of_par)
{
    typedef struct
    {
        const char *reason;
        uint16_t stored_id;
    } par_nvm_load_error_ctx_t;

    par_status_t status = ePAR_OK;
    par_status_t op_status = ePAR_OK;
    par_num_t par_num = 0U;
    uint16_t i = 0U;
    par_nvm_data_obj_t obj_data = { 0 };
    uint8_t crc_calc = 0U;
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

        op_status = gp_store->read(obj_addr, (uint32_t)sizeof(par_nvm_data_obj_t), (uint8_t *)&obj_data);
        if (ePAR_OK != op_status)
        {
            status = ePAR_ERROR_NVM;
            err.reason = "read-failed";
            goto out;
        }

        crc_calc = par_nvm_calc_obj_crc(&obj_data);
        if (crc_calc != obj_data.crc)
        {
            status = ePAR_ERROR_CRC;
            err.reason = "crc-mismatch";
            err.stored_id = obj_data.id;
            op_status = status;
            goto out;
        }

        op_status = par_nvm_get_num_by_persist_idx(i, &par_num);
        if (ePAR_OK != op_status)
        {
            status = ePAR_ERROR;
            err.reason = "persist-slot-invalid";
            err.stored_id = obj_data.id;
            goto out;
        }

        par_cfg = par_get_config(par_num);
        if ((NULL == par_cfg) || (false == par_cfg->persistent) || (par_cfg->persist_idx != i))
        {
            status = ePAR_ERROR;
            err.reason = "persist-map-invalid";
            err.stored_id = obj_data.id;
            op_status = status;
            goto out;
        }

        if (obj_data.id != par_cfg->id)
        {
            status = ePAR_ERROR;
            err.reason = "id-mismatch";
            err.stored_id = obj_data.id;
            op_status = status;
            goto out;
        }

        op_status = par_set_fast(par_num, &obj_data.data);
        if (ePAR_OK != op_status)
        {
            status |= op_status;
            err.reason = "restore-failed";
            err.stored_id = obj_data.id;
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
            err.stored_id = par_cfg->id;
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

        op_status = gp_store->sync();
        if (ePAR_OK != op_status)
        {
            status |= ePAR_ERROR_NVM;
            err.reason = "sync-failed";
            goto out;
        }
        PAR_DBG_PRINT("PAR_NVM: appended %u new persistent slots and rewrote header count to %u",
                      (unsigned)new_par_cnt,
                      (unsigned)PAR_PERSISTENT_COMPILE_COUNT);
    }

out:
#if (1 == PAR_CFG_DEBUG_EN)
    PAR_DBG_PRINT("PAR_NVM: Loading all persistent parameters with status: %s", par_get_status_str(status));
    PAR_DBG_PRINT("PAR_NVM: Nb. of stored pars in NVM: %u", (unsigned)num_of_par);
    PAR_DBG_PRINT("PAR_NVM: Nb. of live persistent: %u", (unsigned)PAR_PERSISTENT_COMPILE_COUNT);

    if (NULL != err.reason)
    {
        PAR_DBG_PRINT(
            "PAR_NVM: load error slot=%u, addr=0x%08lX, stored_id=%u, par_num=%u, expected_id=%u, name=%s, reason=%s, op_status=%s, final_status=%s",
            (unsigned)i,
            (unsigned long)par_nvm_addr_from_persist_idx(i),
            (unsigned)err.stored_id,
            (unsigned)((i < PAR_PERSIST_SLOT_MAP_CAPACITY) ? g_par_persist_slot_to_par_num[i] : 0U),
            (unsigned)((i < PAR_PERSIST_SLOT_MAP_CAPACITY) ? par_get_config(g_par_persist_slot_to_par_num[i])->id : 0U),
            PAR_NVM_DBG_NAME_ARG((i < PAR_PERSIST_SLOT_MAP_CAPACITY) ? par_get_config(g_par_persist_slot_to_par_num[i]) : NULL),
            err.reason,
            par_get_status_str(op_status),
            par_get_status_str(status));
    }
#endif
    return status;
}
/**
 * @brief Resolve, validate, and initialize the mounted storage backend.
 *
 * @return Status of operation.
 */
static par_status_t par_nvm_init_nvm(void)
{
    par_status_t status = ePAR_OK;
    bool is_nvm_init = false;

    gp_store = par_store_backend_get_api();
    gb_is_nvm_owner = false;

    /* Validate the mounted backend once before any operation callback is used. */
    if ((NULL == gp_store) || (NULL == gp_store->init) || (NULL == gp_store->deinit) ||
        (NULL == gp_store->is_init) || (NULL == gp_store->read) || (NULL == gp_store->write) ||
        (NULL == gp_store->erase) || (NULL == gp_store->sync))
    {
        PAR_DBG_PRINT("PAR_NVM: No valid parameter storage backend is wired!");
        status = ePAR_ERROR_INIT;
    }

    if (ePAR_OK == status)
    {
        (void)gp_store->is_init(&is_nvm_init);
    }

    if ((ePAR_OK == status) && (false == is_nvm_init))
    {
        const par_status_t store_status = gp_store->init();
        if (ePAR_OK != store_status)
        {
            status = ePAR_ERROR_INIT;
            PAR_DBG_PRINT("PAR_NVM: backend init failed, %u", (unsigned)store_status);
        }
        else
        {
            gb_is_nvm_owner = true;
        }
    }

    return status;
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
    bool need_set_default = false;
    bool need_rewrite_nvm = false;

    status = par_nvm_init_nvm();
    if (ePAR_OK != status)
    {
        return status;
    }

    gb_is_init = true;
    per_par_nb = PAR_PERSISTENT_COMPILE_COUNT;

    if (per_par_nb == 0U)
    {
        return (par_status_t)(status | ePAR_WAR_NO_PERSISTENT);
    }

    /* Step 1: validate header */
    detect_status = par_nvm_validate_header(&head_obj);

#if (1 == PAR_CFG_TABLE_ID_CHECK_EN)
    /* Step 2: validate table-ID only when header is valid */
    if (ePAR_OK == (detect_status & ePAR_STATUS_ERROR_MASK))
    {
        const uint32_t live_table_id = par_nvm_table_id_calc();
        if (head_obj.table_id != live_table_id)
        {
            detect_status |= ePAR_ERROR_TABLE_ID;
        }
    }
#endif

    /* Step 3: load payload only when previous checks are valid */
    if (ePAR_OK == (detect_status & ePAR_STATUS_ERROR_MASK))
    {
        detect_status |= par_nvm_load_all(head_obj.obj_nb);
    }

    /* Step 4: classify recovery action from detected issues */
    if (0U != (detect_status & ePAR_ERROR_TABLE_ID))
    {
        PAR_DBG_PRINT("PAR_NVM: Table-ID mismatch detected; restoring defaults and rebuilding managed NVM image.");
        need_set_default = true;
        need_rewrite_nvm = true;
    }

    if (0U != (detect_status & ePAR_ERROR_CRC))
    {
        PAR_DBG_PRINT("PAR_NVM: CRC corruption detected; rebuilding NVM from defaults.");
        need_set_default = true;
        need_rewrite_nvm = true;
    }

    /*
     * ePAR_ERROR here represents generic header/signature validation failure
     * from par_nvm_validate_header().
     */
    if (0U != (detect_status & ePAR_ERROR))
    {
        PAR_DBG_PRINT("PAR_NVM: Header/signature mismatch detected; rebuilding NVM from defaults.");
        need_set_default = true;
        need_rewrite_nvm = true;
    }

    /*
     * NVM access failures are not considered fully recoverable.
     * Restore live values to defaults, but keep ePAR_ERROR_NVM in final status.
     */
    if (0U != (detect_status & ePAR_ERROR_NVM))
    {
        PAR_DBG_PRINT("PAR_NVM: NVM access error detected; restoring live values to defaults without forced rewrite.");
        need_set_default = true;
    }

    /* Step 5: perform recovery */
    if (true == need_set_default)
    {
        status |= par_set_all_to_default();
        status |= ePAR_WAR_SET_TO_DEF;
    }

    if (true == need_rewrite_nvm)
    {
        status |= par_nvm_reset_all();
        status |= ePAR_WAR_NVM_REWRITTEN;
    }

    /*
     * Step 6: finalize returned status.
     *
     * Recoverable problems (header/signature mismatch, CRC mismatch,
     * table-ID mismatch) should not remain as error bits once recovery
     * succeeded. Keep only warnings for those cases.
     *
     * Non-recoverable backend/storage access failures remain reported.
     */
    if (0U != (detect_status & ePAR_ERROR_NVM))
    {
        status |= ePAR_ERROR_NVM;
    }

    return status;
}
/**
 * @brief De-Initialize parameter NVM handling.
 *
 * @return Status of de-init.
 */
par_status_t par_nvm_deinit(void)
{
    par_status_t status = ePAR_OK;

    if (true == gb_is_init)
    {
        if (true == gb_is_nvm_owner)
        {
            const par_status_t store_status = gp_store->deinit();
            if (ePAR_OK != store_status)
            {
                status = ePAR_ERROR;
                PAR_DBG_PRINT("PAR_NVM: backend deinit failed, %u", (unsigned)store_status);
            }
        }

        if (ePAR_OK == status)
        {
            gb_is_init = false;
            gb_is_nvm_owner = false;
            gp_store = NULL;
        }
    }
    else
    {
        status = ePAR_ERROR;
    }

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
 * @param nvm_sync Perform NVM sync after parameter write.
 * @return Status of operation.
 */
par_status_t par_nvm_write(const par_num_t par_num, const bool nvm_sync)
{
    par_status_t status = ePAR_OK;
    par_nvm_data_obj_t obj_data = { 0 };
    uint32_t par_addr = 0UL;

    PAR_ASSERT(true == gb_is_init);
    PAR_ASSERT(par_num < ePAR_NUM_OF);

    if (true == gb_is_init)
    {
        if (par_num < ePAR_NUM_OF)
        {
            const par_cfg_t * const par_cfg = par_get_config(par_num);
            if (true == par_cfg->persistent)
            {
                par_status_t store_status = ePAR_OK;

                par_get(par_num, (uint32_t *)&obj_data.data);
                obj_data.id = par_cfg->id;
                /* size is a descriptor/check field; current fixed-slot format always stores 4. */
                obj_data.size = PAR_NVM_DATA_SLOT_SIZE;
                obj_data.crc = par_nvm_calc_obj_crc(&obj_data);
                par_addr = par_nvm_get_nvm_lut_addr(obj_data.id);
                store_status = gp_store->write(par_addr,
                                               (uint32_t)sizeof(par_nvm_data_obj_t),
                                               (const uint8_t *)&obj_data);
                if (ePAR_OK != store_status)
                {
                    status |= ePAR_ERROR_NVM;
                    PAR_DBG_PRINT("PAR_NVM: parameter write failed, par_num=%u id=%u addr=0x%08lX err=%u",
                                  (unsigned)par_num,
                                  (unsigned)obj_data.id,
                                  (unsigned long)par_addr,
                                  (unsigned)store_status);
                }

                if ((true == nvm_sync) && (ePAR_OK == (status & ePAR_STATUS_ERROR_MASK)))
                {
                    const par_status_t sync_status = gp_store->sync();
                    if (ePAR_OK != sync_status)
                    {
                        status |= ePAR_ERROR_NVM;
                        PAR_DBG_PRINT("PAR_NVM: sync failed after parameter write, par_num=%u err=%u",
                                      (unsigned)par_num,
                                      (unsigned)sync_status);
                    }
                }
            }
            else
            {
                status = ePAR_ERROR;
            }
        }
        else
        {
            status = ePAR_ERROR;
        }
    }
    else
    {
        status = ePAR_ERROR_INIT;
    }

    return status;
}
/**
 * @brief Store all parameter value to NVM.
 *
 * @return Status of operation.
 */
par_status_t par_nvm_write_all(void)
{
    par_status_t status = ePAR_OK;

    PAR_ASSERT(true == gb_is_init);

    if (true == gb_is_init)
    {
        /* Mark the header invalid before bulk rewrite and commit that state. */
        {
            const par_status_t store_status = gp_store->erase(PAR_NVM_HEAD_SIGN_ADDR, PAR_NVM_HEAD_SIGN_SIZE);
            if (ePAR_OK != store_status)
            {
                status |= ePAR_ERROR_NVM;
                PAR_DBG_PRINT("PAR_NVM: signature erase failed, %u", (unsigned)store_status);
            }
        }

        if (ePAR_OK == (status & ePAR_STATUS_ERROR_MASK))
        {
            for (par_num_t par_num = 0U; par_num < ePAR_NUM_OF; par_num++)
            {
                if (true == par_is_persistent(par_num))
                {
                    status |= par_nvm_write(par_num, false);
                    if (ePAR_OK != (status & ePAR_STATUS_ERROR_MASK))
                    {
                        PAR_DBG_PRINT("PAR_NVM: bulk write aborted, par_num=%u id=%u addr=0x%08lX err=%u",
                                      (unsigned)par_num,
                                      (unsigned)par_get_config(par_num)->id,
                                      (unsigned long)par_nvm_get_nvm_lut_addr(par_get_config(par_num)->id),
                                      (unsigned)status);
                        break;
                    }
                }
            }
        }

        /* Restore a valid header only after the full rewrite completes successfully. */
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

        PAR_DBG_PRINT("PAR_NVM: Storing all to NVM status: %s", par_get_status_str(status));
    }
    else
    {
        status = ePAR_ERROR_INIT;
    }

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

    if (true == gb_is_init)
    {
        status |= par_nvm_write_all();
    }
    else
    {
        status = ePAR_ERROR_INIT;
    }

    return status;
}
/**
 * @} <!-- END GROUP -->
 */

#endif /* 1 == PAR_CFG_NVM_EN */
