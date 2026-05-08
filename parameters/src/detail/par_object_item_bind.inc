/**
 * @file par_object_item_bind.inc
 * @brief Bind object-row item macros for one `par_table.def` expansion site.
 *
 * @details The including translation unit must define both
 * `PAR_OBJECT_ITEM_ENABLED_HANDLER` and `PAR_OBJECT_ITEM_DISABLED_HANDLER`
 * before including this fragment. Each enabled object row maps to the enabled
 * handler. Each disabled object row maps to the disabled handler.
 *
 * This fragment intentionally has no include guards because it is designed for
 * repeated X-macro expansion sites.
 * @author wdfk-prog
 * @version 1.0
 * @date 2026-04-23
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 *
 * @note :
 * @par Change Log:
 * Date       Version Author        Description
 * 2026-04-23 1.0     wdfk-prog     first version
 */
#ifndef PAR_OBJECT_ITEM_ENABLED_HANDLER
#error "PAR_OBJECT_ITEM_ENABLED_HANDLER must be defined before including par_object_item_bind.inc"
#endif /* !defined(PAR_OBJECT_ITEM_ENABLED_HANDLER) */
#ifndef PAR_OBJECT_ITEM_DISABLED_HANDLER
#error "PAR_OBJECT_ITEM_DISABLED_HANDLER must be defined before including par_object_item_bind.inc"
#endif /* !defined(PAR_OBJECT_ITEM_DISABLED_HANDLER) */

#if (1 == PAR_CFG_ENABLE_TYPE_STR)
#define PAR_ITEM_STR PAR_OBJECT_ITEM_ENABLED_HANDLER
#else
#define PAR_ITEM_STR PAR_OBJECT_ITEM_DISABLED_HANDLER
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */

#if (1 == PAR_CFG_ENABLE_TYPE_BYTES)
#define PAR_ITEM_BYTES PAR_OBJECT_ITEM_ENABLED_HANDLER
#else
#define PAR_ITEM_BYTES PAR_OBJECT_ITEM_DISABLED_HANDLER
#endif /* (1 == PAR_CFG_ENABLE_TYPE_BYTES) */

#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U8)
#define PAR_ITEM_ARR_U8 PAR_OBJECT_ITEM_ENABLED_HANDLER
#else
#define PAR_ITEM_ARR_U8 PAR_OBJECT_ITEM_DISABLED_HANDLER
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U8) */

#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U16)
#define PAR_ITEM_ARR_U16 PAR_OBJECT_ITEM_ENABLED_HANDLER
#else
#define PAR_ITEM_ARR_U16 PAR_OBJECT_ITEM_DISABLED_HANDLER
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U16) */

#if (1 == PAR_CFG_ENABLE_TYPE_ARR_U32)
#define PAR_ITEM_ARR_U32 PAR_OBJECT_ITEM_ENABLED_HANDLER
#else
#define PAR_ITEM_ARR_U32 PAR_OBJECT_ITEM_DISABLED_HANDLER
#endif /* (1 == PAR_CFG_ENABLE_TYPE_ARR_U32) */
