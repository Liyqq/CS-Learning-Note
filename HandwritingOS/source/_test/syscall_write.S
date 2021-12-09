;模拟两种使用Linux系统调用的方式
;
;1.C语言库函数进行系统调用
;2.直接通过0x80中断进行系统调用
;---------------------------------------------------------------------------
section .data
str_c_lib: db "C library says: Hello World!", 0xa   ;0xa为LF(LineFeed)的ASCII码
str_c_lib_len equ $ - str_c_lib

str_syscall: db "System call says: Hello World!", 0xa
str_syscall_len equ $ - str_syscall


section .text
global _start
_start:
;;;;;;;;;; 方式1：模拟C语言中系统调用库函数write ;;;;;;;;;;
;在Linux系统中，一切设备都看作文件。而每打开一个文件，就有一个代表该打开文件的文件描述符。
;程序启动时默认打开三个I/O设备文件：
;    标准输入文件stdin，得到文件描述符 0。
;    标准输出文件stdout，得到文件描述符 1。
;    标准错误输出文件stderr，得到文件描述符 2。
    
    ;按照C语言调用约定，从右到左压入参数
    push str_c_lib_len
    push str_c_lib
    push 1                  

    call simu_write

    ;按照C语言调用约定，由调用者回收栈空间
    add esp, 4*3            
;;;;;;;;;; 方式1：模拟C语言中系统调用库函数write结束 ;;;;;;;;;;
    


;;;;;;;;;; 方式2：跨过库函数，直接进行系统调用 ;;;;;;;;;;
;当输入的参数小于等于5个时，Linux用寄存器传递参数。
;   EAX寄存器用来存储子功能号（寄存器 eip、ebp、esp 是不能使用的）。5个参数存放在
;   以下寄存器中，传送参数的顺序如下：
;       EBX：存储第1个参数。
;       ECX：存储第2个参数。
;       EDX：存储第3个参数。
;       ESI：存储第4个参数。
;       EDI：存储第5个参数。
;当参数个数大于5个时，把参数按照顺序放入连续的内存区域，并将该区域的首地址放到EBX寄存器。
    mov eax, 4                  ;第4号子功能是write系统调用
    mov ebx, 1
    mov ecx, str_syscall
    mov edx, str_syscall_len
    int 0x80
;;;;;;;;;; 方式2：跨过库函数，直接进行系统调用结束 ;;;;;;;;;;


;;;;; 退出程序 ;;;;;
    mov eax, 1                  ;第1号子功能是exit系统调用
    int 0x80


;---------------------------- START ------------------------------
;功能：模拟C库函数中系统调用函数write
;输入：(fd, buf, count)
;   fd：文件描述符
;   buf：缓冲区指针
;   count：从缓冲区向文件描述符写入的字节数
;输出：
;   无
;
simu_write:
    push ebp                    ;备份EBP
    mov ebp, esp                ;使用EBP作为基址来访问占中元素

    mov eax, 4
    mov ebx, [ebp + 8]          ;第1个参数
    mov ecx, [ebp + 12]         ;第2个参数
    mov edx, [ebp + 16]         ;第3个参数
    int 0x80

    pop ebp

    ret
;----------------------------  END  ------------------------------