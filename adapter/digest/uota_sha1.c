#include "uota_digest.h"
#include "tiny_sha1.h"
#include "rtthread.h"

struct uota_sha1
{
    struct uota_digest parent;
    tiny_sha1_context ctx;
};

static struct uota_sha1 sha1;

static uota_digest_t sha1_ctx_create(void)
{
    tiny_sha1_starts(&sha1.ctx);
    return &sha1.parent;
}

static int sha1_update(uota_digest_t digestor, void* data, int data_len)
{
    struct uota_sha1* sha1_obj = (struct uota_sha1*)digestor;
    tiny_sha1_update(&sha1_obj->ctx, (unsigned char*)data, data_len);
    return 0;
}

static int sha1_finish(uota_digest_t digestor, void* hash)
{
    struct uota_sha1* sha1_obj = (struct uota_sha1*)digestor;
    tiny_sha1_finish(&sha1_obj->ctx, (unsigned char*)hash);
    return 16;
}

static int sha1_destory(uota_digest_t digestor)
{
    struct uota_sha1* sha1_obj = (struct uota_sha1*)digestor;
    tiny_sha1_starts(&sha1.ctx);
    return 0;
}

static struct digest_ops ops = {
    .create = sha1_ctx_create,
    .update = sha1_update,
    .finish = sha1_finish,
    .destory = sha1_destory
};

int uota_sha1_init(void)
{
    sha1.parent.type = UOTA_SHA1;
    sha1.parent.ops = &ops;
    uota_digest_register(&sha1.parent);
    return 0;
}
INIT_APP_EXPORT(uota_sha1_init);
