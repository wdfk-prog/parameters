/**
 * @file par_types.h
 * @brief Declare public parameter types and callback typedefs.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#ifndef _PAR_TYPES_H_
#define _PAR_TYPES_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "par_cfg.h"
#include "par_fwd.h"

/**
 * @addtogroup PARAMETERS_API
 * @{ <!-- BEGIN GROUP -->
 */

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
typedef enum
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
    ePAR_ERROR_ACCESS = 0x0200U,   /**< Access denied by parameter access policy. */
    ePAR_ERROR_TABLE_ID = 0x0400U, /**< Stored parameter-table ID does not match the live table. */

    /* Warning status bits. */
    ePAR_STATUS_WAR_MASK = 0xF800U,
    ePAR_WAR_SET_TO_DEF = 0x0800U,    /**< Parameters set to default. */
    ePAR_WAR_NVM_REWRITTEN = 0x1000U, /**< NVM parameters area completely re-written. */
    ePAR_WAR_NO_PERSISTENT = 0x2000U, /**< No persistent parameters -> set PAR_CFG_NVM_EN to 0. */
    ePAR_WAR_LIMITED = 0x4000U,       /**< Parameter value limited within [min,max]. */
} par_status_t;
/**
 * @brief Parameters type enumeration.
 */
typedef enum
{
    ePAR_TYPE_U8 = 0,  /**< Unsigned 8-bit value. */
    ePAR_TYPE_U16,     /**< Unsigned 16-bit value. */
    ePAR_TYPE_U32,     /**< Unsigned 32-bit value. */
    ePAR_TYPE_I8,      /**< Signed 8-bit value. */
    ePAR_TYPE_I16,     /**< Signed 16-bit value. */
    ePAR_TYPE_I32,     /**< Signed 32-bit value. */
    ePAR_TYPE_F32,     /**< 32-bit floating value. */
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
    ePAR_TYPE_STR,     /**< Fixed-capacity string stored out-of-line. */
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
    ePAR_TYPE_BYTES,   /**< Fixed-capacity byte array stored out-of-line. */
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
    ePAR_TYPE_ARR_U8,  /**< Fixed-size unsigned 8-bit array stored out-of-line. */
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
    ePAR_TYPE_ARR_U16, /**< Fixed-size unsigned 16-bit array stored out-of-line. */
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */
#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
    ePAR_TYPE_ARR_U32, /**< Fixed-size unsigned 32-bit array stored out-of-line. */
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */
    ePAR_TYPE_NUM_OF
} par_type_list_t;
#if (1 == PAR_CFG_ENABLE_ACCESS)
/**
 * @brief Parameter read/write capability bit mask.
 * @details Parameter table rows must use either ePAR_ACCESS_RO or
 * ePAR_ACCESS_RW. The ePAR_ACCESS_NONE value is reserved as a sentinel for
 * invalid metadata lookups and is rejected by compile-time table checks.
 * Write-only access is intentionally unsupported.
 */
typedef enum
{
    ePAR_ACCESS_NONE = 0U,                                    /**< Invalid/no-access sentinel; not valid in table rows. */
    ePAR_ACCESS_READ = (1U << 0),                             /**< External read capability bit. */
    ePAR_ACCESS_WRITE = (1U << 1),                            /**< External write capability bit, valid only with read capability. */
    ePAR_ACCESS_RO = ePAR_ACCESS_READ,                        /**< Parameter read only. */
    ePAR_ACCESS_RW = (ePAR_ACCESS_READ | ePAR_ACCESS_WRITE),  /**< Parameter read/write. */
} par_access_t;
#endif /* (1 == PAR_CFG_ENABLE_ACCESS) */
#if (1 == PAR_CFG_ENABLE_ROLE_POLICY)
/**
 * @brief External role-mask bits used by optional parameter role policy.
 */
typedef enum
{
    ePAR_ROLE_NONE = 0U,                                   /**< No external role granted. */
    ePAR_ROLE_PUBLIC = (1U << 0),                          /**< End-user/public role. */
    ePAR_ROLE_SERVICE = (1U << 1),                         /**< Service/maintenance role. */
    ePAR_ROLE_DEVELOPER = (1U << 2),                       /**< Developer/debug role. */
    ePAR_ROLE_MANUFACTURING = (1U << 3),                   /**< Manufacturing/production role. */
    ePAR_ROLE_ALL = (ePAR_ROLE_PUBLIC | ePAR_ROLE_SERVICE | ePAR_ROLE_DEVELOPER | ePAR_ROLE_MANUFACTURING), /**< All external roles. */
} par_role_t;
#endif /* (1 == PAR_CFG_ENABLE_ROLE_POLICY) */
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
#endif /* (1 == PAR_CFG_ENABLE_RANGE) */

/**
 * @brief Scalar parameter metadata stored in the unified parameter table.
 */
typedef struct
{
#if (1 == PAR_CFG_ENABLE_RANGE)
    par_range_t range; /**< Scalar value range. */
#endif /* (1 == PAR_CFG_ENABLE_RANGE) */
    par_type_t def;    /**< Scalar default value. */
} par_scalar_cfg_t;

#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
/**
 * @brief Default initializer descriptor for one fixed-capacity object parameter.
 */
typedef struct
{
    const uint8_t *p_data; /**< Pointer to default payload bytes, or NULL for an empty default. */
    uint16_t len;          /**< Default payload length in bytes. */
} par_obj_init_t;

/**
 * @brief Payload-length range metadata for one fixed-capacity object parameter.
 * @details For string rows, max_len is the stored payload capacity and does
 * not include the trailing NUL written by par_get_str() and
 * par_get_default_str(). String max_len is therefore limited to
 * UINT16_MAX - 1U so a uint16_t buffer-size argument can still represent the
 * required read buffer size.
 */
typedef struct
{
    uint16_t min_len; /**< Minimum accepted payload length in bytes. */
    uint16_t max_len; /**< Maximum accepted payload length in bytes. */
} par_obj_range_t;

/**
 * @brief Fixed-capacity object metadata stored in the unified parameter table.
 * @details Object parameters keep their live payload in the shared object pool,
 * while the table stores static metadata such as element size, accepted
 * payload-length range, and default payload descriptor.
 */
typedef struct
{
    uint16_t elem_size;    /**< Object element size in bytes. */
    par_obj_range_t range; /**< Accepted payload-length range in bytes. */
    par_obj_init_t def;    /**< Default payload descriptor. */
} par_object_cfg_t;
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

/**
 * @brief Type-specific metadata stored by one parameter-table entry.
 * @details When any object type is enabled, scalar and object metadata are
 * mutually exclusive because one table row always resolves to exactly one
 * value type. The union keeps the metadata table compact without changing the
 * scalar access model. When all object types are disabled, only scalar
 * metadata remains and the object member is compiled out.
 */
#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
typedef union
{
    par_scalar_cfg_t scalar; /**< Scalar metadata. */
    par_object_cfg_t object; /**< Object metadata. */
} par_value_cfg_t;
#else
typedef struct
{
    par_scalar_cfg_t scalar; /**< Scalar metadata. */
} par_value_cfg_t;
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

/**
 * @brief Parameter data settings.
 * @note The exact object size depends on enabled metadata fields and target ABI.
 */
struct par_cfg_s
{
#if (1 == PAR_CFG_ENABLE_NAME)
    const char *name;         /**< Parameter display name. */
#endif /* (1 == PAR_CFG_ENABLE_NAME) */
    par_value_cfg_t value_cfg; /**< Type-specific scalar or object metadata. */
#if (1 == PAR_CFG_ENABLE_UNIT)
    const char *unit;         /**< Parameter engineering unit string. */
#endif /* (1 == PAR_CFG_ENABLE_UNIT) */
#if (1 == PAR_CFG_ENABLE_DESC)
    const char *desc;         /**< Parameter description string. */
#endif /* (1 == PAR_CFG_ENABLE_DESC) */
#if (1 == PAR_CFG_ENABLE_ID)
    uint16_t id;              /**< External parameter ID. */
#endif /* (1 == PAR_CFG_ENABLE_ID) */
    par_type_list_t type;     /**< Parameter type. */
#if (1 == PAR_CFG_ENABLE_ACCESS)
    par_access_t access;      /**< External access capability mask. */
#endif /* (1 == PAR_CFG_ENABLE_ACCESS) */
#if (1 == PAR_CFG_ENABLE_ROLE_POLICY)
    par_role_t read_roles;    /**< External roles allowed to read the parameter. */
    par_role_t write_roles;   /**< External roles allowed to write the parameter. */
#endif /* (1 == PAR_CFG_ENABLE_ROLE_POLICY) */
#if (1 == PAR_CFG_NVM_EN)
    bool persistent;          /**< Parameter persistence flag. */
    uint16_t persist_idx;     /**< Persistent slot index or PAR_PERSIST_IDX_INVALID. */
#endif /* (1 == PAR_CFG_NVM_EN) */
};
/**
 * @brief Device Parameters on-change callback.
 * @note The callback runs only on the normal setter path. Startup default
 * initialization, raw restore paths, fast setters, and bitwise fast setters do
 * not invoke it.
 * @note Keep the callback synchronous, short, and non-blocking. Avoid long I/O,
 * waits, sleeps, or any operation that extends the parameter lock hold time.
 * @note The callback is invoked synchronously from the normal scalar setter
 * path while the setter still owns the parameter lock. Do not call parameter
 * APIs for the same parameter from the callback. Avoid cross-parameter set/save
 * flows unless the application owns a strict lock-order policy.
 * @note Re-entering the parameter module from this callback is an advanced usage
 * pattern and must be reviewed carefully at application level. Prefer deferring
 * follow-up writes or persistence work to an application task or event queue.
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
#if (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
/**
 * @brief Device Parameters object validation callback.
 * @param par_num Parameter number.
 * @param p_data Pointer to the candidate payload bytes.
 * @param len Candidate payload length in bytes.
 * @return True when the payload is accepted; otherwise false.
 * @note Object setters do not provide a core on-change callback. Integrations
 * that need object change notifications should wrap successful object setter
 * calls at a higher layer.
 */
typedef bool (*pf_par_obj_validation_t)(const par_num_t par_num,
                                        const uint8_t *p_data,
                                        const uint16_t len);
#endif /* (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */

/** @} <!-- END GROUP --> */

#endif /* !defined(_PAR_TYPES_H_) */
