;内核中断程序
;
;1.实现简单的内核中断处理程序，仅限时间中断
;
;---------------------------------------------------------------------------
;


[bits 32]
;若在相关的异常中cpu已经自动压入了错误码，为保持栈中格式统一，这里不做操作。
%define ERROR_CODE nop   

;若在相关的异常中cpu没有压入错误码，为了统一栈中格式，就手工压入一个0占位
%define ZERO push 0 


;---------------------------- START ------------------------------
;功能：定义中断处理程序的宏
;输入：
;   参数1：中断向量号
;   参数2：由于不是每个中断都会产生错误码，
;         所以此参数用于是否指定占位符用于对齐栈操作
;           ERROR_CODE：该中断有错误码产生，且cpu已经自动压入错误码，无任何操作
;           ZERO：该中断没有错误码产生，为对齐栈操作，压入0占位
;输出：
;   无
;
%macro VECTOR 2
section .text

;每个中断处理程序都要压入中断向量号，所以一个中断类型对应一个中断处理程序。
;自己知道自己的中断向量号是多少。
intr%1_entry:
    %2                      ;中断若有错误码会压在EIP后面 
    
    push intr_str
    call put_str
    add esp, 4

    ;如果是从片上进入的中断，除了往从片上发送EOI外，还要往主片上发送EOI。
    mov al, 0x20            ;中断结束命令EOI，OCW2第5位是EOI位，0x20=0010_0000b
    out 0xa0, al            ;向从片发送
    out 0x20, al            ;向主片发送

    add esp, 4              ;回收栈空间，跳过中断号

    iret

section .data
    ;存储各个中断入口程序的地址。编译器会将同类型的section合并到一个大的segment中，
    ;因此，每个定义的中断处理程序的开始地址(4字节)会和同样属性的intr_entry_table
    ;组合成数组，且intr_entry_table相当于数组首地址。
    dd intr%1_entry    
    
%endmacro
;----------------------------  END  ------------------------------

extern put_str

section .data

intr_str db "interrupt occur!", 0xa, 0

global intr_entry_table
intr_entry_table:



VECTOR 0x00, ZERO
VECTOR 0x01, ZERO
VECTOR 0x02, ZERO
VECTOR 0x03, ZERO
VECTOR 0x04, ZERO
VECTOR 0x05, ZERO
VECTOR 0x06, ZERO
VECTOR 0x07, ZERO
VECTOR 0x08, ERROR_CODE
VECTOR 0x09, ZERO
VECTOR 0x0a, ERROR_CODE
VECTOR 0x0b, ERROR_CODE
VECTOR 0x0c, ZERO
VECTOR 0x0d, ERROR_CODE
VECTOR 0x0e, ERROR_CODE
VECTOR 0x0f, ZERO
VECTOR 0x10, ZERO
VECTOR 0x11, ERROR_CODE
VECTOR 0x12, ZERO
VECTOR 0x13, ZERO
VECTOR 0x14, ZERO
VECTOR 0x15, ZERO
VECTOR 0x16, ZERO
VECTOR 0x17, ZERO
VECTOR 0x18, ERROR_CODE
VECTOR 0x19, ZERO
VECTOR 0x1a, ERROR_CODE
VECTOR 0x1b, ERROR_CODE
VECTOR 0x1c, ZERO
VECTOR 0x1d, ERROR_CODE
VECTOR 0x1e, ERROR_CODE
VECTOR 0x1f, ZERO
VECTOR 0x20, ZERO