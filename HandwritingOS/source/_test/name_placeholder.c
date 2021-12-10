/*
* @Author: Yooj
* @Date:   2021-12-10 23:01:26
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-10 23:08:30
*/
#include <stdio.h>

int main(void)
{
    int in_a = 18, in_b = 3, out = 0;
    asm("divb %[divisor];       \
        movb %%al, %[result]    \
        "
        :[result]"=m"(out)
        :"a"(in_a), [divisor]"m"(in_b)
        :
        );
    printf("result is %d\n", out);


    return 0;
}
