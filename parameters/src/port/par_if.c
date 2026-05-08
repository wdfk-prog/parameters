/**
 * @file par_if.c
 * @brief Implement the parameter interface layer.
 * @author Ziga Miklosic
 * @version V3.0.1
 * @date 2026-01-29
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-01-29 V3.0.1  Ziga Miklosic first version
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
 * @brief Initialize low level interface.
 *
 * @note Default weak implementation keeps the interface optional.
 * Integrator may provide a strong definition in port/par_if_port.c.
 *
 * @return Status of initialization.
 */
PAR_PORT_WEAK par_status_t par_if_init(void)
{
    return ePAR_OK;
}
/**
 * @brief De-initialize low level interface.
 *
 * @note Default weak implementation keeps the interface optional.
 * Integrator may provide a strong definition in port/par_if_port.c.
 *
 * @return Status of de-initialization.
 */
PAR_PORT_WEAK par_status_t par_if_deinit(void)
{
    return ePAR_OK;
}
/**
 * @brief Acquire mutex for specified parameter.
 *
 * @note Default weak implementation does not provide locking.
 * Integrator may provide a strong definition in port/par_if_port.c.
 *
 * @param par_num Parameter number (enumeration).
 * @return Status of operation.
 */
PAR_PORT_WEAK par_status_t par_if_aquire_mutex(const par_num_t par_num)
{
    (void)par_num;
    return ePAR_OK;
}
/**
 * @brief Release mutex for specified parameter.
 *
 * @note Default weak implementation does not provide locking.
 * Integrator may provide a strong definition in port/par_if_port.c.
 *
 * @param par_num Parameter number (enumeration).
 */
PAR_PORT_WEAK void par_if_release_mutex(const par_num_t par_num)
{
    (void)par_num;
}
/**
 * @brief Accumulate CRC-16/CCITT-FALSE over serialized header bytes.
 *
 * @details Default weak implementation used when the integrator does not
 * provide a platform-specific override.
 *
 * @param crc Current CRC-16 accumulator value.
 * @param p_data Pointer to serialized bytes.
 * @param size Number of bytes to process.
 * @return Updated CRC-16 value.
 */
PAR_PORT_WEAK uint16_t par_if_crc16_accumulate(uint16_t crc, const uint8_t * const p_data, const uint32_t size)
{
    const uint16_t poly = 0x1021U;

    if (0U == size)
    {
        return crc;
    }

    PAR_ASSERT(NULL != p_data);

    for (uint32_t i = 0U; i < size; ++i)
    {
        crc ^= (uint16_t)((uint16_t)p_data[i] << 8U);
        for (uint8_t bit = 0U; bit < 8U; ++bit)
        {
            if (0U != (crc & 0x8000U))
            {
                crc = (uint16_t)((crc << 1U) ^ poly);
            }
            else
            {
                crc = (uint16_t)(crc << 1U);
            }
        }
    }

    return crc;
}
/**
 * @brief Accumulate CRC-8 over serialized record bytes.
 *
 * @details Default weak implementation used when the integrator does not
 * provide a platform-specific override.
 *
 * @param crc Current CRC-8 accumulator value.
 * @param p_data Pointer to serialized bytes.
 * @param size Number of bytes to process.
 * @return Updated CRC-8 value.
 */
PAR_PORT_WEAK uint8_t par_if_crc8_accumulate(uint8_t crc, const uint8_t * const p_data, const uint32_t size)
{
    const uint8_t poly = 0x07U;

    if (0U == size)
    {
        return crc;
    }

    PAR_ASSERT(NULL != p_data);

    for (uint32_t i = 0U; i < size; ++i)
    {
        crc ^= p_data[i];
        for (uint8_t bit = 0U; bit < 8U; ++bit)
        {
            if (0U != (crc & 0x80U))
            {
                crc = (uint8_t)((crc << 1U) ^ poly);
            }
            else
            {
                crc = (uint8_t)(crc << 1U);
            }
        }
    }

    return crc;
}

#else

/**
 * @brief USER INCLUDES BEGIN...
 */

#include "common/utils/src/utils.h"
#include "cmsis_os2.h"

/**
 * @brief USER INCLUDES END...
 */
/**
 * @brief Compile-time definitions.
 */
/**
 * @brief USER DEFINITIONS BEGIN...
 */

/**
 * @brief Parameter mutex timeout.
 * @details Unit: ms.
 */
#define PAR_CFG_MUTEX_TIMEOUT_MS (10)

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
static osMutexId_t g_par_mutex_id = NULL;
const osMutexAttr_t g_par_mutex_attr = {
    .name = "par",
    .attr_bits = (osMutexPrioInherit),
};
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
    g_par_mutex_id = osMutexNew(&g_par_mutex_attr);

    if (NULL == g_par_mutex_id)
    {
        status = ePAR_ERROR;
    }

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
    osMutexId_t mutex_id = g_par_mutex_id;

    if (NULL == mutex_id)
    {
        status = ePAR_OK;
    }
    else if (osOK != osMutexDelete(mutex_id))
    {
        status = ePAR_ERROR;
    }
    g_par_mutex_id = NULL;

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

    UNUSED(par_num);
    if (osOK != osMutexAcquire(g_par_mutex_id, PAR_CFG_MUTEX_TIMEOUT_MS))
    {
        status = ePAR_ERROR;
    }

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
    UNUSED(par_num);
    osMutexRelease(g_par_mutex_id);
}

#endif /* (1 == PAR_CFG_IF_PORT_EN) */
/**
 * @} <!-- END GROUP -->
 */
