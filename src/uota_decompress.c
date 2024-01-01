/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-12-26     tzy          first implementation
 */

#include <stddef.h>
#include "uota_decompress.h"

#define DECOMPRESSOR_SIZE       (8)
static uota_decompress_t decompress_obj[DECOMPRESSOR_SIZE];

uota_decompress_t uota_decompress_create(compress_type_t type)
{
    for (int i = 0; i < DECOMPRESSOR_SIZE; i++)
    {
        if (decompress_obj[i] && decompress_obj[i]->type == type)
            return decompress_obj[i]->ops->create();
    }
    return NULL;
}

int uota_decompress_start(uota_decompress_t dec, const char* partion_name, int offect, int size)
{
    return dec->ops->start(dec, partion_name, offect, size);
}

int uota_decompress_destory(uota_decompress_t dec)
{
    return dec->ops->destory(dec);
}

void uota_decompress_set_callback(uota_decompress_t dec, decompress_callback callback)
{
    dec->callback = callback;
}

int uota_decompress_register(uota_decompress_t dec)
{
    for (int i = 0; i < DECOMPRESSOR_SIZE; i++)
    {
        if (!decompress_obj[i])
        {
            decompress_obj[i] = dec;
            return i;
        }
    }

    return -1;
}

int uota_decompress_unregister(uota_decompress_t dec)
{
    for (int i = 0; i < DECOMPRESSOR_SIZE; i++)
    {
        if (decompress_obj[i] == dec)
        {
            decompress_obj[i] = NULL;
            return i;
        }
    }

    return -1;
}
