/**
 * @file par_def.h
 * @brief Declare parameter-definition types and compile-time enumerations.
 * @author wdfk-prog ()
 * @version 1.0
 * @date 2026-03-27
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author      Description
 * 2026-03-27 1.0     wdfk-prog   first version
 */

/**
 * @addtogroup PAR_DEF
 * @{ <!-- BEGIN GROUP -->
 */

#ifndef _PAR_DEF_CORE_H_
#define _PAR_DEF_CORE_H_

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Include dependencies.
 */
#include <stdint.h>
#include <stdbool.h>
/**
 * @brief Compile-time definitions.
 *
 * @note <stdbool.h> is required here because par_table.def uses true/false in
 * the persistence column, and this header expands that column in
 * PAR_PERSISTENT_COMPILE_COUNT.
 */
typedef struct par_cfg_s par_cfg_t;
/**
 * @brief List of device parameters.
 * @note Must be started with 0! @note Enum expansion is intentionally configuration-independent: PAR_ITEM_F32 always maps to PAR_ITEM_ENUM. F32 enable/disable fail-fast is enforced in par_def.c.
 */
#define PAR_ITEM_ENUM(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_) enum_,
enum
{
#define PAR_ITEM_U8  PAR_ITEM_ENUM
#define PAR_ITEM_U16 PAR_ITEM_ENUM
#define PAR_ITEM_U32 PAR_ITEM_ENUM
#define PAR_ITEM_I8  PAR_ITEM_ENUM
#define PAR_ITEM_I16 PAR_ITEM_ENUM
#define PAR_ITEM_I32 PAR_ITEM_ENUM
#define PAR_ITEM_F32 PAR_ITEM_ENUM
#include "../../par_table.def"
#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32

    ePAR_NUM_OF
};
#undef PAR_ITEM_ENUM
typedef uint16_t par_num_t;
/**
 * @brief Compile-time storage group counts derived from par_table.def.
 * @note These constants are used by layout and static storage allocation.
 */
#define PAR_ITEM_COUNT_ONE(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)  +1u
#define PAR_ITEM_COUNT_ZERO(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_) +0u

enum
{
    PAR_LAYOUT_COMPILE_COUNT8 = 0u
#define PAR_ITEM_U8  PAR_ITEM_COUNT_ONE
#define PAR_ITEM_U16 PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_U32 PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_I8  PAR_ITEM_COUNT_ONE
#define PAR_ITEM_I16 PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_I32 PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_F32 PAR_ITEM_COUNT_ZERO
#include "../../par_table.def"
#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32
};

enum
{
    PAR_LAYOUT_COMPILE_COUNT16 = 0u
#define PAR_ITEM_U8  PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_U16 PAR_ITEM_COUNT_ONE
#define PAR_ITEM_U32 PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_I8  PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_I16 PAR_ITEM_COUNT_ONE
#define PAR_ITEM_I32 PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_F32 PAR_ITEM_COUNT_ZERO
#include "../../par_table.def"
#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32
};

enum
{
    PAR_LAYOUT_COMPILE_COUNT32 = 0u
#define PAR_ITEM_U8  PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_U16 PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_U32 PAR_ITEM_COUNT_ONE
#define PAR_ITEM_I8  PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_I16 PAR_ITEM_COUNT_ZERO
#define PAR_ITEM_I32 PAR_ITEM_COUNT_ONE
#define PAR_ITEM_F32 PAR_ITEM_COUNT_ONE
#include "../../par_table.def"
#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32
};

enum
{
    PAR_LAYOUT_COMPILE_COUNT_SUM = (PAR_LAYOUT_COMPILE_COUNT8 + PAR_LAYOUT_COMPILE_COUNT16 + PAR_LAYOUT_COMPILE_COUNT32)
};

#define PAR_ITEM_PERSIST_COUNT(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_) + ((pers_) ? 1u : 0u)
enum
{
    PAR_PERSISTENT_COMPILE_COUNT = 0u
#define PAR_ITEM_U8  PAR_ITEM_PERSIST_COUNT
#define PAR_ITEM_U16 PAR_ITEM_PERSIST_COUNT
#define PAR_ITEM_U32 PAR_ITEM_PERSIST_COUNT
#define PAR_ITEM_I8  PAR_ITEM_PERSIST_COUNT
#define PAR_ITEM_I16 PAR_ITEM_PERSIST_COUNT
#define PAR_ITEM_I32 PAR_ITEM_PERSIST_COUNT
#define PAR_ITEM_F32 PAR_ITEM_PERSIST_COUNT
#include "../../par_table.def"
#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32
};
#undef PAR_ITEM_PERSIST_COUNT

#undef PAR_ITEM_COUNT_ONE
#undef PAR_ITEM_COUNT_ZERO
/**
 * @brief Function declarations.
 */
const par_cfg_t *par_cfg_get_table(void);
const par_cfg_t *par_cfg_get(const par_num_t par_num);
/**
 * @brief Return the number of configuration entries.
 * @return Configuration table size.
 */
uint32_t par_cfg_get_table_size(void);


#ifdef __cplusplus
}
#endif
/**
 * @} <!-- END GROUP -->
 */

#endif /* _PAR_DEF_CORE_H_ */
