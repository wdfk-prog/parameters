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

#include "../../par_cfg.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 *     Module version
 */
#define PAR_VER_MAJOR       ( 3 )
#define PAR_VER_MINOR       ( 0 )
#define PAR_VER_DEVELOP     ( 1 )

/**
 *   Parameter status
 */
enum
{
    ePAR_OK                 = 0U,           /**<Normal operation */

    // Errors
    ePAR_STATUS_ERROR_MASK  = 0x00FFU,
    ePAR_ERROR              = 0x0001U,      /**<General parameter error */
    ePAR_ERROR_INIT         = 0x0002U,      /**<Parameter initialization error or usage before initialization */
    ePAR_ERROR_NVM          = 0x0004U,      /**<Parameter storage to NVM error */
    ePAR_ERROR_CRC          = 0x0008U,      /**<Parameter CRC corrupted */
    ePAR_ERROR_TYPE         = 0x0010U,      /**<Using invalid API function for given parameter data type */
    ePAR_ERROR_MUTEX        = 0x0020U,      /**<Acquiring mutex failed  */
    ePAR_ERROR_VALUE        = 0x0040U,      /**<Invalid parameter value (validation failed) */

    // Warnings
    ePAR_STATUS_WAR_MASK    = 0xFF00U,
    ePAR_WAR_SET_TO_DEF     = 0x0100U,      /**<Parameters set to default */
    ePAR_WAR_NVM_REWRITTEN  = 0x0200U,      /**<NVM parameters area completely re-written */
    ePAR_WAR_NO_PERSISTANT  = 0x0400U,      /**<No persistent parameters -> set PAR_CFG_NVM_EN to 0 */
    ePAR_WAR_LIMITED        = 0x0800U,      /**<Parameter value limited within [min,max] */
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
    par_access_t    access;     /**<Parameter access from external device point-of-view */
    bool            persistant; /**<Parameter persistence flag */
} par_cfg_t;

/**
 *  Device Parameters on change callback
 */
typedef void (*pf_par_on_change_cb_t)(const par_num_t par_num, const par_type_t new_val, const par_type_t old_val);

typedef struct par_on_change_cb
{
    const pf_par_on_change_cb_t callback;       /**<Callback function pointer */
    const par_num_t             par_num_first;  /**<First (or only) parameter */
    const par_num_t             par_num_last;   /**<Last parameter */
    const bool                  is_range;       /**<Is on change for range of parameters? */
    struct par_on_change_cb **  next;           /**<Pointer to next callback block */
} par_on_change_cb_t;

#define PAR_DEFINE_ON_CHANGE_CB(name, par, cb)          \
    static const par_on_change_cb_t name =              \
    {                                                   \
        .callback      = cb,                            \
        .par_num_first = par,                           \
        .is_range      = false,                         \
        .next          = &(par_on_change_cb_t*){NULL},  \
    }

#define PAR_DEFINE_ON_CHANGE_RANGE_CB(name, par_first, par_last, cb)    \
    static const par_on_change_cb_t name =                              \
    {                                                                   \
        .callback      = cb,                                            \
        .par_num_first = par_first,                                     \
        .par_num_last  = par_last,                                      \
        .is_range      = true,                                          \
        .next          = &(par_on_change_cb_t*){NULL},                  \
    }

/**
 *  Device Parameters validation
 */
typedef bool (*pf_par_validation_t)(const par_num_t par_num, const par_type_t val);

typedef struct par_validation
{
    const pf_par_validation_t  valid_func;      /**<Validation function pointer */
    const par_num_t            par_num_first;   /**<First (or only) parameter */
    const par_num_t            par_num_last;    /**<Last parameter */
    const bool                 is_range;        /**<Is validation for range of parameters? */
    struct par_validation **   next;            /**<Pointer to next callback block */
} par_validation_t;

#define PAR_DEFINE_VALIDATION(name, par, validation)    \
    static const par_validation_t name =                \
    {                                                   \
        .valid_func    = validation,                    \
        .par_num_first = par,                           \
        .is_range      = false,                         \
        .next          = &(par_validation_t*){NULL},    \
    }

#define PAR_DEFINE_VALIDATION_RANGE(name, par_first, par_last, validation)  \
    static const par_validation_t name =                                    \
    {                                                                       \
        .valid_func    = validation,                                        \
        .par_num_first = par_first,                                         \
        .par_num_last  = par_last,                                          \
        .is_range      = true,                                              \
        .next          = &(par_validation_t*){NULL},                        \
    }

////////////////////////////////////////////////////////////////////////////////
// Functions Prototypes
////////////////////////////////////////////////////////////////////////////////
par_status_t par_init    (void);
par_status_t par_deinit  (void);
bool         par_is_init (void);

// Setting parameter value API (module must be first initialized before using those func)
par_status_t par_set                (const par_num_t par_num, const void * p_val);
par_status_t par_set_by_id          (const uint16_t id, const void * p_val);
par_status_t par_set_u8             (const par_num_t par_num, const uint8_t val);
par_status_t par_set_i8             (const par_num_t par_num, const int8_t val);
par_status_t par_set_u16            (const par_num_t par_num, const uint16_t val);
par_status_t par_set_i16            (const par_num_t par_num, const int16_t val);
par_status_t par_set_u32            (const par_num_t par_num, const uint32_t val);
par_status_t par_set_i32            (const par_num_t par_num, const int32_t val);
par_status_t par_set_f32            (const par_num_t par_num, const float32_t val);
par_status_t par_set_u8_fast        (const par_num_t par_num, const uint8_t val);
par_status_t par_set_i8_fast        (const par_num_t par_num, const int8_t val);
par_status_t par_set_u16_fast       (const par_num_t par_num, const uint16_t val);
par_status_t par_set_i16_fast       (const par_num_t par_num, const int16_t val);
par_status_t par_set_u32_fast       (const par_num_t par_num, const uint32_t val);
par_status_t par_set_i32_fast       (const par_num_t par_num, const int32_t val);
par_status_t par_set_f32_fast       (const par_num_t par_num, const float32_t val);
par_status_t par_set_to_default     (const par_num_t par_num);
par_status_t par_set_all_to_default (void);

/**
 *  @brief   Type-generic macro to set a parameter value.
 *
 *          This macro uses C11 _Generic to automatically select the appropriate
 *          setter function based on the data type of the "value" variable.
 *
 *  @param[in]  par_num - Parameter number (enumeration)
 *  @param[in]  value   - The new value of a parameter
 */
#define PAR_SET(par_num, value) _Generic((value),       \
    uint8_t:    par_set_u8,                             \
    bool:       par_set_u8,                             \
    uint16_t:   par_set_u16,                            \
    uint32_t:   par_set_u32,                            \
    int8_t:     par_set_i8,                             \
    int16_t:    par_set_i16,                            \
    int32_t:    par_set_i32,                            \
    float32_t:  par_set_f32,                            \
    default:    par_set_f32                             \
)(par_num, value)

// Getting parameter value API (module must be first initialized before using those func)
par_status_t par_get            (const par_num_t par_num, void * const p_val);
par_status_t par_get_by_id      (const uint16_t id, void * const p_val);
uint8_t      par_get_u8         (const par_num_t par_num);
int8_t       par_get_i8         (const par_num_t par_num);
uint16_t     par_get_u16        (const par_num_t par_num);
int16_t      par_get_i16        (const par_num_t par_num);
uint32_t     par_get_u32        (const par_num_t par_num);
int32_t      par_get_i32        (const par_num_t par_num);
float32_t    par_get_f32        (const par_num_t par_num);
par_status_t par_get_default    (const par_num_t par_num, void * const p_val);
bool         par_is_changed     (const par_num_t par_num);

/**
 *  @brief   Type-generic macro to retrieve a parameter value.
 *
 *          This macro uses C11 _Generic to automatically select the appropriate
 *          getter function based on the data type of the "dest" variable.
 *
 *  @param[in]   par_num - The unique identifier (ID) of the parameter to retrieve.
 *  @param[in]   dest    - The destination variable where the value will be stored.
 *                         The type of this variable determines the getter used (e.g., uint8_t, float).
 */
#define PAR_GET(par_num, dest) _Generic((dest), 	\
    uint8_t:   dest = par_get_u8(par_num),      	\
    bool:      dest = par_get_u8(par_num),      	\
    uint16_t:  dest = par_get_u16(par_num),     	\
    uint32_t:  dest = par_get_u32(par_num),     	\
    int8_t:    dest = par_get_i8(par_num),      	\
    int16_t:   dest = par_get_i16(par_num),     	\
    int32_t:   dest = par_get_i32(par_num),     	\
    float32_t: dest = par_get_f32(par_num),     	\
    default:   dest = par_get_f32(par_num)     	    \
)

// Parameter configurations API (usage without module init pre-step)
const par_cfg_t *   par_get_config      (const par_num_t par_num);
const char *        par_get_name        (const par_num_t par_num);
par_range_t         par_get_range       (const par_num_t par_num);
const char *        par_get_unit        (const par_num_t par_num);
const char *        par_get_desc        (const par_num_t par_num);
par_type_list_t     par_get_type        (const par_num_t par_num);
par_access_t        par_get_access      (const par_num_t par_num);
bool                par_is_persistant   (const par_num_t par_num);
par_status_t        par_get_num_by_id   (const uint16_t id, par_num_t * const p_par_num);
par_status_t        par_get_id_by_num   (const par_num_t par_num, uint16_t * const p_id);

// Parameter NVM storage API
#if ( 1 == PAR_CFG_NVM_EN )
    par_status_t par_set_n_save (const par_num_t par_num, const void * p_val);
    par_status_t par_save_all   (void);
    par_status_t par_save       (const par_num_t par_num);
    par_status_t par_save_by_id (const uint16_t par_id);
    par_status_t par_save_clean (void);
#endif

// Registration API
par_status_t par_register_on_change_cb  (const par_on_change_cb_t * const cb);
par_status_t par_register_validation    (const par_validation_t * const validation);

#if ( PAR_CFG_DEBUG_EN )
    const char * par_get_status_str(const par_status_t status);
#endif

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

#endif // _PAR_H_
