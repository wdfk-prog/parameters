/**
 * @file fal_cfg.h
 * @brief Minimal FAL configuration for CI package backend compile smoke.
 */
#ifndef AUTOGEN_PM_CI_FAL_CFG_H
#define AUTOGEN_PM_CI_FAL_CFG_H

/** @brief Flash device name used by the CI FAL compile profile. */
#define AUTOGEN_PM_CI_FAL_FLASH_NAME "autogen_pm_ci_flash"

/** @brief FAL partition name bound by the CI flash-ee profile. */
#define AUTOGEN_PM_CI_FAL_PARTITION_NAME "autogen_pm"

/** @brief CI FAL flash region size in bytes. */
#define AUTOGEN_PM_CI_FAL_FLASH_SIZE (4096U)

/** @brief CI FAL erase block size in bytes. */
#define AUTOGEN_PM_CI_FAL_BLOCK_SIZE (64U)

extern const struct fal_flash_dev g_autogen_pm_ci_fal_flash;

/** @brief Flash-device table consumed by the RT-Thread FAL package. */
#define FAL_FLASH_DEV_TABLE                  \
{                                            \
    &g_autogen_pm_ci_fal_flash,              \
}

#ifdef FAL_PART_HAS_TABLE_CFG
/**
 * @brief Declare one CI FAL partition entry.
 * @param _name Partition name string.
 * @param _offset Partition start offset within the CI flash device.
 * @param _len Partition length in bytes.
 */
#define AUTOGEN_PM_CI_FAL_PART(_name, _offset, _len) \
    {                                                \
        FAL_PART_MAGIC_WORD,                         \
        _name,                                       \
        AUTOGEN_PM_CI_FAL_FLASH_NAME,                \
        _offset,                                     \
        _len,                                        \
        0                                            \
    }

/** @brief Partition table consumed by the RT-Thread FAL package. */
#define FAL_PART_TABLE                                                          \
{                                                                               \
    AUTOGEN_PM_CI_FAL_PART(AUTOGEN_PM_CI_FAL_PARTITION_NAME, 0x00000000,        \
                           AUTOGEN_PM_CI_FAL_FLASH_SIZE),                      \
}
#endif /* defined(FAL_PART_HAS_TABLE_CFG) */

#define FAL_PART_TABLE_FLASH_DEV_NAME AUTOGEN_PM_CI_FAL_FLASH_NAME
#define FAL_PART_TABLE_END_OFFSET     AUTOGEN_PM_CI_FAL_FLASH_SIZE

#endif /* !defined(AUTOGEN_PM_CI_FAL_CFG_H) */
