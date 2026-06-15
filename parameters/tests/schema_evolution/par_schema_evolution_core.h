/**
 * @file par_schema_evolution_core.h
 * @brief Declare reusable NVM schema-evolution acceptance helpers.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */

#ifndef _PAR_SCHEMA_EVOLUTION_CORE_H_
#define _PAR_SCHEMA_EVOLUTION_CORE_H_

#include <stdarg.h>

#include "par_test.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/**
 * @brief Output callback used by schema-evolution helpers.
 * @param p_ctx User context supplied when the callback is registered.
 * @param p_fmt printf-like format string.
 * @param args Format argument list.
 */
typedef void (*par_schema_evolution_vprint_fn_t)(void *p_ctx, const char *p_fmt, va_list args);

/**
 * @brief Verification mode for one schema-evolution image.
 */
typedef enum
{
    PAR_SCHEMA_EVOLUTION_VERIFY_BASE = 0,                  /**< Verify prepared V1 baseline values after reboot. */
    PAR_SCHEMA_EVOLUTION_VERIFY_SCALAR_APPEND,             /**< Verify scalar tail append; object behavior follows placement mode. */
    PAR_SCHEMA_EVOLUTION_VERIFY_OBJECT_APPEND,             /**< Verify object append while V1 scalar/object rows are retained. */
    PAR_SCHEMA_EVOLUTION_VERIFY_MIXED_APPEND,              /**< Verify scalar and object append with placement-aware object checks. */
    PAR_SCHEMA_EVOLUTION_VERIFY_SCALAR_APPEND_OBJECT_REBUILD, /**< Verify scalar append plus object contract rebuild. */
    PAR_SCHEMA_EVOLUTION_VERIFY_OBJECT_REBUILD,            /**< Verify object-only rebuild while scalar rows retain V1 values. */
    PAR_SCHEMA_EVOLUTION_VERIFY_SCALAR_REBUILD,            /**< Verify scalar rebuild with flexible object result. */
    PAR_SCHEMA_EVOLUTION_VERIFY_SCALAR_REBUILD_OBJECT_RETAIN,  /**< Verify scalar rebuild with retained objects. */
    PAR_SCHEMA_EVOLUTION_VERIFY_SCALAR_REBUILD_OBJECT_DEFAULT, /**< Verify scalar rebuild with default objects. */
    PAR_SCHEMA_EVOLUTION_VERIFY_FULL_REBUILD               /**< Verify managed full rebuild to compiled defaults. */
} par_schema_evolution_verify_mode_t;

/**
 * @brief Set the output callback used by schema-evolution helpers.
 * @param p_vprint Print callback that receives a va_list.
 * @param p_ctx User context passed to the callback.
 */
void par_schema_evolution_set_vprint(par_schema_evolution_vprint_fn_t p_vprint, void *p_ctx);

/**
 * @brief Return a stable name for one schema-evolution verify mode.
 * @param mode Verification mode.
 * @return Constant mode name.
 */
const char *par_schema_evolution_verify_mode_name(par_schema_evolution_verify_mode_t mode);

/**
 * @brief Prepare the V1 fixture NVM image with non-default retained values.
 * @return Process-like return code, 0 on success.
 */
int par_schema_evolution_prepare(void);

/**
 * @brief Print the active fixture ID map and object placement expectation.
 * @return Process-like return code, 0 on success.
 */
int par_schema_evolution_dump(void);

/**
 * @brief Verify one requested schema-evolution scenario.
 * @param mode Expected behavior mode.
 * @return Process-like return code, 0 on success.
 */
int par_schema_evolution_verify(par_schema_evolution_verify_mode_t mode);

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* _PAR_SCHEMA_EVOLUTION_CORE_H_ */
