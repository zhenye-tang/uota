/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-12-26     tzy          first implementation
 */

#include <stddef.h>
#include "uota_digest.h"

#define DIGESTOR_SIZE       (8)
static uota_digest_t digest_obj[DIGESTOR_SIZE] = { 0 };

uota_digest_t uota_digest_create(digest_type_t type)
{
    for (int i = 0; i < DIGESTOR_SIZE; i++)
    {
        if (digest_obj[i] && digest_obj[i]->type)
            return digest_obj[i];
    }
    return NULL;
}

int uota_digest_update(uota_digest_t digestor, void* data, int data_len)
{
    return digestor->ops->update(digestor, data, data_len);
}

int uota_digest_finish(uota_digest_t digestor, void* hash)
{
    return digestor->ops->finish(digestor, hash);
}

int uota_digest_destory(uota_digest_t digestor)
{
    return digestor->ops->destory(digestor);
}

int uota_digest_register(uota_digest_t digestor)
{
    for (int i = 0; i < DIGESTOR_SIZE; i++)
    {
        if (!digest_obj[i])
        {
            digest_obj[i] = digestor;
            return i;
        }
    }

    return -1;
}

int uota_digest_unregister(uota_digest_t digestor)
{
    for (int i = 0; i < DIGESTOR_SIZE; i++)
    {
        if (digest_obj[i] == digestor)
        {
            digest_obj[i] = NULL;
            return i;
        }
    }

    return -1;
}


