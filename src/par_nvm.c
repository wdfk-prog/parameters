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
 * Parameters stored into NVM in little endianness format.
 *
 * For details how parameters are handled in NVM go look at the.
 * documentation.
 *
 * @note RULES OF "PAR_CFG_TABLE_ID_CHECK_EN" SETTINGS:
 *
 * It is normal that parameter table will change during development.
 * and therefore code will detect table change between "RAM" and "NVM".
 * (detection of par table change enable/disable with.
 * "PAR_CFG_TABLE_ID_CHECK_EN" setting).
 *
 * But as SW is released and potential at customer, developer must not.
 * change the pre-existing parameter table as it will loose all values.
 * stored inside NVM. Therefore it is recommended to disable table ID.
 * detection after first release of SW. Adding new parameters to pre-existing.
 * table has no harm at all nor does it have any side effects.
 */
/**
 * @brief Include dependencies.
 */
#include "par_nvm.h"
#include "par_cfg.h"
#include "par_if.h"

#if ( 1 == PAR_CFG_NVM_EN )

#include <assert.h>
#include <string.h>

#include "middleware/nvm/nvm/src/nvm.h"

/**
 * @brief Check NVM module compatibility.
 */
_Static_assert(2 == NVM_VER_MAJOR);
_Static_assert(1 <= NVM_VER_MINOR);
/**
 * @brief Compile-time definitions.
 */
/**
 * @brief Parameter signature and size in bytes.
 */
#define PAR_NVM_SIGN      (0xFF00AA55)
#define PAR_NVM_SIGN_SIZE (4U)

/**
 * @brief Parameter header number of object size.
 * @details Unit: byte.
 */
#define PAR_NVM_NB_OF_OBJ_SIZE (2U)

/**
 * @brief Parameter CRC size.
 * @details Unit: byte.
 */
#define PAR_NVM_CRC_SIZE (2U)

/**
 * @brief Parameter configuration hash size.
 * @details Unit: byte.
 */
#define PAR_NVM_HASH_SIZE (32U)

/**
 * @brief Parameter NVM header content address start.
 *
 * @note This is offset to reserved NVM region. For absolute address.
 * add that value to NVM start region.
 */
#define PAR_NVM_HEAD_ADDR           (0x00)
#define PAR_NVM_HEAD_SIGN_ADDR      (PAR_NVM_HEAD_ADDR)
#define PAR_NVM_HEAD_NB_OF_OBJ_ADDR (PAR_NVM_HEAD_SIGN_ADDR + PAR_NVM_SIGN_SIZE)
#define PAR_NVM_HEAD_CRC_ADDR       (PAR_NVM_HEAD_NB_OF_OBJ_ADDR + PAR_NVM_NB_OF_OBJ_SIZE)
#define PAR_NVM_HEAD_HASH_ADDR      (PAR_NVM_HEAD_CRC_ADDR + PAR_NVM_CRC_SIZE)

/**
 * @brief Parameters first data object start address.
 * @details Unit: byte.
 */
#define PAR_NVM_FIRST_DATA_OBJ_ADDR (PAR_NVM_HEAD_HASH_ADDR + PAR_NVM_HASH_SIZE)

/**
 * @brief Parameter NVM header object.
 */
typedef struct
{
    uint32_t sign;      /**< Signature. */
    uint16_t obj_nb;    /**< Stored data object number. */
    uint16_t crc;       /**< Header CRC. */
} par_nvm_head_obj_t;
/**
 * @brief Parameter NVM data object.
 */
typedef struct
{
    uint16_t id;        /**< Parameter ID. */
    uint8_t size;       /**< Size of parameter data block. */
    uint8_t crc;        /**< CRC of parameter value. */
    par_type_t data;    /**< 4-byte storage for parameter value. */
} par_nvm_data_obj_t;
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
 * @brief Ownership guard for the underlying NVM module.
 */
static bool gb_is_nvm_owner = false;
/**
 * @brief Parameter NVM lut.
 */
static par_nvm_lut_t g_par_nvm_data_obj_addr[ePAR_NUM_OF] = { 0 };
/**
 * @brief Function declarations.
 */
static par_status_t par_nvm_load_all(const uint16_t num_of_par);
static par_status_t par_nvm_restore_fast(const par_num_t par_num, const void *p_val);

static par_status_t par_nvm_corrupt_signature(void);
static par_status_t par_nvm_read_header(par_nvm_head_obj_t * const p_head_obj);
static par_status_t par_nvm_write_header(const uint16_t num_of_par);
static par_status_t par_nvm_validate_header(uint16_t * const p_num_of_par);

static uint16_t par_nvm_calc_crc(const uint8_t * const p_data, const uint8_t size);
static uint8_t par_nvm_calc_obj_crc(const par_nvm_data_obj_t * const p_obj);
static uint16_t par_nvm_get_per_par(void);

static void par_nvm_build_new_nvm_lut(void);
static uint32_t par_nvm_get_nvm_lut_addr(const uint16_t id);
static bool par_nvm_is_in_nvm_lut(const uint16_t id);

#if (1 == PAR_CFG_TABLE_ID_CHECK_EN)
static par_status_t par_nvm_erase_signature(void);
static par_status_t par_nvm_check_table_id(const uint8_t * const p_table_id);
static par_status_t par_nvm_write_table_id(const uint8_t * const p_table_id);
#endif

static par_status_t par_nvm_init_nvm(void);
static par_status_t par_nvm_sync(void);
/**
 * @brief Function declarations and definitions.
 */
static par_status_t par_nvm_restore_fast(const par_num_t par_num, const void *p_val)
{
    if (NULL == p_val)
    {
        return ePAR_ERROR_PARAM;
    }

    switch (par_get_type(par_num))
    {
    case ePAR_TYPE_U8:
        return par_set_u8_fast(par_num, *(const uint8_t *)p_val);

    case ePAR_TYPE_I8:
        return par_set_i8_fast(par_num, *(const int8_t *)p_val);

    case ePAR_TYPE_U16:
        return par_set_u16_fast(par_num, *(const uint16_t *)p_val);

    case ePAR_TYPE_I16:
        return par_set_i16_fast(par_num, *(const int16_t *)p_val);

    case ePAR_TYPE_U32:
        return par_set_u32_fast(par_num, *(const uint32_t *)p_val);

    case ePAR_TYPE_I32:
        return par_set_i32_fast(par_num, *(const int32_t *)p_val);

#if (1 == PAR_CFG_ENABLE_TYPE_F32)
    case ePAR_TYPE_F32:
        return par_set_f32_fast(par_num, *(const float32_t *)p_val);
#endif

    case ePAR_TYPE_NUM_OF:
    default:
        return ePAR_ERROR_TYPE;
    }
}
/**
 * Corrupt parameter signature to NVM.
 *
 * @brief Return ePAR_OK if signature corrupted OK. In case of NVM error it returns.
 * ePAR_ERROR_NVM.
 *
 * @return Status of operation.
 */
static par_status_t par_nvm_corrupt_signature(void)
{
    par_status_t status = ePAR_OK;

    if (eNVM_OK != nvm_erase(PAR_CFG_NVM_REGION, PAR_NVM_HEAD_SIGN_ADDR, PAR_NVM_SIGN_SIZE))
    {
        status = ePAR_ERROR_NVM;
        PAR_DBG_PRINT("PAR_NVM: NVM error during signature corruption!");
    }

    return status;
}

#if (1 == PAR_CFG_TABLE_ID_CHECK_EN)
/**
 * @brief Erase parameter signature from NVM.
 *
 * @return Status of operation.
 */
static par_status_t par_nvm_erase_signature(void)
{
    par_status_t status = ePAR_OK;

    status = nvm_erase(PAR_CFG_NVM_REGION, PAR_NVM_SIGNATURE_ADDR_OFFSET, 4U);

    return status;
}
/**
 * Check unique parameter table ID.
 *
 * @brief This function check for parameter configuration table change while.
 * some parameters are already stored in NVM. First it read stored.
 * table ID from NVM and then compare it with current table ID (live.
 * from RAM). In case of mismatched it return error.
 *
 * Table ID is being calculated based on hash algorithm (SHA-256).
 *
 * @param p_table_id Pointer to reference table ID (in "RAM").
 * @return Status of operation.
 */
static par_status_t par_nvm_check_table_id(const uint8_t * const p_table_id)
{
    par_status_t status = ePAR_OK;
    uint8_t nvm_table_id[32] = { 0 };

    if (eNVM_OK != nvm_read(eNVM_REGION_EEPROM_RUN_PAR, PAR_NVM_TABLE_ID_ADDR_OFFSET, 32U, (uint8_t *)&nvm_table_id))
    {
        status = ePAR_ERROR_NVM;
    }
    else
    {
        if (0 == memcmp(&nvm_table_id, p_table_id, 32U))
        {
            status = ePAR_OK;
        }

        else
        {
            status = ePAR_ERROR;
        }
    }

    return status;
}
/**
 * @brief Write unique parameter table ID to NVM.
 *
 * @param p_table_id Pointer to reference table ID (in "RAM").
 * @return Status of operation.
 */
static par_status_t par_nvm_write_table_id(const uint8_t * const p_table_id)
{
    par_status_t status = ePAR_OK;

    if (eNVM_OK != nvm_write(eNVM_REGION_EEPROM_RUN_PAR, PAR_NVM_TABLE_ID_ADDR_OFFSET, 32U, p_table_id))
    {
        status = ePAR_ERROR_NVM;
    }

    return status;
}

#endif /* 1 == PAR_CFG_TABLE_ID_CHECK_EN */
/**
 * @brief Read parameter NVM header.
 *
 * @param p_head_obj Pointer to parameter NVM header.
 * @return Status of operation.
 */
static par_status_t par_nvm_read_header(par_nvm_head_obj_t * const p_head_obj)
{
    par_status_t status = ePAR_OK;

    PAR_ASSERT(NULL != p_head_obj);

    if (eNVM_OK != nvm_read(PAR_CFG_NVM_REGION, PAR_NVM_HEAD_ADDR, sizeof(par_nvm_head_obj_t), (uint8_t *)p_head_obj))
    {
        status = ePAR_ERROR_NVM;
        PAR_DBG_PRINT("PAR_NVM: NVM error during header read!");
    }

    return status;
}
/**
 * @brief Write parameter NVM header.
 *
 * @param num_of_par Number of persistent parameters that are stored in NVM.
 * @return Status of operation.
 */
static par_status_t par_nvm_write_header(const uint16_t num_of_par)
{
    par_status_t status = ePAR_OK;
    par_nvm_head_obj_t head_obj = { 0 };
    head_obj.obj_nb = num_of_par;
    head_obj.crc = par_nvm_calc_crc((uint8_t *)&head_obj.obj_nb, PAR_NVM_NB_OF_OBJ_SIZE);
    head_obj.sign = PAR_NVM_SIGN;
    if (eNVM_OK != nvm_write(PAR_CFG_NVM_REGION, PAR_NVM_HEAD_ADDR, sizeof(par_nvm_head_obj_t), (const uint8_t *)&head_obj))
    {
        status = ePAR_ERROR_NVM;
        PAR_DBG_PRINT("PAR_NVM: NVM error during header write!");
    }

    PAR_DBG_PRINT("PAR_NVM: Write NVM header with %d nb. of object", num_of_par);

    return status;
}
/**
 * @brief Validate parameter NVM header.
 *
 * @param p_num_of_par Pointer to number of persistent parameters that are stored in NVM.
 * @return Status of operation.
 */
static par_status_t par_nvm_validate_header(uint16_t * const p_num_of_par)
{
    par_status_t status = ePAR_OK;
    par_nvm_head_obj_t obj_head = { 0 };
    uint16_t crc_calc = 0;
    status = par_nvm_read_header(&obj_head);
    if (ePAR_ERROR_NVM != status)
    {
        if (PAR_NVM_SIGN == obj_head.sign)
        {
            crc_calc = par_nvm_calc_crc((uint8_t *)&obj_head.obj_nb, PAR_NVM_NB_OF_OBJ_SIZE);
            if (crc_calc == obj_head.crc)
            {
                *p_num_of_par = obj_head.obj_nb;
                PAR_DBG_PRINT("PAR_NVM: HVM header OK! Nb. of stored obj: %d", obj_head.obj_nb);
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
 * @brief Calculate CRC-16.
 *
 * @param p_data Pointer to data.
 * @param size Size of data to calc crc.
 * @return Calculated CRC.
 */
static uint16_t par_nvm_calc_crc(const uint8_t * const p_data, const uint8_t size)
{
    const uint16_t poly = 0x1021U;    // CRC-16-CCITT
    const uint16_t seed = 0x1234U;    // Custom seed
    uint16_t crc16 = seed;
    PAR_ASSERT(NULL != p_data);
    PAR_ASSERT(size > 0);

    for (uint8_t i = 0; i < size; i++)
    {
        crc16 = (crc16 ^ (p_data[i] << 8U));

        for (uint8_t j = 0U; j < 8U; j++)
        {
            if (crc16 & 0x8000)
            {
                crc16 = ((crc16 << 1U) ^ poly);
            }
            else
            {
                crc16 = (crc16 << 1U);
            }
        }
    }

    return crc16;
}
/**
 * @brief Calculate parameter data object CRC.
 *
 * @param p_obj Pointer to data object.
 * @return Calculated CRC.
 */
static uint8_t par_nvm_calc_obj_crc(const par_nvm_data_obj_t * const p_obj)
{
    uint16_t crc = 0;
    uint8_t rtn_crc = 0;

    crc = par_nvm_calc_crc((const uint8_t *)&p_obj->id, 2U);
    crc ^= par_nvm_calc_crc((const uint8_t *)&p_obj->size, 1U);
    crc ^= par_nvm_calc_crc((const uint8_t *)&p_obj->data.u8, 4U);
    rtn_crc = (crc & 0xFFU);

    return rtn_crc;
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
    nvm_status_t nvm_status = eNVM_OK;
    uint8_t crc_calc = 0;
    uint16_t per_par_nb = 0;
    uint16_t new_par_cnt = 0;
    for (i = 0; i < num_of_par; i++)
    {
        /* Each NVM object currently occupies 8 bytes. */
        obj_addr = ((8 * i) + PAR_NVM_FIRST_DATA_OBJ_ADDR);
        nvm_status = nvm_read(PAR_CFG_NVM_REGION, obj_addr, sizeof(par_nvm_data_obj_t), (uint8_t *)&obj_data);
        if (eNVM_OK == nvm_status)
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
                            (void)par_nvm_restore_fast(par_num, &obj_data.data);
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
            break;
        }
    }

    PAR_DBG_PRINT("PAR_NVM: Loading all persistent parameters with status: %s", par_get_status_str(status));
    PAR_DBG_PRINT("PAR_NVM: Nb. of stored pars in NVM: %d", num_of_par);
    PAR_DBG_PRINT("PAR_NVM: Nb. of live persistent: \t%d", par_nvm_get_per_par());
    if (ePAR_OK == status)
    {
        for (i = 0; i < ePAR_NUM_OF; i++)
        {
            const par_cfg_t * const par_cfg = par_get_config(par_num);

            if (true == par_cfg->persistent)
            {
                if (false == par_nvm_is_in_nvm_lut(par_cfg->id))
                {
                    /* Extend the LUT for newly added persistent parameters. */
                    g_par_nvm_data_obj_addr[per_par_nb].id = par_cfg->id;
                    g_par_nvm_data_obj_addr[per_par_nb].addr = obj_addr + (8U * (new_par_cnt + 1U));
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
            status |= par_nvm_sync();

#if (PAR_CFG_DEBUG_EN)
            PAR_DBG_PRINT("PAR_NVM: Added %d new parameters to NVM LUT table!", new_par_cnt);
#endif
        }
    }

    return status;
}
/**
 * @brief Get total number of persistent parameters.
 *
 * @return Number of persistent parameters.
 */
static uint16_t par_nvm_get_per_par(void)
{
    uint16_t num_of_per_par = 0U;

    for (par_num_t par_num = 0; par_num < ePAR_NUM_OF; par_num++)
    {
        if (true == par_get_config(par_num)->persistent)
        {
            num_of_per_par++;
        }
    }

    return num_of_per_par;
}
/**
 * @brief Build new parameter NVM LUT table.
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

            /* NVM data objects currently use a fixed 8-byte stride. */
            else
            {
                g_par_nvm_data_obj_addr[per_par_nb].addr = (g_par_nvm_data_obj_addr[per_par_nb - 1].addr + 8U);
            }

            g_par_nvm_data_obj_addr[per_par_nb].id = par_cfg->id;
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
 * @brief Initialize NVM module.
 *
 * @return Status of operation.
 */
static par_status_t par_nvm_init_nvm(void)
{
    par_status_t status = ePAR_OK;
    bool is_nvm_init = false;

    gb_is_nvm_owner = false;
    (void)nvm_is_init(&is_nvm_init);
    if (false == is_nvm_init)
    {
        if (eNVM_OK != nvm_init())
        {
            status = ePAR_ERROR_INIT;
            PAR_DBG_PRINT("PAR_NVM: NVM module init error!");
        }
        else
        {
            gb_is_nvm_owner = true;
        }
    }

    return status;
}
/**
 * @brief Sync NVM module.
 *
 * @return Status of operation.
 */
static par_status_t par_nvm_sync(void)
{
    par_status_t status = ePAR_OK;

    if (eNVM_OK != nvm_sync(PAR_CFG_NVM_REGION))
    {
        status = ePAR_ERROR_NVM;
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
 * @return Status of initialization.
 */
par_status_t par_nvm_init(void)
{
    par_status_t status = ePAR_OK;
    uint16_t obj_nb = 0U;
    uint16_t per_par_nb = 0U;
    status = par_nvm_init_nvm();
    if (ePAR_OK == status)
    {
        gb_is_init = true;
        per_par_nb = par_nvm_get_per_par();
        if (per_par_nb > 0)
        {
            status = par_nvm_validate_header(&obj_nb);
            if (ePAR_OK == status)
            {
#if (PAR_CFG_TABLE_ID_CHECK_EN)
                    /* TODO: add table-ID consistency check. */
#endif

                status = par_nvm_load_all(obj_nb);
                if (ePAR_ERROR_CRC == status)
                {
                    status = par_nvm_reset_all();
                    status |= (ePAR_WAR_SET_TO_DEF | ePAR_WAR_NVM_REWRITTEN);
                }

                else if (ePAR_ERROR_NVM == status)
                {
                    /*
                     * Fall back to defaults if NVM contents are only partially
                     * usable. A mixed state of restored and default values is
                     * unsafe.
                     */
                    par_set_all_to_default();
                    status |= ePAR_WAR_SET_TO_DEF;
                }
            }

            else if ((ePAR_ERROR == status) || (ePAR_ERROR_CRC == status))
            {
                status = par_nvm_reset_all();
                status |= (ePAR_WAR_SET_TO_DEF | ePAR_WAR_NVM_REWRITTEN);
            }

        }

        else
        {
            status |= ePAR_WAR_NO_PERSISTENT;
            PAR_DBG_PRINT("PAR_NVM: No persistent parameters... Nothing to do...");
        }
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
            if (eNVM_OK != nvm_deinit())
            {
                status = ePAR_ERROR;
            }
        }

        if (ePAR_OK == status)
        {
            gb_is_init = false;
            gb_is_nvm_owner = false;
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
                par_get(par_num, (uint32_t *)&obj_data.data);
                obj_data.id = par_cfg->id;
                /* Current implementation stores fixed-size 8-byte objects. */
                obj_data.size = 4U;
                obj_data.crc = par_nvm_calc_obj_crc(&obj_data);
                par_addr = par_nvm_get_nvm_lut_addr(obj_data.id);
                if (eNVM_OK != nvm_write(PAR_CFG_NVM_REGION, par_addr, sizeof(par_nvm_data_obj_t), (const uint8_t *)&obj_data))
                {
                    status |= ePAR_ERROR_NVM;
                }

                if (true == nvm_sync)
                {
                    status |= par_nvm_sync();
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
        /* Mark the header invalid while rewriting the NVM image. */
        status |= par_nvm_corrupt_signature();
        for (par_num_t par_num = 0U; par_num < ePAR_NUM_OF; par_num++)
        {
            if (true == par_is_persistent(par_num))
            {
                status |= par_nvm_write(par_num, false);
            }
        }

        /* Restore a valid header after the bulk rewrite completes. */
        status |= par_nvm_write_header(par_nvm_get_per_par());
        status |= par_nvm_sync();

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
