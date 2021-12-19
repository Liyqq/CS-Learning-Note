#ifndef __LIB_KERNEL_IO_H
#define __LIB_KERNEL_IO_H
#include "stdint.h"

/**
 * outb - 向指定的端口中写入一个字节的数据
 * @param port : 端口号，可容纳Intel支持的65536个端口号
 * @param data : 数据，一字节数据
 */
static inline void outb(uint16_t port, uint8_t data)
{
    /**
     * a表示使用寄存器AL或AX或EAX
     * N表示对端口号使用立即数约束，即0~255
     * d表示使用DX存储端口号
     *
     * %b0对应AL，%w1对应DX
     */
    asm volatile ("outb %b0, %w1" : : "a"(data), "Nd"(port));
}


/**
 * outsw - 向指定的端口中写入addr地址处共word_count个字
 * @param port       : 端口号，可容纳Intel支持的65536个端口号
 * @param addr       : 内存起始地址，指定待写入端口的数据在内存中的起始地址
 * @param word_count : 字数，指定写入端口的字的数量
 */
static inline void outsw(uint16_t port, const void* addr, uint32_t word_count)
{
    /**
     * +表示段寄存器既作为输入又做为输出
     * S表示寄存器ESI或SI
     * 
     * outsw指令是将DS:ESI处16位的数据写入port指定的端口号，
     * 设置段描述符时，将DS、ES和SS段的选择子都设置为相同的值，
     * 不用担心数据错乱。
     */
    asm volatile ("cld; \
                  rep outsw" 
                  : "+S"(addr), "+c"(word_count)
                  : "d"(port));
}


/**
 * inb - 从指定的端口中读入一个字节并返回
 * @param  port : 端口号，可容纳Intel支持的65536个端口号
 * @return      : 从端口读入的一字节数据
 */
static inline uint8_t inb(uint16_t port)
{
    uint8_t data;
    /**
     * =仅用于output中，限制内存或寄存器为只写
     */
    asm volatile ("inb %w1. %b0" : "=a"(data) : "Nd"(port));
    return data;
}


/**
 * insw - 向指定的端口中读入word_count个字写入addr指定的内存地址中
 * @param port       : 端口号，可容纳Intel支持的65536个端口号
 * @param addr       : 内存起始地址，指定从端口中读取d数据在存放在内存中的起始地址
 * @param word_count : 字数，指读入内存中字的数量
 */
static inline void insw(uint16_t port, void* addr, uint32_t word_count)
{
    /**
     * D表示寄存器EDI或DI
     * 
     * insw是将从端口port中读入一个字内容写入ES:EDI指定的内存中，
     * 设置段描述符时，将DS、ES和SS段的选择子都设置为相同的值，
     * 不用担心数据错乱。
     */
    asm volatile ("cld; \
                  rep insw"
                  : "+D"(addr), "+c"(word_count)
                  : "d"(port)
                  :"memory");
}



#endif // __LIB_KERNEL_IO_H