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
#include <stdbool.h>
#include <string.h>

#include "par.h"
#include "par_atomic.h"
#include "par_layout.h"
#include "par_nvm.h"
#include "par_if.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////
#if ( 1 == PAR_CFG_ENABLE_ID )
/*
 * http://www.citi.umich.edu/techreports/reports/citi-tr-00-1.pdf
 *
 * GoldenRatio = ~(Math.pow(2, 32) / ((Math.sqrt(5) - 1) / 2)) + 1
 */
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

PAR_STATIC_ASSERT(par_atomic_u8_i8_same_size, sizeof(par_atomic_u8_t) == sizeof(par_atomic_i8_t));
PAR_STATIC_ASSERT(par_atomic_u8_i8_same_align, PAR_ALIGNOF(par_atomic_u8_t) == PAR_ALIGNOF(par_atomic_i8_t));
PAR_STATIC_ASSERT(par_atomic_u16_i16_same_size, sizeof(par_atomic_u16_t) == sizeof(par_atomic_i16_t));
PAR_STATIC_ASSERT(par_atomic_u16_i16_same_align, PAR_ALIGNOF(par_atomic_u16_t) == PAR_ALIGNOF(par_atomic_i16_t));
PAR_STATIC_ASSERT(par_atomic_u32_i32_same_size, sizeof(par_atomic_u32_t) == sizeof(par_atomic_i32_t));
PAR_STATIC_ASSERT(par_atomic_u32_i32_same_align, PAR_ALIGNOF(par_atomic_u32_t) == PAR_ALIGNOF(par_atomic_i32_t));
PAR_STATIC_ASSERT(par_atomic_u32_f32_same_size, sizeof(par_atomic_u32_t) == sizeof(par_atomic_f32_t));
PAR_STATIC_ASSERT(par_atomic_u32_f32_same_align, PAR_ALIGNOF(par_atomic_u32_t) == PAR_ALIGNOF(par_atomic_f32_t));

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

/**
 *     Initialization guard
 */
static bool gb_is_init = false;

/**
 * Parameter callback table.
 *
 * @note Keep runtime hooks separate from par_cfg_t metadata table.
 */
#if (( 1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION ) || ( 1 == PAR_CFG_ENABLE_CHANGE_CALLBACK ))
static struct
{
#if ( 1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION )
    pf_par_validation_t  validation;    /**< Validation callback function (or NULL). */
#endif
#if ( 1 == PAR_CFG_ENABLE_CHANGE_CALLBACK )
    pf_par_on_change_cb_t on_change;    /**< On change callback function (or NULL). */
#endif
} g_par_cb_table[ePAR_NUM_OF];
#endif

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
 *  Static typed storage backing parameter live values.
 *
 * @note  Zero-length groups are mapped to size 1 arrays for compiler portability.
 *        Private implementation fragments. Do not include outside par.c.
 */
#include "par_storage_init.inc"

#if ( 1 == PAR_CFG_ENABLE_RESET_ALL_RAW )
/**
 *  Runtime default mirror storage for raw reset-all API.
 *
 * @note Mirrors are initialized in par_init() from current live defaults
 *       after F32 startup patch and before optional NVM load.
 */
static par_atomic_u8_t  gs_par_u8_default_mirror[PAR_STORAGE_NONZERO(PAR_STORAGE_COUNT8)]   = {0};
static par_atomic_u16_t gs_par_u16_default_mirror[PAR_STORAGE_NONZERO(PAR_STORAGE_COUNT16)]  = {0};
static par_atomic_u32_t gs_par_u32_default_mirror[PAR_STORAGE_NONZERO(PAR_STORAGE_COUNT32)]  = {0};
#endif

/**
 *  Parameter live values divided by its type in RAM
 */
static par_atomic_u8_t *     gpu8_par_value = gs_par_u8_storage;
static par_atomic_i8_t *     gpi8_par_value = (par_atomic_i8_t *)gs_par_u8_storage;
static par_atomic_u16_t *    gpu16_par_value = gs_par_u16_storage;
static par_atomic_i16_t *    gpi16_par_value = (par_atomic_i16_t *)gs_par_u16_storage;
static par_atomic_u32_t *    gpu32_par_value = gs_par_u32_storage;
static par_atomic_i32_t *    gpi32_par_value = (par_atomic_i32_t *)gs_par_u32_storage;
#if ( 1 == PAR_CFG_ENABLE_TYPE_F32 )
static par_atomic_f32_t *    gpf32_par_value = (par_atomic_f32_t *)gs_par_u32_storage;
#endif

/**
 *  Offset table compatibility alias.
 *
 * @note  Layout offsets are now owned by par_layout and accessed via getter.
 *        Keep this local alias so existing indexed access sites remain unchanged.
 */
#define g_par_offset    (par_layout_get_offset_table())

/**
 *  Private getters and setters
 */
#define PAR_GET_U8_PRIV(par_num)        PAR_ATOMIC_LOAD(u8, &gpu8_par_value[g_par_offset[par_num]])
#define PAR_GET_I8_PRIV(par_num)        PAR_ATOMIC_LOAD(i8, &gpi8_par_value[g_par_offset[par_num]])
#define PAR_GET_U16_PRIV(par_num)       PAR_ATOMIC_LOAD(u16, &gpu16_par_value[g_par_offset[par_num]])
#define PAR_GET_I16_PRIV(par_num)       PAR_ATOMIC_LOAD(i16, &gpi16_par_value[g_par_offset[par_num]])
#define PAR_GET_U32_PRIV(par_num)       PAR_ATOMIC_LOAD(u32, &gpu32_par_value[g_par_offset[par_num]])
#define PAR_GET_I32_PRIV(par_num)       PAR_ATOMIC_LOAD(i32, &gpi32_par_value[g_par_offset[par_num]])
#if ( 1 == PAR_CFG_ENABLE_TYPE_F32 )
#define PAR_GET_F32_PRIV(par_num)       PAR_ATOMIC_LOAD(f32, &gpf32_par_value[g_par_offset[par_num]])
#endif

#define PAR_SET_U8_PRIV(par_num, val)   PAR_ATOMIC_STORE(u8, &gpu8_par_value[g_par_offset[par_num]], (val))
#define PAR_SET_I8_PRIV(par_num, val)   PAR_ATOMIC_STORE(i8, &gpi8_par_value[g_par_offset[par_num]], (val))
#define PAR_SET_U16_PRIV(par_num, val)  PAR_ATOMIC_STORE(u16, &gpu16_par_value[g_par_offset[par_num]], (val))
#define PAR_SET_I16_PRIV(par_num, val)  PAR_ATOMIC_STORE(i16, &gpi16_par_value[g_par_offset[par_num]], (val))
#define PAR_SET_U32_PRIV(par_num, val)  PAR_ATOMIC_STORE(u32, &gpu32_par_value[g_par_offset[par_num]], (val))
#define PAR_SET_I32_PRIV(par_num, val)  PAR_ATOMIC_STORE(i32, &gpi32_par_value[g_par_offset[par_num]], (val))
#if ( 1 == PAR_CFG_ENABLE_TYPE_F32 )
#define PAR_SET_F32_PRIV(par_num, val)  PAR_ATOMIC_STORE(f32, &gpf32_par_value[g_par_offset[par_num]], (val))
#endif

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
#if ( 1 == PAR_CFG_ENABLE_ID )
static inline uint32_t  par_hash_id                     (const uint16_t id);
static par_status_t     par_build_and_validate_id_map   (const par_cfg_t * const p_par_cfg);
#endif
#if ( 1 == PAR_CFG_NVM_EN )
static bool         par_is_value_changed            (const par_num_t par_num, const void * p_val);
#endif /* ( 1 == PAR_CFG_NVM_EN ) */
static par_status_t     par_check_table_validity        (const par_cfg_t * const p_par_cfg);

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
#if ( 1 == PAR_CFG_ENABLE_DESC ) && ( 1 == PAR_CFG_ENABLE_DESC_CHECK )
/**
*        Validate parameter description string
*
* @note         Default weak implementation only prohibits comma character.
*               Application may override this symbol with stronger policy.
*
* @param[in]    p_desc - Parameter description
* @return       true if description is valid
*/
////////////////////////////////////////////////////////////////////////////////
PAR_PORT_WEAK bool par_port_is_desc_valid(const char * const p_desc)
{
    return ((NULL == p_desc) || (NULL == strchr(p_desc, ',')));
}
#endif

#if ( 1 == PAR_CFG_ENABLE_TYPE_F32 )
////////////////////////////////////////////////////////////////////////////////
/**
*        Patch F32 defaults into shared u32 storage as bit-patterns
*
* @note         Integer defaults are already initialized at definition time.
*               F32 defaults are patched once after layout offsets are available.
*
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void par_patch_f32_defaults_from_table(void)
{
    for ( par_num_t par_num = 0; par_num < ePAR_NUM_OF; par_num++ )
    {
        const par_cfg_t * const p_cfg = par_cfg_get( par_num );
        if ( ePAR_TYPE_F32 == p_cfg->type )
        {
            PAR_SET_F32_PRIV( par_num, p_cfg->def.f32 );
        }
    }
}
#endif

////////////////////////////////////////////////////////////////////////////////
/**
 *        Bind static space for live parameter values
*/
////////////////////////////////////////////////////////////////////////////////
static void par_bind_storage_layout(void)
{
    // Initialize and obtain parameter layout (offset map + per-width counts)
    par_layout_init();

    // Calculate full RAM size for static storage groups
    PAR_DBG_PRINT("Total RAM consumption for parameters value: %u bytes", (unsigned)
    (((uint32_t)par_layout_get_count().count32 * 4u) +
    ((uint32_t)par_layout_get_count().count16 * 2u) +
    ((uint32_t)par_layout_get_count().count8)));
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
        PAR_ASSERT(( ePAR_TYPE_F32 == p_par_cfg[i].type ) ? 
        ((( p_par_cfg[i].range.min.f32 < p_par_cfg[i].range.max.f32 ) && 
        ( p_par_cfg[i].def.f32 <= p_par_cfg[i].range.max.f32 )) && 
        (  p_par_cfg[i].range.min.f32 <= p_par_cfg[i].def.f32 )) 
        : ( 1 ));
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

#if ( 1 == PAR_CFG_ENABLE_DESC ) && ( 1 == PAR_CFG_ENABLE_DESC_CHECK )
        if ( false == par_port_is_desc_valid( p_par_cfg[i].desc))
        {
            status = ePAR_ERROR_INIT;
            PAR_DBG_PRINT( "ERR, Parameter %d description is invalid!", i );
            PAR_ASSERT( 0 );
            break;
        }
#endif
    }

    return status;
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
    status |= par_check_table_validity( par_cfg_get_table());

    // Bind storage layout
    par_bind_storage_layout();

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

        /* Set all parameters to default
         * Integer defaults are already initialized at definition time.
         * F32 defaults are patched once after layout offsets are available.
        */
#if ( 1 == PAR_CFG_ENABLE_TYPE_F32 )
            par_patch_f32_defaults_from_table();
#endif

#if ( 1 == PAR_CFG_ENABLE_RESET_ALL_RAW )
            /*
             * Build default mirrors from current live defaults.
             * This snapshot is taken before optional NVM load.
             */
            memcpy( gs_par_u8_default_mirror,  gs_par_u8_storage,  sizeof(gs_par_u8_storage)  );
            memcpy( gs_par_u16_default_mirror, gs_par_u16_storage, sizeof(gs_par_u16_storage) );
            memcpy( gs_par_u32_default_mirror, gs_par_u32_storage, sizeof(gs_par_u32_storage) );
#endif

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

#if ( 1 == PAR_CFG_ENABLE_TYPE_F32 )
        case ePAR_TYPE_F32:
            status = par_set_f32( par_num, *(float32_t*) p_val );
            break;
#endif

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
* Typed setter/getter implementation
* @note Private implementation fragments. Do not include outside par.c.

*/
////////////////////////////////////////////////////////////////////////////////
#include "par_typed_impl.inc"

////////////////////////////////////////////////////////////////////////////////
/**
* Bitwise fast setter implementation
* @note Private implementation fragments. Do not include outside par.c.
*/
////////////////////////////////////////////////////////////////////////////////
#include "par_bitwise_impl.inc"

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
* @note   This function uses normal runtime setter path via par_set_to_default()
*         and keeps setter semantics.
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

#if ( 1 == PAR_CFG_ENABLE_RESET_ALL_RAW )
////////////////////////////////////////////////////////////////////////////////
/**
*        Reset all parameters to default values via raw storage restore
*
* @note         Unlike par_set_all_to_default(), this API restores grouped
*               storage directly from default mirrors instead of iterating over
*               all parameters through the normal setter path.
*
* @note         This path is therefore typically faster for bulk reset, because
*               it avoids per-parameter runtime validation, on-change callback,
*               and setter-side range handling.
*
* @note         Restore is performed per storage width group.
*
* @pre          Parameters must be initialized before usage.
*
* @return       status - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
par_status_t par_reset_all_to_default_raw(void)
{
    PAR_ASSERT( true == par_is_init());
    if ( true != par_is_init()) return ePAR_ERROR_INIT;

    if ( ePAR_OK != par_acquire_mutex((par_num_t)0))
    {
        return ePAR_ERROR_MUTEX;
    }

    memcpy( gs_par_u8_storage,  gs_par_u8_default_mirror,  sizeof(gs_par_u8_storage)  );
    memcpy( gs_par_u16_storage, gs_par_u16_default_mirror, sizeof(gs_par_u16_storage) );
    memcpy( gs_par_u32_storage, gs_par_u32_default_mirror, sizeof(gs_par_u32_storage) );

    par_release_mutex((par_num_t)0);

    PAR_DBG_PRINT( "PAR: Raw reset all parameters to default" );
    return ePAR_OK;
}
#endif

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

#if ( 1 == PAR_CFG_ENABLE_TYPE_F32 )
        case ePAR_TYPE_F32:
            *p_has_changed = (par_get_f32(par_num) != par_cfg->def.f32);
            break;
#endif

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

#if ( 1 == PAR_CFG_ENABLE_TYPE_F32 )
        case ePAR_TYPE_F32:
            *(float32_t*) p_val = par_get_f32(par_num);
            break;
#endif

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

#if ( 1 == PAR_CFG_ENABLE_TYPE_F32 )
        case ePAR_TYPE_F32:
            *(float32_t*) p_val = (float32_t) par_get_config(par_num)->def.f32;
            break;
#endif

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

#if ( 1 == PAR_CFG_ENABLE_TYPE_F32 )
        case ePAR_TYPE_F32:
            return (bool) (par_get_f32(par_num) != par_get_config(par_num)->def.f32);
#endif

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
        return par_cfg->range;
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
#endif

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

    #if ( 1 == PAR_CFG_ENABLE_TYPE_F32 )
            case ePAR_TYPE_F32:
                value_changed = (par_get_f32(par_num) != *(float32_t*)p_val);
                break;
    #endif

            case ePAR_TYPE_NUM_OF:
            default:
                PAR_ASSERT( 0 );
                break;
        }

        return value_changed;
    }
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
#if ( 1 == PAR_CFG_ENABLE_CHANGE_CALLBACK )
void par_register_on_change_cb(const par_num_t par_num, const pf_par_on_change_cb_t cb)
{
    PAR_ASSERT( par_num < ePAR_NUM_OF );

    g_par_cb_table[par_num].on_change = cb;
}
#endif

////////////////////////////////////////////////////////////////////////////////
/**
*        Register parameter value validation function
*
* @param[in]    par_num     - Parameter number (enumeration)
* @param[in]    validation  - Validation
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
#if ( 1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION )
void par_register_validation(const par_num_t par_num, const pf_par_validation_t validation)
{
    PAR_ASSERT( par_num < ePAR_NUM_OF );

    g_par_cb_table[par_num].validation = validation;
}
#endif

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
