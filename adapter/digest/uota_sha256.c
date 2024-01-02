#include "uota_digest.h"
#include "tiny_sha2.h"
#include "rtthread.h"

struct uota_sha2
{
    struct uota_digest parent;
    tiny_sha2_context ctx;
};

static struct uota_sha2 sha2;

static uota_digest_t sha2_ctx_create(void)
{
    tiny_sha2_starts(&sha2.ctx, 0);
    return &sha2.parent;
}

static int sha2_update(uota_digest_t digestor, void* data, int data_len)
{
    struct uota_sha2* sha2_obj = (struct uota_sha2*)digestor;
    tiny_sha2_update(&sha2_obj->ctx, (unsigned char*)data, data_len);
    return 0;
}

static int sha2_finish(uota_digest_t digestor, void* hash)
{
    struct uota_sha2* sha2_obj = (struct uota_sha2*)digestor;
    tiny_sha2_finish(&sha2_obj->ctx, (unsigned char*)hash);
    return 16;
}

static int sha2_destory(uota_digest_t digestor)
{
    struct uota_sha2* sha2_obj = (struct uota_sha2*)digestor;
    tiny_sha2_starts(&sha2.ctx, 0);
    return 0;
}

static struct digest_ops ops = {
    .create = sha2_ctx_create,
    .update = sha2_update,
    .finish = sha2_finish,
    .destory = sha2_destory
};

int uota_sha2_init()
{
    sha2.parent.type = UOTA_SHA256;
    sha2.parent.ops = &ops;
    uota_digest_register(&sha2.parent);
    return 0;
}
INIT_APP_EXPORT(uota_sha2_init);
