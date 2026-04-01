/**
 * @file par_nvm_table_id.h
 * @brief Declare the parameter-table ID hash adapter.
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
#ifndef _PAR_NVM_TABLE_ID_H_
#define _PAR_NVM_TABLE_ID_H_

#include <stdint.h>
/**
 * @brief Calculate the live parameter-table ID.
 *
 * @details The digest covers only metadata that changes the binary
 * compatibility of the persisted NVM image:
 * - PAR_CFG_TABLE_ID_SCHEMA_VER
 * - persistent-parameter count
 * - persistent-parameter order
 * - parameter type
 * - parameter ID
 *
 * Default values, ranges, names, units, descriptions, and access flags are
 * intentionally excluded because they do not change the serialized NVM object
 * layout used by par_nvm.c.
 *
 * @return Platform-native 32-bit FNV-1a digest.
 */
uint32_t par_nvm_table_id_calc(void);

#endif /* _PAR_NVM_TABLE_ID_H_ */
