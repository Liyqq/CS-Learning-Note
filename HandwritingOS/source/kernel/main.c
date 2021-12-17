/*
* @Author: Yooj
* @Date:   2021-12-08 01:25:50
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-17 23:26:18
*/

#include "print.h"
#include "init.h"



/* 测试用例函数声明 */
void print_test(void);
void intr_test(void);

// int _start(void) // Linux链接默认程序入口函数名
int main(void)
{
    // print_test();    

    intr_test();


    while (1);
    return 0;
}





void print_test(void)
{
    /* put_char打印函数测试 */ 
    /*
    put_char('K');
    put_char('e');
    put_char('r');
    put_char('n');
    put_char('e');
    put_char('l');
    put_char('\n'); // 换行
    put_char('1');
    put_char('2');
    put_char('\b'); // 退格
    put_char('3');
    */
    
    /* put_str打印函数测试 */ 
    /*
    put_str("I am the Kernel.\n");
    */
   
    /* put_int打印函数测试 */
    /*
    put_int(0);
    put_char('\n');
    put_int(9);
    put_char('\n');
    put_int(14);
    put_char('\n');
    put_int(0x12345678);
    put_char('\n');
    put_int(0x00e005af);
    put_char('\n');
    put_int(0x00000000);
    */
   
    /* put_char、put_str、put_int打印函数总测试 */
    put_char('C');
    put_char('h');
    put_char('a');
    put_char('r');
    put_char('\n');

    put_str("I am the Kernel.\n");

    put_int(0x00e015af);
}



void intr_test(void)
{
    put_str("I am the Kernel.\n");
    init_all();

    asm volatile ("sti"); // 演示中断处理，临时开中断
}