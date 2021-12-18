#ifndef __KERNEL_INTERRUPT_H
#define __KERNEL_INTERRUPT_H
#include "stdint.h"

typedef void* intr_handler;

void idt_init(void);



/**
 * 定义中断的两种状态：
 * INTR_OFF=0：关中断
 * INTR_ON=1：开中断
 */
typedef enum intr_status
{
    INTR_OFF,       // 关中断
    INTR_ON         // 开中断
} intr_status;

intr_status intr_get_status(void);                // 获取中断状态
intr_status intr_set_status(intr_status status);  // 设置中断状态
intr_status intr_enable(void);                    // 开中断
intr_status intr_disable(void);                   // 关中断


#endif // __KERNEL_INTERRUPT_H