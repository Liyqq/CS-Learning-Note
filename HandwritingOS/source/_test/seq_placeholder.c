/*
* @Author: Yooj
* @Date:   2021-12-10 22:44:57
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-10 22:58:25
*/
#include <stdio.h>

int main(void)
{
    int in_a = 0x12345678, in_b = 0;

    /* 将in_a中传入一个双字，即0x12345678 */
    asm("movl %1, %0;" : "=m"(in_b) : "a"(in_a));
    printf("double word in_b is 0x%x\n", in_b);
    in_b = 0;

    /* 将in_a中传入一个字，默认传入低16位，即0x5678 */
    asm("movw %1, %0;" : "=m"(in_b) : "a"(in_a));
    printf("word in_b is 0x%x\n", in_b);
    in_b = 0;

    /* 将in_a中传入一个字节，默认传入低8位，即0x78 */
    asm("movb %1, %0;" : "=m"(in_b) : "a"(in_a));
    // asm("movb %b1, %0;" : "=m"(in_b) : "a"(in_a)); // 指定传入低8位
    printf("low byte in_b is 0x%x\n", in_b);
    in_b = 0;

    /* 将in_a中传入一个字节，指定传入高8位，即0x56 */
    asm("movb %h1, %0;" : "=m"(in_b) : "a"(in_a));
    printf("high byte in_b is 0x%x\n", in_b);

    return 0;
}