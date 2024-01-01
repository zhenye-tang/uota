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
#include "dfs_file.h"
#include <dfs_fs.h>
#include <unistd.h>

#define MAGIC_NUM                       0x756F7461
#define UOTA_TEMP_BUFFER_SIZE           (2048)

static uint8_t temp_buffer[UOTA_TEMP_BUFFER_SIZE];

struct uota_head
{
    char magic[4];                      /* magic: 'uota' */
    uint32_t image_size;                     /* 镜像大小, 包括头大小 */
    char image_version[8];              /* 镜像版本号 */
    char image_partition[8];            /* 镜像所在的分区名字 */
    unsigned char image_digest[32];     /* 整个镜像的指纹 md5 */
    uint32_t image_digest_len;          /*  */   
    digest_type_t digest_type;          /* 采用的摘要算法, 为了验证固件完整性, 必选项, hash(image.bin) */
    compress_type_t compress_type;           /* 采用的压缩算法, 非必选, compress(image.bin/image.diff) */
    uint32_t raw_size;                      /* 镜像原始尺寸 */
};

#define UOTA_HEAD_SIZE       (sizeof(struct uota_head))

int uota_image_check(const char* partition_name);
static unsigned int read_u32(const unsigned char* buf);
static int uota_decompress_test(const char* partition_name);

static int digest_test()
{
    static uint8_t buff[1024];
    uint8_t digest[16];
    struct uota_head head;
    int read_size;
    uota_digest_t digest_obj = uota_digest_create(0);
    int fd = open("/nor/yes.md5", O_RDONLY);

    while ((read_size = read(fd, buff, 1024)) > 0)
    {
        uota_digest_update(digest_obj, buff, read_size);
    }
    uota_digest_finish(digest_obj, digest);
    for (int i = 0; i < 16; i++)
    {
        rt_kprintf("%x ", digest[i]);
    }
    rt_kprintf("\n");

    close(fd);
}
MSH_CMD_EXPORT(digest_test, digest_test);

static int print_header(void)
{
    struct uota_head head;
    const struct fal_partition* partition = fal_partition_find("download");
    fal_partition_read(partition, 0, (uint8_t*)&head, sizeof(struct uota_head));

    int magic = read_u32(head.magic);

    if (magic == MAGIC_NUM)
        rt_kprintf("yes\n");

    rt_kprintf("magic = %s\n", head.magic);
    rt_kprintf("image_size = %d\n", head.image_size);
    rt_kprintf("image_version = %s\n", head.image_version);
    rt_kprintf("image_partition = %s\n", head.image_partition);
    rt_kprintf("image_digest_len = %d\n", head.image_digest_len);
    rt_kprintf("digest_type = %d\n", head.digest_type);
    rt_kprintf("compress_type = %d\n", head.compress_type);
    rt_kprintf("raw_size = %d\n", head.raw_size);

    for (int i = 0; i < head.image_digest_len; i++)
    {
        rt_kprintf("%x ", head.image_digest[i]);
    }
    rt_kprintf("\n");

    if(uota_image_check("download") == 0)
        uota_decompress_test("download");

    return 0;
}
MSH_CMD_EXPORT(print_header, print_header);

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
    int err = -1;
    struct uota_head head;
    unsigned char image_digest[32] = {0};
    unsigned char calc_image_digest[32] = {0};
    uint32_t image_size = 0, offect = UOTA_HEAD_SIZE;
    int loop_num = 0, reman_size = 0, digest_len = 0;

    if (fal_partition_read(partition, 0, (uint8_t*)&head, UOTA_HEAD_SIZE) == UOTA_HEAD_SIZE)
    {
        rt_memcpy(image_digest, head.image_digest, head.image_digest_len);
        rt_memset(head.image_digest, 0, sizeof(head.image_digest));
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
        if (loop_num == -1 && !reman_size && !rt_memcmp(image_digest, calc_image_digest, digest_len))
        {
            err = 0;
        }
    }

    return err;
}

int uota_image_check(const char* partition_name)
{
    int err = -1;
    struct uota_head head;
    uota_digest_t digest_obj = NULL;
    const struct fal_partition* partition = fal_partition_find(partition_name);

    int success = (
            (partition) &&
            (fal_partition_read(partition, 0, (uint8_t *)&head, UOTA_HEAD_SIZE) == UOTA_HEAD_SIZE) &&
            (read_u32(head.magic) == MAGIC_NUM) &&
            (digest_obj = uota_digest_create(head.digest_type))
        );

    err = success ? uota_image_digest_veryfi(partition, digest_obj) : err;

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

static int raw_file = -1;

#include "lz4.h"

static int check_lz4(void)
{
    static uint8_t block_buffer[18*1024];
    static uint8_t raw_buffer[18 * 1024];

    int fd = raw_file = open("/nor/rtt.rbl", O_RDONLY);
    int dst_fd = raw_file = open("rtt.bin", O_WRONLY | O_CREAT | O_TRUNC);
    read(fd, block_buffer, sizeof(struct uota_head));
    int compress_size = 0;
    LZ4_streamDecode_t lz4 = {0};

    for (;;)
    {
        int block_size = read(fd, &compress_size, 4);
        if (block_size <= 0)
        {
            break;
        }

        int read_size = read(fd, block_buffer, compress_size);
        int raw_size = LZ4_decompress_safe_continue(&lz4, block_buffer, raw_buffer, read_size, 18 * 1024);
        write(dst_fd, raw_buffer, raw_size);
    }
    close(fd);
    close(dst_fd);
    return 0;
}
MSH_CMD_EXPORT(check_lz4, check_lz4);

static int uota_decompress_callback(void* raw_data, int data_len)
{
    if (raw_file < 0)
        raw_file = open("raw_file.dll", O_WRONLY | O_CREAT | O_TRUNC);

    if (raw_file > 0 && data_len)
    {
        write(raw_file, raw_data, data_len);
    }

    if (data_len == 0)
    {
        close(raw_file);
        raw_file = -1;
        rt_kprintf("decompress success.\n");
    }
    return data_len;
}

static int uota_decompress_test(const char* partition_name)
{
    const struct fal_partition* partition = fal_partition_find(partition_name);
    int err = -1;
    struct uota_head head;
    unsigned char image_digest[32] = { 0 };
    unsigned char calc_image_digest[32] = { 0 };
    uint32_t image_size = 0, offect = UOTA_HEAD_SIZE;
    int loop_num = 0, reman_size = 0, digest_len = 0;
    uota_decompress_t decompressor = uota_decompress_create(UOTA_LZ4);
    fal_partition_read(partition, 0, &head, UOTA_HEAD_SIZE);

    uota_decompress_set_callback(decompressor, uota_decompress_callback);
    uota_decompress_start(decompressor, partition_name, UOTA_HEAD_SIZE, head.image_size - UOTA_HEAD_SIZE);
    uota_decompress_destory(decompressor);
    return 0;
}


/* 开始搬运固件，将固件搬运到指定的位置 */
int uota_image_upgrade(const char* src_partition, const char* dst_partition)
{




    return 0;
}

