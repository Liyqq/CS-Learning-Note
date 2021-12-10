/*
* @Author: Yooj
* @Date:   2021-12-11 00:01:19
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-11 00:19:56
*/
#include <stdio.h>

int main(void)
{
    int in_a = 1, sum = 0;
    /* 通用约束：
     *         约束仅用于input部分，表示可与output和input部分中第n个操作数
     *         使用相同的寄存器或内存。
     */
    
    // "%I"(2)：表示传入一个立即数，并用立即数I约束(0~31间立即数)，gcc将其转
    //          换为汇编中立即数$2；
    //          %表示约束I对应操作数可以和下个输入所约束的操作数对换位置。
    // "0"(in_a)：使用"0"通用约束，表示要求gcc将分配给C变量in_a的寄存器或
    //            内存与分配给序号0对应的寄存器或内存相同。
    //            即sum被分配为EAX，in_a也被分配为一样的EAX。
    asm("addl %1, %0;" : "=a"(sum) :"%I"(2), "0"(in_a));
    printf("sum is %d\n", sum);

    return 0;
}