/**
 * @file par_if_port.c
 * @brief Implement the platform-specific parameter interface port.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-03-12
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-03-12 1.0     wdfk-prog    first version
 */
/**
 * @addtogroup PAR_IF
 * @{ <!-- BEGIN GROUP -->
 *
 * @brief Interface layer for device parameters.
 *
 * Put code that is platform depended inside code block start with.
 * "USER_CODE_BEGIN" and with end of "USER_CODE_END".
 */
/**
 * @brief Include dependencies.
 */
#include "par_if.h"

#if (1 == PAR_CFG_IF_PORT_EN)

/**
 * @brief USER INCLUDES BEGIN...
 */

/**
 * @brief USER INCLUDES END...
 */
/**
 * @brief Compile-time definitions.
 */
/**
 * @brief USER DEFINITIONS BEGIN...
 */

/*
 * Optional override point:
 * This port file may also provide strong par_if_crc16_accumulate() and
 * par_if_crc8_accumulate() definitions when the target wants to route
 * parameter NVM integrity checks through a platform layer.
 *
 * If no strong override is provided here or elsewhere, the portable weak
 * software defaults from parameters/src/port/par_if.c are used.
 */

/**
 * @brief USER DEFINITIONS END...
 */
/**
 * @brief Module-scope variables.
 */
/**
 * @brief USER VARIABLES BEGIN...
 */

/**
 * @brief Parameters OS mutex.
 */
static struct rt_mutex g_par_mutex;

/**
 * @brief Mutex initialization state flag.
 */
static bool g_par_mutex_ready = false;
/**
 * @brief USER VARIABLES END...
 */
/**
 * @brief Function declarations and definitions.
 */
/**
 * @brief Initialize low level interface.
 *
 * @note User shall provide definition of that function based on used platform!
 *
 * @return Status of initialization.
 */
par_status_t par_if_init(void)
{
    par_status_t status = ePAR_OK;
#if (1 == PAR_CFG_MUTEX_EN)
    if (RT_EOK != rt_mutex_init(&g_par_mutex, "par", RT_IPC_FLAG_PRIO))
    {
        status = ePAR_ERROR;
        PAR_ERR_PRINT("PAR_IF: mutex init failed");
    }
    else
    {
        g_par_mutex_ready = true;
        PAR_DBG_PRINT("PAR_IF: mutex initialized, timeout_ms=%u", (unsigned)PAR_CFG_MUTEX_TIMEOUT_MS);
    }
#else
    g_par_mutex_ready = false;
    PAR_DBG_PRINT("PAR_IF: mutex protection disabled");
#endif /* (1 == PAR_CFG_MUTEX_EN) */

    return status;
}
/**
 * @brief De-initialize low level interface.
 *
 * @note User shall provide definition of that function based on used platform!
 *
 * @return Status of de-initialization.
 */
par_status_t par_if_deinit(void)
{
    par_status_t status = ePAR_OK;
#if (1 == PAR_CFG_MUTEX_EN)
    if (true == g_par_mutex_ready)
    {
        if (RT_EOK != rt_mutex_detach(&g_par_mutex))
        {
            status = ePAR_ERROR;
            PAR_ERR_PRINT("PAR_IF: mutex detach failed");
        }
        else
        {
            PAR_DBG_PRINT("PAR_IF: mutex detached");
        }
    }

    g_par_mutex_ready = false;
#endif /* (1 == PAR_CFG_MUTEX_EN) */

    return status;
}
/**
 * @brief Acquire mutex for specified parameter.
 *
 * @note User shall provide definition of that function based on used platform!
 *
 * If mutex is not needed leave empty space between user code begin and end.
 *
 * @param par_num Parameter number (enumeration).
 * @return Status of operation.
 */
par_status_t par_if_aquire_mutex(const par_num_t par_num)
{
    par_status_t status = ePAR_OK;
    RT_UNUSED(par_num);
#if (1 == PAR_CFG_MUTEX_EN)
    if (RT_EOK != rt_mutex_take(&g_par_mutex, rt_tick_from_millisecond(PAR_CFG_MUTEX_TIMEOUT_MS)))
    {
        status = ePAR_ERROR;
        PAR_ERR_PRINT("PAR_IF: mutex take timed out, par_num=%u", (unsigned)par_num);
    }
#endif /* (1 == PAR_CFG_MUTEX_EN) */

    return status;
}
/**
 * @brief Release mutex for specified parameter.
 *
 * @note User shall provide definition of that function based on used platform!
 *
 * If mutex is not needed leave empty space between user code begin and end.
 *
 * @param par_num Parameter number (enumeration).
 * @return Status of operation.
 */
void par_if_release_mutex(const par_num_t par_num)
{
    RT_UNUSED(par_num);
#if (1 == PAR_CFG_MUTEX_EN)
    (void)rt_mutex_release(&g_par_mutex);
#endif /* (1 == PAR_CFG_MUTEX_EN) */
}


#endif /* (1 == PAR_CFG_IF_PORT_EN) */
/**
 * @} <!-- END GROUP -->
 */
