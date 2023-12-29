#include "lz4.h"
#include <stdint.h>
#include <dfs_file.h>
#include <unistd.h>

/* 使用 lz4.h 中的初级 api 完成文件压缩 */

#define COMPRESS_BUFFER_SIZE            (32*1024)

struct lz4_compress
{
    int src;
    int dst;
    int src_file_size;
    LZ4_stream_t lz4_ctx;
    LZ4_streamDecode_t lz4_decode;
    uint8_t compress_buffer[COMPRESS_BUFFER_SIZE];
    uint8_t raw_buffer[COMPRESS_BUFFER_SIZE];
};

static struct lz4_compress compressor;

static int lz4_compress_start(const char *src, const char *dst)
{
    int read_size = 0;
    int comp_size = 0;
    compressor.src = open(src, O_RDONLY);
    compressor.dst = open(dst, O_WRONLY | O_CREAT | O_TRUNC);
    compressor.src_file_size = lseek(compressor.src, 0, SEEK_END);
    lseek(compressor.src, 0, SEEK_SET);
    do
    {
        read_size = read(compressor.src, compressor.raw_buffer, COMPRESS_BUFFER_SIZE);
        if (read_size)
        {
            comp_size = LZ4_compress_fast_continue(&compressor.lz4_ctx, compressor.raw_buffer, compressor.compress_buffer, read_size, COMPRESS_BUFFER_SIZE, 0);
            write(compressor.dst, &comp_size, sizeof(comp_size));
            write(compressor.dst, compressor.compress_buffer, comp_size);
            if (read_size != COMPRESS_BUFFER_SIZE)
            {
                rt_kprintf("yes!!\n");
            }
        }
        compressor.src_file_size -= read_size;
    } while (compressor.src_file_size);
    close(compressor.src);
    close(compressor.dst);
    return 0;
}

static int lz4_decompress_start(const char* src, const char* dst)
{
    int read_size = 0;
    int decomp_size = 0;
    int block_size = 0;
    compressor.src = open(src, O_RDONLY);
    compressor.dst = open(dst, O_WRONLY | O_CREAT | O_TRUNC);
    compressor.src_file_size = lseek(compressor.src, 0, SEEK_END);
    lseek(compressor.src, 0, SEEK_SET);
    do
    {
        decomp_size = read(compressor.src, &block_size, sizeof(block_size));
        if (decomp_size)
        {
            read_size = read(compressor.src, compressor.compress_buffer, block_size);
        }

        if (read_size)
        {
            decomp_size = LZ4_decompress_safe_continue(&compressor.lz4_decode, compressor.compress_buffer, compressor.raw_buffer, read_size, COMPRESS_BUFFER_SIZE);
            write(compressor.dst, compressor.raw_buffer, decomp_size);
        }
        compressor.src_file_size -= read_size + sizeof(block_size);

    } while (compressor.src_file_size);

    close(compressor.src);
    close(compressor.dst);
    return 0;
}

static int lz4_msh(int argc, char* argv[])
{
    /* lz4 -c source dist */
    if (argc < 4)
        return -1;

    if (rt_strcmp("-c", argv[1]) == 0)
    {
        lz4_compress_start(argv[2], argv[3]);
    }
    else if (rt_strcmp("-d", argv[1]) == 0)
    {
        lz4_decompress_start(argv[2], argv[3]);
    }

    return 0;
}
MSH_CMD_EXPORT(lz4_msh, lz4_msh);