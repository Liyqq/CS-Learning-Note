;调用C文件中c_print函数，并实现打印函数asm_print
;
;---------------------------------------------------------------------------
section .data
;0xa为LF(LineFeed)的ASCII码，0表示字符串结束符'\0'的ASCII码
string: db "asm_print says Hello World!", 0xa, 0x0
string_len equ $ - string

section .text
extern c_print
global _start
_start:
;;;;;;;;;; 调用C文件中c_print函数 ;;;;;;;;;;
    ;调用c_print(str)
    push string
    call c_print
    add esp, 4                  ;回收栈空间



;;;;;;;;;; 调用C文件中c_print函数结束 ;;;;;;;;;;
    

;;;;; 退出程序 ;;;;;
    mov eax, 1                  ;第1号子功能是exit系统调用
    int 0x80

global asm_print
;---------------------------- START ------------------------------
;功能：向标准输出(stdout)即屏幕打印字符
;输入：(str, size)
;   str：向屏幕输出的字符串
;   size：str的字节长度
;输出：
;   无
;
asm_print:
    push ebp                    ;备份EBP
    mov ebp, esp                ;使用EBP作为基址来访问占中元素

    mov eax, 4
    mov ebx, 1                  ;标准输出(stdout)文件描述符
    mov ecx, [ebp + 8]          ;第1个参数
    mov edx, [ebp + 12]         ;第2个参数
    
    int 0x80

    pop ebp

    ret
;----------------------------  END  ------------------------------