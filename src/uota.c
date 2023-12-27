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

#define MAGIC_NUM                       0x756F7461
#define UOTA_TEMP_BUFFER_SIZE           (2048)

static uint8_t temp_buffer[UOTA_TEMP_BUFFER_SIZE];

struct uota_head
{
    char magic[4];                      /* magic: 'uota' */
    int image_size;                     /* 镜像大小, 包括头大小 */
    char image_version[8];              /* 镜像版本号 */
    char image_partition[8];            /* 镜像所在的分区名字 */
    unsigned char image_digest[32];     /* 整个镜像的指纹 md5 */
    unsigned char image_raw_digest[32]; /*  md5 */
    uint16_t image_digest_len;          /*  */   
    uint16_t image_raw_digest_len;
    digest_type_t digest_type;          /* 采用的摘要算法, 为了验证固件完整性, 必选项, hash(image.bin) */
    compress_type_t compress;           /* 采用的压缩算法, 非必选, compress(image.bin/image.diff) */
    int differential;                   /* 是否是差分包 */
    int raw_sizel;                      /* 镜像原始尺寸 */
};

static int header_size(void)
{
    rt_kprintf("%d.\n", sizeof(struct uota_head));
    return 0;
}
MSH_CMD_EXPORT(header_size, header_size);

static unsigned int read_u32(const unsigned char* buf)
{
    unsigned int va = buf[3];
    va = va << 8 | buf[2];
    va = va << 8 | buf[1];
    va = va << 8 | buf[0];
    return va;
}

static int uota_calca_digest(const struct fal_partition* partition, uota_digest_t digestor, int image_size, uint8_t *digest)
{
    int read_num = image_size / UOTA_TEMP_BUFFER_SIZE;
    int remain_size = image_size % UOTA_TEMP_BUFFER_SIZE;
    int index = 0, digest_len = 0;
    while (read_num)
    {
        fal_partition_read(partition, index, temp_buffer, UOTA_TEMP_BUFFER_SIZE);
        uota_digest_update(digestor, temp_buffer, UOTA_TEMP_BUFFER_SIZE);
        index += UOTA_TEMP_BUFFER_SIZE;
        read_num--;
    }
    fal_partition_read(partition, index, temp_buffer, remain_size);
    uota_digest_update(digestor, temp_buffer, remain_size);
    digest_len = uota_digest_finish(digestor, digest);
    uota_digest_destory(digestor);
    return digest_len;
}

int uota_image_check(const char* partition_name)
{
    int err = -1;
    struct uota_head head;
    unsigned char image_digest[32];
    unsigned char calc_image_digest[32];
    uota_digest_t digest_obj = NULL;
    const struct fal_partition* partition = fal_partition_find(partition_name);
    int header_size = sizeof(struct uota_head);

    int success = (
            (partition) &&
            (fal_partition_read(partition, 0, (uint8_t *)&head, header_size) == header_size) &&
            (read_u32(head.magic) == MAGIC_NUM) &&
            (digest_obj = uota_digest_create(head.digest_type))
        );

    if (success)
    {
        rt_memcpy(image_digest, head.image_digest, head.image_digest_len);
        rt_memset(head.image_digest, 0, sizeof(head.image_digest));

        /* check image */
        uota_calca_digest(partition, digest_obj, head.image_size, calc_image_digest);
        if (!rt_memcmp(calc_image_digest, image_digest, head.image_digest_len))
            err = 0;
        /* 解压后，继续验证签名 */
    }

    return err;
}

/* 获取镜像尺寸 */
int uota_get_image_size(const char* partition_name)
{
    /*  */

    return 0;
}

/* 获取镜像原始尺寸，解压后的固件大小 */
int uota_get_image_raw_size(const char* partition_name)
{
    /*  */

    return 0;
}

/* 开始搬运固件，将固件搬运到指定的位置 */
int uota_image_upgrade(const char* src_partition, const char* dst_partition)
{
    return 0;
}

