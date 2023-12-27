#include "uota_decompress.h"
#include "lz4.h"
#include <string.h>
#include <stdint.h>

#define LZ4_RAW_BUFFER_SIZE             (2048)

struct uota_lz4
{
    struct uota_decompress parent;
    LZ4_streamDecode_t ctx;
    uint8_t raw_buffer[LZ4_RAW_BUFFER_SIZE];
};

static struct uota_lz4 lz4;

static uota_decompress_t lz4_ctx_create(void)
{
    memset(&lz4.ctx, 0, sizeof(LZ4_streamDecode_t));
    LZ4_setStreamDecode(&lz4.ctx, 0, 0);
    return &lz4.parent;
}

static int lz4_update(uota_decompress_t dec, void* data, int data_len)
{
    struct uota_lz4* lz4_obj = (struct uota_lz4*)dec;
    int package_size = LZ4_decompress_safe_continue(&lz4_obj->ctx, data, lz4_obj->raw_buffer, data_len, LZ4_RAW_BUFFER_SIZE);

    if (package_size && lz4_obj->parent.callback)
    {
        lz4_obj->parent.callback(lz4_obj->raw_buffer, package_size);
    }
    else if(package_size < 0)
    {
        /* TODO: 异常处理 */
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
    return 0;
}

static struct decompress_ops ops = {
    .create = lz4_ctx_create,
    .update = lz4_update,
    .finish = lz4_finish,
    .destory = lz4_destory
};

int uota_lz4_init()
{
    lz4.parent.type = UOTA_LZ4;
    lz4.parent.ops = &ops;
    uota_decompress_register(&lz4.parent);
    return 0;
}