/*
* @Author: Yooj
* @Date:   2021-12-19 14:30:53
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-26 14:57:21
*/

#include "global.h"
#include "debug.h"
#include "string.h"


/**
 * memset - 将dst_起始的size个字节的内存单元设置为value
 * @param dst_  : 目的内存单元的起始地址
 * @param value : 目的内存单元内需要设置的值
 * @param size  : 需要设置的字节数
 */
void memset(void* dst_, uint8_t value, uint32_t size)
{
    ASSERT(dst_ != NULL);
    
    uint8_t* dst = (uint8_t*)dst_;
    while (size-- > 0)
    {
        *dst++ = value;
    }
}


/**
 * memcpy - 将src_起始的内存中size个字节数据复制到dst_起始的内存中
 * @param dst_ : 目的内存单元的起始地址
 * @param src_ : 源内存单元的起始地址
 * @param size : 需要复制的字节数
 */
void memcpy(void* dst_, const void* src_, uint32_t size)
{
    ASSERT((dst_ != NULL) && (src_ != NULL));
    
    uint8_t* dst = (uint8_t)dst_;
    const uint8_t* src = (uint8_t)src_;
    while (size-- > 0)
    {
        *dst++ = *src++;
    }
}


/**
 * memcmp - 连续比较以地址a_和地址b_开头的size个字节
 * @param  a_   : 比较的前值
 * @param  b_   : 比较的后值
 * @param  size : 比较的字节数
 * @return      : 若a_等于b_，则返回0；
 *                若a_大于b_，则返回+1；
 *                若a_小于b_，则返回-1
 */
int memcmp(const void* a_, const void* b_, uint32_t size)
{
    const char* a = a_;
    const char* b = b_;
    ASSERT((a != NULL) || (b != NULL));
    
    while (size-- > 0)
    {
        if (*a != *b)
        {
            return (*a > *b) ? 1 : -1;
        }
        ++a;
        ++b;
    }
    return 0;
}



/**
 * strcpy - 将字符串从src_起始的内存单元复制到dst_起始的内存单元
 * @param  dst_ : 目的字符串的内存单元起始地址
 * @param  src_ : 源字符串的内存单元起始地址
 * @return      : 复制后目的字符串内存单元起始地址
 */
char* strcpy(char* dst_, const char* src_)
{
    ASSERT((dst_ != NULL) && (src_ != NULL));

    char* res = dst_;
    while((*dst_++ = *src_++));
    return res;
}


/**
 * strlen - 获取传入字符串的长度
 * @param  str : 字符串起始内存地址
 * @return     : 字符串的长度
 */
uint32_t strlen(const char* str)
{
    ASSERT(str != NULL);

    const char* p = str;
    while(*p++);
    return (p - str - 1);
}


/**
 * strcmp - 比较两个字符串a和字符串b
 * @param  a : 比较的前值
 * @param  b : 比较的后值
 * @return   [description]
 */
uint8_t strcmp(const char* a, const char* b)
{
    ASSERT((a != NULL) && (b != NULL));

    while ((*a != 0) && (*a == *b))
    {
        ++a;
        ++b;
    }

    /**
     * a指向字符小于b指向字符则返回-1；
     * a指向字符大于b指向字符则返回+1，即(*a > *b)结果为true=1；
     * a指向字符等于b指向字符则返回0，即(*a > *b)结果为false=0；
     */
    return (*a < *b) ? -1 : (*a > *b);
}


/**
 * strchr - 从左到右查找字符串str中首次出现字符chr的内存地址
 * @param  str : 字符串首地址
 * @param  chr : 待查找的字符
 * @return     : 若查找到对应字符则返回该字符地址；
 *               若未查找到对应字符则返回NULL
 */
char* strchr(const char* str, const uint8_t chr)
{
    ASSERT(str != NULL);

    while (*str != 0)
    {
        if (*str == chr)
        {
            return (char*)str;
        }
        ++str;
    }
    return NULL;
}


/**
 * strrchr - 从右到左查找字符串str中首次出现字符chr的内存地址
 * @param  str : 字符串首地址
 * @param  chr : 待查找的字符
 * @return     : 若查找到对应字符则返回该字符地址；
 *               若未查找到对应字符则返回NULL
 */
char* strrchr(const char* str, const uint8_t chr)
{
    ASSERT(str != NULL);

    /* 从左到右遍历字符串，使用last_char记录目标字符最后一次出现在字符串中的地址 */
    const char* last_char = NULL;
    while (*str != 0)
    {
        if (*str == chr)
        {
            last_char = str;
        }
        ++str;
    }
    return (char*)last_char;
}


/**
 * strcat - 将字符串src_拼接到到字符串dst_后。
 * @param  dst_ : 目的内存单元起始地址
 * @param  src_ : 源字符串的内存单元起始地址
 * @return      : 返回拼接后字符串的首地址
 */
char* strcat(char* dst_, const char* src_)
{
    ASSERT((dst_ != NULL) && (src_ != NULL));

    char* str = dst_;
    while (*str++);
    --str;

    while ((*str++ = *src_++));
    return dst_;
}

/**
 * strchrs - 获取字符chr在字符串str中出现的次数
 * @param  str : 字符串首地址
 * @param  chr : 待查找的字符
 * @return     : 字符chr在字符串str中出现的次数
 */
uint32_t strchrs(const char* str, uint8_t chr)
{
    ASSERT(str != NULL)
    
    uint32_t char_count = 0;
    const char* p = str;
    while (*p != 0)
    {
        if (*p == chr)
        {
            ++char_count;
        }
        ++p;
    }
    return char_count;
}

