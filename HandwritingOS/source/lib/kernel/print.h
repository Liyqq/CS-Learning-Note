#ifndef __LIB_KERNEL_PRINT_H
#define __LIB_KERNEL_PRINT_H 

#include "stdint.h"

void put_char(uint8_t char_ascii);

void put_str(char* str_ascii);

void put_int(uint32_t num);        // 以16进制打印

void set_cursor(uint32_t cursor_pos);

#endif // __LIB_KERNEL_PRINT_H