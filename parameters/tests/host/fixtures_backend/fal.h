/**
 * @file fal.h
 * @brief Minimal FAL declarations for host backend adapter smoke tests.
 */
#ifndef PAR_TEST_FAL_H
#define PAR_TEST_FAL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/** @brief Minimal FAL flash device stub. */
struct fal_flash_dev
{
    long len;       /**< Device size in bytes. */
    int blk_size;   /**< Erase block size in bytes. */
};

/** @brief Minimal FAL partition stub. */
struct fal_partition
{
    const char *name;        /**< Partition name. */
    const char *flash_name;  /**< Owning flash device name. */
    long offset;             /**< Partition offset. */
    long len;                /**< Partition size in bytes. */
};

const struct fal_partition *fal_partition_find(const char *name);
const struct fal_flash_dev *fal_flash_device_find(const char *name);
int fal_partition_read(const struct fal_partition *part, long offset, unsigned char *buf, size_t size);
int fal_partition_write(const struct fal_partition *part, long offset, const unsigned char *buf, size_t size);
int fal_partition_erase(const struct fal_partition *part, long offset, size_t size);

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* !defined(PAR_TEST_FAL_H) */
