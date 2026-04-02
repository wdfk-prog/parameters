/**
 * @file par.h
 * @brief Declare the public device-parameter API.
 * @author Ziga Miklosic
 * @version V3.0.1
 * @date 2026-01-29
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-01-29 V3.0.1  Ziga Miklosic
 */
/**
 * @addtogroup PARAMETERS_API
 * @{ <!-- BEGIN GROUP -->
 */

#ifndef _PAR_H_
#define _PAR_H_
/**
 * @brief Include dependencies.
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "par_cfg.h"
/**
 * @brief Compile-time definitions.
 */
/**
 * @brief Module version.
 */
#define PAR_VER_MAJOR   (3)
#define PAR_VER_MINOR   (0)
#define PAR_VER_DEVELOP (2)

/**
 * @brief Parameter status.
 */
enum
{
    ePAR_OK = 0U, /**< Normal operation. */

    /* Error status bits. */
    ePAR_STATUS_ERROR_MASK = 0x07FFU,
    ePAR_ERROR = 0x0001U,          /**< General parameter error. */
    ePAR_ERROR_INIT = 0x0002U,     /**< Parameter initialization error or usage before initialization. */
    ePAR_ERROR_NVM = 0x0004U,      /**< Parameter storage to NVM error. */
    ePAR_ERROR_CRC = 0x0008U,      /**< Parameter CRC corrupted. */
    ePAR_ERROR_TYPE = 0x0010U,     /**< Using invalid API function for given parameter data type. */
    ePAR_ERROR_MUTEX = 0x0020U,    /**< Acquiring mutex failed. */
    ePAR_ERROR_VALUE = 0x0040U,    /**< Invalid parameter value (validation failed). */
    ePAR_ERROR_PARAM = 0x0080U,    /**< Invalid function argument. */
    ePAR_ERROR_PAR_NUM = 0x0100U,  /**< Invalid parameter number. */
    ePAR_ERROR_ACCESS = 0x0200U,   /**< Write access denied by parameter access policy. */
    ePAR_ERROR_TABLE_ID = 0x0400U, /**< Stored parameter-table ID does not match the live table. */

    /* Warning status bits. */
    ePAR_STATUS_WAR_MASK = 0xF800U,
    ePAR_WAR_SET_TO_DEF = 0x0800U,    /**< Parameters set to default. */
    ePAR_WAR_NVM_REWRITTEN = 0x1000U, /**< NVM parameters area completely re-written. */
    ePAR_WAR_NO_PERSISTENT = 0x2000U, /**< No persistent parameters -> set PAR_CFG_NVM_EN to 0. */
    ePAR_WAR_LIMITED = 0x4000U,       /**< Parameter value limited within [min,max]. */
};
typedef uint16_t par_status_t;
/**
 * @brief Parameters type enumeration.
 */
enum
{
    ePAR_TYPE_U8 = 0, /**< Unsigned 8-bit value. */
    ePAR_TYPE_U16,    /**< Unsigned 16-bit value. */
    ePAR_TYPE_U32,    /**< Unsigned 32-bit value. */
    ePAR_TYPE_I8,     /**< Signed 8-bit value. */
    ePAR_TYPE_I16,    /**< Signed 16-bit value. */
    ePAR_TYPE_I32,    /**< Signed 32-bit value. */
    ePAR_TYPE_F32,    /**< 32-bit floating value. */
    ePAR_TYPE_NUM_OF
};
typedef uint8_t par_type_list_t;
/**
 * @brief Parameter R/W access.
 */
enum
{
    ePAR_ACCESS_RO = 0, /**< Parameter read only. */
    ePAR_ACCESS_RW      /**< Parameter read/write. */
};
typedef uint8_t par_access_t;
/**
 * @brief 32-bit floating data type definition.
 */
typedef float float32_t;
/**
 * @brief Supported data types.
 */
typedef union
{
    uint8_t u8;    /**< Unsigned 8-bit value. */
    uint16_t u16;  /**< Unsigned 16-bit value. */
    uint32_t u32;  /**< Unsigned 32-bit value. */
    int8_t i8;     /**< Signed 8-bit value. */
    int16_t i16;   /**< Signed 16-bit value. */
    int32_t i32;   /**< Signed 32-bit value. */
    float32_t f32; /**< 32-bit floating value. */
} par_type_t;
/**
 * @brief Parameter value range.
 */
#if (1 == PAR_CFG_ENABLE_RANGE)
typedef struct
{
    par_type_t min; /**< Minimum value. */
    par_type_t max; /**< Maximum value. */
} par_range_t;
#endif

/**
 * @brief Parameter data settings.
 * @note The exact object size depends on enabled metadata fields and target ABI.
 */
typedef struct par_cfg_s
{
#if (1 == PAR_CFG_ENABLE_NAME)
    const char *name;      /**< Name of variable. */
#endif
#if (1 == PAR_CFG_ENABLE_RANGE)
    par_range_t range;     /**< Range of parameter. */
#endif
    par_type_t def;        /**< Default value of parameter. */
#if (1 == PAR_CFG_ENABLE_UNIT)
    const char *unit;      /**< Unit of parameter. */
#endif
#if (1 == PAR_CFG_ENABLE_DESC)
    const char *desc;      /**< Parameter description. */
#endif
#if (1 == PAR_CFG_ENABLE_ID)
    uint16_t id;           /**< Variable ID. */
#endif
    par_type_list_t type;  /**< Parameter type. */
#if (1 == PAR_CFG_ENABLE_ACCESS)
    par_access_t access;   /**< Parameter access from external device point-of-view. */
#endif
#if (1 == PAR_CFG_ENABLE_PERSIST)
    bool persistent;       /**< Parameter persistence flag. */
    uint16_t persist_idx;  /**< Persistent slot index or PAR_PERSIST_IDX_INVALID. */
#endif
} par_cfg_t;
/**
 * @brief Device Parameters on-change callback.
 * @note The callback runs only on the normal setter path. Startup default
 * initialization, raw restore paths, fast setters, and bitwise fast setters do
 * not invoke it.
 * @note Keep the callback synchronous, short, and non-blocking. Avoid long I/O,
 * waits, sleeps, or any operation that extends the parameter lock hold time.
 * @note Re-entering the parameter module from this callback is an advanced usage
 * pattern and must be reviewed carefully at application level.
 */
typedef void (*pf_par_on_change_cb_t)(const par_num_t par_num, const par_type_t new_val, const par_type_t old_val);
/**
 * @brief Device Parameters validation callback.
 * @note Validation runs only on the normal setter path. Startup default
 * initialization, raw restore paths, fast setters, and bitwise fast setters do
 * not invoke it.
 * @note Keep validation synchronous, short, and non-blocking. Avoid long I/O,
 * waits, sleeps, or any operation that extends the parameter lock hold time.
 * @note Re-entering the parameter module from validation is an advanced usage
 * pattern and must be reviewed carefully at application level.
 */
typedef bool (*pf_par_validation_t)(const par_num_t par_num, const par_type_t val);
/**
 * @brief Function declarations.
 */
/**
 * @brief Initialize the parameter module.
 * @return Operation status.
 */
par_status_t par_init(void);
/**
 * @brief Deinitialize the parameter module.
 * @return Operation status.
 */
par_status_t par_deinit(void);
/**
 * @brief Report whether the parameter module is initialized.
 * @return True when the module is initialized; otherwise false.
 */
bool par_is_init(void);
/**
 * @brief Acquire the parameter lock for one parameter path.
 * @param par_num Parameter number.
 * @return Operation status.
 */
par_status_t par_acquire_mutex(const par_num_t par_num);
/**
 * @brief Release the parameter lock for one parameter path.
 * @param par_num Parameter number.
 */
void par_release_mutex(const par_num_t par_num);
/**
 * @brief Typed parameter setter API.
 */
/**
 * @brief Macro-generated typed API note.
 * @details par_set_u8/i8/u16/i16/u32/i32(/f32), par_set_xxx_fast, and
 * par_get_xxx are generated by macros. Their declarations are listed here,
 * while the implementations are emitted in par.c through
 * #include "par_typed_impl.inc".
 */
/**
 * @brief Set one parameter from a typed input pointer.
 * @param par_num Parameter number.
 * @param p_val Pointer to the input value.
 * @return Operation status.
 */
par_status_t par_set(const par_num_t par_num, const void *p_val);
/**
 * @brief Set one parameter from a typed input pointer through the unchecked fast path.
 *
 * @note This generic fast setter validates initialization, parameter number,
 * and pointer arguments, then dispatches to the matching typed
 * `par_set_xxx_fast()` implementation.
 *
 * @note It intentionally bypasses access enforcement, runtime validation
 * callbacks, and on-change callbacks. Range limiting still follows
 * the typed fast setter behavior.
 *
 * @param par_num Parameter number.
 * @param p_val Pointer to the input value.
 * @return Operation status.
 */
par_status_t par_set_fast(const par_num_t par_num, const void *p_val);
#if (1 == PAR_CFG_ENABLE_ID)
/**
 * @brief Set one parameter by external parameter ID.
 * @param id External parameter ID.
 * @param p_val Pointer to the input value.
 * @return Operation status.
 */
par_status_t par_set_by_id(const uint16_t id, const void *p_val);
#endif
/**
 * @brief Set one U8 parameter.
 * @param par_num Parameter number.
 * @param val U8 value to write.
 * @return Operation status.
 */
par_status_t par_set_u8(const par_num_t par_num, const uint8_t val);
/**
 * @brief Set one I8 parameter.
 * @param par_num Parameter number.
 * @param val I8 value to write.
 * @return Operation status.
 */
par_status_t par_set_i8(const par_num_t par_num, const int8_t val);
/**
 * @brief Set one U16 parameter.
 * @param par_num Parameter number.
 * @param val U16 value to write.
 * @return Operation status.
 */
par_status_t par_set_u16(const par_num_t par_num, const uint16_t val);
/**
 * @brief Set one I16 parameter.
 * @param par_num Parameter number.
 * @param val I16 value to write.
 * @return Operation status.
 */
par_status_t par_set_i16(const par_num_t par_num, const int16_t val);
/**
 * @brief Set one U32 parameter.
 * @param par_num Parameter number.
 * @param val U32 value to write.
 * @return Operation status.
 */
par_status_t par_set_u32(const par_num_t par_num, const uint32_t val);
/**
 * @brief Set one I32 parameter.
 * @param par_num Parameter number.
 * @param val I32 value to write.
 * @return Operation status.
 */
par_status_t par_set_i32(const par_num_t par_num, const int32_t val);
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
/**
 * @brief Set one F32 parameter.
 * @param par_num Parameter number.
 * @param val F32 value to write.
 * @return Operation status.
 */
par_status_t par_set_f32(const par_num_t par_num, const float32_t val);
#endif
/**
 * @brief Fast typed setters.
 *
 * @note These APIs are performance-oriented public entry points.
 * Callers must guarantee the module is initialized, par_num is valid,.
 * and the selected typed API matches the parameter type.
 *
 * @note They intentionally bypass access enforcement, runtime validation.
 * callbacks, and on-change callbacks. Range limiting still follows.
 * build-time PAR_CFG_ENABLE_RANGE.
 */
/**
 * @brief Set one U8 parameter through the unchecked fast path.
 * @param par_num Parameter number.
 * @param val U8 value to write.
 * @return Operation status.
 */
par_status_t par_set_u8_fast(const par_num_t par_num, const uint8_t val);
/**
 * @brief Set one I8 parameter through the unchecked fast path.
 * @param par_num Parameter number.
 * @param val I8 value to write.
 * @return Operation status.
 */
par_status_t par_set_i8_fast(const par_num_t par_num, const int8_t val);
/**
 * @brief Set one U16 parameter through the unchecked fast path.
 * @param par_num Parameter number.
 * @param val U16 value to write.
 * @return Operation status.
 */
par_status_t par_set_u16_fast(const par_num_t par_num, const uint16_t val);
/**
 * @brief Set one I16 parameter through the unchecked fast path.
 * @param par_num Parameter number.
 * @param val I16 value to write.
 * @return Operation status.
 */
par_status_t par_set_i16_fast(const par_num_t par_num, const int16_t val);
/**
 * @brief Set one U32 parameter through the unchecked fast path.
 * @param par_num Parameter number.
 * @param val U32 value to write.
 * @return Operation status.
 */
par_status_t par_set_u32_fast(const par_num_t par_num, const uint32_t val);
/**
 * @brief Set one I32 parameter through the unchecked fast path.
 * @param par_num Parameter number.
 * @param val I32 value to write.
 * @return Operation status.
 */
par_status_t par_set_i32_fast(const par_num_t par_num, const int32_t val);
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
/**
 * @brief Set one F32 parameter through the unchecked fast path.
 * @param par_num Parameter number.
 * @param val F32 value to write.
 * @return Operation status.
 */
par_status_t par_set_f32_fast(const par_num_t par_num, const float32_t val);
#endif
/**
 * @brief Fast bitwise setters for flags/bitmask parameters.
 *
 * @note These APIs are intended only for U8/U16/U32 parameters that model.
 * flags or bitmasks. They are not a general-purpose replacement for the.
 * normal setter APIs used with ranged numeric values.
 *
 * @note They follow the same trust model as other fast setters: callers must.
 * guarantee the module is initialized, par_num is valid, and the typed.
 * API matches the parameter type.
 *
 * @note They intentionally bypass runtime validation callbacks, on-change.
 * callbacks, and normal setter range semantics.
 */
/**
 * @brief Apply a fast bitwise AND update to one U8 parameter.
 * @param par_num Parameter number.
 * @param val U8 mask value.
 * @return Operation status.
 */
par_status_t par_bitand_set_u8_fast(const par_num_t par_num, const uint8_t val);
/**
 * @brief Apply a fast bitwise AND update to one U16 parameter.
 * @param par_num Parameter number.
 * @param val U16 mask value.
 * @return Operation status.
 */
par_status_t par_bitand_set_u16_fast(const par_num_t par_num, const uint16_t val);
/**
 * @brief Apply a fast bitwise AND update to one U32 parameter.
 * @param par_num Parameter number.
 * @param val U32 mask value.
 * @return Operation status.
 */
par_status_t par_bitand_set_u32_fast(const par_num_t par_num, const uint32_t val);
/**
 * @brief Apply a fast bitwise OR update to one U8 parameter.
 * @param par_num Parameter number.
 * @param val U8 mask value.
 * @return Operation status.
 */
par_status_t par_bitor_set_u8_fast(const par_num_t par_num, const uint8_t val);
/**
 * @brief Apply a fast bitwise OR update to one U16 parameter.
 * @param par_num Parameter number.
 * @param val U16 mask value.
 * @return Operation status.
 */
par_status_t par_bitor_set_u16_fast(const par_num_t par_num, const uint16_t val);
/**
 * @brief Apply a fast bitwise OR update to one U32 parameter.
 * @param par_num Parameter number.
 * @param val U32 mask value.
 * @return Operation status.
 */
par_status_t par_bitor_set_u32_fast(const par_num_t par_num, const uint32_t val);
/**
 * @brief Reset one parameter to its configured default value through the internal. fast typed setter path.
 * @note This API bypasses access enforcement, runtime validation callbacks,. and on-change callbacks. Range limiting still follows the typed fast. setter behavior.
 */
/**
 * @brief Restore one parameter to its configured default value.
 * @param par_num Parameter number.
 * @return Operation status.
 */
par_status_t par_set_to_default(const par_num_t par_num);
/**
 * @brief Reset all parameters to their default values.
 * @note When PAR_CFG_ENABLE_RESET_ALL_RAW = 1, this public API forwards to. par_reset_all_to_default_raw() for the fastest bulk restore path.
 * @note When raw reset is disabled, this API iterates through. par_set_to_default(), which uses the internal fast typed setter path.
 */
/**
 * @brief Restore all parameters to their configured default values.
 * @return Operation status.
 */
par_status_t par_set_all_to_default(void);
#if (1 == PAR_CFG_ENABLE_RESET_ALL_RAW)
/**
 * @brief Reset all parameters through raw storage restore.
 *
 * @note This path is typically faster than par_set_all_to_default(), because.
 * it restores grouped storage directly instead of resetting parameters.
 * one by one through the normal setter path.
 *
 * @note This path bypasses normal setter hooks (validation, on-change callback,.
 * and setter-side range behavior).
 */
/**
 * @brief Restore all parameters through the raw grouped-storage path.
 * @return Operation status.
 */
par_status_t par_reset_all_to_default_raw(void);
#endif

/**
 * @brief Report whether one parameter differs from its default value.
 * @param par_num Parameter number.
 * @param p_has_changed Pointer to the changed-state output flag.
 * @return Operation status.
 */
par_status_t par_has_changed(const par_num_t par_num, bool * const p_has_changed);
/**
 * @brief Typed macro wrappers for parameter set.
 */
#define PAR_SET_U8(par_num, value)  par_set_u8((par_num), (value))
#define PAR_SET_I8(par_num, value)  par_set_i8((par_num), (value))
#define PAR_SET_U16(par_num, value) par_set_u16((par_num), (value))
#define PAR_SET_I16(par_num, value) par_set_i16((par_num), (value))
#define PAR_SET_U32(par_num, value) par_set_u32((par_num), (value))
#define PAR_SET_I32(par_num, value) par_set_i32((par_num), (value))
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
#define PAR_SET_F32(par_num, value) par_set_f32((par_num), (value))
#endif

/**
 * @brief Getting parameter value API (module must be first initialized before using those func).
 */
/**
 * @brief Read one parameter into a typed output pointer.
 * @param par_num Parameter number.
 * @param p_val Pointer to the output value.
 * @return Operation status.
 */
par_status_t par_get(const par_num_t par_num, void * const p_val);
#if (1 == PAR_CFG_ENABLE_ID)
/**
 * @brief Read one parameter by external parameter ID.
 * @param id External parameter ID.
 * @param p_val Pointer to the output value.
 * @return Operation status.
 */
par_status_t par_get_by_id(const uint16_t id, void * const p_val);
#endif
/**
 * @brief Read one U8 parameter.
 * @param par_num Parameter number.
 * @param p_val Pointer to the U8 output value.
 * @return Operation status.
 */
par_status_t par_get_u8(const par_num_t par_num, uint8_t * const p_val);
/**
 * @brief Read one I8 parameter.
 * @param par_num Parameter number.
 * @param p_val Pointer to the I8 output value.
 * @return Operation status.
 */
par_status_t par_get_i8(const par_num_t par_num, int8_t * const p_val);
/**
 * @brief Read one U16 parameter.
 * @param par_num Parameter number.
 * @param p_val Pointer to the U16 output value.
 * @return Operation status.
 */
par_status_t par_get_u16(const par_num_t par_num, uint16_t * const p_val);
/**
 * @brief Read one I16 parameter.
 * @param par_num Parameter number.
 * @param p_val Pointer to the I16 output value.
 * @return Operation status.
 */
par_status_t par_get_i16(const par_num_t par_num, int16_t * const p_val);
/**
 * @brief Read one U32 parameter.
 * @param par_num Parameter number.
 * @param p_val Pointer to the U32 output value.
 * @return Operation status.
 */
par_status_t par_get_u32(const par_num_t par_num, uint32_t * const p_val);
/**
 * @brief Read one I32 parameter.
 * @param par_num Parameter number.
 * @param p_val Pointer to the I32 output value.
 * @return Operation status.
 */
par_status_t par_get_i32(const par_num_t par_num, int32_t * const p_val);
#if (1 == PAR_CFG_ENABLE_TYPE_F32)
/**
 * @brief Read one F32 parameter.
 * @param par_num Parameter number.
 * @param p_val Pointer to the F32 output value.
 * @return Operation status.
 */
par_status_t par_get_f32(const par_num_t par_num, float32_t * const p_val);
#endif
/**
 * @brief Read the configured default value for one parameter.
 * @param par_num Parameter number.
 * @param p_val Pointer to the output value.
 * @return Operation status.
 */
par_status_t par_get_default(const par_num_t par_num, void * const p_val);
/**
 * @brief Parameter configurations API (usage without module init pre-step).
 */
const par_cfg_t *par_get_config(const par_num_t par_num);
#if (1 == PAR_CFG_ENABLE_NAME)
const char *par_get_name(const par_num_t par_num);
#endif
#if (1 == PAR_CFG_ENABLE_RANGE)
/**
 * @brief Return the configured range for one parameter.
 * @param par_num Parameter number.
 * @return Configured parameter range.
 */
par_range_t par_get_range(const par_num_t par_num);
#endif
#if (1 == PAR_CFG_ENABLE_UNIT)
const char *par_get_unit(const par_num_t par_num);
#endif
#if (1 == PAR_CFG_ENABLE_DESC)
const char *par_get_desc(const par_num_t par_num);
#endif
/**
 * @brief Return the configured data type for one parameter.
 * @param par_num Parameter number.
 * @return Parameter data type.
 */
par_type_list_t par_get_type(const par_num_t par_num);
#if (1 == PAR_CFG_ENABLE_ACCESS)
/**
 * @brief Return parameter access metadata used by public setter access enforcement.
 * @details when PAR_CFG_ENABLE_ACCESS = 1.
 */
/**
 * @brief Return the configured external access policy for one parameter.
 * @param par_num Parameter number.
 * @return Configured access policy.
 */
par_access_t par_get_access(const par_num_t par_num);
#endif
#if (1 == PAR_CFG_ENABLE_PERSIST)
/**
 * @brief Report whether one parameter is marked persistent.
 * @param par_num Parameter number.
 * @return True when the parameter is persistent; otherwise false.
 */
bool par_is_persistent(const par_num_t par_num);
#endif
#if (1 == PAR_CFG_ENABLE_ID)
/**
 * @brief Resolve an external parameter ID to an internal parameter number.
 * @param id External parameter ID.
 * @param p_par_num Pointer to the resolved parameter number.
 * @return Operation status.
 */
par_status_t par_get_num_by_id(const uint16_t id, par_num_t * const p_par_num);
/**
 * @brief Resolve an internal parameter number to an external parameter ID.
 * @param par_num Parameter number.
 * @param p_id Pointer to the resolved external parameter ID.
 * @return Operation status.
 */
par_status_t par_get_id_by_num(const par_num_t par_num, uint16_t * const p_id);
#endif

/**
 * @brief Parameter NVM storage API.
 */
#if (1 == PAR_CFG_NVM_EN)
/**
 * @brief Set one parameter and persist it immediately.
 * @param par_num Parameter number.
 * @param p_val Pointer to the input value.
 * @return Operation status.
 */
par_status_t par_set_n_save(const par_num_t par_num, const void *p_val);
/**
 * @brief Persist all persistent parameters.
 * @return Operation status.
 */
par_status_t par_save_all(void);
/**
 * @brief Persist one parameter.
 * @param par_num Parameter number.
 * @return Operation status.
 */
par_status_t par_save(const par_num_t par_num);
#if (1 == PAR_CFG_ENABLE_ID)
/**
 * @brief Persist one parameter by external parameter ID.
 * @param par_id External parameter ID.
 * @return Operation status.
 */
par_status_t par_save_by_id(const uint16_t par_id);
#endif
/**
 * @brief Rewrite the managed parameter NVM area.
 * @return Operation status.
 */
par_status_t par_save_clean(void);
#endif

/**
 * @brief Registration API.
 */
#if (1 == PAR_CFG_ENABLE_CHANGE_CALLBACK)
/**
 * @brief Register per-parameter on-change callback used by the normal setter path.
 *
 * @note Registered callback is not used by startup default initialization,.
 * raw restore paths, fast setters, or bitwise fast setters.
 */
/**
 * @brief Register a change callback for one parameter.
 * @param par_num Parameter number.
 * @param cb Change callback function pointer.
 */
void par_register_on_change_cb(const par_num_t par_num, const pf_par_on_change_cb_t cb);
#endif
#if (1 == PAR_CFG_ENABLE_RUNTIME_VALIDATION)
/**
 * @brief Register per-parameter runtime validation callback used by the normal.
 * setter path.
 *
 * @note Registered validation is not used by startup default initialization,.
 * raw restore paths, fast setters, or bitwise fast setters.
 */
/**
 * @brief Register a validation callback for one parameter.
 * @param par_num Parameter number.
 * @param validation Validation callback function pointer.
 */
void par_register_validation(const par_num_t par_num, const pf_par_validation_t validation);
#endif

#if (1 == PAR_CFG_ENABLE_DESC) && (1 == PAR_CFG_ENABLE_DESC_CHECK)
/**
 * @brief Validate a description string in the port layer.
 * @param p_desc Pointer to the description string.
 * @return True when the description is valid; otherwise false.
 */
PAR_PORT_WEAK bool par_port_is_desc_valid(const char * const p_desc);
#endif

#if (PAR_CFG_DEBUG_EN)
const char *par_get_status_str(const par_status_t status);
#endif
/**
 * @} <!-- END GROUP -->
 */

#endif /* _PAR_H_ */
