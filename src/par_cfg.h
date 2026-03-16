// Copyright (c) 2026 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      par_cfg.h
*@brief    	Configuration for device parameters
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@date      29.01.2026
*@version   V3.0.1
*/
////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup PAR_CFG
* @{ <!-- BEGIN GROUP -->
*
* 	Configuration for device parameters.
*/
////////////////////////////////////////////////////////////////////////////////

#ifndef _PAR_CFG_H_
#define _PAR_CFG_H_

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include "par_def.h"

// USER CODE BEGIN...

/**
 *  Platform adaptation bridge
 *
 * @note   Port layer may override default PAR_CFG_* settings.
 *
 * @note   This header is included unconditionally.
 *         Integrator shall provide "par_cfg_port.h" in include path.
 *         If no platform override is needed, create an empty stub header
 *         with include guard (for example in port/par_cfg_port.h).
 */
#include "par_cfg_port.h"

// USER CODE END...

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 *  Enable/Disable storing persistent parameters to NVM
 */
#ifndef PAR_CFG_NVM_EN
    #define PAR_CFG_NVM_EN                          ( 1 )
#endif

/**
 *  NVM parameter region option
 *
 * @note   User shall select region based on nvm_cfg.h region
 *         definitions "nvm_region_name_t".
 *
 *         Don't care if "PAR_CFG_NVM_EN" set to 0.
 */
#ifndef PAR_CFG_NVM_REGION
    #define PAR_CFG_NVM_REGION                      ( eNVM_REGION_INT_FLASH_DEV_PAR )
#endif

/**
 *  Enable/Disable parameter table unique ID checking
 *
 * @note   Base on hash unique ID is being calculated with purpose to detect
 *         device and stored parameter table difference.
 *
 *         Must be disabled once the device is released in order to prevent
 *         loss of calibrated data stored in NVM.
 *
 * @pre    "PAR_CFG_NVM_EN" must be enabled otherwise it does not make sense
 *         to calculate ID at all.
 */
#ifndef PAR_CFG_TABLE_ID_CHECK_EN
    #define PAR_CFG_TABLE_ID_CHECK_EN               ( 0 )
#endif

#if ( 1 == PAR_CFG_NVM_EN )
    #ifndef DEBUG
        #undef PAR_CFG_TABLE_ID_CHECK_EN
        #define PAR_CFG_TABLE_ID_CHECK_EN          ( 0 )
    #endif
#endif

/**
 *  Enable/Disable debug mode
 */
#ifndef PAR_CFG_DEBUG_EN
    #define PAR_CFG_DEBUG_EN                        ( 1 )
#endif

#ifndef DEBUG
    #undef PAR_CFG_DEBUG_EN
    #define PAR_CFG_DEBUG_EN                        ( 0 )
#endif

/**
 *  Enable/Disable assertions
 */
#ifndef PAR_CFG_ASSERT_EN
    #define PAR_CFG_ASSERT_EN                       ( 1 )
#endif

#ifndef DEBUG
    #undef PAR_CFG_ASSERT_EN
    #define PAR_CFG_ASSERT_EN                       ( 0 )
#endif

/**
 *  Platform hook fallbacks
 */
#ifndef PAR_PORT_ASSERT
    #define PAR_PORT_ASSERT(x)                      do { (void)(x); } while (0)
#endif

#ifndef PAR_PORT_LOG
    #define PAR_PORT_LOG(tag, ...)                  do { (void)(tag); } while (0)
#endif

#ifndef PAR_PORT_STATIC_ASSERT
    #define PAR_PORT_STATIC_ASSERT(name, expn)      typedef char _static_assert_##name[(expn) ? 1 : -1]
#endif

/**
 *  Package compile-time assert
 */
#define PAR_STATIC_ASSERT(name, expn)               PAR_PORT_STATIC_ASSERT(name, expn);

/**
 *  Resolve log/assert routing mode before default macro emission
 */
#if !defined(PAR_CFG_PORT_HOOK_EN)
    #define PAR_CFG_USE_PORT_HOOKS                  ( 0 )
#elif ( 1 == PAR_CFG_PORT_HOOK_EN )
    #define PAR_CFG_USE_PORT_HOOKS                  ( 1 )
#else
    #define PAR_CFG_USE_PORT_HOOKS                  ( 0 )
#endif

/**
 *  Debug communication port macros
 */
#if ( 0 == PAR_CFG_USE_PORT_HOOKS )
    #if ( 1 == PAR_CFG_DEBUG_EN )
        #ifndef PAR_CFG_DIRECT_LOG
            #define PAR_CFG_DIRECT_LOG(...)         ( cli_printf((char *) __VA_ARGS__) )
        #endif
        #define PAR_DBG_PRINT(...)                  PAR_CFG_DIRECT_LOG(__VA_ARGS__)
    #else
        #define PAR_DBG_PRINT(...)                  { ; }
    #endif
#else
    #if ( 1 == PAR_CFG_DEBUG_EN )
        #define PAR_DBG_PRINT(...)                  PAR_PORT_LOG(__VA_ARGS__)
    #else
        #define PAR_DBG_PRINT(...)                  { ; }
    #endif
#endif

/**
 *  Assertion macros
 */
#if ( 0 == PAR_CFG_USE_PORT_HOOKS )
    #if ( 1 == PAR_CFG_ASSERT_EN )
        #ifndef PAR_CFG_DIRECT_ASSERT
            #ifdef PROJ_CFG_ASSERT
                #define PAR_CFG_DIRECT_ASSERT(x)    PROJ_CFG_ASSERT(x)
            #else
                #define PAR_CFG_DIRECT_ASSERT(x)    do { (void)(x); } while (0)
            #endif
        #endif
        #define PAR_ASSERT(x)                       PAR_CFG_DIRECT_ASSERT(x)
    #else
        #define PAR_ASSERT(x)                       { ; }
    #endif
#else
    #if ( 1 == PAR_CFG_ASSERT_EN )
        #define PAR_ASSERT(x)                       PAR_PORT_ASSERT(x)
    #else
        #define PAR_ASSERT(x)                       { ; }
    #endif
#endif

#undef PAR_CFG_USE_PORT_HOOKS

/**
 *  Invalid configuration catcher
 *
 * @note   Shall be intact by end user!
 */
#if ( 0 == PAR_CFG_NVM_EN ) && ( 1 == PAR_CFG_TABLE_ID_CHECK_EN )
    #error "Parameter settings invalid: Disable table ID checking (PAR_CFG_TABLE_ID_CHECK_EN)!"
#endif

/**
 *  Extended package configurations (non-template additions)
 */
#ifndef PAR_CFG_MUTEX_EN
    #define PAR_CFG_MUTEX_EN                        ( 1 )
#endif

/**
 *  Parameter mutex timeout
 *
 *  Unit: ms
 */
#ifndef PAR_CFG_MUTEX_TIMEOUT_MS
    #define PAR_CFG_MUTEX_TIMEOUT_MS                ( 10 )
#endif

/**
 *  Enable/Disable port-specific par_if backend
 */
#ifndef PAR_CFG_IF_PORT_EN
    #define PAR_CFG_IF_PORT_EN                      ( 0 )
#endif

/**
 *  Enable/Disable port hooks for log/assert
 */
#ifndef PAR_CFG_PORT_HOOK_EN
    #define PAR_CFG_PORT_HOOK_EN                    ( 0 )
#endif

/**
 *  Enable/Disable parameter range metadata (min/max)
 */
#ifndef PAR_CFG_ENABLE_RANGE
    #define PAR_CFG_ENABLE_RANGE                    ( 1 )
#endif

/**
 *  Enable/Disable parameter name metadata
 */
#ifndef PAR_CFG_ENABLE_NAME
    #define PAR_CFG_ENABLE_NAME                     ( 1 )
#endif

/**
 *  Enable/Disable parameter unit metadata
 */
#ifndef PAR_CFG_ENABLE_UNIT
    #define PAR_CFG_ENABLE_UNIT                     ( 1 )
#endif

/**
 *  Enable/Disable parameter description metadata
 */
#ifndef PAR_CFG_ENABLE_DESC
    #define PAR_CFG_ENABLE_DESC                     ( 1 )
#endif

/**
 *  Enable/Disable parameter ID metadata
 */
#ifndef PAR_CFG_ENABLE_ID
    #define PAR_CFG_ENABLE_ID                       ( 1 )
#endif

/**
 *  Enable/Disable parameter access metadata
 */
#ifndef PAR_CFG_ENABLE_ACCESS
    #define PAR_CFG_ENABLE_ACCESS                   ( 1 )
#endif

/**
 *  Enable/Disable parameter persistence metadata
 */
#ifndef PAR_CFG_ENABLE_PERSIST
    #define PAR_CFG_ENABLE_PERSIST                  ( 1 )
#endif

/**
 *  Enable/Disable description comma check
 *
 * @note  Default follows PAR_CFG_ENABLE_DESC.
 */
#ifndef PAR_CFG_ENABLE_DESC_COMMA_CHECK
    #define PAR_CFG_ENABLE_DESC_COMMA_CHECK         ( PAR_CFG_ENABLE_DESC )
#endif

/**
 *  Configuration dependency checks for optional fields/features
 */
#if ( 1 == PAR_CFG_NVM_EN ) && ( 0 == PAR_CFG_ENABLE_ID )
    #error "Parameter settings invalid: NVM requires PAR_CFG_ENABLE_ID = 1!"
#endif

#if ( 1 == PAR_CFG_NVM_EN ) && ( 0 == PAR_CFG_ENABLE_PERSIST )
    #error "Parameter settings invalid: NVM requires PAR_CFG_ENABLE_PERSIST = 1!"
#endif

////////////////////////////////////////////////////////////////////////////////
// Functions Prototypes
////////////////////////////////////////////////////////////////////////////////
const par_cfg_t * par_cfg_get_table      (void);
const par_cfg_t * par_cfg_get            (const par_num_t par_num);
uint32_t     par_cfg_get_table_size (void);

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

#endif // _PAR_CFG_H_
