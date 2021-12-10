/*
* @Author: Yooj
* @Date:   2021-12-10 23:19:56
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-11 00:02:32
*/
#include <stdio.h>

int main(void)
{
    /* 
     * =操作数修饰符表示寄存器或内存是只写的。
     * +操作数修饰符表示寄存器或内存先被读，再被写入。
     * &操作数修饰符表示寄存器或内存被操作数独占，只供output使用，&一定与约束名紧靠。
     */
    
    int in_a = 1, in_b = 2;
    asm("addl %%ebx, %%eax;" : "+a"(in_a) : "b"(in_b));
    printf("in_a is %d\n\n", in_a);

    /*********************************************************************/

    char* fmt = "Hello, World!\n";  // length = 14
    int ret_cnt = 0, test = 0; // ret_cnt记录printf返回值，test用于干扰返回值。

    /* 没有&修饰符，EAX由gcc分配，test可以干扰返回值*/
    asm("pushl %1;      \
        call printf;    \
        addl $4, %%esp; \
        movl $6, %2     \
        "
        :"=a"(ret_cnt)
        :"m"(fmt), "r"(test)
        );
    printf("The number of bytes writted without & modifier is %d\n", ret_cnt);

    ret_cnt = 0;
    test = 0;
    /* 加上&修饰符后，EAX将只用于output中，test无法干扰返回值*/
    asm("pushl %1;      \
        call printf;    \
        addl $4, %%esp; \
        movl $6, %2     \
        "
        :"=&a"(ret_cnt)
        :"m"(fmt), "r"(test)
        );
    printf("The number of bytes writted with & modifier is %d\n", ret_cnt);
    
    return 0;
}