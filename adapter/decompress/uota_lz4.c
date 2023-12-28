#include "uota_decompress.h"
#include "lz4.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <fal.h>

#define LZ4_RAW_BUFFER_SIZE             (16*1026)

struct uota_lz4
{
    struct uota_decompress parent;
    LZ4_streamDecode_t ctx;
    uint8_t *raw_buffer;
    uint8_t *compress_buffer;
};

static struct uota_lz4 lz4;

static uota_decompress_t lz4_ctx_create(void)
{
    memset(&lz4.ctx, 0, sizeof(LZ4_streamDecode_t));
    lz4.raw_buffer = malloc(LZ4_RAW_BUFFER_SIZE);
    lz4.compress_buffer = malloc(LZ4_RAW_BUFFER_SIZE);
    return &lz4.parent;
}

static int lz4_start(uota_decompress_t dec, const char* partion_name, int offect, int size)
{
    struct uota_lz4* lz4_obj = (struct uota_lz4*)dec;
    fal_partition_t partition = fal_partition_find(partion_name);
    int total_size = size;
    int decompress_block_size;
    int read_size = 0;
    if (partition)
    {
        do
        {
            /* read compress size */
            read_size = fal_partition_read(partition, offect, &decompress_block_size, sizeof(int));
            if (read_size)
            {
                /* read a block size */
                offect += read_size;
                read_size = fal_partition_read(partition, offect, lz4_obj->compress_buffer, decompress_block_size);

                int raw_size = LZ4_decompress_safe_continue(&lz4_obj->ctx, lz4_obj->compress_buffer, lz4_obj->raw_buffer, decompress_block_size, LZ4_RAW_BUFFER_SIZE);
                if (lz4_obj->parent.callback)
                    lz4_obj->parent.callback(lz4_obj->raw_buffer, raw_size);
            }
            total_size -= read_size + sizeof(int);
        } while (total_size);
    }

    return 0;
}

static int lz4_finish(uota_decompress_t dec)
{
    (void)dec;
    return 0;
}

static int lz4_destory(uota_decompress_t dec)
{
    struct uota_lz4* lz4_obj = (struct uota_lz4*)dec;
    memset(&lz4_obj->ctx, 0, sizeof(LZ4_streamDecode_t));
    free(lz4_obj->raw_buffer);

    return 0;
}

static struct decompress_ops ops = {
    .create = lz4_ctx_create,
    .start = lz4_start,
    .destory = lz4_destory
};

int uota_lz4_init()
{
    lz4.parent.type = UOTA_LZ4;
    lz4.parent.ops = &ops;
    uota_decompress_register(&lz4.parent);
    return 0;
}