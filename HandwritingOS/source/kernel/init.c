/*
* @Author: Yooj
* @Date:   2021-12-17 00:04:53
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-17 00:20:11
*/

#include "print.h"
#include "init.h"
#include "interrupt.h"


/**
 * init_all - 初始化
 */
void init_all(void)
{
    put_str("int_all\n");
    idt_init();
}