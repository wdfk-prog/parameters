// Copyright (c) 2026 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      par.c
*@brief     Device parameters API functions
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@author    Matej Otic
*@email     otic.matej@dancing-bits.com
*@date      22.01.2026
*@version   V3.0.0
*/
////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup PARAMETERS_API
* @{ <!-- BEGIN GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "par.h"
#include "par_nvm.h"
#include "../../par_if.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

/**
 *     Initialization guard
 */
static bool gb_is_init = false;

/**
 *      Parameter callback functions
 */
static par_on_change_cb_t * gp_par_cb = NULL;

/**
 *      Parameter validation functions
 */
static par_validation_t * gp_par_validations = NULL;

/**
 *     Parameter active value that is stored in RAM and its
 *     address offsets
 */
static uint8_t * gpu8_par_value = NULL;
static uint32_t gu32_par_addr_offset[ ePAR_NUM_OF ] = { 0 };

#if ( PAR_CFG_DEBUG_EN )

    /**
     *     Status strings
     */
    static const char * gs_status[] =
    {
        "OK",

        "ERROR",
        "ERROR INIT",
        "ERROR NVM",
        "ERROR CRC",
        "ERROR TYPE",
        "ERROR MUTEX",
        "ERROR_VALUE",

        "WARN SET TO DEF",
        "WARN NVM REWRITTEN",
        "NO PERSISTENT",
        "LIMITED",
    };
#endif

////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
////////////////////////////////////////////////////////////////////////////////
static const uint8_t *  par_allocate_ram_space          (void);
static uint32_t         par_calc_ram_usage              (void);
static par_status_t     par_check_table_validy          (const par_cfg_t * const p_par_cfg);
static uint32_t         par_get_type_size               (const par_type_list_t type);
static bool             par_is_value_changed            (const par_num_t par_num, const void * p_val);
static void             par_raise_on_change_callback    (const par_num_t par_num, const par_type_t new_val, const par_type_t old_val);

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*        Allocate space for live parameter values
*
* @return       Pointer to allocated RAM space for parameter values
*/
////////////////////////////////////////////////////////////////////////////////
static const uint8_t * par_allocate_ram_space(void)
{
    // Calculate total size of RAM
    const uint32_t ram_size = par_calc_ram_usage();

    // Allocate space in RAM
    const uint8_t * par_val_space = malloc( ram_size );

    return par_val_space;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Calculate total size for parameter live values and assign
*        address offset in RAM
*
* @return   Size of RAM space for parameters value in bytes
*/
////////////////////////////////////////////////////////////////////////////////
static uint32_t par_calc_ram_usage(void)
{
    uint32_t total_size = 0U;

    // First fit u32, i32 and f32 into RAM
    for ( par_num_t par_num = 0; par_num < ePAR_NUM_OF; par_num++ )
    {
        if  (   ( ePAR_TYPE_U32 == par_get_type(par_num))
            ||  ( ePAR_TYPE_I32 == par_get_type(par_num))
            ||  ( ePAR_TYPE_F32 == par_get_type(par_num)))
        {
            // Store par RAM address offset
            gu32_par_addr_offset[par_num] = total_size;

            // Accumulate total RAM space
            total_size += par_get_type_size(par_get_type(par_num));
        }
    }

    // Then fit u16 and i16 into RAM
    for ( par_num_t par_num = 0; par_num < ePAR_NUM_OF; par_num++ )
    {
        if  (   ( ePAR_TYPE_U16 == par_get_type(par_num))
            ||  ( ePAR_TYPE_I16 == par_get_type(par_num)))
        {
            // Store par RAM address offset
            gu32_par_addr_offset[par_num] = total_size;

            // Accumulate total RAM space
            total_size += par_get_type_size(par_get_type(par_num));
        }
    }

    // Finally fit u8 and i8 into RAM
    for ( par_num_t par_num = 0; par_num < ePAR_NUM_OF; par_num++ )
    {
        if  (   ( ePAR_TYPE_U8 == par_get_type(par_num))
            ||  ( ePAR_TYPE_I8 == par_get_type(par_num)))
        {
            // Store par RAM address offset
            gu32_par_addr_offset[par_num] = total_size;

            // Accumulate total RAM space
            total_size += par_get_type_size(par_get_type(par_num));
        }
    }

    PAR_DBG_PRINT( "Total RAM consumption for parameters value: %d bytes", total_size );

    return total_size;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Check that parameter table is correctly defined
*
* @param[in]    p_par_cfg - Pointer to parameters table
* @return       status    - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static par_status_t par_check_table_validy(const par_cfg_t * const p_par_cfg)
{
    par_status_t status = ePAR_OK;

    // For each parameter
    for ( uint32_t i = 0; i < ePAR_NUM_OF; i++ )
    {
        // Compare parameters IDs
        for ( uint32_t j = 0; j < ePAR_NUM_OF; j++ )
        {
            if ( i != j )
            {
                // Check for two identical IDs
                if ( p_par_cfg[i].id == p_par_cfg[j].id )
                {
                    status = ePAR_ERROR_INIT;
                    PAR_DBG_PRINT( "Parameter table error: Duplicate ID!" );
                    PAR_ASSERT( 0 );
                    break;
                }
            }
        }

        /**
         *     Check for correct MIN, MAX and DEF value definitions
         *
         *    1. Check that MAX is larger than MIN
         *    2. Check that DEF is equal or less than MAX
         *    3. Check that DEF is equal or more than MIN
         */
        PAR_ASSERT(( ePAR_TYPE_U8 == p_par_cfg[i].type )     ? ((( p_par_cfg[i].min.u8 < p_par_cfg[i].max.u8 ) && ( p_par_cfg[i].def.u8 <= p_par_cfg[i].max.u8 )) && (  p_par_cfg[i].min.u8 <= p_par_cfg[i].def.u8 )) : ( 1 ));
        PAR_ASSERT(( ePAR_TYPE_I8 == p_par_cfg[i].type )     ? ((( p_par_cfg[i].min.i8 < p_par_cfg[i].max.i8 ) && ( p_par_cfg[i].def.i8 <= p_par_cfg[i].max.i8 )) && (  p_par_cfg[i].min.i8 <= p_par_cfg[i].def.i8 )) : ( 1 ));
        PAR_ASSERT(( ePAR_TYPE_U16 == p_par_cfg[i].type )    ? ((( p_par_cfg[i].min.u16 < p_par_cfg[i].max.u16 ) && ( p_par_cfg[i].def.u16 <= p_par_cfg[i].max.u16 )) && (  p_par_cfg[i].min.u16 <= p_par_cfg[i].def.u16 )) : ( 1 ));
        PAR_ASSERT(( ePAR_TYPE_I16 == p_par_cfg[i].type )    ? ((( p_par_cfg[i].min.i16 < p_par_cfg[i].max.i16 ) && ( p_par_cfg[i].def.i16 <= p_par_cfg[i].max.i16 )) && (  p_par_cfg[i].min.i16 <= p_par_cfg[i].def.i16 )) : ( 1 ));
        PAR_ASSERT(( ePAR_TYPE_U32 == p_par_cfg[i].type )    ? ((( p_par_cfg[i].min.u32 < p_par_cfg[i].max.u32 ) && ( p_par_cfg[i].def.u32 <= p_par_cfg[i].max.u32 )) && (  p_par_cfg[i].min.u32 <= p_par_cfg[i].def.u32 )) : ( 1 ));
        PAR_ASSERT(( ePAR_TYPE_I32 == p_par_cfg[i].type )    ? ((( p_par_cfg[i].min.i32 < p_par_cfg[i].max.i32 ) && ( p_par_cfg[i].def.i32 <= p_par_cfg[i].max.i32 )) && (  p_par_cfg[i].min.i32 <= p_par_cfg[i].def.i32 )) : ( 1 ));
        PAR_ASSERT(( ePAR_TYPE_F32 == p_par_cfg[i].type )    ? ((( p_par_cfg[i].min.f32 < p_par_cfg[i].max.f32 ) && ( p_par_cfg[i].def.f32 <= p_par_cfg[i].max.f32 )) && (  p_par_cfg[i].min.f32 <= p_par_cfg[i].def.f32 )) : ( 1 ));
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Get parameter type size
*
* @param[in]    type    - Data type of parameter
* @return       Parameter data type size
*/
////////////////////////////////////////////////////////////////////////////////
static uint32_t par_get_type_size(const par_type_list_t type)
{
    switch ( type )
    {
        case ePAR_TYPE_U8:
            return sizeof( uint8_t );

        case ePAR_TYPE_I8:
            return sizeof( int8_t );

        case ePAR_TYPE_U16:
            return sizeof( uint16_t );

        case ePAR_TYPE_I16:
            return sizeof( int16_t );

        case ePAR_TYPE_U32:
            return sizeof( uint32_t );

        case ePAR_TYPE_I32:
            return sizeof( int32_t );

        case ePAR_TYPE_F32:
            return sizeof( float32_t );
            break;

        case ePAR_TYPE_NUM_OF:
        default:
            PAR_ASSERT(0);
            return 0;
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Is parameter value changed
*
* @param[in]    par_num - Parameter number
* @param[in]    p_val   - Parameter value
* @return       True if parameter value is different from current
*/
////////////////////////////////////////////////////////////////////////////////
static bool par_is_value_changed(const par_num_t par_num, const void * p_val)
{
    bool value_changed = false;

    switch ( par_get_type(par_num))
    {
        case ePAR_TYPE_U8:
            value_changed = (par_get_u8(par_num) != *(uint8_t*)p_val);
            break;

        case ePAR_TYPE_I8:
            value_changed = (par_get_i8(par_num) != *(int8_t*)p_val);
            break;

        case ePAR_TYPE_U16:
            value_changed = (par_get_u16(par_num) != *(uint16_t*)p_val);
            break;

        case ePAR_TYPE_I16:
            value_changed = (par_get_i16(par_num) != *(int16_t*)p_val);
            break;

        case ePAR_TYPE_U32:
            value_changed = (par_get_u32(par_num) != *(uint32_t*)p_val);
            break;

        case ePAR_TYPE_I32:
            value_changed = (par_get_i32(par_num) != *(int32_t*)p_val);
            break;

        case ePAR_TYPE_F32:
            value_changed = (par_get_f32(par_num) != *(float32_t*)p_val);
            break;

        case ePAR_TYPE_NUM_OF:
        default:
            PAR_ASSERT( 0 );
            break;
    }

    return value_changed;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Check and raise callback
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    new_val - New parameter value
* @param[in]    old_val - Old parameter value
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void par_raise_on_change_callback(const par_num_t par_num, const par_type_t new_val, const par_type_t old_val)
{
    // Value changed
    if ( new_val.u32 != old_val.u32 )
    {
        for (const par_on_change_cb_t * cb = gp_par_cb; NULL != cb; cb=(*cb->next))
        {
            if ( par_num == cb->par_num )
            {
                cb->callback( par_num, new_val, old_val );
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Validated parameter value
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    val     - Parameter value
* @return       True if value is accepted
*/
////////////////////////////////////////////////////////////////////////////////
static bool par_validate_value(const par_num_t par_num, const par_type_t val)
{
    for (const par_validation_t * validation = gp_par_validations; NULL != validation; validation=(*validation->next))
    {
        if ( par_num == validation->par_num )
        {
            return validation->valid_func( par_num, val );
        }
    }

    // Return true if validation function is not registered for given parameter
    return true;
}

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup API_FUNCTIONS
* @{ <!-- BEGIN GROUP -->
*
*   Following function are part of Device Parameter module API.
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*        Device Parameters initialization
*
*    At init parameter table is being check for correct definition, allocation
*    in RAM space for parameters live values and additionaly interface to
*    platform is being done.
*
* @return   status - Status of initialization
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_init(void)
{
    par_status_t status = ePAR_OK;

    PAR_ASSERT( false == par_is_init());
    if ( false != par_is_init()) return ePAR_ERROR_INIT;

    // Check if par table is defined correctly
    status |= par_check_table_validy( par_cfg_get_table());

    // Allocate space in RAM
    gpu8_par_value = (uint8_t*) par_allocate_ram_space();
    PAR_ASSERT( NULL != gpu8_par_value );

    // Initialize parameter interface
    status |= par_if_init();

    // Init succeed
    PAR_ASSERT(ePAR_OK == status);
    if ( ePAR_OK == status )
    {
        gb_is_init = true;

        // Set all parameters to default
        par_set_all_to_default();

        #if ( 1 == PAR_CFG_NVM_EN )
            // Init and load parameters from NVM
            status |= par_nvm_init();
        #endif
    }

 	PAR_DBG_PRINT( "PAR: Parameters initialized with status: %s", par_get_status_str( status ));

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        De-initialize Device Parameters
*
* @return status - Status of de-initialization
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_deinit(void)
{
    par_status_t status = ePAR_OK;

    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    #if ( 1 == PAR_CFG_NVM_EN )
        // Init and load parameters from NVM
        status = par_nvm_deinit();
    #endif

    // Module de-initialized
    gb_is_init = false;

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Get initialization flag
*
* @return       Initialization state
*/
////////////////////////////////////////////////////////////////////////////////
bool par_is_init(void)
{
    return gb_is_init;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set parameter value
*
* @note     Mandatory to cast input argument to appropriate type. E.g.:
*
* @code
*             float32_t my_val = 1.234f;
*             par_set( ePAR_MY_VAR, (float32_t*) &my_val );
* @endcode
*
* @note     Input is parameter number (enumeration) defined in par_cfg.h and not
*           parameter ID number!
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    p_val   - Pointer to value
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_set(const par_num_t par_num, const void * p_val)
{
    par_status_t status = ePAR_OK;

    switch ( par_get_type(par_num))
    {
        case ePAR_TYPE_U8:
            status = par_set_u8( par_num, *(uint8_t*) p_val );
            break;

        case ePAR_TYPE_I8:
            status = par_set_i8( par_num, *(int8_t*) p_val );
            break;

        case ePAR_TYPE_U16:
            status = par_set_u16( par_num, *(uint16_t*) p_val );
            break;

        case ePAR_TYPE_I16:
            status = par_set_i16( par_num, *(int16_t*) p_val );
            break;

        case ePAR_TYPE_U32:
            status = par_set_u32( par_num, *(uint32_t*) p_val );
            break;

        case ePAR_TYPE_I32:
            status = par_set_i32( par_num, *(int32_t*) p_val );
            break;

        case ePAR_TYPE_F32:
            status = par_set_f32( par_num, *(float32_t*) p_val );
            break;

        case ePAR_TYPE_NUM_OF:
        default:
            PAR_ASSERT( 0 );
            return ePAR_ERROR_TYPE;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set parameter value by ID
*
* @param[in]    id      - Parameter ID number
* @param[in]    p_val   - Pointer to value
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_set_by_id(const uint16_t id, const void * p_val)
{
    par_num_t par_num;

    if ( ePAR_OK == par_get_num_by_id( id, &par_num ))
    {
        return par_set( par_num, p_val );
    }
    else
    {
        return ePAR_ERROR;
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set unsigned 8-bit parameter
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    val     - Value of parameter
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_set_u8(const par_num_t par_num, const uint8_t val)
{
    par_status_t status = ePAR_OK;

    // Check initialization
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    // Check for invalid type
    PAR_ASSERT( ePAR_TYPE_U8 == par_get_type(par_num));
    if( ePAR_TYPE_U8 != par_get_type(par_num)) return ePAR_ERROR_TYPE;

    // Get mutex
    if ( ePAR_OK == par_if_aquire_mutex())
    {
        const par_range_t range = par_get_range(par_num);
        const par_type_t old_val = {.u8 = *(uint8_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]]};

        if ( val > range.max.u8 )
        {
            *(uint8_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]] = range.max.u8;
            status = ePAR_WAR_LIMITED;
        }
        else if ( val < range.min.u8 )
        {
            *(uint8_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]] = range.min.u8;
            status = ePAR_WAR_LIMITED;
        }
        else
        {
            // Validated parameter value
            if ( par_validate_value( par_num, (par_type_t){.u8 = val}))
            {
                *(uint8_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]] = (uint8_t) val;
                status = ePAR_OK;
            }
            else
            {
                status = ePAR_ERROR_VALUE;
            }
        }

        (void) par_if_release_mutex();

        // Raise on change callback
        const par_type_t new_val = {.u8 = *(uint8_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]]};
        par_raise_on_change_callback( par_num, new_val, old_val );
    }
    else
    {
        status = ePAR_ERROR_MUTEX;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set signed 8-bit parameter
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    val     - Value of parameter
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_set_i8(const par_num_t par_num, const int8_t val)
{
    par_status_t status = ePAR_OK;

    // Check initialization
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    // Check for invalid type
    PAR_ASSERT( ePAR_TYPE_I8 == par_get_type(par_num));
    if( ePAR_TYPE_I8 != par_get_type(par_num)) return ePAR_ERROR_TYPE;

    // Get mutex
    if ( ePAR_OK == par_if_aquire_mutex())
    {
        const par_range_t range = par_get_range(par_num);
        const par_type_t old_val = {.i8 = *(int8_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]]};

        if ( val > range.max.i8 )
        {
            *(int8_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]] = range.max.i8;
            status = ePAR_WAR_LIMITED;
        }
        else if ( val < range.min.i8 )
        {
            *(int8_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]] = range.min.i8;
            status = ePAR_WAR_LIMITED;
        }
        else
        {
            // Validated parameter value
            if ( par_validate_value( par_num, (par_type_t){.i8 = val}))
            {
                *(int8_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]] = (int8_t) val;
                status = ePAR_OK;
            }
            else
            {
                status = ePAR_ERROR_VALUE;
            }
        }

        (void) par_if_release_mutex();

        // Raise on change callback
        const par_type_t new_val = {.i8 = *(int8_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]]};
        par_raise_on_change_callback( par_num, new_val, old_val );
    }
    else
    {
        status = ePAR_ERROR_MUTEX;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set unsigned 16-bit parameter
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    val     - Value of parameter
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_set_u16(const par_num_t par_num, const uint16_t val)
{
    par_status_t status = ePAR_OK;

    // Check initialization
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    // Check for invalid type
    PAR_ASSERT( ePAR_TYPE_U16 == par_get_type(par_num));
    if( ePAR_TYPE_U16 != par_get_type(par_num)) return ePAR_ERROR_TYPE;

    // Get mutex
    if ( ePAR_OK == par_if_aquire_mutex())
    {
        const par_range_t range = par_get_range(par_num);
        const par_type_t old_val = {.u16 = *(uint16_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]]};

        if ( val > range.max.u16 )
        {
            *(uint16_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]] = range.max.u16;
            status = ePAR_WAR_LIMITED;
        }
        else if ( val < range.min.u16 )
        {
            *(uint16_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]] = range.min.u16;
            status = ePAR_WAR_LIMITED;
        }
        else
        {
            // Validated parameter value
            if ( par_validate_value( par_num, (par_type_t){.u16 = val}))
            {
                *(uint16_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]] = (uint16_t) val;
                status = ePAR_OK;
            }
            else
            {
                status = ePAR_ERROR_VALUE;
            }
        }

        (void) par_if_release_mutex();

        // Raise on change callback
        const par_type_t new_val = {.u16 = *(uint16_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]]};
        par_raise_on_change_callback( par_num, new_val, old_val );
    }
    else
    {
        status = ePAR_ERROR_MUTEX;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set signed 16-bit parameter
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    val     - Value of parameter
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_set_i16(const par_num_t par_num, const int16_t val)
{
    par_status_t status = ePAR_OK;

    // Check initialization
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    // Check for invalid type
    PAR_ASSERT( ePAR_TYPE_I16 == par_get_type(par_num));
    if( ePAR_TYPE_I16 != par_get_type(par_num)) return ePAR_ERROR_TYPE;

    // Get mutex
    if ( ePAR_OK == par_if_aquire_mutex())
    {
        const par_range_t range = par_get_range(par_num);
        const par_type_t old_val = {.i16 = *(int16_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]]};

        if ( val > range.max.i16 )
        {
            *(int16_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]] = range.max.i16;
            status = ePAR_WAR_LIMITED;
        }
        else if ( val < range.min.i16 )
        {
            *(int16_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]] = range.min.i16;
            status = ePAR_WAR_LIMITED;
        }
        else
        {
            // Validated parameter value
            if ( par_validate_value( par_num, (par_type_t){.i16 = val}))
            {
                *(int16_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]] = (int16_t) val;
                status = ePAR_OK;
            }
            else
            {
                status = ePAR_ERROR_VALUE;
            }
        }

        (void) par_if_release_mutex();

        // Raise on change callback
        const par_type_t new_val = {.i16 = *(int16_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]]};
        par_raise_on_change_callback( par_num, new_val, old_val );
    }
    else
    {
        status = ePAR_ERROR_MUTEX;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set unsigned 32-bit parameter
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    val     - Value of parameter
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_set_u32(const par_num_t par_num, const uint32_t val)
{
    par_status_t status = ePAR_OK;

    // Check initialization
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    // Check for invalid type
    PAR_ASSERT( ePAR_TYPE_U32 == par_get_type(par_num));
    if( ePAR_TYPE_U32 != par_get_type(par_num)) return ePAR_ERROR_TYPE;

    // Get mutex
    if ( ePAR_OK == par_if_aquire_mutex())
    {
        const par_range_t range = par_get_range(par_num);
        const par_type_t old_val = {.u32 = *(uint32_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]]};

        if ( val > range.max.u32 )
        {
            *(uint32_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]] = range.max.u32;
            status = ePAR_WAR_LIMITED;
        }
        else if ( val < range.min.u32 )
        {
            *(uint32_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]] = range.min.u32;
            status = ePAR_WAR_LIMITED;
        }
        else
        {
            // Validated parameter value
            if ( par_validate_value( par_num, (par_type_t){.u32 = val}))
            {
                *(uint32_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]] = (uint32_t) val;
                status = ePAR_OK;
            }
            else
            {
                status = ePAR_ERROR_VALUE;
            }
        }

        (void) par_if_release_mutex();

        // Raise on change callback
        const par_type_t new_val = {.u32 = *(uint32_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]]};
        par_raise_on_change_callback( par_num, new_val, old_val );
    }
    else
    {
        status = ePAR_ERROR_MUTEX;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set signed 32-bit parameter
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    val     - Value of parameter
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_set_i32(const par_num_t par_num, const int32_t val)
{
    par_status_t status = ePAR_OK;

    // Check initialization
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    // Check for invalid type
    PAR_ASSERT( ePAR_TYPE_I32 == par_get_type(par_num));
    if( ePAR_TYPE_I32 != par_get_type(par_num)) return ePAR_ERROR_TYPE;

    // Get mutex
    if ( ePAR_OK == par_if_aquire_mutex())
    {
        const par_range_t range = par_get_range(par_num);
        const par_type_t old_val = {.i32 = *(int32_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]]};

        if ( val > range.max.i32 )
        {
            *(int32_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]] = range.max.i32;
            status = ePAR_WAR_LIMITED;
        }
        else if ( val < range.min.i32 )
        {
            *(int32_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]] = range.min.i32;
            status = ePAR_WAR_LIMITED;
        }
        else
        {
            // Validated parameter value
            if ( par_validate_value( par_num, (par_type_t){.i32 = val}))
            {
                *(int32_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]] = (int32_t) val;
                status = ePAR_OK;
            }
            else
            {
                status = ePAR_ERROR_VALUE;
            }
        }

        (void) par_if_release_mutex();

        // Raise on change callback
        const par_type_t new_val = {.i32 = *(int32_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]]};
        par_raise_on_change_callback( par_num, new_val, old_val );
    }
    else
    {
        status = ePAR_ERROR_MUTEX;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set floating value parameter
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    val     - Value of parameter
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_set_f32(const par_num_t par_num, const float32_t val)
{
    par_status_t status = ePAR_OK;

    // Check initialization
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    // Check for invalid type
    PAR_ASSERT( ePAR_TYPE_F32 == par_get_type(par_num));
    if( ePAR_TYPE_F32 != par_get_type(par_num)) return ePAR_ERROR_TYPE;

    // Get mutex
    if ( ePAR_OK == par_if_aquire_mutex())
    {
        const par_range_t range = par_get_range(par_num);
        const par_type_t old_val = {.f32 = *(float32_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]]};

        if ( val > range.max.f32 )
        {
            *(float32_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]] = range.max.f32;
            status = ePAR_WAR_LIMITED;
        }
        else if ( val < range.min.f32 )
        {
            *(float32_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]] = range.min.f32;
            status = ePAR_WAR_LIMITED;
        }
        else
        {
            // Validated parameter value
            if ( par_validate_value( par_num, (par_type_t){.f32 = val}))
            {
                *(float32_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]] = (float32_t) val;
                status = ePAR_OK;
            }
            else
            {
                status = ePAR_ERROR_VALUE;
            }
        }

        (void) par_if_release_mutex();

        // Raise on change callback
        const par_type_t new_val = {.f32 = *(float32_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]]};
        par_raise_on_change_callback( par_num, new_val, old_val );
    }
    else
    {
        status = ePAR_ERROR_MUTEX;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set parameter to default value
*
* @pre    Parameters must be initialised before usage!
*
* @param[in]    par_num    - Parameter number (enumeration)
* @return       status     - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_set_to_default(const par_num_t par_num)
{
    return par_set(par_num, &(par_get_config(par_num)->def));
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set all parameters to default value
*
* @pre    Parameters must be initialised before usage!
*
* @return    status - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_set_all_to_default(void)
{
    for ( uint32_t par_num = 0; par_num < ePAR_NUM_OF; par_num++ )
    {
        // Ignore return as it is not possible to return other that OK
        (void) par_set_to_default( par_num );
    }

    PAR_DBG_PRINT( "PAR: Setting all parameters to default" );
    return ePAR_OK;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Get parameter value
*
* @note     Mandatory to cast input argument to appropriate type. E.g.:
*
* @code
*             float32_t my_val = 0.0f;
*             par_get( ePAR_MY_VAR, (float32_t*) &my_val );
* @endcode
*
* @note         Input is parameter number (enumeration) defined in par_cfg.h and not
*               parameter ID number!
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[out]   p_val   - Parameter value
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_get(const par_num_t par_num, void * const p_val)
{
    switch ( par_get_type(par_num))
    {
        case ePAR_TYPE_U8:
            *(uint8_t*) p_val = par_get_u8(par_num);
            break;

        case ePAR_TYPE_I8:
            *(int8_t*) p_val = par_get_i8(par_num);
            break;

        case ePAR_TYPE_U16:
            *(uint16_t*) p_val = par_get_u16(par_num);
            break;

        case ePAR_TYPE_I16:
            *(int16_t*) p_val = par_get_i16(par_num);
            break;

        case ePAR_TYPE_U32:
            *(uint32_t*) p_val = par_get_u32(par_num);
            break;

        case ePAR_TYPE_I32:
            *(int32_t*) p_val = par_get_i32(par_num);
            break;

        case ePAR_TYPE_F32:
            *(float32_t*) p_val = par_get_f32(par_num);
            break;

        case ePAR_TYPE_NUM_OF:
        default:
            PAR_ASSERT( 0 );
            return ePAR_ERROR_TYPE;
    }

    return ePAR_OK;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Get parameter value by ID
*
* @param[in]    id      - Parameter ID number
* @param[out]   p_val   - Pointer to value
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_get_by_id(const uint16_t id, void * const p_val)
{
    par_num_t par_num;

    if ( ePAR_OK == par_get_num_by_id( id, &par_num ))
    {
        return par_get( par_num, p_val );
    }
    else
    {
        return ePAR_ERROR;
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Get unsigned 8-bit parameter value
*
* @param[in]    par_num - Parameter number (enumeration)
* @return       value   - Value of parameter
*/
////////////////////////////////////////////////////////////////////////////////
uint8_t par_get_u8(const par_num_t par_num)
{
    uint8_t val = 0;

    // Check initialization
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    // Check for invalid type
    PAR_ASSERT( ePAR_TYPE_U8 == par_get_type(par_num));
    if( ePAR_TYPE_U8 != par_get_type(par_num)) return ePAR_ERROR_TYPE;

    // Get mutex
    if ( ePAR_OK == par_if_aquire_mutex())
    {
        val = *(uint8_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]];

        (void) par_if_release_mutex();
    }

    return val;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Get signed 8-bit parameter value
*
* @param[in]    par_num - Parameter number (enumeration)
* @return       value   - Value of parameter
*/
////////////////////////////////////////////////////////////////////////////////
int8_t par_get_i8(const par_num_t par_num)
{
    int8_t val = 0;

    // Check initialization
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    // Check for invalid type
    PAR_ASSERT( ePAR_TYPE_I8 == par_get_type(par_num));
    if( ePAR_TYPE_I8 != par_get_type(par_num)) return ePAR_ERROR_TYPE;

    // Get mutex
    if ( ePAR_OK == par_if_aquire_mutex())
    {
        val = *(int8_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]];

        (void) par_if_release_mutex();
    }

    return val;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Get unsigned 16-bit parameter value
*
* @param[in]    par_num - Parameter number (enumeration)
* @return       value   - Value of parameter
*/
////////////////////////////////////////////////////////////////////////////////
uint16_t par_get_u16(const par_num_t par_num)
{
    uint16_t val = 0;

    // Check initialization
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    // Check for invalid type
    PAR_ASSERT( ePAR_TYPE_U16 == par_get_type(par_num));
    if( ePAR_TYPE_U16 != par_get_type(par_num)) return ePAR_ERROR_TYPE;

    // Get mutex
    if ( ePAR_OK == par_if_aquire_mutex())
    {
        val = *(uint16_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]];

        (void) par_if_release_mutex();
    }

    return val;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Get signed 16-bit parameter value
*
* @param[in]    par_num - Parameter number (enumeration)
* @return       value   - Value of parameter
*/
////////////////////////////////////////////////////////////////////////////////
int16_t par_get_i16(const par_num_t par_num)
{
    int16_t val = 0;

    // Check initialization
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    // Check for invalid type
    PAR_ASSERT( ePAR_TYPE_I16 == par_get_type(par_num));
    if( ePAR_TYPE_I16 != par_get_type(par_num)) return ePAR_ERROR_TYPE;

    // Get mutex
    if ( ePAR_OK == par_if_aquire_mutex())
    {
        val = *(int16_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]];

        (void) par_if_release_mutex();
    }

    return val;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Get unsigned 32-bit parameter value
*
* @param[in]    par_num - Parameter number (enumeration)
* @return       value   - Value of parameter
*/
////////////////////////////////////////////////////////////////////////////////
uint32_t par_get_u32(const par_num_t par_num)
{
    uint32_t val = 0;

    // Check initialization
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    // Check for invalid type
    PAR_ASSERT( ePAR_TYPE_U32 == par_get_type(par_num));
    if( ePAR_TYPE_U32 != par_get_type(par_num)) return ePAR_ERROR_TYPE;

    // Get mutex
    if ( ePAR_OK == par_if_aquire_mutex())
    {
        val = *(uint32_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]];

        (void) par_if_release_mutex();
    }

    return val;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Get signed 32-bit parameter value
*
* @param[in]    par_num - Parameter number (enumeration)
* @return       value   - Value of parameter
*/
////////////////////////////////////////////////////////////////////////////////
int32_t par_get_i32(const par_num_t par_num)
{
    int32_t val = 0;

    // Check initialization
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    // Check for invalid type
    PAR_ASSERT( ePAR_TYPE_I32 == par_get_type(par_num));
    if( ePAR_TYPE_I32 != par_get_type(par_num)) return ePAR_ERROR_TYPE;

    // Get mutex
    if ( ePAR_OK == par_if_aquire_mutex())
    {
        val = *(int32_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]];

        (void) par_if_release_mutex();
    }

    return val;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Get floating value parameter value
*
* @param[in]    par_num - Parameter number (enumeration)
* @return       value   - Value of parameter
*/
////////////////////////////////////////////////////////////////////////////////
float32_t par_get_f32(const par_num_t par_num)
{
    float32_t val = 0.0f;

    // Check initialization
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    // Check for invalid type
    PAR_ASSERT( ePAR_TYPE_F32 == par_get_type(par_num));
    if( ePAR_TYPE_F32 != par_get_type(par_num)) return ePAR_ERROR_TYPE;

    // Get mutex
    if ( ePAR_OK == par_if_aquire_mutex())
    {
        val = *(float32_t*)&gpu8_par_value[gu32_par_addr_offset[par_num]];

        (void) par_if_release_mutex();
    }

    return val;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Get parameter default value
**
* @param[in]    par_num - Parameter number (enumeration)
* @param[out]   p_val   - Parameter default value
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_get_default(const par_num_t par_num, void * const p_val)
{
    // Check initialization
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    switch ( par_get_type( par_num ))
    {
        case ePAR_TYPE_U8:
            *(uint8_t*) p_val = (uint8_t) par_get_config(par_num)->def.u8;
            break;

        case ePAR_TYPE_I8:
            *(int8_t*) p_val = (int8_t) par_get_config(par_num)->def.i8;
            break;

        case ePAR_TYPE_U16:
            *(uint16_t*) p_val = (uint16_t) par_get_config(par_num)->def.u16;
            break;

        case ePAR_TYPE_I16:
            *(int16_t*) p_val = (int16_t) par_get_config(par_num)->def.i16;
            break;

        case ePAR_TYPE_U32:
            *(uint32_t*) p_val = (uint32_t) par_get_config(par_num)->def.u32;
            break;

        case ePAR_TYPE_I32:
            *(int32_t*) p_val = (int32_t) par_get_config(par_num)->def.i32;
            break;

        case ePAR_TYPE_F32:
            *(float32_t*) p_val = (float32_t) par_get_config(par_num)->def.f32;
            break;

        case ePAR_TYPE_NUM_OF:
        default:
            PAR_ASSERT( 0 );
            return ePAR_ERROR_TYPE;
    }

    return ePAR_OK;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Check if parameter changed from its default value
*
* @param[in]    par_num - Parameter number (enumeration)
* @return       true if parameter value has been changed
*/
////////////////////////////////////////////////////////////////////////////////
bool par_is_changed(const par_num_t par_num)
{
    switch ( par_get_type(par_num))
    {
        case ePAR_TYPE_U8:
            return (bool) (par_get_u8(par_num) != par_get_config(par_num)->def.u8);

        case ePAR_TYPE_I8:
            return (bool) (par_get_i8(par_num) != par_get_config(par_num)->def.i8);

        case ePAR_TYPE_U16:
            return (bool) (par_get_u16(par_num) != par_get_config(par_num)->def.u16);

        case ePAR_TYPE_I16:
            return (bool) (par_get_i16(par_num) != par_get_config(par_num)->def.i16);

        case ePAR_TYPE_U32:
            return (bool) (par_get_u32(par_num) != par_get_config(par_num)->def.u32);

        case ePAR_TYPE_I32:
            return (bool) (par_get_i32(par_num) != par_get_config(par_num)->def.i32);

        case ePAR_TYPE_F32:
            return (bool) (par_get_f32(par_num) != par_get_config(par_num)->def.f32);

        case ePAR_TYPE_NUM_OF:
        default:
            PAR_ASSERT( 0 );
            return false;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Get parameter configurations
*
* @note  In case parameter is not found it return NULL!
*
* @param[in]    par_num  - Parameter number (enumeration)
* @return       Parameter configuration
*/
////////////////////////////////////////////////////////////////////////////////
const par_cfg_t * par_get_config(const par_num_t par_num)
{
    // Invalid parameter
    PAR_ASSERT( par_num < ePAR_NUM_OF );
    if ( par_num >= ePAR_NUM_OF ) return NULL;

    return (const par_cfg_t*) par_cfg_get(par_num);
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Get parameter name
*
* @param[in]    par_num  - Parameter number (enumeration)
* @return       Parameter name
*/
////////////////////////////////////////////////////////////////////////////////
const char * par_get_name(const par_num_t par_num)
{
    const par_cfg_t * const par_cfg = par_get_config(par_num);

    if ( NULL != par_cfg )
    {
        return par_cfg->name;
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Get parameter value range
*
* @param[in]    par_num - Parameter number (enumeration)
* @return       Parameter min/max range
*/
////////////////////////////////////////////////////////////////////////////////
par_range_t par_get_range(const par_num_t par_num)
{
    par_range_t range = {0};
    const par_cfg_t * const par_cfg = par_get_config(par_num);

    if ( NULL != par_cfg )
    {
        range.min = par_cfg->min;
        range.max = par_cfg->max;
    }

    return range;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Get parameter unit
*
* @param[in]    par_num  - Parameter number (enumeration)
* @return       Parameter unit
*/
////////////////////////////////////////////////////////////////////////////////
const char * par_get_unit(const par_num_t par_num)
{
    const par_cfg_t * const par_cfg = par_get_config(par_num);

    if ( NULL != par_cfg )
    {
        return par_cfg->unit;
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Get parameter description
*
* @param[in]    par_num  - Parameter number (enumeration)
* @return       Parameter description
*/
////////////////////////////////////////////////////////////////////////////////
const char * par_get_desc(const par_num_t par_num)
{
    const par_cfg_t * const par_cfg = par_get_config(par_num);

    if ( NULL != par_cfg )
    {
        return par_cfg->desc;
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Get parameter type
*
* @param[in]    par_num - Parameter number (enumeration)
* @return       Parameter data type
*/
////////////////////////////////////////////////////////////////////////////////
par_type_list_t par_get_type(const par_num_t par_num)
{
    const par_cfg_t * const par_cfg = par_get_config(par_num);

    if ( NULL != par_cfg )
    {
        return par_cfg->type;
    }

    return ePAR_TYPE_NUM_OF;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Get parameter access (RO, RW)
*
* @param[in]    par_num  - Parameter number (enumeration)
* @return       Parameter access
*/
////////////////////////////////////////////////////////////////////////////////
par_access_t par_get_access(const par_num_t par_num)
{
    const par_cfg_t * const par_cfg = par_get_config(par_num);

    if ( NULL != par_cfg )
    {
        return par_cfg->access;
    }

    return ePAR_ACCESS_RO;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Is parameter persistant (does it stores to NVM)
*
* @param[in]    par_num  - Parameter number (enumeration)
* @return       True if parameter persistant
*/
////////////////////////////////////////////////////////////////////////////////
bool par_is_persistant(const par_num_t par_num)
{
    const par_cfg_t * const par_cfg = par_get_config(par_num);

    if ( NULL != par_cfg )
    {
        return par_cfg->persistant;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Get parameter number (enumeration) by ID
*
* @param[in]    id          - Parameter ID
* @param[out]   p_par_num   - Pointer to parameter enumeration number
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_get_num_by_id(const uint16_t id, par_num_t * const p_par_num)
{
    if ( NULL != p_par_num )
    {
        for (uint32_t par_num = 0; par_num < ePAR_NUM_OF; par_num++ )
        {
            const par_cfg_t * const par_cfg = par_get_config(par_num);

            if (( NULL != par_cfg ) && ( id == par_cfg->id ))
            {
                *p_par_num = par_num;
                return ePAR_OK;
            }
        }
    }

    return ePAR_ERROR;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Get parameter ID by number (enumeration)
*
* @param[in]    par_num - Parameter number
* @param[out]   p_id    - Pointer to parameter ID
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_get_id_by_num(const par_num_t par_num, uint16_t * const p_id)
{
    if ( NULL != p_id )
    {
        const par_cfg_t * const par_cfg = par_get_config(par_num);

        if ( NULL != par_cfg )
        {
            *p_id = par_cfg->id;
            return ePAR_OK;
        }
    }

    return ePAR_ERROR;
}

#if ( 1 == PAR_CFG_NVM_EN )
    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Set parameter value and save to NVM if value changed
    *
    * @note     Mandatory to cast input argument to appropriate type. E.g.:
    *
    * @code
    *             float32_t my_val = 1.234f;
    *             par_set( ePAR_MY_VAR, (float32_t*) &my_val );
    * @endcode
    *
    * @note     Input is parameter number (enumeration) defined in par_cfg.h and not
    *           parameter ID number!
    *
    * @param[in]    par_num - Parameter number (enumeration)
    * @param[in]    p_val   - Pointer to value
    * @return       status  - Status of operation
    */
    ////////////////////////////////////////////////////////////////////////////////
    par_status_t par_set_n_save(const par_num_t par_num, const void * p_val)
    {
        // Check if parameter value is about to change
        const bool value_change = par_is_value_changed( par_num, p_val );

        // Set parameter
        par_status_t status = par_set(par_num, p_val);

        // Parameter set OK and value has been changed -> makes sense to store to NVM
        if (( ePAR_OK == status ) && value_change )
        {
            status |= par_save(par_num);
        }

        return status;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Store all parameters value to NVM
    *
    * @pre        NVM storage must be initialized first and "PAR_CFG_NVM_EN"
    *             settings must be enabled.
    *
    * @return       status - Status of operation
    */
    ////////////////////////////////////////////////////////////////////////////////
    par_status_t par_save_all(void)
    {
        // Check initialization
        PAR_ASSERT( true == par_is_init());
        if ( true != par_is_init()) return ePAR_ERROR_INIT;

        return par_nvm_write_all();
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Store single parameter value to NVM
    *
    * @pre        NVM storage must be initialized first and "PAR_CFG_NVM_EN"
    *             settings must be enabled.
    *
    * @param[in]    par_num - Parameter number (enumeration)
    * @return       status  - Status of operation
    */
    ////////////////////////////////////////////////////////////////////////////////
    par_status_t par_save(const par_num_t par_num)
    {
        // Check initialization
        PAR_ASSERT( true == par_is_init());
        if ( true != par_is_init()) return ePAR_ERROR_INIT;

        return par_nvm_write( par_num, true );
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Store single parameter value to NVM by its ID value
    *
    * @pre        NVM storage must be initialized first and "PAR_CFG_NVM_EN"
    *             settings must be enabled.
    *
    * @code
    *             // Use case
    *             // Store par from ID 10 to 32
    *             uint8_t par_id;
    *
    *             for ( par_id = 10; par_id < 32; par_id++ )
    *             {
    *                  status |= par_save_by_id( par_id )
    *             }
    *
    * @endcode
    *
    * @param[in]    par_id  - Parameter ID number
    * @return       status  - Status of operation
    */
    ////////////////////////////////////////////////////////////////////////////////
    par_status_t par_save_by_id(const uint16_t par_id)
    {
        par_num_t par_num = 0;

        // Check initialization
        PAR_ASSERT( true == par_is_init());
        if ( true != par_is_init()) return ePAR_ERROR_INIT;

        if ( ePAR_OK == par_get_num_by_id( par_id, &par_num ))
        {
            return par_save( par_num );
        }

        return ePAR_ERROR;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Clean all stored parameters inside NVM
    *
    * @note     This function shall be locked as it will erase complete parameter
    *           region of NVM space. Shall be used only during
    *
    * @pre      NVM storage must be initialized first and "PAR_CFG_NVM_EN"
    *           settings must be enabled.
    *
    * @return        status - Status of operation
    */
    ////////////////////////////////////////////////////////////////////////////////
    par_status_t par_save_clean(void)
    {
        // Check initialization
        PAR_ASSERT( true == par_is_init());
        if ( true != par_is_init()) return ePAR_ERROR_INIT;

        return par_nvm_reset_all();
    }

#endif

////////////////////////////////////////////////////////////////////////////////
/**
*        Register parameter on change callback
*
* @param[cb]    cb      - Callback
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_register_on_change_cb(const par_on_change_cb_t * const cb)
{
    static par_on_change_cb_t * prev_cb = NULL;

    PAR_ASSERT( NULL != cb );
    PAR_ASSERT( NULL != cb->callback );
    if ( NULL == cb ) return ePAR_ERROR;
    if ( NULL == cb->callback) return ePAR_ERROR;

    // Get mutex
    if ( ePAR_OK == par_if_aquire_mutex())
    {
        // First registration -> store the start of the callback linked list
        if ( NULL == gp_par_cb )
        {
            gp_par_cb = (par_on_change_cb_t*) cb;
        }
        else
        {
            (*prev_cb->next) = (par_on_change_cb_t*) cb;
        }

        // Store previous callback
        prev_cb = (par_on_change_cb_t*) cb;

        (void) par_if_release_mutex();
    }
    else
    {
        return ePAR_ERROR_MUTEX;
    }

    return ePAR_OK;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Register parameter value validation function
*
* @param[cb]    validation  - Validation
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_register_validation(const par_validation_t * const validation)
{
    static par_validation_t * prev_validation = NULL;

    PAR_ASSERT( NULL != validation );
    PAR_ASSERT( NULL != validation->valid_func );
    if ( NULL == validation ) return ePAR_ERROR;
    if ( NULL == validation->valid_func) return ePAR_ERROR;

    // Get mutex
    if ( ePAR_OK == par_if_aquire_mutex())
    {
        // First registration -> store the start of the callback linked list
        if ( NULL == gp_par_validations )
        {
            gp_par_validations = (par_validation_t*) validation;
        }
        else
        {
            (*prev_validation->next) = (par_validation_t*) validation;
        }

        // Store previous callback
        prev_validation = (par_validation_t*) validation;

        (void) par_if_release_mutex();
    }
    else
    {
        return ePAR_ERROR_MUTEX;
    }

    return ePAR_OK;
}

#if ( PAR_CFG_DEBUG_EN )

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *        Get status string description
    *
    * @param[in]    status  - Parameter status
    * @return       str     - Parameter status description
    */
    ////////////////////////////////////////////////////////////////////////////////
    const char * par_get_status_str(const par_status_t status)
    {
        uint8_t i = 0;
        const char * str = "N/A";

        if ( ePAR_OK == status  )
        {
            str = (const char*) gs_status[0];
        }
        else
        {
            for ( i = 0; i < 16; i++ )
            {
                if ( status & ( 1<<i ))
                {
                    str =  (const char*) gs_status[i+1];
                    break;
                }
            }
        }

        return str;
    }
#endif

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
