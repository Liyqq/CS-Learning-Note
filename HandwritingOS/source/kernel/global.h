#ifndef __KERNEL_GLOBAL_H
#define __KERNEL_GLOBAL_H

#include "stdint.h"


#define RPL0 0
#define RPL1 1
#define RPL2 2
#define RPL3 3


#define TI_GDT 0
#define TI_LDT 1


#define SELECTOR_K_CODE     ((1 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_K_DATA     ((2 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_K_STACK    SELECTOR_K_DATA
#define SELECTOR_K_GS       ((3 << 3) + (TI_GDT << 2) + RPL0)


/********** IDT描述符属性 **********/ 
#define IDT_DESC_P 1
#define IDT_DESC_DPL0 0
#define IDT_DESC_DPL3 3
#define IDT_DESC_32_TYPE 0xe
#define IDT_DESC_16_TYPE 0x6

#define IDT_DESC_ATTR_DPL_0 \
    ((IDT_DESC_P << 7) + (IDT_DESC_DPL0 << 5) + IDT_DESC_32_TYPE)

#define IDT_DESC_ATTR_DPL_3 \
    ((IDT_DESC_P << 7) + (IDT_DESC_DPL3 << 5) + IDT_DESC_32_TYPE)


#define NULL ( (void*)0 )
#define bool int
#define true    1
#define false   0

#endif // __KERNEL_GLOBAL_H