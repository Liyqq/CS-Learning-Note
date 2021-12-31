/*
* @Author: Yooj
* @Date:   2021-12-17 00:04:53
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-31 19:52:26
*/

#include "print.h"
#include "init.h"
#include "interrupt.h"
#include "timer.h"
#include "memory.h"
#include "thread.h"

/**
 * init_all - 初始化
 */
void init_all(void)
{
    put_str("int_all\n");
    idt_init();     // 初始化中断
    timer_init();   // 初始化PIT
    mem_init();     // 初始化内存池
    thread_init();  // 初始化线程相关结构
}