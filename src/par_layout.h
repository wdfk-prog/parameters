// Copyright (c) 2026 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      par_layout.h
*@brief     Parameter storage layout abstraction
*@author    wdfk-prog
*@email     1425075683@qq.com
*@date      16.03.2026
*@version   V3.0.1
*/
////////////////////////////////////////////////////////////////////////////////

#ifndef _PAR_LAYOUT_H_
#define _PAR_LAYOUT_H_

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>

#include "par_cfg.h"
#include "par_def.h"

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    uint16_t count8;
    uint16_t count16;
    uint16_t count32;
} par_layout_count_t;

#if ( PAR_CFG_LAYOUT_SOURCE == PAR_CFG_LAYOUT_COMPILE_SCAN )
    #define PAR_STORAGE_COUNT8     (PAR_LAYOUT_COMPILE_COUNT8)
    #define PAR_STORAGE_COUNT16    (PAR_LAYOUT_COMPILE_COUNT16)
    #define PAR_STORAGE_COUNT32    (PAR_LAYOUT_COMPILE_COUNT32)
#elif ( PAR_CFG_LAYOUT_SOURCE == PAR_CFG_LAYOUT_SCRIPT )
    #include PAR_CFG_LAYOUT_STATIC_INCLUDE
    #define PAR_STORAGE_COUNT8     (PAR_LAYOUT_STATIC_COUNT8)
    #define PAR_STORAGE_COUNT16    (PAR_LAYOUT_STATIC_COUNT16)
    #define PAR_STORAGE_COUNT32    (PAR_LAYOUT_STATIC_COUNT32)
#else
    #error "Unsupported PAR_CFG_LAYOUT_SOURCE value!"
#endif

#define PAR_STORAGE_NONZERO(count_)    (((count_) > 0u) ? (count_) : 1u)

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////
const uint16_t * par_layout_get_offset_table(void);
uint16_t         par_layout_get_offset(const par_num_t par_num);
par_layout_count_t par_layout_get_count(void);
void               par_layout_init(void);

#ifdef __cplusplus
}
#endif

#endif // _PAR_LAYOUT_H_
