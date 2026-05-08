/**
 * @file par_nvm_table_id.c
 * @brief Implement the parameter-table ID hash adapter.
 * @author wdfk-prog
 * @version 1.1
 * @date 2026-04-11
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-03-30 1.0     wdfk-prog     first version
 * 2026-04-11 1.1     wdfk-prog     add stored-prefix table-ID calculation
 */
#include "par.h"
#include "fnv.h"
#include "par_nvm_table_id.h"
#include "par_object.h"

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
 * @brief Calculate the live compatibility digest for one stored persistent prefix.
 *
 * @details The digest covers only metadata that affects the managed NVM image
 * compatibility of the stored persistent prefix: schema version, selected
 * record layout, stored scalar persistent count, persistent-parameter order,
 * and parameter type. Self-describing layouts additionally hash the external
 * parameter ID. Payload-only layouts intentionally exclude external parameter
 * IDs and rely on PAR_CFG_TABLE_ID_SCHEMA_VER to invalidate semantic-only
 * prefix remaps that keep the same byte layout. Under the single-target
 * native-endian profile, each scalar is hashed exactly as it is represented in
 * memory on the running platform.
 *
 * @param persistent_count Number of persistent slots covered by the digest.
 * @return Live digest for that stored prefix.
 */
uint32_t par_nvm_table_id_calc_for_count(const uint16_t persistent_count)
{
    Fnv32_t hval = FNV1_32A_INIT;
    uint32_t serialized_size = 0U;
    uint16_t hashed_count = 0U;
    const uint32_t schema_version = (uint32_t)PAR_CFG_TABLE_ID_SCHEMA_VER;
    const uint32_t record_layout = (uint32_t)PAR_CFG_NVM_RECORD_LAYOUT;

    PAR_ASSERT(persistent_count <= (uint16_t)PAR_PERSISTENT_COMPILE_COUNT);

    par_nvm_table_id_hash_update(&hval, &serialized_size, &schema_version, (uint32_t)sizeof(schema_version));
    par_nvm_table_id_hash_update(&hval, &serialized_size, &record_layout, (uint32_t)sizeof(record_layout));
    par_nvm_table_id_hash_update(&hval, &serialized_size, &persistent_count, (uint32_t)sizeof(persistent_count));

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

        par_nvm_table_id_hash_update(&hval, &serialized_size, &type, (uint32_t)sizeof(type));

#if (1 == PAR_CFG_NVM_RECORD_LAYOUT_HAS_STORED_ID)
        {
            const uint16_t parameter_id = par_cfg_get_param_id_const(par_num);
            par_nvm_table_id_hash_update(&hval, &serialized_size, &parameter_id, (uint32_t)sizeof(parameter_id));
        }
#endif /* (1 == PAR_CFG_NVM_RECORD_LAYOUT_HAS_STORED_ID) */
        hashed_count++;
    }

    PAR_ASSERT(hashed_count == persistent_count);
    PAR_ASSERT(serialized_size > 0U);
    return (uint32_t)hval;
}
