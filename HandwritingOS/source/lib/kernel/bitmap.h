#ifndef __LIB_KERNEL_BITMAP_H
#define __LIB_KERNEL_BITMAP_H

#include "global.h"

#define BITMAP_MASK 1

typedef struct bitmap
{
    uint32_t bitmap_bytes_len;
    uint8_t* bits; // 遍历位图整体上以字节为单位，细节上以位为单位。
} bitmap;

void bitmap_init(bitmap* btmp);

bool bitmap_scan_test(bitmap* btmp, uint32_t bit_index);

int bitmap_scan(bitmap* btmp, uint32_t count);

void bitmap_set(bitmap* btmp, uint32_t bit_index, int8_t value);

#endif // __LIB_KERNEL_BITMAP_H
