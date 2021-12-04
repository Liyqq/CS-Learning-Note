;内核加载器 Kernel Loader
;
;实现从实模式到保护模式的三步走
;---------------------------------------------------------------------------
%include "boot.inc"
section loader vstart=LOADER_BASE_MEM_ADDR
jmp loader_start

;;;;;;;;;; 构建GDT及其内部的描述符 ;;;;;;;;;;
;GDT只是内存中一块区域
;
;GDT中第0个段描述不可用全部置0
GDT_BASE:   dd  0x00000000          
            dd  0x00000000

;代码段描述符，一个dd为4字节，段描述符为8字节，上面为低4字节
CODE_DESC:  dd  0x0000ffff          ;与高32位中的段界限部分组成0xfffff的界限
            dd  DESC_CODE_HIGH32

;数据段和栈段描述符，使用数据段来表示栈段，此时栈段向上扩展
DATA_STACK_DESC:    dd   0x0000ffff
                    dd   DESC_DATA_HIGH32

;显存段描述符
;只支持文本模式的输出，所以为了方便显存操作，显存段不采用平坦模型。
;直接把段基址置为文本模式的起始地址0xb8000，段大小为0xbffff-0xb8000=0x7fff，段粒度为4K
VIDEO_DESC: dd 0x80000007           ;limit=(0xbffff-0xb8000)/4K=0x7
            dd DESC_VIDEO_HIGH32    ;此时的DPL已经为0

GDT_SIZE    equ     $ - GDT_BASE
GDT_LIMIT   equ     GDT_SIZE - 1

times 60 dq 0                       ;此处预留60个段描述符的slot

;下面定义GDT的指针，前2个字节表示GDT界限，后4个字节表示GDT起始地址
GDT_ptr dw  GDT_LIMIT
        dd  GDT_BASE
;;;;;;;;;; 构建GDT及其内部的描述符结束 ;;;;;;;;;;


;;;;;;;;;; GDT中3个描述符对应选择子 ;;;;;;;;;;

SELECTOR_CODE   equ (0x0001 << 3) + SELECTOR_TI_GDT + SELECTOR_RPL_0
;上行代码相当于(CODE_DESC + GDT_SIZE)/8 + SELECTOR_TI_GDT + SELECOTR_RPL_0
SELECTOR_DATA   equ (0x0002 << 3) + SELECTOR_TI_GDT + SELECTOR_RPL_0
SELECTOR_VIDEO  equ (0x0003 << 3) + SELECTOR_TI_GDT + SELECTOR_RPL_0
;;;;;;;;;; GDT中3个描述符对应选择子结束 ;;;;;;;;;;

loader_msg db "Loader in real mode."
loader_msg_length equ $ - loader_msg


;---------------------------- START ------------------------------
loader_start:
;
;清屏利用BOIS提供的0x13号功能，打印字符串。
;---------------------------------------------------
;INT 0x10  功能号：0x13  功能描述：打印字符串
;---------------------------------------------------
;输入：
;    AH：子功能号=0x13
;    AL：显示输出方式
;        0——字符串中只含显示字符，其显示属性在BL中。显示后，光标位置不变
;        1——字符串中只含显示字符，其显示属性在BL中。显示后，光标位置改变
;        2——字符串中含显示字符和显示属性。显示后，光标位置不变
;        3——字符串中含显示字符和显示属性。显示后，光标位置改变
;    BH：页码
;    BL：属性(若AL=0x00或0x01)
;    CX：字符串长度
;    (DH, DL)：坐标（行、列）
;    ES:BP：字符串的地址
;无返回值
    mov sp, LOADER_BASE_MEM_ADDR
    mov bp, loader_msg
    mov cx, loader_msg_length
    mov ax, 0x1301
    mov bx, 0x001f                  ;页号为0，蓝底粉红字
    mov dx, 0x1800                  ;在第18行显示字符串

    int 0x10

;----------------------- 三步进入保护模式 -----------------------;
;1.打开A20地址线
;2.在GDTR寄存器中加载GDT地址以及偏移量
;3.将CR0的PE位置1

;--------------- 1.打开A20地址线 ---------------
;若A20Gate被打开，当访问0x100000~0x10FFEF之间的地址时，CPU将真正访问这块物理内存。
;若A20Gate被禁止，当访问0x100000~0x10FFEF之间的地址时，CPU将采用8086/8088的地址回绕。
;
;地址回绕是为了兼容8086/8088的实模式。
;打开A20Gate的方式是极其简单的，将端口0x92的第1位置1就可以了
    in al, 0x92
    or al, 0000_0010b
    out 0x92, al

;--------------- 2.在GDTR寄存器中加载GDT地址以及偏移量 ---------------
;全局描述符表位于内存中，需要用专门的寄存器指向它后，CPU才知道它在内存的哪里。
;这个专门的寄存器便是GDTR寄存器(GDT Register)，专门用来存储GDT的内存地址及大小。
;GDTR是个48位的寄存器，其格式如下：
;
;47                               15               0
;+--------------------------------+----------------+
;|          GDT base addr         |    GDT limit   |
;+--------------------------------+----------------+
;可使用lgdt指令(Load GDT)加载GDT
    lgdt [GDT_ptr]

;--------------- 3.将CR0的PE位置1 ---------------
;控制寄存器是CPU的窗口，既可以用来展示CPU的内部状态，也可用于控制CPU的运行机制。
;CR0寄存器(32位)中的第0位，即PE位(Protection Enable)用于启用保护模式，是保护模式的开关。
;当打开此位后(PE位置1)，CPU才真正进入保护模式。
    mov eax, cr0
    or eax, 0x0000_0001
    mov cr0, eax
;----------------------- 三步进入保护模式结束 -----------------------;

;刷新流水线，避免分支预测的影响,这种cpu流水线的优化策略，最怕jmp跳转，
;这将导致之前做的预测失效，从而起到了刷新的作用。
    jmp SELECTOR_CODE:p_mode_start 
    
;----------------------------  END  ------------------------------

;;;;;;;;;; 进入保护模式中打印"P"字符 ;;;;;;;;;;

[bits 32]
p_mode_start:
    mov ax, SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov esp, LOADER_STACK_TOP_MEM_ADDR

    mov ax, SELECTOR_VIDEO
    mov gs, ax

;向文本模式的显存中第80个字符位置（第二行首字符）写入字符P
    mov byte [gs:160], 'P'  
    mov byte [gs:161], 0x1f ;蓝底白字
;;;;;;;;;; 进入保护模式中打印"P"字符结束 ;;;;;;;;;;

    jmp $                   ;程序悬停此处
