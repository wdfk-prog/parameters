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
*@date      29.01.2026
*@version   V3.0.1
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
#include "par_atomic.h"
#include "par_nvm.h"
#include "par_if.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////
/*
 * http://www.citi.umich.edu/techreports/reports/citi-tr-00-1.pdf
 *
 * GoldenRatio = ~(Math.pow(2, 32) / ((Math.sqrt(5) - 1) / 2)) + 1
 */
#if ( 1 == PAR_CFG_ENABLE_ID )
    #define PAR_ID_HASH_GOLDEN_RATIO_32   ( 0x61C88647u )

/**
 *  Minimum number of hash buckets to keep target load factor <= 0.5.
 */
    #define PAR_ID_HASH_MIN_BUCKETS       ((uint32_t)(2u * (uint32_t)ePAR_NUM_OF))

/**
 *  Hash map geometry derived from ePAR_NUM_OF at compile time.
 */
    enum
    {
        PAR_ID_HASH_BITS =
            ( PAR_ID_HASH_MIN_BUCKETS <= ( 1u << 1  )) ? 1u  :
            ( PAR_ID_HASH_MIN_BUCKETS <= ( 1u << 2  )) ? 2u  :
            ( PAR_ID_HASH_MIN_BUCKETS <= ( 1u << 3  )) ? 3u  :
            ( PAR_ID_HASH_MIN_BUCKETS <= ( 1u << 4  )) ? 4u  :
            ( PAR_ID_HASH_MIN_BUCKETS <= ( 1u << 5  )) ? 5u  :
            ( PAR_ID_HASH_MIN_BUCKETS <= ( 1u << 6  )) ? 6u  :
            ( PAR_ID_HASH_MIN_BUCKETS <= ( 1u << 7  )) ? 7u  :
            ( PAR_ID_HASH_MIN_BUCKETS <= ( 1u << 8  )) ? 8u  :
            ( PAR_ID_HASH_MIN_BUCKETS <= ( 1u << 9  )) ? 9u  :
            ( PAR_ID_HASH_MIN_BUCKETS <= ( 1u << 10 )) ? 10u :
            ( PAR_ID_HASH_MIN_BUCKETS <= ( 1u << 11 )) ? 11u :
            ( PAR_ID_HASH_MIN_BUCKETS <= ( 1u << 12 )) ? 12u :
            ( PAR_ID_HASH_MIN_BUCKETS <= ( 1u << 13 )) ? 13u :
            ( PAR_ID_HASH_MIN_BUCKETS <= ( 1u << 14 )) ? 14u :
            ( PAR_ID_HASH_MIN_BUCKETS <= ( 1u << 15 )) ? 15u :
            ( PAR_ID_HASH_MIN_BUCKETS <= ( 1u << 16 )) ? 16u :
            ( PAR_ID_HASH_MIN_BUCKETS <= ( 1u << 17 )) ? 17u : 18u,
        PAR_ID_HASH_SIZE = ( 1u << PAR_ID_HASH_BITS ),
    };

    PAR_STATIC_ASSERT(par_id_hash_size_valid, (PAR_ID_HASH_SIZE >= PAR_ID_HASH_MIN_BUCKETS));
    PAR_STATIC_ASSERT(par_id_hash_bits_valid, ((PAR_ID_HASH_BITS > 0u) && (PAR_ID_HASH_BITS < 32u)));
#endif

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

/**
 *     Initialization guard
 */
static bool gb_is_init = false;

/**
 * Parameter callback table.
 */
static struct
{
    pf_par_validation_t validation;     /**< Validation callback function (or NULL). */
    pf_par_on_change_cb_t on_change;    /**< On change callback function (or NULL). */
} g_par_cb_table[ePAR_NUM_OF];

/**
 *  ID hash map entry.
 */
#if ( 1 == PAR_CFG_ENABLE_ID )
    typedef struct
    {
        uint16_t  id;
        par_num_t par_num;
        uint8_t   used;
    } par_id_map_entry_t;

    /**
     *  Runtime ID hash map.
     */
    static par_id_map_entry_t g_par_id_map[PAR_ID_HASH_SIZE] = {0};

    /**
     *  Initialization guard for ID hash map.
     */
    static bool gb_par_id_map_ready = false;
#endif

/**
 *  Parameter live values divided by its type in RAM
 */
static par_atomic_u8_t *     gpu8_par_value = NULL;
static par_atomic_i8_t *     gpi8_par_value = NULL;
static par_atomic_u16_t *    gpu16_par_value = NULL;
static par_atomic_i16_t *    gpi16_par_value = NULL;
static par_atomic_u32_t *    gpu32_par_value = NULL;
static par_atomic_i32_t *    gpi32_par_value = NULL;
static par_atomic_f32_t *    gpf32_par_value = NULL;

/**
 *  Address offset by parameter enumeration
 */
static uint32_t gu32_par_offset[ ePAR_NUM_OF ] = { 0 };

/**
 *  Private getters and setters
 */
#define PAR_GET_U8_PRIV(par_num)        PAR_ATOMIC_LOAD(u8, &gpu8_par_value[gu32_par_offset[par_num]])
#define PAR_GET_I8_PRIV(par_num)        PAR_ATOMIC_LOAD(i8, &gpi8_par_value[gu32_par_offset[par_num]])
#define PAR_GET_U16_PRIV(par_num)       PAR_ATOMIC_LOAD(u16, &gpu16_par_value[gu32_par_offset[par_num]])
#define PAR_GET_I16_PRIV(par_num)       PAR_ATOMIC_LOAD(i16, &gpi16_par_value[gu32_par_offset[par_num]])
#define PAR_GET_U32_PRIV(par_num)       PAR_ATOMIC_LOAD(u32, &gpu32_par_value[gu32_par_offset[par_num]])
#define PAR_GET_I32_PRIV(par_num)       PAR_ATOMIC_LOAD(i32, &gpi32_par_value[gu32_par_offset[par_num]])
#define PAR_GET_F32_PRIV(par_num)       PAR_ATOMIC_LOAD(f32, &gpf32_par_value[gu32_par_offset[par_num]])

#define PAR_SET_U8_PRIV(par_num, val)   PAR_ATOMIC_STORE(u8, &gpu8_par_value[gu32_par_offset[par_num]], (val))
#define PAR_SET_I8_PRIV(par_num, val)   PAR_ATOMIC_STORE(i8, &gpi8_par_value[gu32_par_offset[par_num]], (val))
#define PAR_SET_U16_PRIV(par_num, val)  PAR_ATOMIC_STORE(u16, &gpu16_par_value[gu32_par_offset[par_num]], (val))
#define PAR_SET_I16_PRIV(par_num, val)  PAR_ATOMIC_STORE(i16, &gpi16_par_value[gu32_par_offset[par_num]], (val))
#define PAR_SET_U32_PRIV(par_num, val)  PAR_ATOMIC_STORE(u32, &gpu32_par_value[gu32_par_offset[par_num]], (val))
#define PAR_SET_I32_PRIV(par_num, val)  PAR_ATOMIC_STORE(i32, &gpi32_par_value[gu32_par_offset[par_num]], (val))
#define PAR_SET_F32_PRIV(par_num, val)  PAR_ATOMIC_STORE(f32, &gpf32_par_value[gu32_par_offset[par_num]], (val))

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
static void             par_allocate_ram_space          (void);
#if ( 1 == PAR_CFG_ENABLE_ID )
static inline uint32_t  par_hash_id                     (const uint16_t id);
static par_status_t     par_build_and_validate_id_map   (const par_cfg_t * const p_par_cfg);
#endif
static par_status_t     par_check_table_validity        (const par_cfg_t * const p_par_cfg);
#if ( 1 == PAR_CFG_NVM_EN )
static bool         par_is_value_changed            (const par_num_t par_num, const void * p_val);
#endif /* ( 1 == PAR_CFG_NVM_EN ) */

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
static void par_allocate_ram_space(void)
{
    uint32_t total_size = 0;
    void * mem = NULL;

    // Group 32-bit types first - Alignment safety
    uint32_t group32_size = 0, group32_count = 0;
    for ( par_num_t par_it = 0; par_it < ePAR_NUM_OF; par_it++ )
    {
        if (    ( ePAR_TYPE_U32 == par_get_type(par_it))
           ||   ( ePAR_TYPE_I32 == par_get_type(par_it))
           ||   ( ePAR_TYPE_F32 == par_get_type(par_it)))
        {
            gu32_par_offset[par_it] = group32_count;
            group32_size += 4;
            group32_count++;
        }
    }

    // Group 16-bit types second
    uint32_t group16_size = 0, group16_count = 0;
    for ( par_num_t par_it = 0; par_it < ePAR_NUM_OF; par_it++ )
    {
        if (    ( ePAR_TYPE_U16 == par_get_type(par_it))
           ||   ( ePAR_TYPE_I16 == par_get_type(par_it)))
        {
            gu32_par_offset[par_it] = group16_count;
            group16_size += 2;
            group16_count++;
        }
    }

    // Group 8-bit types last
    uint32_t group8_size = 0, group8_count = 0;
    for ( par_num_t par_it = 0; par_it < ePAR_NUM_OF; par_it++ )
    {
        if (    ( ePAR_TYPE_U8 == par_get_type(par_it))
           ||   ( ePAR_TYPE_I8 == par_get_type(par_it)))
        {
            gu32_par_offset[par_it] = group8_count;
            group8_size += 1;
            group8_count++;
        }
    }

    // Calculate full RAM size and allocate memory in single shot
    total_size = group32_size + group16_size + group8_size;
    mem = malloc(total_size);

    // 32-bit vars share the first part of the memory
    gpu32_par_value = mem;
    gpf32_par_value = mem;
    gpi32_par_value = mem;

    // 16-bit vars share the middle part of the memory
    gpu16_par_value = mem + group32_size;
    gpi16_par_value = mem + group32_size;

    // 8-bit vars share the last part of the memory
    gpu8_par_value = mem + group32_size + group16_size;
    gpi8_par_value = mem + group32_size + group16_size;

    PAR_DBG_PRINT( "Total RAM consumption for parameters value: %d bytes", total_size );
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Hash parameter ID to bucket index
*
* @param[in]    id  - Parameter ID
* @return       hash index
*/
////////////////////////////////////////////////////////////////////////////////
#if ( 1 == PAR_CFG_ENABLE_ID )
static inline uint32_t par_hash_id(const uint16_t id)
{
    return (((uint32_t) id * PAR_ID_HASH_GOLDEN_RATIO_32 ) >> ( 32u - PAR_ID_HASH_BITS ));
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Build and validate parameter ID hash map
*
* @param[in]    p_par_cfg - Pointer to parameters table
* @return       status    - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static par_status_t par_build_and_validate_id_map(const par_cfg_t * const p_par_cfg)
{
    memset( g_par_id_map, 0, sizeof(g_par_id_map) );
    for ( par_num_t par_num = 0; par_num < ePAR_NUM_OF; par_num++ )
    {
        const uint16_t id = p_par_cfg[par_num].id;
        const uint32_t bucket_idx = par_hash_id( id );
        par_id_map_entry_t * const bucket = &g_par_id_map[bucket_idx];

        if ( 0u == bucket->used )
        {
            bucket->used = 1u;
            bucket->id = id;
            bucket->par_num = par_num;
            continue;
        }

        if ( bucket->id == id )
        {
            PAR_DBG_PRINT( "ERR, Duplicate parameter ID %u!", (unsigned) id );
            PAR_ASSERT( 0 );
            return ePAR_ERROR_INIT;
        }

        PAR_DBG_PRINT( "ERR, Hash collision: ID %u conflicts with ID %u at bucket %u!",
            (unsigned) id, (unsigned) bucket->id, (unsigned) bucket_idx );
        PAR_DBG_PRINT( "ERR, Please regenerate IDs or adjust hash parameters." );
        PAR_ASSERT( 0 );
        return ePAR_ERROR_INIT;
    }

    return ePAR_OK;
}
#endif

////////////////////////////////////////////////////////////////////////////////
/**
*        Check that parameter table is correctly defined
*
* @param[in]    p_par_cfg - Pointer to parameters table
* @return       status    - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static par_status_t par_check_table_validity(const par_cfg_t * const p_par_cfg)
{
    par_status_t status = ePAR_OK;

#if ( 1 == PAR_CFG_ENABLE_ID )
    // Build and validate runtime ID hash map
    status = par_build_and_validate_id_map( p_par_cfg );
    if ( ePAR_OK != status )
    {
        return status;
    }
#endif

    // For each parameter
    for ( uint32_t i = 0; i < ePAR_NUM_OF; i++ )
    {
#if ( 1 == PAR_CFG_ENABLE_RANGE )
        /*
         * Keep F32 range/default validation in runtime.
         *
         * On some embedded/legacy GCC toolchains, float comparisons used in
         * typedef-based static asserts may be treated as non-constant
         * expressions and trigger file-scope VLA warnings.
         */
        PAR_ASSERT(( ePAR_TYPE_F32 == p_par_cfg[i].type )    ? ((( p_par_cfg[i].min.f32 < p_par_cfg[i].max.f32 ) && ( p_par_cfg[i].def.f32 <= p_par_cfg[i].max.f32 )) && (  p_par_cfg[i].min.f32 <= p_par_cfg[i].def.f32 )) : ( 1 ));
#endif

#if ( 1 == PAR_CFG_ENABLE_NAME )
        if ( NULL == p_par_cfg[i].name )
        {
            status = ePAR_ERROR_INIT;
            PAR_DBG_PRINT( "ERR, Parameter %d name missing!", i );
            PAR_ASSERT( 0 );
            break;
        }
#endif

#if ( 1 == PAR_CFG_ENABLE_DESC )
        if ( NULL == p_par_cfg[i].desc )
        {
            status = ePAR_ERROR_INIT;
            PAR_DBG_PRINT( "ERR, Parameter %d description missing!", i );
            PAR_ASSERT( 0 );
            break;
        }
#endif

#if ( 1 == PAR_CFG_ENABLE_DESC ) && ( 1 == PAR_CFG_ENABLE_DESC_COMMA_CHECK )
        // ',' is prohibited in parameter description
        // NOTE: ',' is used as column separator and will break PC tool side parser logic in case of usage in description!
        if ( NULL != strchr( p_par_cfg[i].desc, ',' ))
        {
            status = ePAR_ERROR_INIT;
            PAR_DBG_PRINT( "ERR, Parameter %d description contains comma!", i );
            PAR_ASSERT( 0 );
            break;
        }
#endif
    }

    return status;
}

#if ( 1 == PAR_CFG_NVM_EN )
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
#endif /* ( 1 == PAR_CFG_NVM_EN ) */

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
    status |= par_check_table_validity( par_cfg_get_table());

    // Allocate space in RAM
    par_allocate_ram_space();

    // Initialize parameter interface
    status |= par_if_init();

    // Init succeed
    PAR_ASSERT(ePAR_OK == status);
    if ( ePAR_OK == status )
    {
        gb_is_init = true;
#if ( 1 == PAR_CFG_ENABLE_ID )
        gb_par_id_map_ready = true;
#endif

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
#if ( 1 == PAR_CFG_ENABLE_ID )
    gb_par_id_map_ready = false;
#endif

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
*        Try to acquire mutex for specified parameter
*
* @param[in]    par_num - Parameter number (enumeration)
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_acquire_mutex(const par_num_t par_num)
{
    PAR_ASSERT( par_num < ePAR_NUM_OF );

    return par_if_aquire_mutex(par_num);
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Try to acquire mutex for specified parameter
*
* @param[in]    par_num - Parameter number (enumeration)
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
void par_release_mutex(const par_num_t par_num)
{
    PAR_ASSERT( par_num < ePAR_NUM_OF );

    par_if_release_mutex(par_num);
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
#if ( 1 == PAR_CFG_ENABLE_ID )
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
#endif

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

    pf_par_validation_t validation = g_par_cb_table[par_num].validation;
    pf_par_on_change_cb_t on_change = g_par_cb_table[par_num].on_change;
    // Get mutex
    if ( ePAR_OK == par_acquire_mutex(par_num))
    {
        const par_type_t old_val = {.u8 = PAR_GET_U8_PRIV( par_num )};
        
        // Validated parameter value
        if ((validation == NULL) || validation(par_num, (par_type_t){.u8 = val}))
        {
            status = par_set_u8_fast( par_num, val );
        }
        else
        {
            status = ePAR_ERROR_VALUE;
        }

        // Raise on change callback
        const par_type_t new_val = {.u8 = PAR_GET_U8_PRIV( par_num )};
        if ((on_change != NULL) && (new_val.u8 != old_val.u8))
        {
            on_change(par_num, new_val, old_val);
        }

        par_release_mutex(par_num);
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

    pf_par_validation_t validation = g_par_cb_table[par_num].validation;
    pf_par_on_change_cb_t on_change = g_par_cb_table[par_num].on_change;
    // Get mutex
    if ( ePAR_OK == par_acquire_mutex(par_num))
    {
        const par_type_t old_val = {.i8 = PAR_GET_I8_PRIV( par_num )};

        // Validated parameter value
        if ((validation == NULL) || validation(par_num, (par_type_t){.i8 = val}))
        {
            status = par_set_i8_fast( par_num, val );
        }
        else
        {
            status = ePAR_ERROR_VALUE;
        }

        // Raise on change callback
        const par_type_t new_val = {.i8 = PAR_GET_I8_PRIV( par_num )};
        if ((on_change != NULL) && (new_val.i8 != old_val.i8))
        {
            on_change(par_num, new_val, old_val);
        }

        par_release_mutex(par_num);
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

    pf_par_validation_t validation = g_par_cb_table[par_num].validation;
    pf_par_on_change_cb_t on_change = g_par_cb_table[par_num].on_change;
    // Get mutex
    if ( ePAR_OK == par_acquire_mutex(par_num))
    {
        const par_type_t old_val = {.u16 = PAR_GET_U16_PRIV( par_num )};

        // Validated parameter value
        if ((validation == NULL) || validation(par_num, (par_type_t){.u16 = val}))
        {
            status = par_set_u16_fast( par_num, val );
        }
        else
        {
            status = ePAR_ERROR_VALUE;
        }

        // Raise on change callback
        const par_type_t new_val = {.u16 = PAR_GET_U16_PRIV( par_num )};
        if ((on_change != NULL) && (new_val.u16 != old_val.u16))
        {
            on_change(par_num, new_val, old_val);
        }

        par_release_mutex(par_num);
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

    pf_par_validation_t validation = g_par_cb_table[par_num].validation;
    pf_par_on_change_cb_t on_change = g_par_cb_table[par_num].on_change;
    // Get mutex
    if ( ePAR_OK == par_acquire_mutex(par_num))
    {
        const par_type_t old_val = {.i16 = PAR_GET_I16_PRIV( par_num )};

        // Validated parameter value
        if ((validation == NULL) || validation(par_num, (par_type_t){.i16 = val}))
        {
            status = par_set_i16_fast( par_num, val );
        }
        else
        {
            status = ePAR_ERROR_VALUE;
        }

        // Raise on change callback
        const par_type_t new_val = {.i16 = PAR_GET_I16_PRIV( par_num )};
        if ((on_change != NULL) && (new_val.i16 != old_val.i16))
        {
            on_change(par_num, new_val, old_val);
        }

        par_release_mutex(par_num);
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

    pf_par_validation_t validation = g_par_cb_table[par_num].validation;
    pf_par_on_change_cb_t on_change = g_par_cb_table[par_num].on_change;
    // Get mutex
    if ( ePAR_OK == par_acquire_mutex(par_num))
    {
        const par_type_t old_val = {.u32 = PAR_GET_U32_PRIV( par_num )};

        // Validated parameter value
        if ((validation == NULL) || validation(par_num, (par_type_t){.u32 = val}))
        {
            status = par_set_u32_fast( par_num, val );
        }
        else
        {
            status = ePAR_ERROR_VALUE;
        }

        // Raise on change callback
        const par_type_t new_val = {.u32 = PAR_GET_U32_PRIV( par_num )};
        if ((on_change != NULL) && (new_val.u32 != old_val.u32))
        {
            on_change(par_num, new_val, old_val);
        }

        par_release_mutex(par_num);
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

    pf_par_validation_t validation = g_par_cb_table[par_num].validation;
    pf_par_on_change_cb_t on_change = g_par_cb_table[par_num].on_change;
    // Get mutex
    if ( ePAR_OK == par_acquire_mutex(par_num))
    {
        const par_type_t old_val = {.i32 = PAR_GET_I32_PRIV( par_num )};

        // Validated parameter value
        if ((validation == NULL) || validation(par_num, (par_type_t){.i32 = val}))
        {
            status = par_set_i32_fast( par_num, val );
        }
        else
        {
            status = ePAR_ERROR_VALUE;
        }

        // Raise on change callback
        const par_type_t new_val = {.i32 = PAR_GET_I32_PRIV( par_num )};
        if ((on_change != NULL) && (new_val.i32 != old_val.i32))
        {
            on_change(par_num, new_val, old_val);
        }

        par_release_mutex(par_num);
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

    pf_par_validation_t validation = g_par_cb_table[par_num].validation;
    pf_par_on_change_cb_t on_change = g_par_cb_table[par_num].on_change;
    // Get mutex
    if ( ePAR_OK == par_acquire_mutex(par_num))
    {
        const par_type_t old_val = {.f32 = PAR_GET_F32_PRIV( par_num )};

        // Validated parameter value
        if ((validation == NULL) || validation(par_num, (par_type_t){.f32 = val}))
        {
            status = par_set_f32_fast( par_num, val );
        }
        else
        {
            status = ePAR_ERROR_VALUE;
        }

        // Raise on change callback
        const par_type_t new_val = {.f32 = PAR_GET_F32_PRIV( par_num )};
        if ((on_change != NULL) && (new_val.f32 != old_val.f32))
        {
            on_change(par_num, new_val, old_val);
        }

        par_release_mutex(par_num);
    }
    else
    {
        status = ePAR_ERROR_MUTEX;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set unsigned 8-bit parameter fast
*
* @note Using *(volatile uint8_t*) prevents store tearing as explained
*       here: https://lwn.net/Articles/793253/
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    val     - Value of parameter
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_set_u8_fast(const par_num_t par_num, const uint8_t val)
{
    PAR_ASSERT( true == par_is_init());
    PAR_ASSERT( ePAR_TYPE_U8 == par_get_type(par_num));

#if ( 1 == PAR_CFG_ENABLE_RANGE )
    const par_range_t range = par_get_range(par_num);

    if ( val > range.max.u8 )
    {
        PAR_SET_U8_PRIV( par_num, range.max.u8 );
        return ePAR_WAR_LIMITED;
    }
    else if ( val < range.min.u8 )
    {
        PAR_SET_U8_PRIV( par_num, range.min.u8 );
        return ePAR_WAR_LIMITED;
    }
    else
    {
        PAR_SET_U8_PRIV( par_num, val );
        return ePAR_OK;
    }
#else
    PAR_SET_U8_PRIV( par_num, val );
    return ePAR_OK;
#endif
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set signed 8-bit parameter fast
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    val     - Value of parameter
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_set_i8_fast(const par_num_t par_num, const int8_t val)
{
    PAR_ASSERT( true == par_is_init());
    PAR_ASSERT( ePAR_TYPE_I8 == par_get_type(par_num));

#if ( 1 == PAR_CFG_ENABLE_RANGE )
    const par_range_t range = par_get_range(par_num);

    if ( val > range.max.i8 )
    {
        PAR_SET_I8_PRIV( par_num, range.max.i8 );
        return ePAR_WAR_LIMITED;
    }
    else if ( val < range.min.i8 )
    {
        PAR_SET_I8_PRIV( par_num, range.min.i8 );
        return ePAR_WAR_LIMITED;
    }
    else
    {
        PAR_SET_I8_PRIV( par_num, val );
        return ePAR_OK;
    }
#else
    PAR_SET_I8_PRIV( par_num, val );
    return ePAR_OK;
#endif
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set unsigned 16-bit parameter fast
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    val     - Value of parameter
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_set_u16_fast(const par_num_t par_num, const uint16_t val)
{
    PAR_ASSERT( true == par_is_init());
    PAR_ASSERT( ePAR_TYPE_U16 == par_get_type(par_num));

#if ( 1 == PAR_CFG_ENABLE_RANGE )
    const par_range_t range = par_get_range(par_num);

    if ( val > range.max.u16 )
    {
        PAR_SET_U16_PRIV( par_num, range.max.u16 );
        return ePAR_WAR_LIMITED;
    }
    else if ( val < range.min.u16 )
    {
        PAR_SET_U16_PRIV( par_num, range.min.u16 );
        return ePAR_WAR_LIMITED;
    }
    else
    {
        PAR_SET_U16_PRIV( par_num, val );
        return ePAR_OK;
    }
#else
    PAR_SET_U16_PRIV( par_num, val );
    return ePAR_OK;
#endif
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set signed 16-bit parameter fast
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    val     - Value of parameter
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_set_i16_fast(const par_num_t par_num, const int16_t val)
{
    PAR_ASSERT( true == par_is_init());
    PAR_ASSERT( ePAR_TYPE_I16 == par_get_type(par_num));

#if ( 1 == PAR_CFG_ENABLE_RANGE )
    const par_range_t range = par_get_range(par_num);

    if ( val > range.max.i16 )
    {
        PAR_SET_I16_PRIV( par_num, range.max.i16 );
        return ePAR_WAR_LIMITED;
    }
    else if ( val < range.min.i16 )
    {
        PAR_SET_I16_PRIV( par_num, range.min.i16 );
        return ePAR_WAR_LIMITED;
    }
    else
    {
        PAR_SET_I16_PRIV( par_num, val );
        return ePAR_OK;
    }
#else
    PAR_SET_I16_PRIV( par_num, val );
    return ePAR_OK;
#endif
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set unsigned 16-bit parameter fast
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    val     - Value of parameter
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_set_u32_fast(const par_num_t par_num, const uint32_t val)
{
    PAR_ASSERT( true == par_is_init());
    PAR_ASSERT( ePAR_TYPE_U32 == par_get_type(par_num));

#if ( 1 == PAR_CFG_ENABLE_RANGE )
    const par_range_t range = par_get_range(par_num);

    if ( val > range.max.u32 )
    {
        PAR_SET_U32_PRIV( par_num, range.max.u32 );
        return ePAR_WAR_LIMITED;
    }
    else if ( val < range.min.u32 )
    {
        PAR_SET_U32_PRIV( par_num, range.min.u32 );
        return ePAR_WAR_LIMITED;
    }
    else
    {
        PAR_SET_U32_PRIV( par_num, val );
        return ePAR_OK;
    }
#else
    PAR_SET_U32_PRIV( par_num, val );
    return ePAR_OK;
#endif
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set signed 32-bit parameter fast
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    val     - Value of parameter
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_set_i32_fast(const par_num_t par_num, const int32_t val)
{
    PAR_ASSERT( true == par_is_init());
    PAR_ASSERT( ePAR_TYPE_I32 == par_get_type(par_num));

#if ( 1 == PAR_CFG_ENABLE_RANGE )
    const par_range_t range = par_get_range(par_num);

    if ( val > range.max.i32 )
    {
        PAR_SET_I32_PRIV( par_num, range.max.i32 );
        return ePAR_WAR_LIMITED;
    }
    else if ( val < range.min.i32 )
    {
        PAR_SET_I32_PRIV( par_num, range.min.i32 );
        return ePAR_WAR_LIMITED;
    }
    else
    {
        PAR_SET_I32_PRIV( par_num, val );
        return ePAR_OK;
    }
#else
    PAR_SET_I32_PRIV( par_num, val );
    return ePAR_OK;
#endif
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set floating value parameter fast
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    val     - Value of parameter
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_set_f32_fast(const par_num_t par_num, const float32_t val)
{
    PAR_ASSERT( true == par_is_init());
    PAR_ASSERT( ePAR_TYPE_F32 == par_get_type(par_num));

#if ( 1 == PAR_CFG_ENABLE_RANGE )
    const par_range_t range = par_get_range(par_num);

    if ( val > range.max.f32 )
    {
        PAR_SET_F32_PRIV( par_num, range.max.f32 );
        return ePAR_WAR_LIMITED;
    }
    else if ( val < range.min.f32 )
    {
        PAR_SET_F32_PRIV( par_num, range.min.f32 );
        return ePAR_WAR_LIMITED;
    }
    else
    {
        PAR_SET_F32_PRIV( par_num, val );
        return ePAR_OK;
    }
#else
    PAR_SET_F32_PRIV( par_num, val );
    return ePAR_OK;
#endif
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set unsigned 8-bit parameter ANDing with current set value fast
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    val     - Value of parameter
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_bitand_set_u8_fast(const par_num_t par_num, const uint8_t val)
{
    PAR_ASSERT( true == par_is_init());
    PAR_ASSERT( ePAR_TYPE_U8 == par_get_type(par_num));

#if ( 1 == PAR_CFG_ENABLE_RANGE )
    const par_range_t range = par_get_range(par_num);

    if ( val > range.max.u8 )
    {
        PAR_ATOMIC_FETCH_AND(u8, &gpu8_par_value[gu32_par_offset[par_num]], range.max.u8);
        return ePAR_WAR_LIMITED;
    }
    else if ( val < range.min.u8 )
    {
        PAR_ATOMIC_FETCH_AND(u8, &gpu8_par_value[gu32_par_offset[par_num]], range.min.u8);
        return ePAR_WAR_LIMITED;
    }
    else
    {
        PAR_ATOMIC_FETCH_AND(u8, &gpu8_par_value[gu32_par_offset[par_num]], val);
        return ePAR_OK;
    }
#else
    PAR_ATOMIC_FETCH_AND(u8, &gpu8_par_value[gu32_par_offset[par_num]], val);
    return ePAR_OK;
#endif
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set unsigned 16-bit parameter ANDing with current set value  fast
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    val     - Value of parameter
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_bitand_set_u16_fast(const par_num_t par_num, const uint16_t val)
{
    PAR_ASSERT( true == par_is_init());
    PAR_ASSERT( ePAR_TYPE_U16 == par_get_type(par_num));

#if ( 1 == PAR_CFG_ENABLE_RANGE )
    const par_range_t range = par_get_range(par_num);

    if ( val > range.max.u16 )
    {
        PAR_ATOMIC_FETCH_AND(u16, &gpu16_par_value[gu32_par_offset[par_num]], range.max.u16);
        return ePAR_WAR_LIMITED;
    }
    else if ( val < range.min.u16 )
    {
        PAR_ATOMIC_FETCH_AND(u16, &gpu16_par_value[gu32_par_offset[par_num]], range.min.u16);
        return ePAR_WAR_LIMITED;
    }
    else
    {
        PAR_ATOMIC_FETCH_AND(u16, &gpu16_par_value[gu32_par_offset[par_num]], val);
        return ePAR_OK;
    }
#else
    PAR_ATOMIC_FETCH_AND(u16, &gpu16_par_value[gu32_par_offset[par_num]], val);
    return ePAR_OK;
#endif
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set unsigned 16-bit parameter ANDing with current set value  fast
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    val     - Value of parameter
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_bitand_set_u32_fast(const par_num_t par_num, const uint32_t val)
{
    PAR_ASSERT( true == par_is_init());
    PAR_ASSERT( ePAR_TYPE_U32 == par_get_type(par_num));

#if ( 1 == PAR_CFG_ENABLE_RANGE )
    const par_range_t range = par_get_range(par_num);

    if ( val > range.max.u32 )
    {
        PAR_ATOMIC_FETCH_AND(u32, &gpu32_par_value[gu32_par_offset[par_num]], range.max.u32);
        return ePAR_WAR_LIMITED;
    }
    else if ( val < range.min.u32 )
    {
        PAR_ATOMIC_FETCH_AND(u32, &gpu32_par_value[gu32_par_offset[par_num]], range.min.u32);
        return ePAR_WAR_LIMITED;
    }
    else
    {
        PAR_ATOMIC_FETCH_AND(u32, &gpu32_par_value[gu32_par_offset[par_num]], val);
        return ePAR_OK;
    }
#else
    PAR_ATOMIC_FETCH_AND(u32, &gpu32_par_value[gu32_par_offset[par_num]], val);
    return ePAR_OK;
#endif
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set unsigned 8-bit parameter ORing with current set value fast
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    val     - Value of parameter
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_bitor_set_u8_fast(const par_num_t par_num, const uint8_t val)
{
    PAR_ASSERT( true == par_is_init());
    PAR_ASSERT( ePAR_TYPE_U8 == par_get_type(par_num));

#if ( 1 == PAR_CFG_ENABLE_RANGE )
    const par_range_t range = par_get_range(par_num);

    if ( val > range.max.u8 )
    {
        PAR_ATOMIC_FETCH_OR(u8, &gpu8_par_value[gu32_par_offset[par_num]], range.max.u8);
        return ePAR_WAR_LIMITED;
    }
    else if ( val < range.min.u8 )
    {
        PAR_ATOMIC_FETCH_OR(u8, &gpu8_par_value[gu32_par_offset[par_num]], range.min.u8);
        return ePAR_WAR_LIMITED;
    }
    else
    {
        PAR_ATOMIC_FETCH_OR(u8, &gpu8_par_value[gu32_par_offset[par_num]], val);
        return ePAR_OK;
    }
#else
    PAR_ATOMIC_FETCH_OR(u8, &gpu8_par_value[gu32_par_offset[par_num]], val);
    return ePAR_OK;
#endif
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set unsigned 16-bit parameter ORing with current set value  fast
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    val     - Value of parameter
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_bitor_set_u16_fast(const par_num_t par_num, const uint16_t val)
{
    PAR_ASSERT( true == par_is_init());
    PAR_ASSERT( ePAR_TYPE_U16 == par_get_type(par_num));

#if ( 1 == PAR_CFG_ENABLE_RANGE )
    const par_range_t range = par_get_range(par_num);

    if ( val > range.max.u16 )
    {
        PAR_ATOMIC_FETCH_OR(u16, &gpu16_par_value[gu32_par_offset[par_num]], range.max.u16);
        return ePAR_WAR_LIMITED;
    }
    else if ( val < range.min.u16 )
    {
        PAR_ATOMIC_FETCH_OR(u16, &gpu16_par_value[gu32_par_offset[par_num]], range.min.u16);
        return ePAR_WAR_LIMITED;
    }
    else
    {
        PAR_ATOMIC_FETCH_OR(u16, &gpu16_par_value[gu32_par_offset[par_num]], val);
        return ePAR_OK;
    }
#else
    PAR_ATOMIC_FETCH_OR(u16, &gpu16_par_value[gu32_par_offset[par_num]], val);
    return ePAR_OK;
#endif
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Set unsigned 16-bit parameter ORing with current set value  fast
*
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    val     - Value of parameter
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_bitor_set_u32_fast(const par_num_t par_num, const uint32_t val)
{
    PAR_ASSERT( true == par_is_init());
    PAR_ASSERT( ePAR_TYPE_U32 == par_get_type(par_num));

#if ( 1 == PAR_CFG_ENABLE_RANGE )
    const par_range_t range = par_get_range(par_num);

    if ( val > range.max.u32 )
    {
        PAR_ATOMIC_FETCH_OR(u32, &gpu32_par_value[gu32_par_offset[par_num]], range.max.u32);
        return ePAR_WAR_LIMITED;
    }
    else if ( val < range.min.u32 )
    {
        PAR_ATOMIC_FETCH_OR(u32, &gpu32_par_value[gu32_par_offset[par_num]], range.min.u32);
        return ePAR_WAR_LIMITED;
    }
    else
    {
        PAR_ATOMIC_FETCH_OR(u32, &gpu32_par_value[gu32_par_offset[par_num]], val);
        return ePAR_OK;
    }
#else
    PAR_ATOMIC_FETCH_OR(u32, &gpu32_par_value[gu32_par_offset[par_num]], val);
    return ePAR_OK;
#endif
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
    for ( par_num_t par_num = 0; par_num < ePAR_NUM_OF; par_num++ )
    {
        // Ignore return as it is not possible to return other that OK
        (void) par_set_to_default( par_num );
    }

    PAR_DBG_PRINT( "PAR: Setting all parameters to default" );
    return ePAR_OK;
}

////////////////////////////////////////////////////////////////////////////////
/**
*		Check if parameter changed from its default value
*
* @param[in]	par_num       - Parameter number (enumeration)
* @param[out]	p_has_changed - Pointer to changed indication
* @return		status         - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_has_changed(const par_num_t par_num, bool *const p_has_changed)
{
    const par_cfg_t * const par_cfg = par_get_config(par_num);

    switch ( par_cfg->type )
    {
        case ePAR_TYPE_U8:
            *p_has_changed = (par_get_u8(par_num) != par_cfg->def.u8);
            break;

        case ePAR_TYPE_I8:
            *p_has_changed = (par_get_i8(par_num) != par_cfg->def.i8);
            break;

        case ePAR_TYPE_U16:
            *p_has_changed = (par_get_u16(par_num) != par_cfg->def.u16);
            break;

        case ePAR_TYPE_I16:
            *p_has_changed = (par_get_i16(par_num) != par_cfg->def.i16);
            break;

        case ePAR_TYPE_U32:
            *p_has_changed = (par_get_u32(par_num) != par_cfg->def.u32);
            break;

        case ePAR_TYPE_I32:
            *p_has_changed = (par_get_i32(par_num) != par_cfg->def.i32);
            break;

        case ePAR_TYPE_F32:
            *p_has_changed = (par_get_f32(par_num) != par_cfg->def.f32);
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
#if ( 1 == PAR_CFG_ENABLE_ID )
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
#endif

////////////////////////////////////////////////////////////////////////////////
/**
*        Get unsigned 8-bit parameter value
*
* @note Returning as *(volatile uint8_t*) prevent load tearing as explained
*       here: https://lwn.net/Articles/793253/
*
* @param[in]    par_num - Parameter number (enumeration)
* @return       value   - Value of parameter
*/
////////////////////////////////////////////////////////////////////////////////
uint8_t par_get_u8(const par_num_t par_num)
{
    // Check initialization
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    // Check for invalid type
    PAR_ASSERT( ePAR_TYPE_U8 == par_get_type(par_num));
    if( ePAR_TYPE_U8 != par_get_type(par_num)) return ePAR_ERROR_TYPE;

    return PAR_GET_U8_PRIV( par_num );
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
    // Check initialization
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    // Check for invalid type
    PAR_ASSERT( ePAR_TYPE_I8 == par_get_type(par_num));
    if( ePAR_TYPE_I8 != par_get_type(par_num)) return ePAR_ERROR_TYPE;

    return PAR_GET_I8_PRIV( par_num );
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
    // Check initialization
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    // Check for invalid type
    PAR_ASSERT( ePAR_TYPE_U16 == par_get_type(par_num));
    if( ePAR_TYPE_U16 != par_get_type(par_num)) return ePAR_ERROR_TYPE;

    return PAR_GET_U16_PRIV( par_num );
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
    // Check initialization
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    // Check for invalid type
    PAR_ASSERT( ePAR_TYPE_I16 == par_get_type(par_num));
    if( ePAR_TYPE_I16 != par_get_type(par_num)) return ePAR_ERROR_TYPE;

    return PAR_GET_I16_PRIV( par_num );
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
    // Check initialization
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    // Check for invalid type
    PAR_ASSERT( ePAR_TYPE_U32 == par_get_type(par_num));
    if( ePAR_TYPE_U32 != par_get_type(par_num)) return ePAR_ERROR_TYPE;

    return PAR_GET_U32_PRIV( par_num );
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
    // Check initialization
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    // Check for invalid type
    PAR_ASSERT( ePAR_TYPE_I32 == par_get_type(par_num));
    if( ePAR_TYPE_I32 != par_get_type(par_num)) return ePAR_ERROR_TYPE;

    return PAR_GET_I32_PRIV( par_num );
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
    // Check initialization
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    // Check for invalid type
    PAR_ASSERT( ePAR_TYPE_F32 == par_get_type(par_num));
    if( ePAR_TYPE_F32 != par_get_type(par_num)) return ePAR_ERROR_TYPE;

    return PAR_GET_F32_PRIV( par_num );
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

    return par_cfg_get(par_num);
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Get parameter name
*
* @param[in]    par_num  - Parameter number (enumeration)
* @return       Parameter name
*/
////////////////////////////////////////////////////////////////////////////////
#if ( 1 == PAR_CFG_ENABLE_NAME )
const char * par_get_name(const par_num_t par_num)
{
    const par_cfg_t * const par_cfg = par_get_config(par_num);

    if ( NULL != par_cfg )
    {
        return par_cfg->name;
    }

    return NULL;
}
#endif

////////////////////////////////////////////////////////////////////////////////
/**
*        Get parameter value range
*
* @param[in]    par_num - Parameter number (enumeration)
* @return       Parameter min/max range
*/
////////////////////////////////////////////////////////////////////////////////
#if ( 1 == PAR_CFG_ENABLE_RANGE )
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
#endif

////////////////////////////////////////////////////////////////////////////////
/**
*        Get parameter unit
*
* @param[in]    par_num  - Parameter number (enumeration)
* @return       Parameter unit
*/
////////////////////////////////////////////////////////////////////////////////
#if ( 1 == PAR_CFG_ENABLE_UNIT )
const char * par_get_unit(const par_num_t par_num)
{
    const par_cfg_t * const par_cfg = par_get_config(par_num);

    if ( NULL != par_cfg )
    {
        return par_cfg->unit;
    }

    return NULL;
}
#endif

////////////////////////////////////////////////////////////////////////////////
/**
*        Get parameter description
*
* @param[in]    par_num  - Parameter number (enumeration)
* @return       Parameter description
*/
////////////////////////////////////////////////////////////////////////////////
#if ( 1 == PAR_CFG_ENABLE_DESC )
const char * par_get_desc(const par_num_t par_num)
{
    const par_cfg_t * const par_cfg = par_get_config(par_num);

    if ( NULL != par_cfg )
    {
        return par_cfg->desc;
    }

    return NULL;
}
#endif

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
#if ( 1 == PAR_CFG_ENABLE_ACCESS )
par_access_t par_get_access(const par_num_t par_num)
{
    const par_cfg_t * const par_cfg = par_get_config(par_num);

    if ( NULL != par_cfg )
    {
        return par_cfg->access;
    }

    return ePAR_ACCESS_RO;
}
#endif

////////////////////////////////////////////////////////////////////////////////
/**
*        Is parameter persistant (does it stores to NVM)
*
* @param[in]    par_num  - Parameter number (enumeration)
* @return       True if parameter persistant
*/
////////////////////////////////////////////////////////////////////////////////
#if ( 1 == PAR_CFG_ENABLE_PERSIST )
bool par_is_persistant(const par_num_t par_num)
{
    const par_cfg_t * const par_cfg = par_get_config(par_num);

    if ( NULL != par_cfg )
    {
        return par_cfg->persistant;
    }

    return false;
}
#endif

////////////////////////////////////////////////////////////////////////////////
/**
*        Get parameter number (enumeration) by ID
*
* @param[in]    id          - Parameter ID
* @param[out]   p_par_num   - Pointer to parameter enumeration number
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
#if ( 1 == PAR_CFG_ENABLE_ID )
par_status_t par_get_num_by_id(const uint16_t id, par_num_t * const p_par_num)
{
    if (( NULL != p_par_num ) && ( true == gb_par_id_map_ready ))
    {
        const uint32_t bucket_idx = par_hash_id( id );
        const par_id_map_entry_t * const bucket = &g_par_id_map[bucket_idx];

        if (( 0u != bucket->used ) && ( id == bucket->id ))
        {
            *p_par_num = bucket->par_num;
            return ePAR_OK;
        }
    }

    return ePAR_ERROR;
}
#endif

////////////////////////////////////////////////////////////////////////////////
/**
*        Get parameter ID by number (enumeration)
*
* @param[in]    par_num - Parameter number
* @param[out]   p_id    - Pointer to parameter ID
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
#if ( 1 == PAR_CFG_ENABLE_ID )
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
#endif

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
#if ( 1 == PAR_CFG_ENABLE_ID )
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
#endif

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
* @param[in]    par_num - Parameter number (enumeration)
* @param[in]    cb      - Callback
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
void par_register_on_change_cb(const par_num_t par_num, const pf_par_on_change_cb_t cb)
{
    PAR_ASSERT( par_num < ePAR_NUM_OF );

    g_par_cb_table[par_num].on_change = cb;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Register parameter value validation function
*
* @param[in]    par_num     - Parameter number (enumeration)
* @param[in]    validation  - Validation
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
void par_register_validation(const par_num_t par_num, const pf_par_validation_t validation)
{
    PAR_ASSERT( par_num < ePAR_NUM_OF );

    g_par_cb_table[par_num].validation = validation;
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
