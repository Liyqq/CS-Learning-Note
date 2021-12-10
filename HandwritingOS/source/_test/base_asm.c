/*
* @Author: Yooj
* @Date:   2021-12-10 22:15:02
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-10 22:21:39
*/
#include <stdio.h>

int in_a = 1, in_b = 2, out_sum;

int main(void)
{
    asm("pusha;             \
        movl in_a, %eax;    \
        movl in_b, %ebx;    \
        addl %ebx, %eax;    \
        movl %eax, out_sum; \
        popa;               \
        ");
    printf("sum is %d\n", out_sum);

    return 0;
}