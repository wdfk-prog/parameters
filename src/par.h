// Copyright (c) 2025 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      par.h
*@brief     Device parameters API functions
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@author    Matej Otic
*@email     otic.matej@dancing-bits.com
*@date      06.12.2024
*@version   V2.2.0
*/
////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup PARAMETERS_API
* @{ <!-- BEGIN GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

#ifndef _PAR_H_
#define _PAR_H_

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "../../par_cfg.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 *     Module version
 */
#define PAR_VER_MAJOR       ( 2 )
#define PAR_VER_MINOR       ( 2 )
#define PAR_VER_DEVELOP     ( 0 )

/**
 *   Parameter status
 */
typedef enum
{
    ePAR_OK                     = 0x00,        /**<Normal operation */

    // Errors
    ePAR_ERROR                  = 0x01,        /**<General parameter error */
    ePAR_ERROR_INIT             = 0x02,        /**<Parameter initialization error or usage before initialization */
    ePAR_ERROR_NVM              = 0x04,        /**<Parameter storage to NVM error */
    ePAR_ERROR_CRC              = 0x08,        /**<Parameter CRC corrupted */

    // Warnings
    ePAR_WARN_SET_TO_DEF        = 0x10,        /**<Parameters set to default */
    ePAR_WARN_NVM_REWRITTEN     = 0x20,        /**<NVM parameters area completely re-written */
    ePAR_WARN_NO_PERSISTANT     = 0x40,        /**<No persistent parameters -> set PAR_CFG_NVM_EN to 0 */

} par_status_t;

/**
 *     Parameters type enumeration
 */
typedef enum
{
    ePAR_TYPE_U8 = 0,   /**<Unsigned 8-bit value */
    ePAR_TYPE_U16,      /**<Unsigned 16-bit value */
    ePAR_TYPE_U32,      /**<Unsigned 32-bit value */
    ePAR_TYPE_I8,       /**<Signed 8-bit value */
    ePAR_TYPE_I16,      /**<Signed 16-bit value */
    ePAR_TYPE_I32,      /**<Signed 32-bit value */
    ePAR_TYPE_F32,      /**<32-bit floating value */
    ePAR_TYPE_NUM_OF
}par_type_list_t;

/**
 *     Parameter R/W access
 */
typedef enum
{
    ePAR_ACCESS_RO = 0,     /**<Parameter read only */
    ePAR_ACCESS_RW          /**<Parameter read/write */
}par_io_acess_t;

/**
 *  32-bit floating data type definition
 */
typedef float float32_t;

/**
 *     Supported data types
 */
typedef union
{
    uint8_t   u8;   /**<Unsigned 8-bit value */
    uint16_t  u16;  /**<Unsigned 16-bit value */
    uint32_t  u32;  /**<Unsigned 32-bit value */
    int8_t    i8;   /**<Signed 8-bit value */
    int16_t   i16;  /**<Signed 16-bit value */
    int32_t   i32;  /**<Signed 32-bit value */
    float32_t f32;  /**<32-bit floating value */
} par_type_t;

/**
 *  Parameter value range
 */
typedef struct
{
    par_type_t min; /**<Minimum value */
    par_type_t max; /**<Maximum value */
} par_range_t;

/**
 *     Parameter data settings
 *
 * @note    Single parameter object has size of 28 bytes on
 *             arm-gcc compiler.
 */
typedef struct
{
    const char *    name;       /**<Name of variable */
    par_type_t      min;        /**<Minimum value of parameter */
    par_type_t      max;        /**<Maximum value of parameter */
    par_type_t      def;        /**<Default value of parameter */
    const char *    unit;       /**<Unit of parameter */
    const char *    desc;       /**<Parameter description */
    uint16_t        id;         /**<Variable ID */
    par_type_list_t type;       /**<Parameter type */
    par_io_acess_t  access;     /**<Parameter access from external device point-of-view */
    bool            persistant; /**<Parameter persistence flag */
} par_cfg_t;

////////////////////////////////////////////////////////////////////////////////
// Functions Prototypes
////////////////////////////////////////////////////////////////////////////////
par_status_t par_init    (void);
par_status_t par_deinit  (void);
bool         par_is_init (void);

par_status_t par_set                (const par_num_t par_num, const void * p_val);
par_status_t par_set_to_default     (const par_num_t par_num);
par_status_t par_set_all_to_default (void);
par_status_t par_has_changed        (const par_num_t par_num, bool *const p_has_changed);

par_status_t par_get            (const par_num_t par_num, void * const p_val);
par_status_t par_get_id         (const par_num_t par_num, uint16_t * const p_id);
par_status_t par_get_num_by_id  (const uint16_t id, par_num_t * const p_par_num);
par_status_t par_get_config     (const par_num_t par_num, par_cfg_t * const p_par_cfg);
par_status_t par_get_type_size  (const par_type_list_t type, uint8_t * const p_size);
par_status_t par_get_type       (const par_num_t par_num, par_type_list_t *const p_type);
par_status_t par_get_range      (const par_num_t par_num, par_range_t *const p_range);

#if ( 1 == PAR_CFG_NVM_EN )
    par_status_t par_set_n_save (const par_num_t par_num, const void * p_val);
    par_status_t par_save_all   (void);
    par_status_t par_save       (const par_num_t par_num);
    par_status_t par_save_by_id (const uint16_t par_id);
    par_status_t par_save_clean (void);
#endif

#if ( PAR_CFG_DEBUG_EN )
    const char * par_get_status_str(const par_status_t status);
#endif

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

#endif // _PAR_H_
