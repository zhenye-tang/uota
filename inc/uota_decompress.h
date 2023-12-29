﻿#ifndef __UOTA_DECOMPRESS_H__
#define __UOTA_DECOMPRESS_H__

typedef enum compress_type
{
    UOTA_NONE,
    UOTA_LZ4,
    UOTA_ZIP,
    UOTA_FASTLZ
}compress_type_t;

typedef struct uota_decompress* uota_decompress_t;
typedef int (*decompress_callback)(void* raw_data, int data_len);

struct decompress_ops
{
    uota_decompress_t (*create)(void);
    int (*update)(uota_decompress_t dec, void *src_data, int data_len);
    int (*start)(uota_decompress_t dec, const char* partion_name, int offect, int size);
    int (*destory)(uota_decompress_t dec);
};

struct uota_decompress
{
    struct decompress_ops* ops;
    compress_type_t type;
    decompress_callback callback;
};

uota_decompress_t uota_decompress_create(compress_type_t type);
int uota_decompress_start(uota_decompress_t dec, const char *partion_name, int offect, int size);
int uota_decompress_destory(uota_decompress_t dec);

void uota_decompress_set_callback(uota_decompress_t dec, decompress_callback callback);
int uota_decompress_register(uota_decompress_t dec);
int uota_decompress_unregister(uota_decompress_t dec);



#endif // __UOTA_DECOMPRESS_H__
