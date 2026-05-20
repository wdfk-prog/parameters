/** @brief Enable POSIX declarations used by fork-based reset tests. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif /* !defined(_POSIX_C_SOURCE) */

/**
 * @file test_par_nvm_flash_ee.c
 * @brief Exercise host Flash EE persistence and recovery paths.
 *
 * @copyright Copyright (c) 2026 Ziga Miklosic. Distributed under the MIT license.
 */
#include "test_host_common.h"
#include "rtthread.h"
#include "par_nvm_api.h"
#include "par_store_backend_flash_ee.h"
#include "par_store_backend.h"
#include "par_if.h"
#include "par_nvm_table_id.h"
#include "par_nvm_object_store.h"
#include "par_nvm_object.h"
#include "par_object.h"
#include "par_registration_api.h"
#include "fnv.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

/** @brief Host fake flash size for native Flash EE tests. */
#define HOST_FLASH_SIZE (0x1000U)
/** @brief Host fake flash erase granularity. */
#define HOST_FLASH_ERASE_SIZE (64U)
/** @brief Host fake flash program granularity. */
#define HOST_FLASH_PROGRAM_SIZE (4U)
/** @brief Disable the failpoint counter. */
#define HOST_FLASH_FAIL_DISABLED (-1)
/** @brief Host fake flash image path buffer length. */
#define HOST_FLASH_IMAGE_PATH_LEN (128U)
/** @brief Offset of the serialized scalar NVM object-count field in host flash. */
#define HOST_NVM_HEAD_OBJ_NB_OFFSET ((uint32_t)sizeof(uint32_t))
/** @brief Offset of the serialized scalar NVM table-ID field in host flash. */
#define HOST_NVM_HEAD_TABLE_ID_OFFSET \
    ((uint32_t)sizeof(uint32_t) + (uint32_t)sizeof(uint16_t))
/** @brief Size of the serialized scalar NVM table-ID field in host flash. */
#define HOST_NVM_HEAD_TABLE_ID_SIZE   ((uint32_t)sizeof(uint32_t))

/** @brief Offset of the serialized scalar NVM header CRC field in host flash. */
#define HOST_NVM_HEAD_CRC_OFFSET \
    (HOST_NVM_HEAD_TABLE_ID_OFFSET + HOST_NVM_HEAD_TABLE_ID_SIZE)
/** @brief Parameter signature value stored in the scalar NVM header. */
#define HOST_NVM_SIGN                 (0xFF00AA55UL)
/** @brief Flash-EE bank count used by the native host backend. */
#define HOST_FLASH_EE_BANK_COUNT      (2U)
/** @brief Flash-EE bank header magic. */
#define HOST_FLASH_EE_HEADER_MAGIC    (0x50454548UL)
/** @brief Flash-EE active-bank state word. */
#define HOST_FLASH_EE_HEADER_ACTIVE   (0xFFFF0000UL)
/** @brief Flash-EE append-record commit-unit magic. */
#define HOST_FLASH_EE_RECORD_MAGIC    (0x50454552UL)
/** @brief Flash-EE prepared-bank state word. */
#define HOST_FLASH_EE_HEADER_PREPARE  (0xFFFFFF00UL)
/** @brief Flash-EE bank header size. */
#define HOST_FLASH_EE_HEADER_SIZE     (64U)
/** @brief Flash-EE record metadata size. */
#define HOST_FLASH_EE_RECORD_META_SIZE (12U)
/** @brief Flash-EE line size used by host NVM tests. */
#define HOST_FLASH_EE_LINE_SIZE       ((uint32_t)PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE)
/** @brief Flash-EE logical line count in the host test geometry. */
#define HOST_FLASH_EE_LINE_COUNT \
    ((uint32_t)(PAR_CFG_NVM_BACKEND_FLASH_EE_LOGICAL_SIZE / PAR_CFG_NVM_BACKEND_FLASH_EE_LINE_SIZE))
/** @brief Flash-EE record commit-unit offset. */
#define HOST_FLASH_EE_RECORD_COMMIT_OFFSET \
    (((HOST_FLASH_EE_LINE_SIZE + HOST_FLASH_EE_RECORD_META_SIZE + HOST_FLASH_PROGRAM_SIZE - 1U) / \
      HOST_FLASH_PROGRAM_SIZE) * HOST_FLASH_PROGRAM_SIZE)
/** @brief Flash-EE append-record size used by host NVM tests. */
#define HOST_FLASH_EE_RECORD_SIZE \
    (HOST_FLASH_EE_RECORD_COMMIT_OFFSET + HOST_FLASH_PROGRAM_SIZE)
/** @brief Flash-EE physical bank size in the host fake flash. */
#define HOST_FLASH_EE_BANK_SIZE       (HOST_FLASH_SIZE / HOST_FLASH_EE_BANK_COUNT)
#ifndef PAR_HOST_TEST_PROFILE_NAME
/** @brief Human-readable host NVM profile name printed by matrix tests. */
#define PAR_HOST_TEST_PROFILE_NAME "default"
#endif /* !defined(PAR_HOST_TEST_PROFILE_NAME) */


/** @brief Captured shell output buffer size for NVM shell command tests. */
#define NVM_SHELL_CAPTURE_SIZE (8192U)

/** @brief Captured shell output for NVM shell command tests. */
static char g_nvm_shell_capture[NVM_SHELL_CAPTURE_SIZE];
/** @brief Number of used bytes in g_nvm_shell_capture. */
static size_t g_nvm_shell_capture_used;

#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/** @brief Callback hit counter for NVM callback current-policy tests. */
static unsigned g_nvm_callback_hits;

/**
 * @brief Persist the changed scalar from inside an on-change callback.
 * @param par_num Parameter number that changed.
 * @param new_val New scalar value.
 * @param old_val Previous scalar value.
 */
static void on_nvm_scalar_change_save(const par_num_t par_num,
                                      const par_type_t new_val,
                                      const par_type_t old_val)
{
    (void)new_val;
    (void)old_val;
    g_nvm_callback_hits++;
    (void)par_save(par_num);
}

/**
 * @brief Persist all live values from inside an on-change callback.
 * @param par_num Parameter number that changed.
 * @param new_val New scalar value.
 * @param old_val Previous scalar value.
 */
static void on_nvm_scalar_change_save_all(const par_num_t par_num,
                                          const par_type_t new_val,
                                          const par_type_t old_val)
{
    (void)par_num;
    (void)new_val;
    (void)old_val;
    g_nvm_callback_hits++;
    (void)par_save_all();
}

/**
 * @brief Rewrite clean NVM state from inside an on-change callback.
 * @param par_num Parameter number that changed.
 * @param new_val New scalar value.
 * @param old_val Previous scalar value.
 */
static void on_nvm_scalar_change_save_clean(const par_num_t par_num,
                                            const par_type_t new_val,
                                            const par_type_t old_val)
{
    (void)par_num;
    (void)new_val;
    (void)old_val;
    g_nvm_callback_hits++;
    (void)par_save_clean();
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/**
 * @brief Deinitialize the parameter module from inside an on-change callback.
 * @param par_num Parameter number that changed.
 * @param new_val New scalar value.
 * @param old_val Previous scalar value.
 */
static void on_nvm_scalar_change_deinit(const par_num_t par_num,
                                        const par_type_t new_val,
                                        const par_type_t old_val)
{
    (void)par_num;
    (void)new_val;
    (void)old_val;
    g_nvm_callback_hits++;
    (void)par_deinit();
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */


#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (1 == PAR_CFG_ENABLE_TYPE_STR)
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/** @brief Persist the current object from inside object validation. */
static bool on_nvm_object_validation_save(const par_num_t par_num,
                                          const uint8_t *p_data,
                                          const uint16_t len)
{
    (void)p_data;
    (void)len;
    g_nvm_callback_hits++;
    return (ePAR_OK == par_save(par_num));
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/** @brief Persist all current values from inside object validation. */
static bool on_nvm_object_validation_save_all(const par_num_t par_num,
                                              const uint8_t *p_data,
                                              const uint16_t len)
{
    (void)par_num;
    (void)p_data;
    (void)len;
    g_nvm_callback_hits++;
    return (ePAR_OK == par_save_all());
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */

#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
/** @brief Rewrite clean current values from inside object validation. */
static bool on_nvm_object_validation_save_clean(const par_num_t par_num,
                                                const uint8_t *p_data,
                                                const uint16_t len)
{
    (void)par_num;
    (void)p_data;
    (void)len;
    g_nvm_callback_hits++;
    return (ePAR_OK == par_save_clean());
}
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (1 == PAR_CFG_ENABLE_TYPE_STR) */

#include "nvm_flash_ee/par_nvm_flash_ee_shell_stubs.inc"
#include "nvm_flash_ee/par_nvm_flash_ee_flash_image.inc"
#include "nvm_flash_ee/par_nvm_flash_ee_scalar_record_helpers.inc"
#include "nvm_flash_ee/par_nvm_flash_ee_object_record_helpers.inc"
#include "nvm_flash_ee/par_nvm_flash_ee_backend_ports.inc"
#include "nvm_flash_ee/par_nvm_flash_ee_scalar_recovery_basic_cases.inc"
#include "nvm_flash_ee/par_nvm_flash_ee_scalar_recovery_current_policy_cases.inc"
#include "nvm_flash_ee/par_nvm_flash_ee_checkpoint_cases.inc"
#include "nvm_flash_ee/par_nvm_flash_ee_port_failpoint_cases.inc"
#include "nvm_flash_ee/par_nvm_flash_ee_schema_shell_cases.inc"
#include "nvm_flash_ee/par_nvm_flash_ee_shared_object_basic_cases.inc"
#include "nvm_flash_ee/par_nvm_flash_ee_shared_object_failpoint_cases.inc"
#include "nvm_flash_ee/par_nvm_flash_ee_scalar_failpoint_cases.inc"
#include "nvm_flash_ee/par_nvm_flash_ee_shared_object_corruption_cases.inc"
#include "nvm_flash_ee/par_nvm_flash_ee_dedicated_object_cases.inc"

int main(void)
{
    int result;
#if defined(PAR_HOST_TEST_SCHEMA_EVOLUTION_WRITE)
    static const par_host_test_case_t cases[] = {
        { "nvm_schema_rebuild_write_baseline_image", test_nvm_schema_rebuild_write_baseline_image },
    };
#elif defined(PAR_HOST_TEST_SCHEMA_EVOLUTION_READ)
    static const par_host_test_case_t cases[] = {
        { "nvm_schema_type_change_triggers_table_rebuild_defaults", test_nvm_schema_type_change_triggers_table_rebuild_defaults },
    };
#elif defined(PAR_HOST_TEST_SCHEMA_SLOT_REORDER_READ)
    static const par_host_test_case_t cases[] = {
        { "nvm_schema_slot_reorder_triggers_table_rebuild_defaults", test_nvm_schema_slot_reorder_triggers_table_rebuild_defaults },
    };
#elif defined(PAR_HOST_TEST_SCHEMA_PERSISTENT_REMOVED_READ)
    static const par_host_test_case_t cases[] = {
        { "nvm_schema_persistent_removed_rebuilds_remaining_defaults", test_nvm_schema_persistent_removed_rebuilds_remaining_defaults },
    };
#elif defined(PAR_HOST_TEST_SCHEMA_SCALAR_TO_OBJECT_READ)
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
    static const par_host_test_case_t cases[] = {
        { "nvm_schema_scalar_to_object_rejects_stale_image_current_policy", test_nvm_schema_scalar_to_object_rejects_stale_image_current_policy },
    };
#else
#error "PAR_HOST_TEST_SCHEMA_SCALAR_TO_OBJECT_READ requires PAR_HOST_ENABLE_CURRENT_POLICY_TESTS"
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
#elif defined(PAR_HOST_TEST_SCHEMA_OBJECT_TO_SCALAR_READ)
    static const par_host_test_case_t cases[] = {
        { "nvm_schema_object_to_scalar_rebuilds_scalar_default", test_nvm_schema_object_to_scalar_rebuilds_scalar_default },
    };
#elif defined(PAR_HOST_TEST_SCHEMA_OBJECT_CAPACITY_SHRINK_READ)
    static const par_host_test_case_t cases[] = {
        { "nvm_schema_object_capacity_shrink_rebuilds_object_default", test_nvm_schema_object_capacity_shrink_rebuilds_object_default },
    };
#elif defined(PAR_HOST_TEST_FIXED_OBJECT_INVALID)
    static const par_host_test_case_t cases[] = {
        { "nvm_fixed_object_invalid_rejects_init", test_nvm_fixed_object_invalid_rejects_init },
    };
#elif defined(PAR_HOST_TEST_OBJECT_ONLY)
    static const par_host_test_case_t cases[] = {
        { "nvm_object_only_save_reload_preserves_object", test_nvm_object_only_save_reload_preserves_object },
    };
#elif defined(PAR_HOST_TEST_OBJECT_ARRAY_NVM)
    static const par_host_test_case_t cases[] = {
        { "nvm_object_arr_u16_elem_size_corruption_restores_default", test_nvm_object_arr_u16_elem_size_corruption_restores_default },
        { "nvm_object_arr_u32_capacity_corruption_restores_default", test_nvm_object_arr_u32_capacity_corruption_restores_default },
    };
#else
    static const par_host_test_case_t cases[] = {
        { "nvm_first_boot_formats_and_restores_defaults", test_nvm_first_boot_formats_and_restores_defaults },
        { "nvm_save_reload_preserves_last_committed_scalar_values", test_nvm_save_reload_preserves_last_committed_scalar_values },
        { "nvm_object_save_reload_shared_fixed_addr", test_nvm_object_save_reload_shared_fixed_addr },
        { "nvm_object_updates_do_not_corrupt_scalar_values", test_nvm_object_updates_do_not_corrupt_scalar_values },
        { "nvm_scalar_updates_do_not_corrupt_object_values", test_nvm_scalar_updates_do_not_corrupt_object_values },
        { "flash_ee_failed_program_reload_preserves_last_committed_value", test_flash_ee_failed_program_reload_preserves_last_committed_value },
        { "flash_ee_program_failpoint_matrix_preserves_last_commit", test_flash_ee_program_failpoint_matrix_preserves_last_commit },
        { "flash_ee_repeated_failed_saves_converge_to_last_commit", test_flash_ee_repeated_failed_saves_converge_to_last_commit },
        { "flash_ee_seeded_scalar_restarts_match_model", test_flash_ee_seeded_scalar_restarts_match_model },
        { "flash_ee_representative_program_failpoint_sweep_preserves_last_commit", test_flash_ee_representative_program_failpoint_sweep_preserves_last_commit },
#if (1 == PAR_CFG_NVM_WRITE_VERIFY_EN)
        { "nvm_scalar_write_verify_detects_readback_mismatch", test_nvm_scalar_write_verify_detects_readback_mismatch },
        { "nvm_scalar_write_verify_read_error_is_reported", test_nvm_scalar_write_verify_read_error_is_reported },
#endif /* (1 == PAR_CFG_NVM_WRITE_VERIFY_EN) */
        { "flash_ee_save_all_failpoint_preserves_last_committed_scalar", test_flash_ee_save_all_failpoint_preserves_last_committed_scalar },
        { "flash_ee_save_clean_failpoint_matrix_preserves_last_commit", test_flash_ee_save_clean_failpoint_matrix_preserves_last_commit },
        { "flash_ee_save_clean_checkpoint_erase_failure_preserves_last_commit", test_flash_ee_save_clean_checkpoint_erase_failure_preserves_last_commit },
        { "flash_ee_failed_program_graceful_deinit_commits_live_value", test_flash_ee_failed_program_graceful_deinit_commits_live_value },
        { "flash_ee_corruption_rebuilds_default_value", test_flash_ee_corruption_rebuilds_default_value },
        { "nvm_table_id_mismatch_rebuilds_defaults", test_nvm_table_id_mismatch_rebuilds_defaults },
        { "flash_ee_failed_erase_preserves_existing_bytes", test_flash_ee_failed_erase_preserves_existing_bytes },
        { "flash_ee_many_updates_preserve_last_committed_value", test_flash_ee_many_updates_preserve_last_committed_value },
        { "flash_ee_record_commit_marker_corruption_ignores_partial_record", test_flash_ee_record_commit_marker_corruption_ignores_partial_record },
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
        { "flash_ee_bad_committed_tail_rebuilds_default_current_policy", test_flash_ee_bad_committed_tail_rebuilds_default_current_policy },
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
        { "flash_ee_newer_bad_record_rebuilds_default_current_policy", test_flash_ee_newer_bad_record_rebuilds_default_current_policy },
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
        { "flash_ee_callback_save_during_dispatch_current_policy", test_flash_ee_callback_save_during_dispatch_current_policy },
        { "flash_ee_callback_save_all_during_dispatch_current_policy", test_flash_ee_callback_save_all_during_dispatch_current_policy },
        { "flash_ee_callback_save_clean_during_dispatch_current_policy", test_flash_ee_callback_save_clean_during_dispatch_current_policy },
        { "flash_ee_callback_deinit_during_dispatch_current_policy", test_flash_ee_callback_deinit_during_dispatch_current_policy },
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
        { "flash_ee_checkpoint_power_loss_preserves_previous_active_bank", test_flash_ee_checkpoint_power_loss_preserves_previous_active_bank },
        { "flash_ee_checkpoint_prepare_header_power_loss_ignores_new_bank", test_flash_ee_checkpoint_prepare_header_power_loss_ignores_new_bank },
        { "flash_ee_newer_bank_with_bad_cfg_crc_is_ignored", test_flash_ee_newer_bank_with_bad_cfg_crc_is_ignored },
        { "flash_ee_checkpoint_record_copy_power_loss_preserves_previous_active_bank", test_flash_ee_checkpoint_record_copy_power_loss_preserves_previous_active_bank },
        { "flash_ee_checkpoint_record_copy_failpoint_sweep_preserves_previous_bank", test_flash_ee_checkpoint_record_copy_failpoint_sweep_preserves_previous_bank },
        { "flash_ee_port_rejects_wrapped_ranges", test_flash_ee_port_rejects_wrapped_ranges },
        { "flash_ee_program_one_to_zero_semantics", test_flash_ee_program_one_to_zero_semantics },
        { "flash_ee_partial_line_write_preserves_neighbor_bytes", test_flash_ee_partial_line_write_preserves_neighbor_bytes },
        { "flash_ee_cross_cache_window_write_reload_roundtrip", test_flash_ee_cross_cache_window_write_reload_roundtrip },
        { "flash_ee_erase_partial_line_preserves_outside_range", test_flash_ee_erase_partial_line_preserves_outside_range },
        { "flash_ee_port_failpoint_countdown_matrix", test_flash_ee_port_failpoint_countdown_matrix },
        { "flash_ee_port_read_failpoint_reports_error", test_flash_ee_port_read_failpoint_reports_error },
        { "flash_ee_init_rejects_invalid_geometry_and_recovers", test_flash_ee_init_rejects_invalid_geometry_and_recovers },
        { "nvm_save_by_id_save_all_and_n_save_wrappers", test_nvm_save_by_id_save_all_and_n_save_wrappers },
        { "nvm_scalar_stored_count_smaller_applies_layout_policy", test_nvm_scalar_stored_count_smaller_applies_layout_policy },
        { "nvm_scalar_stored_count_larger_rebuilds_defaults_once", test_nvm_scalar_stored_count_larger_rebuilds_defaults_once },
        { "nvm_obj_n_save_unchanged_skips_backend_write", test_nvm_obj_n_save_unchanged_skips_backend_write },
        { "nvm_str_embedded_nul_n_save_rejected_without_mutation", test_nvm_str_embedded_nul_n_save_rejected_without_mutation },
        { "nvm_wrapper_negative_type_and_init_policies", test_nvm_wrapper_negative_type_and_init_policies },
#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED)
        { "nvm_object_region_profile_bounds", test_nvm_object_region_profile_bounds },
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) */
        { "nvm_save_non_persistent_is_noop_and_preserves_image", test_nvm_save_non_persistent_is_noop_and_preserves_image },
#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) && \
    (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_FIXED)
        { "nvm_object_header_corruption_restores_default_without_scalar_loss", test_nvm_object_header_corruption_restores_default_without_scalar_loss },
        { "nvm_object_record_len_corruption_restores_default_without_scalar_loss", test_nvm_object_record_len_corruption_restores_default_without_scalar_loss },
        { "nvm_object_payload_crc_corruption_restores_default_without_scalar_loss", test_nvm_object_payload_crc_corruption_restores_default_without_scalar_loss },
        { "nvm_object_record_meta_id_corruption_restores_default_without_scalar_loss", test_nvm_object_record_meta_id_corruption_restores_default_without_scalar_loss },
        { "nvm_object_header_body_size_mismatch_rebuilds_objects_only", test_nvm_object_header_body_size_mismatch_rebuilds_objects_only },
        { "nvm_object_header_version_mismatch_rebuilds_objects_only", test_nvm_object_header_version_mismatch_rebuilds_objects_only },
        { "nvm_object_record_type_mismatch_restores_default_without_scalar_loss", test_nvm_object_record_type_mismatch_restores_default_without_scalar_loss },
        { "nvm_object_record_flags_mismatch_restores_default_without_scalar_loss", test_nvm_object_record_flags_mismatch_restores_default_without_scalar_loss },
        { "nvm_object_record_elem_size_mismatch_restores_default_without_scalar_loss", test_nvm_object_record_elem_size_mismatch_restores_default_without_scalar_loss },
        { "nvm_object_record_capacity_mismatch_restores_default_without_scalar_loss", test_nvm_object_record_capacity_mismatch_restores_default_without_scalar_loss },
        { "nvm_object_bytes_payload_crc_corruption_restores_default", test_nvm_object_bytes_payload_crc_corruption_restores_default },
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) && (PAR_CFG_NVM_OBJECT_ADDR_MODE == PAR_CFG_NVM_OBJECT_ADDR_FIXED) */
#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED)
        { "nvm_shared_save_all_failpoint_preserves_last_committed_values", test_nvm_shared_save_all_failpoint_preserves_last_committed_values },
        { "nvm_shared_obj_n_save_program_failpoint_matrix_preserves_last_commit", test_nvm_shared_obj_n_save_program_failpoint_matrix_preserves_last_commit },
        { "nvm_shared_repeated_mixed_failures_converge_to_last_commit", test_nvm_shared_repeated_mixed_failures_converge_to_last_commit },
        { "nvm_shared_seeded_mixed_restarts_match_model", test_nvm_shared_seeded_mixed_restarts_match_model },
        { "nvm_shared_seeded_mixed_random_restarts_match_model", test_nvm_shared_seeded_mixed_random_restarts_match_model },
#if (1 == PAR_CFG_ENABLE_TYPE_STR)
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
        { "nvm_object_validation_save_during_dispatch_current_policy", test_nvm_object_validation_save_during_dispatch_current_policy },
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
        { "nvm_object_validation_save_all_during_dispatch_current_policy", test_nvm_object_validation_save_all_during_dispatch_current_policy },
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
#if PAR_HOST_ENABLE_CURRENT_POLICY_TESTS
        { "nvm_object_validation_save_clean_during_dispatch_current_policy", test_nvm_object_validation_save_clean_during_dispatch_current_policy },
#endif /* PAR_HOST_ENABLE_CURRENT_POLICY_TESTS */
#endif /* (1 == PAR_CFG_ENABLE_TYPE_STR) */
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_SHARED) */
        { "msh_save_persists_live_scalar_after_restart", test_msh_save_persists_live_scalar_after_restart },
        { "msh_save_clean_rewrites_live_values", test_msh_save_clean_rewrites_live_values },
        { "msh_save_reports_backend_error", test_msh_save_reports_backend_error },
#if (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && \
    (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED)
        { "object_dedicated_write_fail_preserves_backend_image", test_object_dedicated_write_fail_preserves_backend_image },
        { "object_dedicated_read_fail_reports_error_and_recovers_object", test_object_dedicated_read_fail_reports_error_and_recovers_object },
        { "object_dedicated_sync_fail_reports_error_and_recovers_object", test_object_dedicated_sync_fail_reports_error_and_recovers_object },
#if (1 == PAR_CFG_NVM_OBJECT_WRITE_VERIFY_EN)
        { "nvm_object_write_verify_detects_payload_mismatch", test_nvm_object_write_verify_detects_payload_mismatch },
        { "nvm_object_write_verify_read_error_is_reported", test_nvm_object_write_verify_read_error_is_reported },
#endif /* (1 == PAR_CFG_NVM_OBJECT_WRITE_VERIFY_EN) */
#if (PAR_CFG_NVM_OBJECT_DEDICATED_BASE_ADDR > 0U)
        { "object_dedicated_nonzero_base_save_reload_and_prefix_stable", test_object_dedicated_nonzero_base_save_reload_and_prefix_stable },
#endif /* (PAR_CFG_NVM_OBJECT_DEDICATED_BASE_ADDR > 0U) */
        { "object_dedicated_erase_fail_aborts_save_all", test_object_dedicated_erase_fail_aborts_save_all },
        { "object_dedicated_n_save_write_fail_preserves_persisted_object", test_object_dedicated_n_save_write_fail_preserves_persisted_object },
        { "object_dedicated_save_all_object_write_fail_scalar_new_object_default", test_object_dedicated_save_all_object_write_fail_scalar_new_object_default },
        { "object_dedicated_save_all_payload_metadata_marker_failpoint_sweep", test_object_dedicated_save_all_payload_metadata_marker_failpoint_sweep },
        { "object_dedicated_repeated_write_failures_converge_to_last_object", test_object_dedicated_repeated_write_failures_converge_to_last_object },
#endif /* (1 == PAR_CFG_NVM_OBJECT_EN) && (1 == PAR_CFG_OBJECT_TYPES_ENABLED) && (PAR_CFG_NVM_OBJECT_STORE_MODE == PAR_CFG_NVM_OBJECT_STORE_DEDICATED) */
    };
#endif /* defined(PAR_HOST_TEST_SCHEMA_EVOLUTION_WRITE) */

    printf("PAR_HOST_NVM_PROFILE %s\n", PAR_HOST_TEST_PROFILE_NAME);
    result = par_host_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
    host_flash_remove_image();
    return result;
}
