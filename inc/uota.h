/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-12-26     tzy          first implementation
 */

#ifndef __UOTA_H__
#define __UOTA_H__

enum error_code
{
    UOTA_OK,
    UOTA_ERROR
};

int uota_image_check(const char *partition_name);
int uota_get_image_size(const char* partition_name);
int uota_image_upgrade(const char *src_partition, const char* dst_partition);

#endif;