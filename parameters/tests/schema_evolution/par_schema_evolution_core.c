/**
 * @file par_schema_evolution_core.c
 * @brief Provide reusable NVM schema-evolution acceptance checks.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "par_schema_evolution_core.h"

#if defined(AUTOGEN_PM_TEST_NVM_SCHEMA_EVOLUTION)

#define PAR_TEST_SCHEMA_BUILD_ENABLED \
    ((1 == PAR_CFG_NVM_EN) && (1 == PAR_CFG_ENABLE_ID) && \
     (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED))

#if PAR_TEST_SCHEMA_BUILD_ENABLED

static par_schema_evolution_vprint_fn_t g_par_schema_evolution_vprint = NULL;
static void *g_p_par_schema_evolution_print_ctx = NULL;

/**
 * @brief Set the output callback used by schema-evolution helpers.
 * @param p_vprint Print callback that receives a va_list.
 * @param p_ctx User context passed to the callback.
 */
void par_schema_evolution_set_vprint(par_schema_evolution_vprint_fn_t p_vprint, void *p_ctx)
{
    g_par_schema_evolution_vprint = p_vprint;
    g_p_par_schema_evolution_print_ctx = p_ctx;
}

/**
 * @brief Print one schema-evolution helper line through the configured callback.
 * @param p_fmt printf-like format string.
 */
static void par_schema_evolution_print(const char * const p_fmt, ...)
{
    va_list args;

    if ((NULL == g_par_schema_evolution_vprint) || (NULL == p_fmt))
    {
        return;
    }

    va_start(args, p_fmt);
    g_par_schema_evolution_vprint(g_p_par_schema_evolution_print_ctx, p_fmt, args);
    va_end(args);
}

/**
 * @brief Base retained scalar parameter ID used by schema-evolution fixtures.
 */
#define PAR_TEST_SCHEMA_ID_BASE_U8 (60006U)

/**
 * @brief Tail scalar parameter ID used by delete/type-change fixtures.
 */
#define PAR_TEST_SCHEMA_ID_TAIL_U16 (60007U)

/**
 * @brief Retained string object parameter ID used by schema-evolution fixtures.
 */
#define PAR_TEST_SCHEMA_ID_NAME_STR (60008U)

/**
 * @brief Retained byte object parameter ID used by schema-evolution fixtures.
 */
#define PAR_TEST_SCHEMA_ID_BLOB_BYTES (60009U)

/**
 * @brief Appended scalar parameter ID used by compatible-append fixtures.
 */
#define PAR_TEST_SCHEMA_ID_NEW_U8 (60010U)

/**
 * @brief Appended string object parameter ID used by compatible-append fixtures.
 */
#define PAR_TEST_SCHEMA_ID_NEW_STR (60011U)

/**
 * @brief V1 default value for the base retained scalar parameter.
 */
#define PAR_TEST_SCHEMA_DEF_BASE_U8 (11U)

/**
 * @brief V1 default value for the tail scalar parameter.
 */
#define PAR_TEST_SCHEMA_DEF_TAIL_U16 (1000U)

/**
 * @brief V2 default value for the appended scalar parameter.
 */
#define PAR_TEST_SCHEMA_DEF_NEW_U8 (55U)

/**
 * @brief V1 prepared value written to the base retained scalar parameter.
 */
#define PAR_TEST_SCHEMA_PREP_BASE_U8 (42U)

/**
 * @brief V1 prepared value written to the tail scalar parameter.
 */
#define PAR_TEST_SCHEMA_PREP_TAIL_U16 (4242U)

/**
 * @brief Maximum object bytes used by the schema-evolution helper.
 */
#define PAR_TEST_SCHEMA_OBJ_BUF_SIZE (16U)

#define PAR_TEST_SCHEMA_PRINT(...) par_schema_evolution_print(__VA_ARGS__)

/**
 * @brief Convert one parameter status to a compact diagnostic string.
 * @param status Parameter API status value.
 * @return Constant status string.
 */
static const char *par_test_schema_status_str(const par_status_t status)
{
    switch (status)
    {
    case ePAR_OK:
        return "OK";
    case ePAR_ERROR:
        return "ERROR";
    case ePAR_ERROR_INIT:
        return "ERROR INIT";
    case ePAR_ERROR_NVM:
        return "ERROR NVM";
    case ePAR_ERROR_CRC:
        return "ERROR CRC";
    case ePAR_ERROR_TYPE:
        return "ERROR TYPE";
    case ePAR_ERROR_MUTEX:
        return "ERROR MUTEX";
    case ePAR_ERROR_VALUE:
        return "ERROR VALUE";
    case ePAR_ERROR_PARAM:
        return "ERROR PARAM";
    case ePAR_ERROR_PAR_NUM:
        return "ERROR PAR NUM";
    case ePAR_ERROR_ACCESS:
        return "ERROR ACCESS";
    case ePAR_ERROR_TABLE_ID:
        return "ERROR TABLE ID";
    default:
        return "MIXED";
    }
}

/**
 * @brief Return true when a parameter status does not contain error bits.
 * @param status Parameter API status value.
 * @return true when no error bit is present.
 */
static bool par_test_schema_status_ok(const par_status_t status)
{
    return (((uint32_t)status & (uint32_t)ePAR_STATUS_ERROR_MASK) == 0U);
}

/**
 * @brief Return true when append is expected to relocate/default object rows.
 *
 * @details Shared after-scalar placement derives the object block address from
 * the compiled scalar block size. A scalar-only tail append therefore moves the
 * object block and the generic core intentionally rebuilds object rows from
 * defaults instead of trying an unsafe flash relocation. Fixed shared placement
 * and dedicated object storage keep object rows independent from scalar growth.
 *
 * @return true when V2 append should verify default object rows.
 */
static bool par_test_schema_append_defaults_objects(void)
{
#if (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED)
#if (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_AFTER_SCALAR)
    return true;
#else
    return false;
#endif /* (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_AFTER_SCALAR) */
#else
    return false;
#endif /* (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) */
}

/**
 * @brief Get the active object storage placement name for diagnostics.
 * @return Constant placement string.
 */
static const char *par_test_schema_object_placement_str(void)
{
#if (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED)
    return "dedicated";
#elif (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_FIXED)
    return "fixed";
#else
    return "after_scalar";
#endif /* (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED) */
}

/**
 * @brief Ensure the parameter module is initialized before helper access.
 *
 * @details Runtime firmware usually calls par_init() during board startup. The
 * schema-evolution helper must reuse that live instance to avoid triggering
 * the par_init() single-initialization assertion from MSH commands.
 *
 * @return Operation status.
 */
static par_status_t par_test_schema_init(void)
{
    if (true == par_is_init())
    {
        return ePAR_OK;
    }

    return par_init();
}

/**
 * @brief Check whether an external parameter ID exists in the compiled table.
 * @param id External parameter ID.
 * @return true when the ID resolves to one live parameter.
 */
static bool par_test_schema_id_exists(const uint16_t id)
{
    par_num_t par_num = 0U;
    return (ePAR_OK == par_get_num_by_id(id, &par_num));
}

/**
 * @brief Print whether an external parameter ID is present in the active table.
 * @param id External parameter ID.
 * @param name Human-readable fixture role.
 */
static void par_test_schema_print_presence(const uint16_t id, const char * const name)
{
    par_num_t par_num = 0U;
    const par_status_t status = par_get_num_by_id(id, &par_num);

    if (ePAR_OK == status)
    {
        const par_cfg_t * const p_cfg = par_get_config(par_num);
        PAR_TEST_SCHEMA_PRINT("SCHEMA_ID id=%u name=%s par=%u type=%u persistent=%u\n",
                              (unsigned)id,
                              name,
                              (unsigned)par_num,
                              (NULL != p_cfg) ? (unsigned)p_cfg->type : 0U,
                              (NULL != p_cfg) ? (unsigned)p_cfg->persistent : 0U);
    }
    else
    {
        PAR_TEST_SCHEMA_PRINT("SCHEMA_ID id=%u name=%s absent status=%s(0x%04X)\n",
                              (unsigned)id,
                              name,
                              par_test_schema_status_str(status),
                              (unsigned)status);
    }
}

/**
 * @brief Persist one scalar U8 value by external ID.
 * @param id External parameter ID.
 * @param value U8 value to store.
 * @return Operation status.
 */
static par_status_t par_test_schema_save_u8(const uint16_t id, const uint8_t value)
{
    par_status_t status = par_set_scalar_by_id(id, &value);

    if (false == par_test_schema_status_ok(status))
    {
        return status;
    }
    return par_save_by_id(id);
}

/**
 * @brief Persist one scalar U16 value by external ID.
 * @param id External parameter ID.
 * @param value U16 value to store.
 * @return Operation status.
 */
static par_status_t par_test_schema_save_u16(const uint16_t id, const uint16_t value)
{
    par_status_t status = par_set_scalar_by_id(id, &value);

    if (false == par_test_schema_status_ok(status))
    {
        return status;
    }
    return par_save_by_id(id);
}

#if (1 == PAR_CFG_ENABLE_TYPE_STR)
/**
 * @brief Persist one string object by external ID.
 * @param id External parameter ID.
 * @param p_value NUL-terminated string value.
 * @return Operation status.
 */
static par_status_t par_test_schema_save_str(const uint16_t id, const char * const p_value)
{
    par_status_t status = par_set_str_by_id(id, p_value);

    if (false == par_test_schema_status_ok(status))
    {
        return status;
    }
    return par_save_by_id(id);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */

#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
/**
 * @brief Persist one byte object by external ID.
 * @param id External parameter ID.
 * @param p_data Pointer to payload bytes.
 * @param len Payload length in bytes.
 * @return Operation status.
 */
static par_status_t par_test_schema_save_bytes(const uint16_t id, const uint8_t * const p_data, const uint16_t len)
{
    par_status_t status = par_set_bytes_by_id(id, p_data, len);

    if (false == par_test_schema_status_ok(status))
    {
        return status;
    }
    return par_save_by_id(id);
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */

/**
 * @brief Read and compare one U8 parameter.
 * @param id External parameter ID.
 * @param expected Expected U8 value.
 * @param label Diagnostic label printed on failure.
 * @return true when the value matches.
 */
static bool par_test_schema_expect_u8(const uint16_t id, const uint8_t expected, const char * const label)
{
    uint8_t actual = 0U;
    const par_status_t status = par_get_scalar_by_id(id, &actual);

    if (false == par_test_schema_status_ok(status))
    {
        PAR_TEST_SCHEMA_PRINT("ERR %s id=%u read_status=%s(0x%04X)\n",
                              label,
                              (unsigned)id,
                              par_test_schema_status_str(status),
                              (unsigned)status);
        return false;
    }
    if (actual != expected)
    {
        PAR_TEST_SCHEMA_PRINT("ERR %s id=%u expected=%u actual=%u\n",
                              label,
                              (unsigned)id,
                              (unsigned)expected,
                              (unsigned)actual);
        return false;
    }
    PAR_TEST_SCHEMA_PRINT("OK %s id=%u value=%u\n", label, (unsigned)id, (unsigned)actual);
    return true;
}

/**
 * @brief Read and compare one U16 parameter.
 * @param id External parameter ID.
 * @param expected Expected U16 value.
 * @param label Diagnostic label printed on failure.
 * @return true when the value matches.
 */
static bool par_test_schema_expect_u16(const uint16_t id, const uint16_t expected, const char * const label)
{
    uint16_t actual = 0U;
    const par_status_t status = par_get_scalar_by_id(id, &actual);

    if (false == par_test_schema_status_ok(status))
    {
        PAR_TEST_SCHEMA_PRINT("ERR %s id=%u read_status=%s(0x%04X)\n",
                              label,
                              (unsigned)id,
                              par_test_schema_status_str(status),
                              (unsigned)status);
        return false;
    }
    if (actual != expected)
    {
        PAR_TEST_SCHEMA_PRINT("ERR %s id=%u expected=%u actual=%u\n",
                              label,
                              (unsigned)id,
                              (unsigned)expected,
                              (unsigned)actual);
        return false;
    }
    PAR_TEST_SCHEMA_PRINT("OK %s id=%u value=%u\n", label, (unsigned)id, (unsigned)actual);
    return true;
}

/**
 * @brief Read and compare one scalar parameter with its compiled default value.
 * @param id External parameter ID.
 * @param label Diagnostic label printed on failure.
 * @return true when the current value matches the compiled default.
 */
static bool par_test_schema_expect_scalar_default(const uint16_t id, const char * const label)
{
    par_num_t par_num = 0U;
    par_type_t actual = { 0 };
    par_type_t expected = { 0 };
    const par_status_t num_status = par_get_num_by_id(id, &par_num);
    const par_cfg_t *p_cfg;
    par_status_t status;

    if (ePAR_OK != num_status)
    {
        PAR_TEST_SCHEMA_PRINT("OK %s id=%u absent\n", label, (unsigned)id);
        return true;
    }

    p_cfg = par_get_config(par_num);
    if ((NULL == p_cfg) || (false == par_test_type_is_scalar(p_cfg->type)))
    {
        PAR_TEST_SCHEMA_PRINT("ERR %s id=%u is not scalar\n", label, (unsigned)id);
        return false;
    }

    status = par_get_scalar_default(par_num, &expected);
    if (false == par_test_schema_status_ok(status))
    {
        PAR_TEST_SCHEMA_PRINT("ERR %s id=%u default_status=%s(0x%04X)\n",
                              label,
                              (unsigned)id,
                              par_test_schema_status_str(status),
                              (unsigned)status);
        return false;
    }

    status = par_get_scalar_by_id(id, &actual);
    if (false == par_test_schema_status_ok(status))
    {
        PAR_TEST_SCHEMA_PRINT("ERR %s id=%u read_status=%s(0x%04X)\n",
                              label,
                              (unsigned)id,
                              par_test_schema_status_str(status),
                              (unsigned)status);
        return false;
    }

    if (false == par_test_scalar_equal(p_cfg->type, &actual, &expected))
    {
        PAR_TEST_SCHEMA_PRINT("ERR %s id=%u default_mismatch type=%u\n", label, (unsigned)id, (unsigned)p_cfg->type);
        return false;
    }

    PAR_TEST_SCHEMA_PRINT("OK %s id=%u default type=%u\n", label, (unsigned)id, (unsigned)p_cfg->type);
    return true;
}

#if (1 == PAR_CFG_ENABLE_TYPE_STR)
/**
 * @brief Read and compare one string object.
 * @param id External parameter ID.
 * @param p_expected Expected NUL-terminated string.
 * @param label Diagnostic label printed on failure.
 * @return true when the value matches.
 */
static bool par_test_schema_expect_str(const uint16_t id, const char * const p_expected, const char * const label)
{
    char actual[PAR_TEST_SCHEMA_OBJ_BUF_SIZE] = { 0 };
    uint16_t actual_len = 0U;
    const par_status_t status = par_get_str_by_id(id, actual, (uint16_t)sizeof(actual), &actual_len);

    if (false == par_test_schema_status_ok(status))
    {
        PAR_TEST_SCHEMA_PRINT("ERR %s id=%u read_status=%s(0x%04X)\n",
                              label,
                              (unsigned)id,
                              par_test_schema_status_str(status),
                              (unsigned)status);
        return false;
    }
    if (0 != strcmp(actual, p_expected))
    {
        PAR_TEST_SCHEMA_PRINT("ERR %s id=%u expected=%s actual=%s len=%u\n",
                              label,
                              (unsigned)id,
                              p_expected,
                              actual,
                              (unsigned)actual_len);
        return false;
    }
    PAR_TEST_SCHEMA_PRINT("OK %s id=%u value=%s len=%u\n", label, (unsigned)id, actual, (unsigned)actual_len);
    return true;
}

/**
 * @brief Read and compare one string object with its compiled default value.
 * @param id External parameter ID.
 * @param label Diagnostic label printed on failure.
 * @return true when the current value matches the compiled default.
 */
static bool par_test_schema_expect_str_default(const uint16_t id, const char * const label)
{
    char actual[PAR_TEST_SCHEMA_OBJ_BUF_SIZE] = { 0 };
    char expected[PAR_TEST_SCHEMA_OBJ_BUF_SIZE] = { 0 };
    uint16_t actual_len = 0U;
    uint16_t expected_len = 0U;
    par_status_t status;

    if (false == par_test_schema_id_exists(id))
    {
        PAR_TEST_SCHEMA_PRINT("OK %s id=%u absent\n", label, (unsigned)id);
        return true;
    }

    status = par_get_default_str_by_id(id, expected, (uint16_t)sizeof(expected), &expected_len);
    if (false == par_test_schema_status_ok(status))
    {
        PAR_TEST_SCHEMA_PRINT("ERR %s id=%u default_status=%s(0x%04X)\n",
                              label,
                              (unsigned)id,
                              par_test_schema_status_str(status),
                              (unsigned)status);
        return false;
    }

    status = par_get_str_by_id(id, actual, (uint16_t)sizeof(actual), &actual_len);
    if (false == par_test_schema_status_ok(status))
    {
        PAR_TEST_SCHEMA_PRINT("ERR %s id=%u read_status=%s(0x%04X)\n",
                              label,
                              (unsigned)id,
                              par_test_schema_status_str(status),
                              (unsigned)status);
        return false;
    }

    if ((actual_len != expected_len) || (0 != strcmp(actual, expected)))
    {
        PAR_TEST_SCHEMA_PRINT("ERR %s id=%u default_mismatch expected=%s actual=%s\n",
                              label,
                              (unsigned)id,
                              expected,
                              actual);
        return false;
    }

    PAR_TEST_SCHEMA_PRINT("OK %s id=%u default=%s len=%u\n", label, (unsigned)id, actual, (unsigned)actual_len);
    return true;
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */

#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
/**
 * @brief Read and compare one byte object.
 * @param id External parameter ID.
 * @param p_expected Expected payload bytes.
 * @param expected_len Expected payload length in bytes.
 * @param label Diagnostic label printed on failure.
 * @return true when the value matches.
 */
static bool par_test_schema_expect_bytes(const uint16_t id,
                                         const uint8_t * const p_expected,
                                         const uint16_t expected_len,
                                         const char * const label)
{
    uint8_t actual[PAR_TEST_SCHEMA_OBJ_BUF_SIZE] = { 0U };
    uint16_t actual_len = 0U;
    const par_status_t status = par_get_bytes_by_id(id, actual, (uint16_t)sizeof(actual), &actual_len);

    if (false == par_test_schema_status_ok(status))
    {
        PAR_TEST_SCHEMA_PRINT("ERR %s id=%u read_status=%s(0x%04X)\n",
                              label,
                              (unsigned)id,
                              par_test_schema_status_str(status),
                              (unsigned)status);
        return false;
    }
    if ((actual_len != expected_len) || (0 != memcmp(actual, p_expected, expected_len)))
    {
        PAR_TEST_SCHEMA_PRINT("ERR %s id=%u expected_len=%u actual_len=%u\n",
                              label,
                              (unsigned)id,
                              (unsigned)expected_len,
                              (unsigned)actual_len);
        return false;
    }
    PAR_TEST_SCHEMA_PRINT("OK %s id=%u len=%u\n", label, (unsigned)id, (unsigned)actual_len);
    return true;
}

/**
 * @brief Read and compare one byte object with its compiled default payload.
 * @param id External parameter ID.
 * @param label Diagnostic label printed on failure.
 * @return true when the current payload matches the compiled default.
 */
static bool par_test_schema_expect_bytes_default(const uint16_t id, const char * const label)
{
    uint8_t actual[PAR_TEST_SCHEMA_OBJ_BUF_SIZE] = { 0U };
    uint8_t expected[PAR_TEST_SCHEMA_OBJ_BUF_SIZE] = { 0U };
    uint16_t actual_len = 0U;
    uint16_t expected_len = 0U;
    par_status_t status;

    if (false == par_test_schema_id_exists(id))
    {
        PAR_TEST_SCHEMA_PRINT("OK %s id=%u absent\n", label, (unsigned)id);
        return true;
    }

    status = par_get_default_bytes_by_id(id, expected, (uint16_t)sizeof(expected), &expected_len);
    if (false == par_test_schema_status_ok(status))
    {
        PAR_TEST_SCHEMA_PRINT("ERR %s id=%u default_status=%s(0x%04X)\n",
                              label,
                              (unsigned)id,
                              par_test_schema_status_str(status),
                              (unsigned)status);
        return false;
    }

    status = par_get_bytes_by_id(id, actual, (uint16_t)sizeof(actual), &actual_len);
    if (false == par_test_schema_status_ok(status))
    {
        PAR_TEST_SCHEMA_PRINT("ERR %s id=%u read_status=%s(0x%04X)\n",
                              label,
                              (unsigned)id,
                              par_test_schema_status_str(status),
                              (unsigned)status);
        return false;
    }

    if ((actual_len != expected_len) || (0 != memcmp(actual, expected, expected_len)))
    {
        PAR_TEST_SCHEMA_PRINT("ERR %s id=%u default_mismatch expected_len=%u actual_len=%u\n",
                              label,
                              (unsigned)id,
                              (unsigned)expected_len,
                              (unsigned)actual_len);
        return false;
    }

    PAR_TEST_SCHEMA_PRINT("OK %s id=%u default_len=%u\n", label, (unsigned)id, (unsigned)actual_len);
    return true;
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */

/**
 * @brief Prepare the V1 fixture NVM image with non-default retained values.
 * @return Process-like return code, 0 on success.
 */
int par_schema_evolution_prepare(void)
{
    const uint8_t blob[] = { 0xA5U, 0x5AU, 0xC3U, 0x3CU };
    par_status_t status = par_test_schema_init();

    if (false == par_test_schema_status_ok(status))
    {
        PAR_TEST_SCHEMA_PRINT("ERR init status=%s(0x%04X)\n", par_test_schema_status_str(status), (unsigned)status);
        return -1;
    }

    status = par_test_schema_save_u8(PAR_TEST_SCHEMA_ID_BASE_U8, PAR_TEST_SCHEMA_PREP_BASE_U8);
    if (false == par_test_schema_status_ok(status))
    {
        PAR_TEST_SCHEMA_PRINT("ERR prepare base_u8 status=%s(0x%04X)\n", par_test_schema_status_str(status), (unsigned)status);
        return -1;
    }

    status = par_test_schema_save_u16(PAR_TEST_SCHEMA_ID_TAIL_U16, PAR_TEST_SCHEMA_PREP_TAIL_U16);
    if (false == par_test_schema_status_ok(status))
    {
        PAR_TEST_SCHEMA_PRINT("ERR prepare tail_u16 status=%s(0x%04X)\n", par_test_schema_status_str(status), (unsigned)status);
        return -1;
    }

#if (1 == PAR_CFG_ENABLE_TYPE_STR)
    status = par_test_schema_save_str(PAR_TEST_SCHEMA_ID_NAME_STR, "hv1");
    if (false == par_test_schema_status_ok(status))
    {
        PAR_TEST_SCHEMA_PRINT("ERR prepare name_str status=%s(0x%04X)\n", par_test_schema_status_str(status), (unsigned)status);
        return -1;
    }
#else
    PAR_TEST_SCHEMA_PRINT("ERR STR support disabled\n");
    return -1;
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */

#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
    status = par_test_schema_save_bytes(PAR_TEST_SCHEMA_ID_BLOB_BYTES, blob, (uint16_t)sizeof(blob));
    if (false == par_test_schema_status_ok(status))
    {
        PAR_TEST_SCHEMA_PRINT("ERR prepare blob_bytes status=%s(0x%04X)\n", par_test_schema_status_str(status), (unsigned)status);
        return -1;
    }
#else
    PAR_TEST_SCHEMA_PRINT("ERR BYTES support disabled\n");
    return -1;
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */

    status = par_save_clean();
    if (false == par_test_schema_status_ok(status))
    {
        PAR_TEST_SCHEMA_PRINT("ERR prepare save_clean status=%s(0x%04X)\n", par_test_schema_status_str(status), (unsigned)status);
        return -1;
    }

    PAR_TEST_SCHEMA_PRINT("PAR_SCHEMA_PREPARED base_u8=%u tail_u16=%u name=hv1 blob=A5-5A-C3-3C\n",
                          (unsigned)PAR_TEST_SCHEMA_PREP_BASE_U8,
                          (unsigned)PAR_TEST_SCHEMA_PREP_TAIL_U16);
    return 0;
}

/**
 * @brief Verify retained V1 values after the same fixture is rebooted.
 * @return true when all checks pass.
 */
static bool par_test_schema_verify_base_values(void)
{
    const uint8_t blob[] = { 0xA5U, 0x5AU, 0xC3U, 0x3CU };
    bool ok = true;

    ok = par_test_schema_expect_u8(PAR_TEST_SCHEMA_ID_BASE_U8, PAR_TEST_SCHEMA_PREP_BASE_U8, "base_u8") && ok;
    ok = par_test_schema_expect_u16(PAR_TEST_SCHEMA_ID_TAIL_U16, PAR_TEST_SCHEMA_PREP_TAIL_U16, "tail_u16") && ok;
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
    ok = par_test_schema_expect_str(PAR_TEST_SCHEMA_ID_NAME_STR, "hv1", "name_str") && ok;
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
    ok = par_test_schema_expect_bytes(PAR_TEST_SCHEMA_ID_BLOB_BYTES, blob, (uint16_t)sizeof(blob), "blob_bytes") && ok;
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */
    return ok;
}

/**
 * @brief Verify default values after an incompatible schema rebuild.
 * @return true when all present known IDs contain default values.
 */
static bool par_test_schema_verify_default_values(void)
{
    bool ok = true;

    ok = par_test_schema_expect_scalar_default(PAR_TEST_SCHEMA_ID_BASE_U8, "base_u8_default") && ok;
    ok = par_test_schema_expect_scalar_default(PAR_TEST_SCHEMA_ID_TAIL_U16, "tail_scalar_default") && ok;
    ok = par_test_schema_expect_scalar_default(PAR_TEST_SCHEMA_ID_NEW_U8, "new_u8_default") && ok;
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
    ok = par_test_schema_expect_str_default(PAR_TEST_SCHEMA_ID_NAME_STR, "name_str_default") && ok;
    ok = par_test_schema_expect_str_default(PAR_TEST_SCHEMA_ID_NEW_STR, "new_str_default") && ok;
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
    ok = par_test_schema_expect_bytes_default(PAR_TEST_SCHEMA_ID_BLOB_BYTES, "blob_bytes_default") && ok;
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */
    return ok;
}

/**
 * @brief Verify all known object rows are absent or restored to compiled defaults.
 * @return true when every known object row is defaulted or absent.
 */
static bool par_test_schema_verify_object_defaults(void)
{
    bool ok = true;

#if (1 == PAR_CFG_ENABLE_TYPE_STR)
    ok = par_test_schema_expect_str_default(PAR_TEST_SCHEMA_ID_NAME_STR, "name_str_default") && ok;
    ok = par_test_schema_expect_str_default(PAR_TEST_SCHEMA_ID_NEW_STR, "new_str_default") && ok;
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
    ok = par_test_schema_expect_bytes_default(PAR_TEST_SCHEMA_ID_BLOB_BYTES, "blob_bytes_default") && ok;
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */
    return ok;
}

/**
 * @brief Verify all V1-compatible object rows retain their prepared values.
 * @return true when compatible object rows retain V1 values and appended rows use defaults.
 */
static bool par_test_schema_verify_object_retained_values(void)
{
    const uint8_t blob[] = { 0xA5U, 0x5AU, 0xC3U, 0x3CU };
    bool ok = true;

#if (1 == PAR_CFG_ENABLE_TYPE_STR)
    ok = par_test_schema_expect_str(PAR_TEST_SCHEMA_ID_NAME_STR, "hv1", "name_str") && ok;
    ok = par_test_schema_expect_str_default(PAR_TEST_SCHEMA_ID_NEW_STR, "new_str_default") && ok;
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
    ok = par_test_schema_expect_bytes(PAR_TEST_SCHEMA_ID_BLOB_BYTES, blob, (uint16_t)sizeof(blob), "blob_bytes") && ok;
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */
    return ok;
}

#if (1 == PAR_CFG_ENABLE_TYPE_STR)
/**
 * @brief Read one string object and accept retained, default, or absent state.
 * @param id External parameter ID.
 * @param p_retained Expected retained V1 string when the row is preserved.
 * @param label Diagnostic label printed on failure.
 * @return true when the row is absent, retained, or defaulted.
 */
static bool par_test_schema_expect_str_retained_or_default(const uint16_t id,
                                                           const char * const p_retained,
                                                           const char * const label)
{
    char actual[PAR_TEST_SCHEMA_OBJ_BUF_SIZE] = { 0 };
    char expected_default[PAR_TEST_SCHEMA_OBJ_BUF_SIZE] = { 0 };
    uint16_t actual_len = 0U;
    uint16_t default_len = 0U;
    par_status_t status;

    if (false == par_test_schema_id_exists(id))
    {
        PAR_TEST_SCHEMA_PRINT("OK %s id=%u absent\n", label, (unsigned)id);
        return true;
    }

    status = par_get_str_by_id(id, actual, (uint16_t)sizeof(actual), &actual_len);
    if (false == par_test_schema_status_ok(status))
    {
        PAR_TEST_SCHEMA_PRINT("ERR %s id=%u read_status=%s(0x%04X)\n",
                              label,
                              (unsigned)id,
                              par_test_schema_status_str(status),
                              (unsigned)status);
        return false;
    }

    if (0 == strcmp(actual, p_retained))
    {
        PAR_TEST_SCHEMA_PRINT("OK %s id=%u retained=%s len=%u\n", label, (unsigned)id, actual, (unsigned)actual_len);
        return true;
    }

    status = par_get_default_str_by_id(id, expected_default, (uint16_t)sizeof(expected_default), &default_len);
    if (false == par_test_schema_status_ok(status))
    {
        PAR_TEST_SCHEMA_PRINT("ERR %s id=%u default_status=%s(0x%04X) actual=%s\n",
                              label,
                              (unsigned)id,
                              par_test_schema_status_str(status),
                              (unsigned)status,
                              actual);
        return false;
    }

    if ((actual_len == default_len) && (0 == strcmp(actual, expected_default)))
    {
        PAR_TEST_SCHEMA_PRINT("OK %s id=%u default=%s len=%u\n", label, (unsigned)id, actual, (unsigned)actual_len);
        return true;
    }

    PAR_TEST_SCHEMA_PRINT("ERR %s id=%u expected_retained=%s expected_default=%s actual=%s len=%u\n",
                          label,
                          (unsigned)id,
                          p_retained,
                          expected_default,
                          actual,
                          (unsigned)actual_len);
    return false;
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */

#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
/**
 * @brief Read one byte object and accept retained, default, or absent state.
 * @param id External parameter ID.
 * @param p_retained Expected retained V1 payload.
 * @param retained_len Expected retained payload length in bytes.
 * @param label Diagnostic label printed on failure.
 * @return true when the row is absent, retained, or defaulted.
 */
static bool par_test_schema_expect_bytes_retained_or_default(const uint16_t id,
                                                             const uint8_t * const p_retained,
                                                             const uint16_t retained_len,
                                                             const char * const label)
{
    uint8_t actual[PAR_TEST_SCHEMA_OBJ_BUF_SIZE] = { 0U };
    uint8_t expected_default[PAR_TEST_SCHEMA_OBJ_BUF_SIZE] = { 0U };
    uint16_t actual_len = 0U;
    uint16_t default_len = 0U;
    par_status_t status;

    if (false == par_test_schema_id_exists(id))
    {
        PAR_TEST_SCHEMA_PRINT("OK %s id=%u absent\n", label, (unsigned)id);
        return true;
    }

    status = par_get_bytes_by_id(id, actual, (uint16_t)sizeof(actual), &actual_len);
    if (false == par_test_schema_status_ok(status))
    {
        PAR_TEST_SCHEMA_PRINT("ERR %s id=%u read_status=%s(0x%04X)\n",
                              label,
                              (unsigned)id,
                              par_test_schema_status_str(status),
                              (unsigned)status);
        return false;
    }

    if ((actual_len == retained_len) && (0 == memcmp(actual, p_retained, retained_len)))
    {
        PAR_TEST_SCHEMA_PRINT("OK %s id=%u retained_len=%u\n", label, (unsigned)id, (unsigned)actual_len);
        return true;
    }

    status = par_get_default_bytes_by_id(id, expected_default, (uint16_t)sizeof(expected_default), &default_len);
    if (false == par_test_schema_status_ok(status))
    {
        PAR_TEST_SCHEMA_PRINT("ERR %s id=%u default_status=%s(0x%04X) actual_len=%u\n",
                              label,
                              (unsigned)id,
                              par_test_schema_status_str(status),
                              (unsigned)status,
                              (unsigned)actual_len);
        return false;
    }

    if ((actual_len == default_len) && (0 == memcmp(actual, expected_default, default_len)))
    {
        PAR_TEST_SCHEMA_PRINT("OK %s id=%u default_len=%u\n", label, (unsigned)id, (unsigned)actual_len);
        return true;
    }

    PAR_TEST_SCHEMA_PRINT("ERR %s id=%u expected_retained_len=%u expected_default_len=%u actual_len=%u\n",
                          label,
                          (unsigned)id,
                          (unsigned)retained_len,
                          (unsigned)default_len,
                          (unsigned)actual_len);
    return false;
}
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */

/**
 * @brief Verify all known object rows are absent, retained, or restored to compiled defaults.
 * @return true when every known object row is in an accepted scalar-rebuild state.
 */
static bool par_test_schema_verify_object_retained_or_default_values(void)
{
    const uint8_t blob[] = { 0xA5U, 0x5AU, 0xC3U, 0x3CU };
    bool ok = true;

#if (1 == PAR_CFG_ENABLE_TYPE_STR)
    ok = par_test_schema_expect_str_retained_or_default(PAR_TEST_SCHEMA_ID_NAME_STR, "hv1", "name_str_retain_or_default") && ok;
    ok = par_test_schema_expect_str_default(PAR_TEST_SCHEMA_ID_NEW_STR, "new_str_default") && ok;
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
    ok = par_test_schema_expect_bytes_retained_or_default(PAR_TEST_SCHEMA_ID_BLOB_BYTES,
                                                          blob,
                                                          (uint16_t)sizeof(blob),
                                                          "blob_bytes_retain_or_default") && ok;
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */
    return ok;
}

/**
 * @brief Verify object rows using the strict default-after-rebuild contract.
 * @return true when all known object rows are defaulted or absent.
 */
static bool par_test_schema_verify_scalar_rebuild_objects_default(void)
{
    PAR_TEST_SCHEMA_PRINT("SCHEMA_VERIFY scalar_rebuild object_placement=%s object_expect=default\n",
                          par_test_schema_object_placement_str());
    return par_test_schema_verify_object_defaults();
}

/**
 * @brief Verify object rows using the strict retain-after-rebuild contract.
 * @return true when compatible object rows retain V1 values.
 */
static bool par_test_schema_verify_scalar_rebuild_objects_retain(void)
{
    PAR_TEST_SCHEMA_PRINT("SCHEMA_VERIFY scalar_rebuild object_placement=%s object_expect=retain\n",
                          par_test_schema_object_placement_str());
    return par_test_schema_verify_object_retained_values();
}

/**
 * @brief Verify object rows using the flexible scalar-rebuild contract.
 * @return true when object rows are retained, defaulted, or absent.
 */
static bool par_test_schema_verify_scalar_rebuild_objects_any(void)
{
    PAR_TEST_SCHEMA_PRINT("SCHEMA_VERIFY scalar_rebuild object_placement=%s object_expect=retain_or_default\n",
                          par_test_schema_object_placement_str());
    return par_test_schema_verify_object_retained_or_default_values();
}

/**
 * @brief Verify object-only rebuild while scalar rows retain V1 values.
 * @return true when scalar rows are retained and object rows are defaulted or absent.
 */
static bool par_test_schema_verify_object_rebuild(void)
{
    bool ok = true;

    ok = par_test_schema_expect_u8(PAR_TEST_SCHEMA_ID_BASE_U8, PAR_TEST_SCHEMA_PREP_BASE_U8, "base_u8") && ok;
    ok = par_test_schema_expect_u16(PAR_TEST_SCHEMA_ID_TAIL_U16, PAR_TEST_SCHEMA_PREP_TAIL_U16, "tail_u16") && ok;
    ok = par_test_schema_expect_scalar_default(PAR_TEST_SCHEMA_ID_NEW_U8, "new_u8_default") && ok;
    ok = par_test_schema_verify_object_defaults() && ok;
    return ok;
}

/**
 * @brief Verify scalar rebuild while allowing either supported object outcome.
 * @return true when scalar rows are defaulted and object rows are retained, defaulted, or absent.
 */
static bool par_test_schema_verify_scalar_rebuild(void)
{
    bool ok = true;

    ok = par_test_schema_expect_scalar_default(PAR_TEST_SCHEMA_ID_BASE_U8, "base_u8_default") && ok;
    ok = par_test_schema_expect_scalar_default(PAR_TEST_SCHEMA_ID_TAIL_U16, "tail_scalar_default") && ok;
    ok = par_test_schema_expect_scalar_default(PAR_TEST_SCHEMA_ID_NEW_U8, "new_u8_default") && ok;
    ok = par_test_schema_verify_scalar_rebuild_objects_any() && ok;
    return ok;
}

/**
 * @brief Verify scalar rebuild with a strict object-retain contract.
 * @return true when scalar rows are defaulted and compatible object rows retain V1 values.
 */
static bool par_test_schema_verify_scalar_rebuild_object_retain(void)
{
    bool ok = true;

    ok = par_test_schema_expect_scalar_default(PAR_TEST_SCHEMA_ID_BASE_U8, "base_u8_default") && ok;
    ok = par_test_schema_expect_scalar_default(PAR_TEST_SCHEMA_ID_TAIL_U16, "tail_scalar_default") && ok;
    ok = par_test_schema_expect_scalar_default(PAR_TEST_SCHEMA_ID_NEW_U8, "new_u8_default") && ok;
    ok = par_test_schema_verify_scalar_rebuild_objects_retain() && ok;
    return ok;
}

/**
 * @brief Verify scalar rebuild with a strict object-default contract.
 * @return true when scalar rows are defaulted and object rows are defaulted or absent.
 */
static bool par_test_schema_verify_scalar_rebuild_object_default(void)
{
    bool ok = true;

    ok = par_test_schema_expect_scalar_default(PAR_TEST_SCHEMA_ID_BASE_U8, "base_u8_default") && ok;
    ok = par_test_schema_expect_scalar_default(PAR_TEST_SCHEMA_ID_TAIL_U16, "tail_scalar_default") && ok;
    ok = par_test_schema_expect_scalar_default(PAR_TEST_SCHEMA_ID_NEW_U8, "new_u8_default") && ok;
    ok = par_test_schema_verify_scalar_rebuild_objects_default() && ok;
    return ok;
}

/**
 * @brief Verify compatible append behavior in a V2 fixture.
 * @return true when retained and appended values match expectations.
 */
static bool par_test_schema_verify_compatible_append(void)
{
    bool ok = par_test_schema_verify_base_values();

    ok = par_test_schema_expect_scalar_default(PAR_TEST_SCHEMA_ID_NEW_U8, "new_u8_default") && ok;
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
    ok = par_test_schema_expect_str_default(PAR_TEST_SCHEMA_ID_NEW_STR, "new_str_default") && ok;
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
    return ok;
}

/**
 * @brief Verify scalar values are retained while object values default after scalar-growth relocation.
 * @return true when retained scalar and default object values match expectations.
 */
static bool par_test_schema_verify_scalar_append_object_rebuild(void)
{
    bool ok = true;

    ok = par_test_schema_expect_u8(PAR_TEST_SCHEMA_ID_BASE_U8, PAR_TEST_SCHEMA_PREP_BASE_U8, "base_u8") && ok;
    ok = par_test_schema_expect_u16(PAR_TEST_SCHEMA_ID_TAIL_U16, PAR_TEST_SCHEMA_PREP_TAIL_U16, "tail_u16") && ok;
    ok = par_test_schema_expect_scalar_default(PAR_TEST_SCHEMA_ID_NEW_U8, "new_u8_default") && ok;
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
    ok = par_test_schema_expect_str_default(PAR_TEST_SCHEMA_ID_NAME_STR, "name_str_default") && ok;
    ok = par_test_schema_expect_str_default(PAR_TEST_SCHEMA_ID_NEW_STR, "new_str_default") && ok;
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
    ok = par_test_schema_expect_bytes_default(PAR_TEST_SCHEMA_ID_BLOB_BYTES, "blob_bytes_default") && ok;
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */
    return ok;
}

/**
 * @brief Verify append with the expected object behavior for this build.
 * @return true when retained and default values match the active placement mode.
 */
static bool par_test_schema_verify_placement_append(void)
{
    PAR_TEST_SCHEMA_PRINT("SCHEMA_VERIFY scalar_append placement=%s object_expect=%s\n",
                          par_test_schema_object_placement_str(),
                          (true == par_test_schema_append_defaults_objects()) ? "default" : "retain");

    if (true == par_test_schema_append_defaults_objects())
    {
        return par_test_schema_verify_scalar_append_object_rebuild();
    }

    return par_test_schema_verify_compatible_append();
}


/**
 * @brief Return a stable name for one schema-evolution verify mode.
 * @param mode Verification mode.
 * @return Constant mode name.
 */
const char *par_schema_evolution_verify_mode_name(const par_schema_evolution_verify_mode_t mode)
{
    switch (mode)
    {
    case PAR_SCHEMA_EVOLUTION_VERIFY_BASE:
        return "base";
    case PAR_SCHEMA_EVOLUTION_VERIFY_SCALAR_APPEND:
        return "scalar_append";
    case PAR_SCHEMA_EVOLUTION_VERIFY_OBJECT_APPEND:
        return "object_append";
    case PAR_SCHEMA_EVOLUTION_VERIFY_MIXED_APPEND:
        return "mixed_append";
    case PAR_SCHEMA_EVOLUTION_VERIFY_SCALAR_APPEND_OBJECT_REBUILD:
        return "scalar_append_object_rebuild";
    case PAR_SCHEMA_EVOLUTION_VERIFY_OBJECT_REBUILD:
        return "object_rebuild";
    case PAR_SCHEMA_EVOLUTION_VERIFY_SCALAR_REBUILD:
        return "scalar_rebuild";
    case PAR_SCHEMA_EVOLUTION_VERIFY_SCALAR_REBUILD_OBJECT_RETAIN:
        return "scalar_rebuild_object_retain";
    case PAR_SCHEMA_EVOLUTION_VERIFY_SCALAR_REBUILD_OBJECT_DEFAULT:
        return "scalar_rebuild_object_default";
    case PAR_SCHEMA_EVOLUTION_VERIFY_FULL_REBUILD:
        return "full_rebuild";
    default:
        return "unknown";
    }
}

/**
 * @brief Verify one requested schema-evolution scenario.
 * @param mode Expected V2 behavior mode.
 * @return Process-like return code, 0 on success.
 */
int par_schema_evolution_verify(const par_schema_evolution_verify_mode_t mode)
{
    bool ok = false;
    const par_status_t status = par_test_schema_init();

    if (false == par_test_schema_status_ok(status))
    {
        PAR_TEST_SCHEMA_PRINT("ERR init status=%s(0x%04X)\n", par_test_schema_status_str(status), (unsigned)status);
        return -1;
    }

    switch (mode)
    {
    case PAR_SCHEMA_EVOLUTION_VERIFY_BASE:
        ok = par_test_schema_verify_base_values();
        break;
    case PAR_SCHEMA_EVOLUTION_VERIFY_SCALAR_APPEND:
        ok = par_test_schema_verify_placement_append();
        break;
    case PAR_SCHEMA_EVOLUTION_VERIFY_MIXED_APPEND:
        ok = par_test_schema_verify_placement_append();
        break;
    case PAR_SCHEMA_EVOLUTION_VERIFY_OBJECT_APPEND:
        ok = par_test_schema_verify_compatible_append();
        break;
    case PAR_SCHEMA_EVOLUTION_VERIFY_SCALAR_APPEND_OBJECT_REBUILD:
        ok = par_test_schema_verify_scalar_append_object_rebuild();
        break;
    case PAR_SCHEMA_EVOLUTION_VERIFY_OBJECT_REBUILD:
        ok = par_test_schema_verify_object_rebuild();
        break;
    case PAR_SCHEMA_EVOLUTION_VERIFY_SCALAR_REBUILD:
        ok = par_test_schema_verify_scalar_rebuild();
        break;
    case PAR_SCHEMA_EVOLUTION_VERIFY_SCALAR_REBUILD_OBJECT_RETAIN:
        ok = par_test_schema_verify_scalar_rebuild_object_retain();
        break;
    case PAR_SCHEMA_EVOLUTION_VERIFY_SCALAR_REBUILD_OBJECT_DEFAULT:
        ok = par_test_schema_verify_scalar_rebuild_object_default();
        break;
    case PAR_SCHEMA_EVOLUTION_VERIFY_FULL_REBUILD:
        ok = par_test_schema_verify_default_values();
        break;
    default:
        ok = false;
        break;
    }

    PAR_TEST_SCHEMA_PRINT("PAR_SCHEMA_VERIFY %s mode=%s(%u)\n",
                          (true == ok) ? "PASS" : "FAIL",
                          par_schema_evolution_verify_mode_name(mode),
                          (unsigned)mode);
    return (true == ok) ? 0 : -1;
}

/**
 * @brief Print the active fixture ID map and known values.
 * @return Process-like return code, always zero after initialization succeeds.
 */
int par_schema_evolution_dump(void)
{
    const par_status_t status = par_test_schema_init();

    if (false == par_test_schema_status_ok(status))
    {
        PAR_TEST_SCHEMA_PRINT("ERR init status=%s(0x%04X)\n", par_test_schema_status_str(status), (unsigned)status);
        return -1;
    }

    PAR_TEST_SCHEMA_PRINT("SCHEMA_CFG object_placement=%s append_object_expect=%s\n",
                          par_test_schema_object_placement_str(),
                          (true == par_test_schema_append_defaults_objects()) ? "default" : "retain");
    par_test_schema_print_presence(PAR_TEST_SCHEMA_ID_BASE_U8, "base_u8");
    par_test_schema_print_presence(PAR_TEST_SCHEMA_ID_TAIL_U16, "tail_u16");
    par_test_schema_print_presence(PAR_TEST_SCHEMA_ID_NAME_STR, "name_str");
    par_test_schema_print_presence(PAR_TEST_SCHEMA_ID_BLOB_BYTES, "blob_bytes");
    par_test_schema_print_presence(PAR_TEST_SCHEMA_ID_NEW_U8, "new_u8");
    par_test_schema_print_presence(PAR_TEST_SCHEMA_ID_NEW_STR, "new_str");
    return 0;
}


#endif /* PAR_TEST_SCHEMA_BUILD_ENABLED */
#endif /* defined(AUTOGEN_PM_TEST_NVM_SCHEMA_EVOLUTION) */
