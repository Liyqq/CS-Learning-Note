/*
* @Author: Yooj
* @Date:   2021-12-11 00:32:29
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-11 00:42:13
*/
#include <stdio.h>

int main(void)
{
    int in_a = 0x1234, in_b = 0;

    /* h –输出寄存器高位部分中的那一字节对应的寄存器名称，如AH、BH、CH、DH。
     * b –输出寄存器中低部分1字节对应的名称，如AL、BL、CL、DL。
     * w –输出寄存器中大小为2个字节对应的部分，如AX、BX、CX、DX。
     * k –输出寄存器的4字节部分，如EAX、EBX、ECX、EDX。
     */
    
    /* w、k和h、b一样，都是操作码，用来指代某种机器模式类型。
     * 
     * 下一行代码gcc会发出警告：
     *  Warning: using `%ax' instead of `%eax' due to `w' suffix
     * asm("movw %1, %0;" : "=m"(in_b) :"a"(in_a)); 
     */
    asm("movw %w1, %0;" : "=m"(in_b) :"a"(in_a));
    printf("in_b now is 0x%x\n", in_b);

    return 0;
}