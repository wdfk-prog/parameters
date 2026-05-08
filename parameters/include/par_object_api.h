/**
 * @file par_object_api.h
 * @brief Declare public object-parameter APIs.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#ifndef _PAR_OBJECT_API_H_
#define _PAR_OBJECT_API_H_

#include "par_types.h"

/**
 * @addtogroup PARAMETERS_API
 * @{ <!-- BEGIN GROUP -->
 */

#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
/**
 * @brief Set one fixed-capacity byte-array parameter.
 * @param par_num Parameter number.
 * @param p_data Pointer to the source bytes.
 * @param len Payload length in bytes.
 * @return Operation status.
 */
par_status_t par_set_bytes(const par_num_t par_num, const uint8_t *p_data, const uint16_t len);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
/**
 * @brief Read one fixed-capacity byte-array parameter.
 * @param par_num Parameter number.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_size Destination buffer size in bytes.
 * @param[out] p_out_len Pointer to the returned payload length in bytes.
 * @return Operation status. Returns ePAR_ERROR_ACCESS when read access is denied.
 */
par_status_t par_get_bytes(const par_num_t par_num, uint8_t *p_buf, const uint16_t buf_size, uint16_t *p_out_len);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
/**
 * @brief Set one fixed-capacity string parameter.
 * @param par_num Parameter number.
 * @param p_str Pointer to a NUL-terminated source string, or NULL
 * for an empty string.
 * @return Operation status.
 */
par_status_t par_set_str(const par_num_t par_num, const char *p_str);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
/**
 * @brief Read one fixed-capacity string parameter.
 * @param par_num Parameter number.
 * @param p_buf Pointer to the destination character buffer.
 * @param buf_size Destination buffer size in bytes, including the trailing '\0'.
 * Must be at least payload length + 1.
 * @param[out] p_out_len Pointer to the returned string length without the trailing '\0'.
 * @return Operation status. Returns ePAR_ERROR_ACCESS when read access is denied.
 */
par_status_t par_get_str(const par_num_t par_num, char *p_buf, const uint16_t buf_size, uint16_t *p_out_len);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
/**
 * @brief Set one fixed-size unsigned 8-bit array parameter.
 * @param par_num Parameter number.
 * @param p_data Pointer to the source elements.
 * @param count Number of elements to write.
 * @return Operation status.
 */
par_status_t par_set_arr_u8(const par_num_t par_num, const uint8_t *p_data, const uint16_t count);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
/**
 * @brief Read one fixed-size unsigned 8-bit array parameter.
 * @param par_num Parameter number.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status. Returns ePAR_ERROR_ACCESS when read access is denied.
 */
par_status_t par_get_arr_u8(const par_num_t par_num, uint8_t *p_buf, const uint16_t buf_count, uint16_t *p_out_count);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
/**
 * @brief Set one fixed-size unsigned 16-bit array parameter.
 * @param par_num Parameter number.
 * @param p_data Pointer to the source elements.
 * @param count Number of elements to write.
 * @return Operation status.
 */
par_status_t par_set_arr_u16(const par_num_t par_num, const uint16_t *p_data, const uint16_t count);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
/**
 * @brief Read one fixed-size unsigned 16-bit array parameter.
 * @param par_num Parameter number.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status. Returns ePAR_ERROR_ACCESS when read access is denied.
 */
par_status_t par_get_arr_u16(const par_num_t par_num, uint16_t *p_buf, const uint16_t buf_count, uint16_t *p_out_count);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
/**
 * @brief Set one fixed-size unsigned 32-bit array parameter.
 * @param par_num Parameter number.
 * @param p_data Pointer to the source elements.
 * @param count Number of elements to write.
 * @return Operation status.
 */
par_status_t par_set_arr_u32(const par_num_t par_num, const uint32_t *p_data, const uint16_t count);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
/**
 * @brief Read one fixed-size unsigned 32-bit array parameter.
 * @param par_num Parameter number.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status. Returns ePAR_ERROR_ACCESS when read access is denied.
 */
par_status_t par_get_arr_u32(const par_num_t par_num, uint32_t *p_buf, const uint16_t buf_count, uint16_t *p_out_count);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */
/**
 * @brief Return the current payload length of one object parameter.
 * @param par_num Parameter number.
 * @param[out] p_len Pointer to the returned payload length in bytes.
 * @return Operation status. Returns ePAR_ERROR_ACCESS when read access is denied.
 */
par_status_t par_get_obj_len(const par_num_t par_num, uint16_t *p_len);
/**
 * @brief Return the configured payload capacity of one object parameter.
 * @param par_num Parameter number.
 * @param[out] p_capacity Pointer to the returned payload capacity in bytes.
 * @return Operation status.
 */
par_status_t par_get_obj_capacity(const par_num_t par_num, uint16_t *p_capacity);
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
/**
 * @brief Read the default byte-array payload for one object parameter.
 * @param par_num Parameter number.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_size Destination buffer size in bytes.
 * @param[out] p_out_len Pointer to the returned default payload length in bytes.
 * @return Operation status.
 */
par_status_t par_get_default_bytes(const par_num_t par_num, uint8_t *p_buf, const uint16_t buf_size, uint16_t *p_out_len);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
/**
 * @brief Read the default string payload for one object parameter.
 * @param par_num Parameter number.
 * @param p_buf Pointer to the destination character buffer.
 * @param buf_size Destination buffer size in bytes, including the trailing '\0'.
 * Must be at least payload length + 1.
 * @param[out] p_out_len Pointer to the returned default string length without the trailing '\0'.
 * @return Operation status.
 */
par_status_t par_get_default_str(const par_num_t par_num, char *p_buf, const uint16_t buf_size, uint16_t *p_out_len);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
/**
 * @brief Read the default unsigned 8-bit array payload for one object parameter.
 * @param par_num Parameter number.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status.
 */
par_status_t par_get_default_arr_u8(const par_num_t par_num, uint8_t *p_buf, const uint16_t buf_count, uint16_t *p_out_count);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
/**
 * @brief Read the default unsigned 16-bit array payload for one object parameter.
 * @param par_num Parameter number.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status.
 */
par_status_t par_get_default_arr_u16(const par_num_t par_num, uint16_t *p_buf, const uint16_t buf_count, uint16_t *p_out_count);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
/**
 * @brief Read the default unsigned 32-bit array payload for one object parameter.
 * @param par_num Parameter number.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status.
 */
par_status_t par_get_default_arr_u32(const par_num_t par_num, uint32_t *p_buf, const uint16_t buf_count, uint16_t *p_out_count);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */
#if (1 == PAR_CFG_ENABLE_ID)
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
/**
 * @brief Set one fixed-capacity byte-array parameter by external ID.
 * @param id External parameter ID.
 * @param p_data Pointer to the source bytes.
 * @param len Payload length in bytes.
 * @return Operation status.
 */
par_status_t par_set_bytes_by_id(const uint16_t id,
                                  const uint8_t *p_data,
                                  const uint16_t len);
/**
 * @brief Read one fixed-capacity byte-array parameter by external ID.
 * @param id External parameter ID.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_size Destination buffer size in bytes.
 * @param[out] p_out_len Pointer to the returned payload length in bytes.
 * @return Operation status. Returns ePAR_ERROR_ACCESS when read access is denied.
 */
par_status_t par_get_bytes_by_id(const uint16_t id,
                                  uint8_t *p_buf,
                                  const uint16_t buf_size,
                                  uint16_t *p_out_len);
/**
 * @brief Read the default byte-array payload by external ID.
 * @param id External parameter ID.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_size Destination buffer size in bytes.
 * @param[out] p_out_len Pointer to the returned default payload length in bytes.
 * @return Operation status.
 */
par_status_t par_get_default_bytes_by_id(const uint16_t id,
                                          uint8_t *p_buf,
                                          const uint16_t buf_size,
                                          uint16_t *p_out_len);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
/**
 * @brief Set one fixed-capacity string parameter by external ID.
 * @param id External parameter ID.
 * @param p_str Pointer to a NUL-terminated source string, or NULL
 * for an empty string.
 * @return Operation status.
 */
par_status_t par_set_str_by_id(const uint16_t id, const char *p_str);
/**
 * @brief Read one fixed-capacity string parameter by external ID.
 * @param id External parameter ID.
 * @param p_buf Pointer to the destination character buffer.
 * @param buf_size Destination buffer size in bytes, including the trailing '\0'.
 * @param[out] p_out_len Pointer to the returned string length without the trailing '\0'.
 * @return Operation status. Returns ePAR_ERROR_ACCESS when read access is denied.
 */
par_status_t par_get_str_by_id(const uint16_t id,
                                char *p_buf,
                                const uint16_t buf_size,
                                uint16_t *p_out_len);
/**
 * @brief Read the default string payload by external ID.
 * @param id External parameter ID.
 * @param p_buf Pointer to the destination character buffer.
 * @param buf_size Destination buffer size in bytes, including the trailing '\0'.
 * @param[out] p_out_len Pointer to the returned default string length without the trailing '\0'.
 * @return Operation status.
 */
par_status_t par_get_default_str_by_id(const uint16_t id,
                                        char *p_buf,
                                        const uint16_t buf_size,
                                        uint16_t *p_out_len);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
/**
 * @brief Set one fixed-size unsigned 8-bit array parameter by external ID.
 * @param id External parameter ID.
 * @param p_data Pointer to the source elements.
 * @param count Number of elements to write.
 * @return Operation status.
 */
par_status_t par_set_arr_u8_by_id(const uint16_t id,
                                   const uint8_t *p_data,
                                   const uint16_t count);
/**
 * @brief Read one fixed-size unsigned 8-bit array parameter by external ID.
 * @param id External parameter ID.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status. Returns ePAR_ERROR_ACCESS when read access is denied.
 */
par_status_t par_get_arr_u8_by_id(const uint16_t id,
                                   uint8_t *p_buf,
                                   const uint16_t buf_count,
                                   uint16_t *p_out_count);
/**
 * @brief Read the default unsigned 8-bit array payload by external ID.
 * @param id External parameter ID.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status.
 */
par_status_t par_get_default_arr_u8_by_id(const uint16_t id,
                                           uint8_t *p_buf,
                                           const uint16_t buf_count,
                                           uint16_t *p_out_count);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
/**
 * @brief Set one fixed-size unsigned 16-bit array parameter by external ID.
 * @param id External parameter ID.
 * @param p_data Pointer to the source elements.
 * @param count Number of elements to write.
 * @return Operation status.
 */
par_status_t par_set_arr_u16_by_id(const uint16_t id,
                                    const uint16_t *p_data,
                                    const uint16_t count);
/**
 * @brief Read one fixed-size unsigned 16-bit array parameter by external ID.
 * @param id External parameter ID.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status. Returns ePAR_ERROR_ACCESS when read access is denied.
 */
par_status_t par_get_arr_u16_by_id(const uint16_t id,
                                    uint16_t *p_buf,
                                    const uint16_t buf_count,
                                    uint16_t *p_out_count);
/**
 * @brief Read the default unsigned 16-bit array payload by external ID.
 * @param id External parameter ID.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status.
 */
par_status_t par_get_default_arr_u16_by_id(const uint16_t id,
                                            uint16_t *p_buf,
                                            const uint16_t buf_count,
                                            uint16_t *p_out_count);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
/**
 * @brief Set one fixed-size unsigned 32-bit array parameter by external ID.
 * @param id External parameter ID.
 * @param p_data Pointer to the source elements.
 * @param count Number of elements to write.
 * @return Operation status.
 */
par_status_t par_set_arr_u32_by_id(const uint16_t id,
                                    const uint32_t *p_data,
                                    const uint16_t count);
/**
 * @brief Read one fixed-size unsigned 32-bit array parameter by external ID.
 * @param id External parameter ID.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status. Returns ePAR_ERROR_ACCESS when read access is denied.
 */
par_status_t par_get_arr_u32_by_id(const uint16_t id,
                                    uint32_t *p_buf,
                                    const uint16_t buf_count,
                                    uint16_t *p_out_count);
/**
 * @brief Read the default unsigned 32-bit array payload by external ID.
 * @param id External parameter ID.
 * @param p_buf Pointer to the destination buffer.
 * @param buf_count Destination capacity in elements.
 * @param[out] p_out_count Pointer to the returned element count.
 * @return Operation status.
 */
par_status_t par_get_default_arr_u32_by_id(const uint16_t id,
                                            uint32_t *p_buf,
                                            const uint16_t buf_count,
                                            uint16_t *p_out_count);
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */
/**
 * @brief Return the current payload length of one object parameter by external ID.
 * @param id External parameter ID.
 * @param[out] p_len Pointer to the returned payload length in bytes.
 * @return Operation status. Returns ePAR_ERROR_ACCESS when read access is denied.
 */
par_status_t par_get_obj_len_by_id(const uint16_t id, uint16_t *p_len);
/**
 * @brief Return the configured object payload capacity by external ID.
 * @param id External parameter ID.
 * @param[out] p_capacity Pointer to the returned payload capacity in bytes.
 * @return Operation status.
 */
par_status_t par_get_obj_capacity_by_id(const uint16_t id, uint16_t *p_capacity);
#endif /* (1 == PAR_CFG_ENABLE_ID) */
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

/** @} <!-- END GROUP --> */

#endif /* !defined(_PAR_OBJECT_API_H_) */
