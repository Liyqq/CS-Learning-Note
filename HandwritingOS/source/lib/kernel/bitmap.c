/*
* @Author: Yooj
* @Date:   2021-12-19 18:59:12
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-19 21:18:13
*/

#include "stdint.h"
#include "string.h"
#include "print.h"
#include "debug.h"
#include "bitmap.h"


/**
 * bitmap_init - 初始化位图
 * @param btmp : 待初始化的位图
 */
void bitmap_init(bitmap* btmp)
{
    memset(btmp->bits, 0, btmp->bitmap_bytes_len);
}


/**
 * bitmap_scan_test - 判断位图中bit_index位置是否为1
 * @param  btmp      : 位图指针
 * @param  bit_index : 位图中位的下标
 * @return           : 位图中bit_index位置对应位为1，返回true
 *                     位图中bit_index位置对应位为0，返回false
 */
bool bitmap_scan_test(bitmap* btmp, uint32_t bit_index)
{
    uint32_t byte_index = bit_index / 8;
    uint32_t bit_in_byte = bit_index % 8;
    return (btmp->bits[byte_index] & (BITMAP_MASK << bit_in_byte));
}


/**
 * bitmap_scan - 在位图中申请连续count个位
 * @param  btmp  : 位图指针
 * @param  count : 申请连续位的个数
 * @return       : 申请成功则返回起始位下标
 *                 申请失败则返回-1
 */
int bitmap_scan(bitmap* btmp, uint32_t count)
{
    uint32_t free_byte_index = 0; // 存在空闲位的字节下标
    /** 先逐个字节比较，找到存在空闲位的字节，
     * 1表示该位已分配，若为0xff，则表示该字节内已无空闲位，向下一字节继续找 
     */
    while ((0xff == btmp->bits[free_byte_index]) && 
           (free_byte_index < btmp->bitmap_bytes_len))
    {
        ++free_byte_index;
    }


    ASSERT(free_byte_index < btmp->bitmap_bytes_len);
    /* 若位图中找不到空闲位则返回-1 */
    if (free_byte_index == btmp->bitmap_bytes_len)
    {
        return -1; 
    }

    /* 若在该位图中找到某字节存在空闲位，逐位比较找出空闲位 */
    int free_bit_in_byte = 0;
    while ((uint8_t)(BITMAP_MASK<<free_bit_in_byte) & btmp->bits[free_bit_in_byte])
    {
        ++free_bit_in_byte;
    }

    int bit_start_index = free_byte_index*8 + free_bit_in_byte; // 空闲位下标
    if (count == 1)
    {
        return bit_start_index;
    }
    

    uint32_t bit_left = btmp->bitmap_bytes_len*8 - bit_start_index; // 记录空闲位数量
    uint32_t next_bit_index = bit_start_index + 1;
    uint32_t cnt = 1; // 记录找到空闲位的个数

    bit_start_index = -1;
    while (bit_left-- > 0)
    {
        /* 下一位是空闲的则cnt加一，否则将cnt清空，重新寻找连续count个位 */
        if (!(bitmap_scan_test(btmp, next_bit_index)))
        {
            ++cnt;
        }
        else
        {
            cnt = 0;
        }

        /* 在剩余空闲位中找到相应的count个位则退出循环 */
        if (cnt == count)
        {
            bit_start_index = next_bit_index - cnt + 1;
            break;
        }
        ++next_bit_index;
    }

    return bit_start_index;
}

/**
 * bitmap_set - 将位图btmp的对应bit_index位设置为value
 * @param btmp      : 位图指针
 * @param bit_index : 位图中待置位的下标
 * @param value     : 对应位置位的值，只能为0或1
 */
void bitmap_set(bitmap* btmp, uint32_t bit_index, int8_t value)
{
    ASSERT((value == 0) || (value == 1));
    uint32_t byte_index = bit_index / 8;
    uint32_t bit_in_byte = bit_index % 8;

    /* 将1任意移动后再取反；或者先取反再移位，可用来对位进行置0操作 */
    if (value)
    {
        btmp->bits[byte_index] |= (BITMAP_MASK << bit_in_byte);
    }
    else
    {
        btmp->bits[byte_index] &= ~(BITMAP_MASK << bit_in_byte);
    }
}

