/*
* @Author: Yooj
* @Date:   2021-12-10 22:18:32
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-10 23:04:41
*/
#include <stdio.h>

int main(void)
{
    int in_a = 1, in_b = 2, out_sum;
    asm("addl %%ebx, %%eax;"
        :"=a"(out_sum)
        :"a"(in_a), "b"(in_b)
        :
        );
    printf("sum is %d\n", out_sum);

    return  0;
}