#include "uota_decompress.h"
#include "lz4.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <fal.h>

#define LZ4_RAW_BUFFER_SIZE             (8*1024)
#define LZ4_COMPRESS_BUFFER_SIZE        LZ4_COMPRESSBOUND(LZ4_RAW_BUFFER_SIZE)

struct uota_lz4
{
    struct uota_decompress parent;
    LZ4_streamDecode_t ctx;
    char *raw_buffer[2];
    char *compress_buffer;
};

static struct uota_lz4 lz4;

static uota_decompress_t lz4_ctx_create(void)
{
    memset(&lz4.ctx, 0, sizeof(LZ4_streamDecode_t));
    LZ4_setStreamDecode(&lz4.ctx, NULL, 0);
    lz4.raw_buffer[0] = malloc(LZ4_RAW_BUFFER_SIZE * 2);
    lz4.raw_buffer[1] = lz4.raw_buffer[0] + LZ4_RAW_BUFFER_SIZE;
    lz4.compress_buffer = malloc(LZ4_COMPRESSBOUND(LZ4_RAW_BUFFER_SIZE));
    return &lz4.parent;
}

static int lz4_start(uota_decompress_t dec, const char* partion_name, int offect, int total_size)
{
    struct uota_lz4* lz4_obj = (struct uota_lz4*)dec;
    const struct fal_partition* partition = fal_partition_find(partion_name);
    int decompress_block_size = 0, read_size = 0, buf_index = 0;

    if (partition)
    {
        do
        {
            read_size = fal_partition_read(partition, offect, (uint8_t*)&decompress_block_size, sizeof(int));
            if (read_size == sizeof(int))
            {
                offect += read_size;
                read_size = fal_partition_read(partition, offect, lz4_obj->compress_buffer, decompress_block_size);
                if(read_size == decompress_block_size)
                {
                    char* dec_ptr = lz4_obj->raw_buffer[buf_index];
                    int raw_size = LZ4_decompress_safe_continue(&lz4_obj->ctx, lz4_obj->compress_buffer, dec_ptr, read_size, LZ4_RAW_BUFFER_SIZE);

                    if (lz4_obj->parent.callback)
                        lz4_obj->parent.callback(dec_ptr, raw_size);
                }
                else
                {
                    break;
                }
                offect += read_size;
                read_size += sizeof(int);
            }
            else
            {
                break;
            }
            total_size -= read_size;
            buf_index = (buf_index + 1) % 2;
        } while (total_size);

        if (lz4_obj->parent.callback)
            lz4_obj->parent.callback(NULL, 0);
    }

    return total_size ? -1 : 0;
}

static int lz4_destory(uota_decompress_t dec)
{
    struct uota_lz4* lz4_obj = (struct uota_lz4*)dec;
    memset(&lz4_obj->ctx, 0, sizeof(LZ4_streamDecode_t));
    free(lz4_obj->raw_buffer[0]);
    free(lz4_obj->compress_buffer);

    return 0;
}

static struct decompress_ops ops = {
    .create = lz4_ctx_create,
    .start = lz4_start,
    .destory = lz4_destory
};

int uota_lz4_init(void)
{
    lz4.parent.type = UOTA_LZ4;
    lz4.parent.ops = &ops;
    uota_decompress_register(&lz4.parent);
    return 0;
}
INIT_APP_EXPORT(uota_lz4_init);