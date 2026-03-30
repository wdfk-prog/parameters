/**
 * @file fnv.h
 * @brief Minimal FNV declarations required by the bundled hash_32a.c implementation.
 *
 * @note This header exists to support the public-domain lcn2/fnv hash_32a.c source.
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
