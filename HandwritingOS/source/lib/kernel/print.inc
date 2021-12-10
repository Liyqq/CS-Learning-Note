;------------------- 视频端选择子 -------------------;
;打印原理是直接写显存，在32位保护模式下对内存的操作是：[段基址(选择子):段内偏移量]，
;所以这就涉及视频段选择子。
;用段寄存器GS来存储视频段选择子的，并且得保证在写显存之前，GS中的值是正确的选择子。
SELECTOR_RPL_0   equ  00b
SELECTOR_TI_GDT  equ  0_00b

SELECTOR_VIDEO equ (0x0003 << 3) + SELECTOR_TI_GDT + SELECTOR_RPL_0
