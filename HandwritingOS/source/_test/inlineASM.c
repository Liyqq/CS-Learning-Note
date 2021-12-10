/*
* @Author: Yooj
* @Date:   2021-12-10 20:31:16
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-10 20:39:40
*/
char* str = "Hello, Inline Assembly!\n"; // length = 24
int count = 0;

int main(void)
{
    /* AT&T Assembly style */
    // __asm__ equals asm
    // __volatile__ equals volatile
    asm("pusha; \
        movl $4, %eax;  \
        movl $1, %ebx;  \
        movl str, %ecx; \
        movl $24, %edx; \
        int $0x80;      \
        mov %eax, count;\
        popa;           \
        ");

    return 0;
}