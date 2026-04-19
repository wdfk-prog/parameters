/**
 * @file par_nvm_table_id.h
 * @brief Declare the parameter-table ID hash adapter.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-03-30
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-03-30 1.0     wdfk-prog     first version
 */
#ifndef _PAR_NVM_TABLE_ID_H_
#define _PAR_NVM_TABLE_ID_H_

#include <stdint.h>
/**
 * @brief Calculate the live compatibility digest for one stored persistent prefix.
 *
 * @details The caller provides the number of stored persistent slots that are
 * part of the managed NVM image. The digest always covers that stored prefix.
 * Layouts with stored IDs additionally hash external parameter IDs, while
 * payload-only layouts intentionally exclude external parameter IDs and track
 * only byte-layout compatibility for that prefix.
 *
 * @param persistent_count Number of persistent slots covered by the digest.
 * @return Platform-native 32-bit FNV-1a digest.
 */
uint32_t par_nvm_table_id_calc_for_count(uint16_t persistent_count);

#endif /* _PAR_NVM_TABLE_ID_H_ */
