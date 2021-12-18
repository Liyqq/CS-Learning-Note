/*
* @Author: Yooj
* @Date:   2021-12-18 22:14:35
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-19 01:33:07
*/
#include "print.h"
#include "interrupt.h"
#include "debug.h"


/**
 * panic_spin - 打印错误发生时的源文件名、行号、函数名以及引发错误的条件，并使程序悬停
 * @param filename  : 错误发生时的源文件名
 * @param line      : 错误发生时在源文件中的行号
 * @param func      : 错误发生时的所在执行的函数名
 * @param condition : 引发错误的条件
 */
void panic_spin(char* filename, 
                int line, 
                const char* func, 
                const char* condition)
{
    intr_disable(); // 有时会单独调用panic_spin，因此在此处关中断。
    
    put_str("\n\n\n!!!!! ERROR !!!!!\n");
    
    put_str("filename:");
    put_str(filename);
    put_str("\n");

    put_str("line:0x");
    put_int(line);
    put_str("\n");

    put_str("function:");
    put_str((char*)func);
    put_str("\n");

    put_str("condition:");
    put_str((char*)condition);
    put_str("\n");

    while (1); // 使程序悬停此处
}