/**
 * @file fnv.h
 * @brief Declare the bundled 32-bit FNV-1a hash helpers used by table-ID support.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-04-19
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-04-19 1.0     wdfk-prog    first version
 */
#ifndef _PAR_PERSIST_FNV_H_
#define _PAR_PERSIST_FNV_H_

#include <stddef.h>
#include <stdint.h>

typedef uint32_t Fnv32_t;

#define FNV1_32A_INIT ((Fnv32_t)0x811C9DC5U)

Fnv32_t fnv_32a_buf(void *buf, size_t len, Fnv32_t hval);
Fnv32_t fnv_32a_str(char *str, Fnv32_t hval);

#endif /* _PAR_PERSIST_FNV_H_ */
