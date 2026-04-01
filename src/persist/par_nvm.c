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
 * @brief Parameter NVM LUT talbe.
 */
typedef struct
{
    uint32_t addr;  /**< Start address of parameter. */
    uint16_t id;    /**< ID of stored parameter. */
    bool valid;     /**< Valid entry. */
} par_nvm_lut_t;
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
 * @brief Parameter NVM lut.
 */
static par_nvm_lut_t g_par_nvm_data_obj_addr[ePAR_NUM_OF] = { 0 };
/**
 * @brief Function declarations.
 */
static uint16_t par_nvm_calc_head_crc(const par_nvm_head_obj_t * const p_head_obj);
static uint8_t par_nvm_calc_obj_crc(const par_nvm_data_obj_t * const p_obj);
static bool par_nvm_is_in_nvm_lut(const uint16_t id);
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
 * @brief Load all parameters value from NVM.
 *
 * @param num_of_par Number of stored parameters inside NVM.
 * @return Status of operation.
 */
static par_status_t par_nvm_load_all(const uint16_t num_of_par)
{
    par_status_t status = ePAR_OK;
    par_num_t par_num = 0;
    uint16_t i = 0;
    uint32_t obj_addr = 0;
    par_nvm_data_obj_t obj_data = { 0 };
    par_status_t store_status = ePAR_OK;
    uint8_t crc_calc = 0;
    uint16_t per_par_nb = 0;
    uint16_t new_par_cnt = 0;
    for (i = 0; i < num_of_par; i++)
    {
        /* NVM persistence is a dense linear array of fixed 8-byte objects. */
        obj_addr = ((PAR_NVM_DATA_OBJ_STRIDE * i) + PAR_NVM_FIRST_DATA_OBJ_ADDR);
        store_status = gp_store->read(obj_addr, (uint32_t)sizeof(par_nvm_data_obj_t), (uint8_t *)&obj_data);
        if (ePAR_OK == store_status)
        {
            crc_calc = par_nvm_calc_obj_crc(&obj_data);
            if (crc_calc == obj_data.crc)
            {
                if (ePAR_OK == par_get_num_by_id(obj_data.id, &par_num))
                {
                    /* Restore only parameters that still exist and remain persistent. */
                    if (true == par_get_config(par_num)->persistent)
                    {
                        if (false == par_nvm_is_in_nvm_lut(obj_data.id))
                        {
                            g_par_nvm_data_obj_addr[per_par_nb].id = obj_data.id;
                            g_par_nvm_data_obj_addr[per_par_nb].addr = obj_addr;
                            g_par_nvm_data_obj_addr[per_par_nb].valid = true;
                            /* Restore through the internal fast path. */
                            (void)par_set_fast(par_num, &obj_data.data);
                            per_par_nb++;
                        }
                    }
                }
            }

            else
            {
                status = ePAR_ERROR_CRC;
                break;
            }
        }
        else
        {
            status = ePAR_ERROR_NVM;
            PAR_DBG_PRINT("PAR_NVM: object read failed, %u", (unsigned)store_status);
            break;
        }
    }

    PAR_DBG_PRINT("PAR_NVM: Loading all persistent parameters with status: %s", par_get_status_str(status));
    PAR_DBG_PRINT("PAR_NVM: Nb. of stored pars in NVM: %d", num_of_par);
    PAR_DBG_PRINT("PAR_NVM: Nb. of live persistent: \t%d", PAR_PERSISTENT_COMPILE_COUNT);
    if (ePAR_OK == status)
    {
        for (i = 0; i < ePAR_NUM_OF; i++)
        {
            const par_cfg_t * const par_cfg = par_get_config(i);

            if (true == par_cfg->persistent)
            {
                if (false == par_nvm_is_in_nvm_lut(par_cfg->id))
                {
                    /* Extend the LUT for newly added persistent parameters. */
                    g_par_nvm_data_obj_addr[per_par_nb].id = par_cfg->id;
                    g_par_nvm_data_obj_addr[per_par_nb].addr = obj_addr + (PAR_NVM_DATA_OBJ_STRIDE * (new_par_cnt + 1U));
                    g_par_nvm_data_obj_addr[per_par_nb].valid = true;
                    par_save(i);

                    per_par_nb++;
                    new_par_cnt++;
                }
            }
        }

        if (new_par_cnt > 0)
        {
            /* The object count only grows when new persistent parameters appear. */
            status |= par_nvm_write_header(num_of_par + new_par_cnt);
            if (ePAR_OK == (status & ePAR_STATUS_ERROR_MASK))
            {
                const par_status_t sync_status = gp_store->sync();
                if (ePAR_OK != sync_status)
                {
                    status |= ePAR_ERROR_NVM;
                    PAR_DBG_PRINT("PAR_NVM: sync failed, %u", (unsigned)sync_status);
                }
            }

#if (PAR_CFG_DEBUG_EN)
            PAR_DBG_PRINT("PAR_NVM: Added %d new parameters to NVM LUT table!", new_par_cnt);
#endif
        }
    }

    return status;
}
/**
 * @brief Build a fresh NVM lookup table from the live persistent set.
 *
 * @details The LUT maps external parameter IDs to their fixed-record address in
 * the linear persisted-object array. This keeps load/save paths anchored on ID
 * instead of assuming that a given parameter meaning is tied permanently to one
 * absolute slot position.
 */
static void par_nvm_build_new_nvm_lut(void)
{
    uint16_t per_par_nb = 0U;
    for (par_num_t par_num = 0; par_num < ePAR_NUM_OF; par_num++)
    {
        const par_cfg_t * const par_cfg = par_get_config(par_num);

        if (true == par_cfg->persistent)
        {
            if (0 == per_par_nb)
            {
                g_par_nvm_data_obj_addr[per_par_nb].addr = PAR_NVM_FIRST_DATA_OBJ_ADDR;
            }

            /* NVM persistence uses a linear list of fixed-size 8-byte objects. */
            else
            {
                g_par_nvm_data_obj_addr[per_par_nb].addr = (g_par_nvm_data_obj_addr[per_par_nb - 1].addr + PAR_NVM_DATA_OBJ_STRIDE);
            }

            g_par_nvm_data_obj_addr[per_par_nb].id = par_cfg->id;
            g_par_nvm_data_obj_addr[per_par_nb].valid = true;
            per_par_nb++;
        }
    }

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
    for (par_num_t par_num = 0; par_num < ePAR_NUM_OF; par_num++)
    {
        if (id == g_par_nvm_data_obj_addr[par_num].id)
        {
            return g_par_nvm_data_obj_addr[par_num].addr;
        }
    }

    return 0;
}
/**
 * @brief Check if parameter is in NVM LUT.
 *
 * @param id Parameter ID.
 * @return Flag that indicated if object is in NVM lut.
 */
static bool par_nvm_is_in_nvm_lut(const uint16_t id)
{
    for (par_num_t par_num = 0; par_num < ePAR_NUM_OF; par_num++)
    {
        if ((id == g_par_nvm_data_obj_addr[par_num].id) && (true == g_par_nvm_data_obj_addr[par_num].valid))
        {
            return true;
        }
    }

    return false;
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
        par_nvm_build_new_nvm_lut();
        status |= par_nvm_write_all();
    }
    else
    {
        status = ePAR_ERROR_INIT;
    }

    return status;
}
/**
 * @brief Print parameter NVM table.
 *
 * @note Only for debugging purposes.
 */
par_status_t par_nvm_print_nvm_lut(void)
{
    par_status_t status = ePAR_OK;

#if (PAR_CFG_DEBUG_EN)
    PAR_DBG_PRINT("PAR_NVM: Parameter NVM look-up table:");
    PAR_DBG_PRINT(" %s\t%s\t%s\t\t%s", "#", "ID", "Addr", "Valid");
    PAR_DBG_PRINT("===============================");

    for (par_num_t par_num = 0; par_num < ePAR_NUM_OF; par_num++)
    {
        PAR_DBG_PRINT(" %d\t%d\t0x%04X\t%d", par_num, g_par_nvm_data_obj_addr[par_num].id,
                      g_par_nvm_data_obj_addr[par_num].addr,
                      g_par_nvm_data_obj_addr[par_num].valid);
        PAR_DBG_PRINT("-----------------------------");
    }
#endif

    return status;
}
/**
 * @} <!-- END GROUP -->
 */

#endif /* 1 == PAR_CFG_NVM_EN */
