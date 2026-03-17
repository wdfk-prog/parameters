// Copyright (c) 2026 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      par_def.h
*@brief     Core parameter definition interface
*@author    wdfk-prog
*@email     1425075683@qq.com
*@date      29.01.2026
*@version   V3.0.1
*/
////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup PAR_DEF
* @{ <!-- BEGIN GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

#ifndef _PAR_DEF_CORE_H_
#define _PAR_DEF_CORE_H_

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////
typedef struct par_cfg_s par_cfg_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  List of device parameters
 *
 * @note Must be started with 0!
 */
#define PAR_ITEM_ENUM(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_) enum_,
enum
{
    #define PAR_ITEM_U8   PAR_ITEM_ENUM
    #define PAR_ITEM_U16  PAR_ITEM_ENUM
    #define PAR_ITEM_U32  PAR_ITEM_ENUM
    #define PAR_ITEM_I8   PAR_ITEM_ENUM
    #define PAR_ITEM_I16  PAR_ITEM_ENUM
    #define PAR_ITEM_I32  PAR_ITEM_ENUM
    #define PAR_ITEM_F32  PAR_ITEM_ENUM
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
 *  Compile-time storage group counts derived from par_table.def.
 *
 * @note
 *  These constants are used by layout and static storage allocation.
 */
#define PAR_ITEM_COUNT_ONE(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_)  + 1u
#define PAR_ITEM_COUNT_ZERO(enum_, id_, name_, min_, max_, def_, unit_, access_, pers_, desc_) + 0u

enum
{
    PAR_LAYOUT_COMPILE_COUNT8 = 0u
    #define PAR_ITEM_U8   PAR_ITEM_COUNT_ONE
    #define PAR_ITEM_U16  PAR_ITEM_COUNT_ZERO
    #define PAR_ITEM_U32  PAR_ITEM_COUNT_ZERO
    #define PAR_ITEM_I8   PAR_ITEM_COUNT_ONE
    #define PAR_ITEM_I16  PAR_ITEM_COUNT_ZERO
    #define PAR_ITEM_I32  PAR_ITEM_COUNT_ZERO
    #define PAR_ITEM_F32  PAR_ITEM_COUNT_ZERO
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
    #define PAR_ITEM_U8   PAR_ITEM_COUNT_ZERO
    #define PAR_ITEM_U16  PAR_ITEM_COUNT_ONE
    #define PAR_ITEM_U32  PAR_ITEM_COUNT_ZERO
    #define PAR_ITEM_I8   PAR_ITEM_COUNT_ZERO
    #define PAR_ITEM_I16  PAR_ITEM_COUNT_ONE
    #define PAR_ITEM_I32  PAR_ITEM_COUNT_ZERO
    #define PAR_ITEM_F32  PAR_ITEM_COUNT_ZERO
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
    #define PAR_ITEM_U8   PAR_ITEM_COUNT_ZERO
    #define PAR_ITEM_U16  PAR_ITEM_COUNT_ZERO
    #define PAR_ITEM_U32  PAR_ITEM_COUNT_ONE
    #define PAR_ITEM_I8   PAR_ITEM_COUNT_ZERO
    #define PAR_ITEM_I16  PAR_ITEM_COUNT_ZERO
    #define PAR_ITEM_I32  PAR_ITEM_COUNT_ONE
    #define PAR_ITEM_F32  PAR_ITEM_COUNT_ONE
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

#undef PAR_ITEM_COUNT_ONE
#undef PAR_ITEM_COUNT_ZERO

////////////////////////////////////////////////////////////////////////////////
// Functions Prototypes
////////////////////////////////////////////////////////////////////////////////
const par_cfg_t * par_cfg_get_table      (void);
const par_cfg_t * par_cfg_get            (const par_num_t par_num);
uint32_t          par_cfg_get_table_size (void);

#ifdef __cplusplus
}
#endif

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

#endif /* _PAR_DEF_CORE_H_ */
