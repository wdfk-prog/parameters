/**
 * @file fal.h
 * @brief Minimal FAL declarations for CI package backend compile smoke.
 */
#ifndef AUTOGEN_PM_CI_FAL_H
#define AUTOGEN_PM_CI_FAL_H

#include <stddef.h>
#include <stdint.h>

/** @brief RT-Thread 32-bit unsigned integer stub. */
typedef uint32_t rt_uint32_t;
/** @brief RT-Thread 8-bit unsigned integer stub. */
typedef uint8_t rt_uint8_t;
/** @brief RT-Thread size stub. */
typedef size_t rt_size_t;

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/** @brief Minimal FAL flash-device stub used by CI package builds. */
struct fal_flash_dev
{
    long len;       /**< Device size in bytes. */
    int blk_size;   /**< Erase block size in bytes. */
};

/** @brief Minimal FAL partition stub used by CI package builds. */
struct fal_partition
{
    const char *name;        /**< Partition name. */
    const char *flash_name;  /**< Owning flash device name. */
    long offset;             /**< Partition offset in the flash device. */
    long len;                /**< Partition size in bytes. */
};

const struct fal_partition *fal_partition_find(const char *name);
const struct fal_flash_dev *fal_flash_device_find(const char *name);
int fal_partition_read(const struct fal_partition *part,
                       rt_uint32_t addr,
                       rt_uint8_t *buf,
                       rt_size_t size);
int fal_partition_write(const struct fal_partition *part,
                        rt_uint32_t addr,
                        const rt_uint8_t *buf,
                        rt_size_t size);
int fal_partition_erase(const struct fal_partition *part,
                        rt_uint32_t addr,
                        rt_size_t size);

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* !defined(AUTOGEN_PM_CI_FAL_H) */
