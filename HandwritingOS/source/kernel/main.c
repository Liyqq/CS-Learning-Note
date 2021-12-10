/*
* @Author: Yooj
* @Date:   2021-12-08 01:25:50
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-10 15:54:20
*/

#include "print.h"


// int _start(void) // Linux链接默认程序入口函数名
int main(void)
{
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

    while (1);
    return 0;
}