#include "uota_digest.h"
#include "tiny_md5.h"
#include "rtthread.h"

struct uota_md5
{
    struct uota_digest parent;
    tiny_md5_context ctx;
};

static struct uota_md5 md5;

static uota_digest_t md5_ctx_create(void)
{
    tiny_md5_starts(&md5.ctx);
    return &md5.parent;
}

static int md5_update(uota_digest_t digestor, void* data, int data_len)
{
    struct uota_md5 *md5_obj = (struct uota_md5 *)digestor;
    tiny_md5_update(&md5_obj->ctx, (unsigned char*)data, data_len);
    return 0;
}

static int md5_finish(uota_digest_t digestor, void* hash)
{
    struct uota_md5* md5_obj = (struct uota_md5*)digestor;
    tiny_md5_finish(&md5_obj->ctx, (unsigned char *)hash);
    return 16;
}

static int md5_destory(uota_digest_t digestor)
{
    struct uota_md5* md5_obj = (struct uota_md5*)digestor;
    tiny_md5_starts(&md5.ctx);
    return 0;
}

static struct digest_ops ops = {
    .create = md5_ctx_create,
    .update = md5_update,
    .finish = md5_finish,
    .destory = md5_destory
};

int uota_md5_init()
{
    md5.parent.type = UOTA_MD5;
    md5.parent.ops = &ops;
    uota_digest_register(&md5.parent);
    return 0;
}
INIT_APP_EXPORT(uota_md5_init, uota_md5_init);
