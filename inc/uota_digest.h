/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-12-26     tzy          first implementation
 */

#ifndef __UOTA_DIGEST_H__
#define __UOTA_DIGEST_H__

typedef enum digest_type
{
    UOTA_MD5,
    UOTA_SHA1,
    UOTA_SHA256,
}digest_type_t;

typedef struct uota_digest* uota_digest_t;

struct digest_ops
{
    uota_digest_t (*create)(void);
    int (*update)(uota_digest_t digestor, void *data, int data_len);
    int (*finish)(uota_digest_t digestor, void* hash);
    int (*destory)(uota_digest_t digestor);
};

struct uota_digest
{
    struct digest_ops* ops;
    digest_type_t type;
};

uota_digest_t uota_digest_create(digest_type_t type);
int uota_digest_update(uota_digest_t digestor, void* data, int data_len);
int uota_digest_finish(uota_digest_t digestor, void* hash);
int uota_digest_destory(uota_digest_t digestor);
int uota_digest_register(uota_digest_t digestor);
int uota_digest_unregister(uota_digest_t digestor);

#endif //__UOTA_DIGEST_H__