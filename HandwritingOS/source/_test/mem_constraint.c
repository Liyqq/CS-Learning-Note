/*
* @Author: Yooj
* @Date:   2021-12-10 22:26:34
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-10 23:04:55
*/

#include <stdio.h>

int main(void)
{
    int in_a = 1, in_b = 2;
    printf("in_b is %d\n", in_b);
    asm("movb %b0, %1;"
        :
        :"a"(in_a), "m"(in_b)
        :
        );
    printf("in_b now is %d\n", in_b);

    return 0;
}