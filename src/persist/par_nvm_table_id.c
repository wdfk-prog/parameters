/**
 * @file par_nvm_table_id.c
 * @brief Implement the parameter-table ID hash adapter.
 * @author wdfk-prog ()
 * @version 1.0
 * @date 2026-03-30
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author      Description
 * 2026-03-30 1.0     wdfk-prog   first version
 */

#include "par.h"
#include "persist/fnv.h"
#include "persist/par_nvm_table_id.h"

/**
 * @brief Serialized table-ID record size for one persisted parameter.
 */
enum
{
    PAR_NVM_TABLE_ID_REC_SIZE = sizeof(((par_cfg_t *)0)->type)
#if (1 == PAR_CFG_ENABLE_ID)
                                + sizeof(((par_cfg_t *)0)->id)
#endif
};

/**
 * @brief Update FNV-1a context with one platform-native scalar image.
 *
 * @param p_hval Pointer to rolling FNV-1a state.
 * @param p_serialized_size Pointer to serialized byte counter.
 * @param p_value Pointer to source scalar.
 * @param value_size Scalar width in bytes. Supported values: 1, 2, 4.
 */
static void par_nvm_table_id_hash_update(Fnv32_t * const p_hval,
                                         uint32_t * const p_serialized_size,
                                         const void * const p_value,
                                         const uint32_t value_size)
{
    *p_hval = fnv_32a_buf((void *)p_value, (size_t)value_size, *p_hval);
    *p_serialized_size += value_size;
}

/**
 * @brief Calculate live parameter-table ID.
 *
 * @details The digest covers only metadata that affects the NVM storage
 * compatibility of persisted parameters: schema version, persisted-parameter
 * count, persisted-parameter order, type, and optional ID field. Under the
 * single-target native-endian profile, each scalar is hashed exactly as it is
 * represented in memory on the running platform.
 */
uint32_t par_nvm_table_id_calc(void)
{
    Fnv32_t hval = FNV1_32A_INIT;
    uint32_t serialized_size = 0U;
    const uint32_t schema_version = (uint32_t)PAR_CFG_TABLE_ID_SCHEMA_VER;
    const uint16_t persistent_count = (uint16_t)PAR_PERSISTENT_COMPILE_COUNT;
    const uint32_t expected_size = (uint32_t)sizeof(schema_version) + (uint32_t)sizeof(persistent_count) + ((uint32_t)persistent_count * (uint32_t)PAR_NVM_TABLE_ID_REC_SIZE);

    par_nvm_table_id_hash_update(&hval, &serialized_size, &schema_version, (uint32_t)sizeof(schema_version));
    par_nvm_table_id_hash_update(&hval, &serialized_size, &persistent_count, (uint32_t)sizeof(persistent_count));

    for (par_num_t par_num = 0U; par_num < ePAR_NUM_OF; par_num++)
    {
        const par_cfg_t * const p_cfg = par_get_config(par_num);
        const uint8_t type = (uint8_t)p_cfg->type;

#if (1 == PAR_CFG_NVM_EN)
        if (false == p_cfg->persistent)
        {
            continue;
        }
#endif

        par_nvm_table_id_hash_update(&hval, &serialized_size, &type, (uint32_t)sizeof(type));
#if (1 == PAR_CFG_ENABLE_ID)
        par_nvm_table_id_hash_update(&hval, &serialized_size, &p_cfg->id, (uint32_t)sizeof(p_cfg->id));
#endif
    }

    PAR_ASSERT(serialized_size == expected_size);
    return (uint32_t)hval;
}
