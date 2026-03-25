// Copyright (c) 2026 Ziga Miklosic
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
*@date      29.01.2026
*@version   V3.0.1
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

#include "par_cfg.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 *     Module version
 */
#define PAR_VER_MAJOR       ( 3 )
#define PAR_VER_MINOR       ( 0 )
#define PAR_VER_DEVELOP     ( 2 )

/**
 *   Parameter status
 */
enum
{
    ePAR_OK                 = 0U,           /**<Normal operation */

    // Errors
    ePAR_STATUS_ERROR_MASK  = 0x01FFU,
    ePAR_ERROR              = 0x0001U,      /**<General parameter error */
    ePAR_ERROR_INIT         = 0x0002U,      /**<Parameter initialization error or usage before initialization */
    ePAR_ERROR_NVM          = 0x0004U,      /**<Parameter storage to NVM error */
    ePAR_ERROR_CRC          = 0x0008U,      /**<Parameter CRC corrupted */
    ePAR_ERROR_TYPE         = 0x0010U,      /**<Using invalid API function for given parameter data type */
    ePAR_ERROR_MUTEX        = 0x0020U,      /**<Acquiring mutex failed  */
    ePAR_ERROR_VALUE        = 0x0040U,      /**<Invalid parameter value (validation failed) */
    ePAR_ERROR_PARAM        = 0x0080U,      /**<Invalid function argument */
    ePAR_ERROR_PAR_NUM      = 0x0100U,      /**<Invalid parameter number */

    // Warnings
    ePAR_STATUS_WAR_MASK    = 0xFE00U,
    ePAR_WAR_SET_TO_DEF     = 0x0200U,      /**<Parameters set to default */
    ePAR_WAR_NVM_REWRITTEN  = 0x0400U,      /**<NVM parameters area completely re-written */
    ePAR_WAR_NO_PERSISTENT  = 0x0800U,      /**<No persistent parameters -> set PAR_CFG_NVM_EN to 0 */
    ePAR_WAR_LIMITED        = 0x1000U,      /**<Parameter value limited within [min,max] */
};
typedef uint16_t par_status_t;

/**
 *     Parameters type enumeration
 */
enum
{
    ePAR_TYPE_U8 = 0,   /**<Unsigned 8-bit value */
    ePAR_TYPE_U16,      /**<Unsigned 16-bit value */
    ePAR_TYPE_U32,      /**<Unsigned 32-bit value */
    ePAR_TYPE_I8,       /**<Signed 8-bit value */
    ePAR_TYPE_I16,      /**<Signed 16-bit value */
    ePAR_TYPE_I32,      /**<Signed 32-bit value */
    ePAR_TYPE_F32,      /**<32-bit floating value */
    ePAR_TYPE_NUM_OF
};
typedef uint8_t par_type_list_t;

/**
 *     Parameter R/W access
 */
enum
{
    ePAR_ACCESS_RO = 0, /**<Parameter read only */
    ePAR_ACCESS_RW      /**<Parameter read/write */
};
typedef uint8_t par_access_t;

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
#if ( 1 == PAR_CFG_ENABLE_RANGE )
typedef struct
{
    par_type_t min; /**<Minimum value */
    par_type_t max; /**<Maximum value */
} par_range_t;
#endif

/**
 *     Parameter data settings
 *
 * @note    Single parameter object has size of 28 bytes on
 *             arm-gcc compiler.
 */
typedef struct par_cfg_s
{
#if ( 1 == PAR_CFG_ENABLE_NAME )
    const char *    name;       /**<Name of variable */
#endif
#if ( 1 == PAR_CFG_ENABLE_RANGE )
    par_range_t     range;      /**<Range of parameter */
#endif
    par_type_t      def;        /**<Default value of parameter */
#if ( 1 == PAR_CFG_ENABLE_UNIT )
    const char *    unit;       /**<Unit of parameter */
#endif
#if ( 1 == PAR_CFG_ENABLE_DESC )
    const char *    desc;       /**<Parameter description */
#endif
#if ( 1 == PAR_CFG_ENABLE_ID )
    uint16_t        id;         /**<Variable ID */
#endif
    par_type_list_t type;       /**<Parameter type */
#if ( 1 == PAR_CFG_ENABLE_ACCESS )
    par_access_t    access;     /**<Parameter access from external device point-of-view */
#endif
#if ( 1 == PAR_CFG_ENABLE_PERSIST )
    bool            persistent; /**<Parameter persistence flag */
#endif
} par_cfg_t;

/**
 *  Device Parameters on change callback
 */
typedef void (*pf_par_on_change_cb_t)(const par_num_t par_num, const par_type_t new_val, const par_type_t old_val);

/**
 *  Device Parameters validation
 */
typedef bool (*pf_par_validation_t)(const par_num_t par_num, const par_type_t val);

////////////////////////////////////////////////////////////////////////////////
// Functions Prototypes
////////////////////////////////////////////////////////////////////////////////
par_status_t par_init    (void);
par_status_t par_deinit  (void);
bool         par_is_init (void);

par_status_t par_acquire_mutex	(const par_num_t par_num);
void         par_release_mutex  (const par_num_t par_num);

// Setting parameter value API (module must be first initialized before using those func)
par_status_t par_set                (const par_num_t par_num, const void * p_val);
#if ( 1 == PAR_CFG_ENABLE_ID )
par_status_t par_set_by_id          (const uint16_t id, const void * p_val);
#endif
par_status_t par_set_u8             (const par_num_t par_num, const uint8_t val);
par_status_t par_set_i8             (const par_num_t par_num, const int8_t val);
par_status_t par_set_u16            (const par_num_t par_num, const uint16_t val);
par_status_t par_set_i16            (const par_num_t par_num, const int16_t val);
par_status_t par_set_u32            (const par_num_t par_num, const uint32_t val);
par_status_t par_set_i32            (const par_num_t par_num, const int32_t val);
#if ( 1 == PAR_CFG_ENABLE_TYPE_F32 )
par_status_t par_set_f32            (const par_num_t par_num, const float32_t val);
#endif
/**
 *  Fast typed setters.
 *
 * @note These APIs are performance-oriented public entry points.
 *       Callers must guarantee the module is initialized, par_num is valid,
 *       and the selected typed API matches the parameter type.
 *
 * @note They intentionally bypass runtime validation callbacks and on-change
 *       callbacks. Range limiting still follows build-time PAR_CFG_ENABLE_RANGE.
 */
par_status_t par_set_u8_fast        (const par_num_t par_num, const uint8_t val);
par_status_t par_set_i8_fast        (const par_num_t par_num, const int8_t val);
par_status_t par_set_u16_fast       (const par_num_t par_num, const uint16_t val);
par_status_t par_set_i16_fast       (const par_num_t par_num, const int16_t val);
par_status_t par_set_u32_fast       (const par_num_t par_num, const uint32_t val);
par_status_t par_set_i32_fast       (const par_num_t par_num, const int32_t val);
#if ( 1 == PAR_CFG_ENABLE_TYPE_F32 )
par_status_t par_set_f32_fast       (const par_num_t par_num, const float32_t val);
#endif
par_status_t par_bitand_set_u8_fast (const par_num_t par_num, const uint8_t val);
par_status_t par_bitand_set_u16_fast(const par_num_t par_num, const uint16_t val);
par_status_t par_bitand_set_u32_fast(const par_num_t par_num, const uint32_t val);
par_status_t par_bitor_set_u8_fast  (const par_num_t par_num, const uint8_t val);
par_status_t par_bitor_set_u16_fast (const par_num_t par_num, const uint16_t val);
par_status_t par_bitor_set_u32_fast (const par_num_t par_num, const uint32_t val);
par_status_t par_set_to_default     (const par_num_t par_num);

/**
 *  Reset all parameters to their default values.
 *
 * @note When PAR_CFG_ENABLE_RESET_ALL_RAW = 1, this public API forwards to
 *       par_reset_all_to_default_raw() for the fastest bulk restore path.
 *
 * @note When raw reset is disabled, this API iterates through the normal
 *       runtime setter path and keeps setter semantics.
 */
par_status_t par_set_all_to_default (void);
#if ( 1 == PAR_CFG_ENABLE_RESET_ALL_RAW )
/**
 *  Reset all parameters through raw storage restore.
 *
 * @note This path is typically faster than par_set_all_to_default(), because
 *       it restores grouped storage directly instead of resetting parameters
 *       one by one through the normal setter path.
 *
 * @note This path bypasses normal setter hooks (validation, on-change callback,
 *       and setter-side range behavior).
 */
par_status_t par_reset_all_to_default_raw(void);
#endif

par_status_t par_has_changed        (const par_num_t par_num, bool *const p_has_changed);

/**
 *  @brief   Typed macro wrappers for parameter set.
 */
#define PAR_SET_U8(par_num, value)     par_set_u8((par_num), (value))
#define PAR_SET_I8(par_num, value)     par_set_i8((par_num), (value))
#define PAR_SET_U16(par_num, value)    par_set_u16((par_num), (value))
#define PAR_SET_I16(par_num, value)    par_set_i16((par_num), (value))
#define PAR_SET_U32(par_num, value)    par_set_u32((par_num), (value))
#define PAR_SET_I32(par_num, value)    par_set_i32((par_num), (value))
#if ( 1 == PAR_CFG_ENABLE_TYPE_F32 )
#define PAR_SET_F32(par_num, value)    par_set_f32((par_num), (value))
#endif

// Getting parameter value API (module must be first initialized before using those func)
par_status_t par_get            (const par_num_t par_num, void * const p_val);
#if ( 1 == PAR_CFG_ENABLE_ID )
par_status_t par_get_by_id      (const uint16_t id, void * const p_val);
#endif
par_status_t par_get_u8         (const par_num_t par_num, uint8_t * const p_val);
par_status_t par_get_i8         (const par_num_t par_num, int8_t * const p_val);
par_status_t par_get_u16        (const par_num_t par_num, uint16_t * const p_val);
par_status_t par_get_i16        (const par_num_t par_num, int16_t * const p_val);
par_status_t par_get_u32        (const par_num_t par_num, uint32_t * const p_val);
par_status_t par_get_i32        (const par_num_t par_num, int32_t * const p_val);
#if ( 1 == PAR_CFG_ENABLE_TYPE_F32 )
par_status_t par_get_f32        (const par_num_t par_num, float32_t * const p_val);
#endif
par_status_t par_get_default    (const par_num_t par_num, void * const p_val);

// Parameter configurations API (usage without module init pre-step)
const par_cfg_t *   par_get_config      (const par_num_t par_num);
#if ( 1 == PAR_CFG_ENABLE_NAME )
const char *        par_get_name        (const par_num_t par_num);
#endif
#if ( 1 == PAR_CFG_ENABLE_RANGE )
par_range_t         par_get_range       (const par_num_t par_num);
#endif
#if ( 1 == PAR_CFG_ENABLE_UNIT )
const char *        par_get_unit        (const par_num_t par_num);
#endif
#if ( 1 == PAR_CFG_ENABLE_DESC )
const char *        par_get_desc        (const par_num_t par_num);
#endif
par_type_list_t     par_get_type        (const par_num_t par_num);
#if ( 1 == PAR_CFG_ENABLE_ACCESS )
par_access_t        par_get_access      (const par_num_t par_num);
#endif
#if ( 1 == PAR_CFG_ENABLE_PERSIST )
bool                par_is_persistent   (const par_num_t par_num);
#endif
#if ( 1 == PAR_CFG_ENABLE_ID )
par_status_t        par_get_num_by_id   (const uint16_t id, par_num_t * const p_par_num);
par_status_t        par_get_id_by_num   (const par_num_t par_num, uint16_t * const p_id);
#endif

// Parameter NVM storage API
#if ( 1 == PAR_CFG_NVM_EN )
    par_status_t par_set_n_save (const par_num_t par_num, const void * p_val);
    par_status_t par_save_all   (void);
    par_status_t par_save       (const par_num_t par_num);
#if ( 1 == PAR_CFG_ENABLE_ID )
    par_status_t par_save_by_id (const uint16_t par_id);
#endif
    par_status_t par_save_clean (void);
#endif

// Registration API
#if ( 1 == PAR_CFG_ENABLE_CHANGE_CALLBACK )
void par_register_on_change_cb  (const par_num_t par_num, const pf_par_on_change_cb_t cb);
#endif
#if ( 1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION )
void par_register_validation    (const par_num_t par_num, const pf_par_validation_t validation);
#endif

#if ( 1 == PAR_CFG_ENABLE_DESC ) && ( 1 == PAR_CFG_ENABLE_DESC_CHECK )
PAR_PORT_WEAK bool par_port_is_desc_valid(const char * const p_desc);
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
