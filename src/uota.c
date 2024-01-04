/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-12-26     tzy          first implementation
 */

#include "uota.h"
#include "fal.h"
#include "uota_decompress.h"
#include "uota_digest.h"
#include <string.h>

#define MAGIC_NUM                       0x756F7461
#define UOTA_TEMP_BUFFER_SIZE           (2048)
#define UOTA_HEAD_SIZE                  (sizeof(struct uota_head))

static uint8_t temp_buffer[UOTA_TEMP_BUFFER_SIZE];

struct uota_head
{
    char magic[4];                      /* magic */
    uint32_t image_size;                /* image size */
    char image_version[8];              /* image version */
    char image_partition[8];            /* partition name */
    unsigned char image_digest[32];     /* images hash */
    uint32_t image_digest_len;          /* images hash len */
    digest_type_t digest_type;          /* digest algo */
    compress_type_t compress_type;      /* compress algo */
    uint32_t raw_size;                  /* image raw size */
};

struct uota_upgrade
{
    const struct fal_partition* src;
    const struct fal_partition* dst;
    int offect;
};

static unsigned int read_u32(const unsigned char* buf)
{
    unsigned int va = buf[0];
    va = va << 8 | buf[1];
    va = va << 8 | buf[2];
    va = va << 8 | buf[3];
    return va;
}

static int uota_image_digest_veryfi(const struct fal_partition* partition, uota_digest_t digestor)
{
    int err = -UOTA_ERROR;
    struct uota_head head;
    unsigned char image_digest[32] = {0};
    unsigned char calc_image_digest[32] = {0};
    uint32_t image_size = 0, offect = UOTA_HEAD_SIZE;
    int loop_num = 0, reman_size = 0, digest_len = 0;

    if (fal_partition_read(partition, 0, (uint8_t*)&head, UOTA_HEAD_SIZE) == UOTA_HEAD_SIZE)
    {
        memcpy(image_digest, head.image_digest, head.image_digest_len);
        memset(head.image_digest, 0, sizeof(head.image_digest));
        image_size = head.image_size;
    }

    if (image_size && image_size <= partition->len)
    {
        image_size -= UOTA_HEAD_SIZE;
        loop_num = image_size / UOTA_TEMP_BUFFER_SIZE;
        reman_size = image_size % UOTA_TEMP_BUFFER_SIZE;

        uota_digest_update(digestor, (void*)&head, UOTA_HEAD_SIZE);
        while (loop_num--)
        {
            if (fal_partition_read(partition, offect, temp_buffer, UOTA_TEMP_BUFFER_SIZE) == UOTA_TEMP_BUFFER_SIZE)
            {
                uota_digest_update(digestor, temp_buffer, UOTA_TEMP_BUFFER_SIZE);
                offect += UOTA_TEMP_BUFFER_SIZE;
            }
            else
            {
                break;
            }
        }

        if (loop_num == -1 && reman_size && fal_partition_read(partition, offect, temp_buffer, reman_size) == reman_size)
        {
            uota_digest_update(digestor, temp_buffer, reman_size);
            reman_size = 0;
        }

        digest_len = uota_digest_finish(digestor, calc_image_digest);
        if (loop_num == -1 && !reman_size && !memcmp(image_digest, calc_image_digest, digest_len))
        {
            err = UOTA_OK;
        }
    }

    return err;
}

int uota_image_check(const char* partition_name)
{
    struct uota_head head;
    uota_digest_t digest_obj = NULL;
    const struct fal_partition* partition = fal_partition_find(partition_name);

    int success = (
            (partition) &&
            (fal_partition_read(partition, 0, (uint8_t *)&head, UOTA_HEAD_SIZE) == UOTA_HEAD_SIZE) &&
            (read_u32(head.magic) == MAGIC_NUM) &&
            (digest_obj = uota_digest_create(head.digest_type))
        );

    return success ? uota_image_digest_veryfi(partition, digest_obj) : -UOTA_ERROR;
}

int uota_get_image_size(const char* partition_name)
{
    int image_size = -1;
    struct uota_head head;
    const struct fal_partition* partition = fal_partition_find(partition_name);
    if (partition && (fal_partition_read(partition, 0, (uint8_t*)&head, UOTA_HEAD_SIZE) == UOTA_HEAD_SIZE))
        image_size = head.image_size;

    return image_size;
}

int uota_get_image_raw_size(const char* partition_name)
{
    int image_raw_size = -1;
    struct uota_head head;
    const struct fal_partition* partition = fal_partition_find(partition_name);
    if (partition && (fal_partition_read(partition, 0, (uint8_t*)&head, UOTA_HEAD_SIZE) == UOTA_HEAD_SIZE))
        image_raw_size = head.raw_size;

    return image_raw_size;
}

static int uota_decompress_callback(void* raw_data, int data_len, void *userdata)
{
    struct uota_upgrade* upgrade = (struct uota_upgrade *)userdata;
    if (raw_data && data_len > 0)
    {
        if (fal_partition_write(upgrade->dst, upgrade->offect, raw_data, data_len) == data_len)
        {
            upgrade->offect += data_len;
        }
    }

    return data_len;
}

static int uota_erase_partition(const struct fal_partition* part, int image_size)
{
    return fal_partition_erase(part, 0, image_size);
}

int uota_image_upgrade(const char* src_partition, const char* dst_partition)
{
    const struct fal_partition* dst_part = NULL, *src_part = NULL;
    uota_decompress_t decompressor = NULL;
    struct uota_head image_header;
    int success = (
            (uota_image_check(src_partition) == 0) &&
            (src_part = fal_partition_find(src_partition)) &&
            (dst_part = fal_partition_find(dst_partition)) &&
            (fal_partition_read(src_part, 0, (uint8_t *)&image_header, UOTA_HEAD_SIZE) == UOTA_HEAD_SIZE) &&
            (dst_part->len >= image_header.raw_size) &&
            (decompressor = uota_decompress_create(image_header.compress_type)) &&
            (uota_erase_partition(dst_part, image_header.raw_size) >= 0)
        );

    struct uota_upgrade upgrade = { src_part , dst_part, 0};

    if (success)
    {
        uota_decompress_set_callback(decompressor, uota_decompress_callback, &upgrade);
        uota_decompress_start(decompressor, src_partition, UOTA_HEAD_SIZE, image_header.image_size - UOTA_HEAD_SIZE);
        uota_decompress_destory(decompressor);
    }

    return success ? UOTA_OK : UOTA_ERROR;
}
