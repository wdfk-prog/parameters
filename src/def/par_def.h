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
 * @brief Sentinel used by par_cfg_t.persist_idx for non-persistent parameters.
 */
#define PAR_PERSIST_IDX_INVALID UINT16_MAX
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

/**
 * @brief Compile-time persistent-slot enumeration derived from par_table.def.
 * @details Only entries flagged with pers_ == true contribute a slot. The
 * resulting PAR_PERSIST_IDX_<enum_> constants are dense and ordered exactly as
 * the source table.
 */
#define PAR_PERSIST_ENUM_SELECT(enum_, pers_) PAR_PERSIST_ENUM_SELECT_I(enum_, pers_)
#define PAR_PERSIST_ENUM_SELECT_I(enum_, pers_) PAR_PERSIST_ENUM_SELECT_##pers_(enum_)
#define PAR_PERSIST_ENUM_SELECT_1(enum_)  PAR_PERSIST_IDX_##enum_,
#define PAR_PERSIST_ENUM_SELECT_0(enum_)
#define PAR_ITEM_PERSIST_ENUM(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)     PAR_PERSIST_ENUM_SELECT(enum_, pers_)
enum
{
#define PAR_ITEM_U8  PAR_ITEM_PERSIST_ENUM
#define PAR_ITEM_U16 PAR_ITEM_PERSIST_ENUM
#define PAR_ITEM_U32 PAR_ITEM_PERSIST_ENUM
#define PAR_ITEM_I8  PAR_ITEM_PERSIST_ENUM
#define PAR_ITEM_I16 PAR_ITEM_PERSIST_ENUM
#define PAR_ITEM_I32 PAR_ITEM_PERSIST_ENUM
#define PAR_ITEM_F32 PAR_ITEM_PERSIST_ENUM
#include "../../par_table.def"
#undef PAR_ITEM_U8
#undef PAR_ITEM_U16
#undef PAR_ITEM_U32
#undef PAR_ITEM_I8
#undef PAR_ITEM_I16
#undef PAR_ITEM_I32
#undef PAR_ITEM_F32

    PAR_PERSISTENT_COMPILE_COUNT,
    PAR_PERSIST_SLOT_MAP_CAPACITY = (PAR_PERSISTENT_COMPILE_COUNT > 0U) ? PAR_PERSISTENT_COMPILE_COUNT : 1U
};
#undef PAR_ITEM_PERSIST_ENUM
#undef PAR_PERSIST_ENUM_SELECT
#undef PAR_PERSIST_ENUM_SELECT_I
#undef PAR_PERSIST_ENUM_SELECT_1
#undef PAR_PERSIST_ENUM_SELECT_0

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
