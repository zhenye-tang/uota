/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-12-29     tzy          first implementation
 */

#include "uota_decompress.h"
#include "zlib.h"
#include <stdlib.h>
#include <fal.h>

#define ZLIB_RAW_BUFFER_SIZE             (16*1024)

struct uota_zlib
{
    struct uota_decompress parent;
    z_stream ctx;
    char *raw_buf;
    char *compress_buf;
};

static struct uota_zlib zlib;

static uota_decompress_t zlib_ctx_create(void)
{
    zlib.ctx.zalloc = Z_NULL;
    zlib.ctx.zfree = Z_NULL;
    zlib.ctx.opaque = Z_NULL;
    zlib.ctx.avail_in = 0;
    zlib.ctx.next_in = Z_NULL;
    zlib.raw_buf = malloc(ZLIB_RAW_BUFFER_SIZE);
    zlib.compress_buf = malloc(ZLIB_RAW_BUFFER_SIZE);
    inflateInit(&zlib.ctx);
    return &zlib.parent;
}

static int zlib_start(uota_decompress_t dec, const char* partion_name, int offect, int size)
{
    int ret = -1, read_size = 0;
    unsigned have;
    const struct fal_partition* partition = fal_partition_find(partion_name);
    struct uota_zlib* zib_obj = (struct uota_zlib*)dec;

    do {
        read_size = fal_partition_read(partition, offect, zib_obj->compress_buf, ZLIB_RAW_BUFFER_SIZE);
        if (read_size <= 0)
        {
            (void)inflateEnd(&zib_obj->ctx);
            break;
        }

        zib_obj->ctx.avail_in = read_size;
        zib_obj->ctx.next_in = zib_obj->compress_buf;
        do {
            zib_obj->ctx.avail_out = ZLIB_RAW_BUFFER_SIZE;
            zib_obj->ctx.next_out = zib_obj->raw_buf;
            ret = inflate(&zib_obj->ctx, Z_NO_FLUSH);
            switch (ret)
            {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&zib_obj->ctx);
                goto __exit;
            }
            have = ZLIB_RAW_BUFFER_SIZE - zib_obj->ctx.avail_out;
            /* call user function */
            if (zib_obj->parent.callback)
                zib_obj->parent.callback(zib_obj->raw_buf, have, zib_obj->parent.userdata);
        } while (zib_obj->ctx.avail_out == 0);
        offect += read_size;
    } while (ret != Z_STREAM_END);

    if (zib_obj->parent.callback)
        zib_obj->parent.callback(NULL, 0, zib_obj->parent.userdata);

__exit:
    (void)inflateEnd(&zib_obj->ctx);
    return ret;
}

static int zlib_destory(uota_decompress_t dec)
{
    struct uota_zlib* zib_obj = (struct uota_zlib*)dec;
    inflateInit(&zib_obj->ctx);
    free(zib_obj->compress_buf);
    free(zib_obj->raw_buf);

    return 0;
}

static struct decompress_ops ops = {
    .create = zlib_ctx_create,
    .start = zlib_start,
    .destory = zlib_destory
};

int uota_zlib_init(void)
{
    zlib.parent.type = UOTA_ZIP;
    zlib.parent.ops = &ops;
    uota_decompress_register(&zlib.parent);
    return 0;
}