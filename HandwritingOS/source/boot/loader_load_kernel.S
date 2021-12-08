;内核加载器 Kernel Loader
;
;1.实现从实模式到保护模式的三步走，并获取计算机内存，打开分页机制
;2.加载内核
;---------------------------------------------------------------------------
%include "boot.inc"
section loader vstart=LOADER_BASE_MEM_ADDR
;此版本loader中直接从mbr.S跳到loader_start,实现内存对齐
;jmp loader_start

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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;total_mem_bytes用于保存内存容量以字节为单位，此位置比较好记。
;当前偏移loader.bin文件头0x200字节，loader.bin的加载地址是0x900，
;故total_mem_bytes内存中的地址是0xb00，将来在内核中咱们会引用此地址。
total_mem_bytes dd 0
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;下面定义GDT的指针，前2个字节表示GDT界限，后4个字节表示GDT起始地址
GDT_ptr dw  GDT_LIMIT
        dd  GDT_BASE
;;;;;;;;;; 构建GDT及其内部的描述符结束 ;;;;;;;;;;

;BIOS中断0x15的子功能0xE820能够获取系统的内存布局，由于系统内存各部分的类型属性不同，
;BIOS就按照类型属性来划分这片系统内存，所以这种查询呈迭代式，每次BIOS只返回一种类型的内存
;信息，直到将所有内存类型返回完毕。子功能0xE820的强大之处是返回的内存信息较丰富，包括多个属
;性字段，所以需要一种格式结构来组织这些数据。用于存储这种描述符的结构称之为
;地址范围描述符(Address Range Descriptor Structure, ARDS)，其格式为：
;+--------------------+--------------------+--------------------+
;|    byte offset     |      attribute     |     description    |
;+--------------------+--------------------+--------------------+
;|         0          |    BaseAddrLow     |     low 32 bits    |
;+--------------------+--------------------+--------------------+
;|         4          |    BaseAddrHigh    |    high 32 bits    |
;+--------------------+--------------------+--------------------+
;|         8          |     LengthLow      |     low 32 bits    |
;+--------------------+--------------------+--------------------+
;|        12          |     LengthHigh     |    high 32 bits    |
;+--------------------+--------------------+--------------------+
;|        16          |        Type        |    memroy type     |
;+--------------------+--------------------+--------------------+
;此结构中的字段大小都是4字节，共5个字段，所以此结构大小为20字节。每次int 0x15之后，BIOS
;就返回这样一个结构的数据。注意，ARDS结构中用64位宽度的属性来描述这段内存基地址（起始地址）
;及其长度，所以表中的基地址和长度都分为低32位和高32位两部分。
;
;Type字段：用来描述这段内存的类型，这里所谓的类型是说明这段内存的用途，即其是可以被
;操作系统使用，还是保留起来不能用。
;    1表示AddressRangeMemory 这段内存可以被操作系统使用
;    2表示AddressRangeReserved 内存使用中或者被系统保留，操作系统不可以用此内存
;    其他表示未定义，目前保留。但是需要操作系统一样将其视为ARR(AddressRangeReserved)
;
;人工对齐:total_mem_bytes4字节+gdt_ptr6字节+ards_buf244字节+ards_nr2，共256字节
ARDS_buffer times 244 db 0
ARDS_number dw 0                ;记录ARDS结构体数量


;;;;;;;;;; GDT中3个描述符对应选择子 ;;;;;;;;;;

;下行代码相当于(CODE_DESC + GDT_SIZE)/8 + SELECTOR_TI_GDT + SELECOTR_RPL_0
SELECTOR_CODE   equ (0x0001 << 3) + SELECTOR_TI_GDT + SELECTOR_RPL_0
SELECTOR_DATA   equ (0x0002 << 3) + SELECTOR_TI_GDT + SELECTOR_RPL_0
SELECTOR_VIDEO  equ (0x0003 << 3) + SELECTOR_TI_GDT + SELECTOR_RPL_0
;;;;;;;;;; GDT中3个描述符对应选择子结束 ;;;;;;;;;;



;---------------------------- START ------------------------------
;loader_start的地址为LOADER_BASE_MEM_ADDR + 0x300
loader_start:
;
;;;;;;;;;; 利用BOIS提供的0x15H号功能，获取内存容量 ;;;;;;;;;;
;
;---------- 利用BIOS中断0x15，子功能0xE820获取内存 ----------
;---------------------------------------------------
;INT 0x15  功能号：0xE820  功能描述：获取内存
;---------------------------------------------------
;输入：
;    EAX：子功能号=0xE820
;    EBX：ARDS后续值：内存信息需要按类型分多次返回，由于每次执行一次中断都只返回一种类
;         型内存的ARDS结构，所以要记录下一个待返回的内存ARDS，在下一次中断调用时通过
;         此值告诉BIOS该返回哪个ARDS。
;         第一次调用时一定要置为0，之后EBX在每次中断返回后，BIOS会更新此值。
;    ES:DI：ARDS缓冲区，BIOS将获取到的内存信息写入此寄存器指向的内存。
;    ECX：ARDS结构的字节大小，用来指示BIOS写入的字节数。调用者和BIOS都同时支持的大小
;         是20字节。
;    EDX：固定为签名标记0x534D4150，此十六进制数字是字符串SMAP的ASCII码，BIOS将调用者正
;         在请求的内存信息写入ES:DI寄存器所指向的ARDS缓冲区后，再用此签名校验其中的信息。
;输出：
;    CF位：若CF位为0表示调用未出错，为1表示调用出错。
;    EAX：字符串SMAP的ASCII码0x534D4150 
;    ES:DI：ARDS缓冲区地址，同输入值是一样的，返回时此结构中已经被BIOS填充了内存信息
;    ECX：BIOS写入到ES:DI所指向的ARDS结构中的字节数，BIOS最小写入20字节。
;    EBX：下一个ARDS的位置。每次中断返回后，BIOS会更新此值，BIOS通过此值可以找到下一个
;         待返回的ARDS结构，不需要改变EBX的值，下一次中断调用时还会用到它。
;         在CF位为0的情况下，若返回后的EBX值为0，表示这是最后一个ARDS结构。
;     
    xor ebx, ebx                    ;第一次调用时EBX需要传入0
    mov edx, 0x534d4150             ;EDX赋值一次后，在循环体中不会改变
    mov di, ARDS_buffer             ;ARDS的缓冲区
;循环获取每个ARDS
.e820_mem_get_loop:                 
    mov eax, 0x0000e820             ;执行int 0x15后，EAX值变为0x534d4150，
                                    ;所以每次执行int前都要更新为子功能号。
    mov ecx, 20                     ;ARDS地址范围描述符结构大小是20字节
    
    int 0x15 

    jc .e820_failed_so_try_e801     ;若CF位为1则有错误发生，尝试0xe801子功能
    add di, cx                      ;使DI增加20字节指向缓冲区中新的ARDS结构位置
    inc word [ARDS_number]          ;记录ARDS的数量
    cmp ebx, 0                      ;若EBX为0且CF不为1，这说明ARDS全部返回。
    jnz .e820_mem_get_loop

;在所有ARDS结构中，找出(base_add_low + length_low)的最大值，即内存的容量。
    mov cx, [ARDS_number]
    mov ebx, ARDS_buffer
    xor edx, edx                    ;EDX为最大的内存容量，先清0
.find_max_mem_area:                 ;无须判断type是否为1,最大的内存块一定是可被使用
    mov eax, [ebx]                  ;base_add_low
    add eax, [ebx+8]                ;length_low
    cmp edx, eax                    ;冒泡排序找出最大，EDX寄存器始终是最大的内存容量
    jge .next_ards
    mov edx, eax                    ;EDX为新的最大总内存大小
.next_ards:
    add ebx, 20                     ;指向缓冲区中下一个ARDS结构
    loop .find_max_mem_area
    jmp .mem_get_ok

;---------- 利用BIOS中断0x15，子功能0xE801获取内存 ----------
;---------------------------------------------------
;INT 0x15  功能号：0xE820  功能描述：获取内存
;---------------------------------------------------
;输入：
;    AX：子功能号：0xE801
;输出：
;    CF位：若CF位为0表示调用未出错，为1表示调用出错。
;    AX：Extended 1，以1KB为单位，只显示15MB以下的内存容量，故最大值为0x3C00，
;        即AX表示的最大内存为0x3c00*1024=15MB 
;    BX：Extended 2，以64KB为单位，内存空间16MB~4GB中连续的单位数量，
;        即内存大小为BX*64*1024字节
;    CX Configured 1， 同AX 
;    DX Configured 2， 同BX
;
;80286拥有24位地址线，其寻址空间是16MB。当时有一些ISA设备要用到地址15MB以上的内存作为缓冲区，
;也就是此缓冲区为1MB大小，所以硬件系统就把这部分内存保留下来，操作系统不可以用此段内存空间。
;保留的这部分内存区域就像不可以访问的黑洞，这就成了内存空洞memory hole。现在虽然很少很少能碰
;到这些老ISA设备了，但为了兼容，这部分空间还是保留下来。
;
;如果检测到的内存容量大于等于16MB，0x15中断返回的结果中，AX*1024必然是小于等于15MB，
;而BX*64*1024肯定大于0。所以内存容量分成两部分展示，只要符合这两个结果，就能检查出内存空洞。
;如果物理内存在16MB以下，此方法就不适用了，但检测到的内存依然会小于实际内存1MB。所以实际的
;物理内存大小，在检测结果的基础上一定要加上1MB。
.e820_failed_so_try_e801:
    mov ax, 0xe801

    int 0x15

    jc .e801_failed_so_try_88       ;若当前E801方法失败，就尝试0x88方法

;1 先算出低15M的内存，AX和CX中是以KB为单位的内存数量，将其转换为以byte为单位
    mov cx, 0x400                   ;CX和AX值一样，CX用做乘数，0x400=1024
    mul cx 

    shl edx, 16                     ;EDX低16位清零
    and eax, 0x0000ffff             ;EAX高16位清零
    or edx, eax                     ;低15MB容量先存储在EDX中
    add edx, 0x00100000             ;AX只是15MB，故要加1MB
    mov esi, edx                    ;先把低15MB的内存容量存入ESI寄存器备份

;2 再将16MB以上的内存转换为byte为单位,寄存器BX和DX中是以64KB为单位的内存数量
    xor eax, eax                    ;32位乘法用到EAX，先清零
    mov ax, bx
    mov ecx, 0x00100000             ;0x0010000十进制为64K
    mul ecx                         ;32位乘法，默认的被乘数是EAX，积为64位，
                                    ;高32位存入EDX，低32位存入EAX。
    add esi, eax                    ;由于此方法只能测出4G以内的内存，
                                    ;故32位EAX足够了，EDX肯定为0，ESI只加EAX便可。
    mov edx, esi                    ;EDX为最终的总内存大小
    jmp .mem_get_ok

;---------- 利用BIOS中断0x15，子功能0x88获取内存 ----------
;---------------------------------------------------
;INT 0x15  功能号：0xE820  功能描述：获取内存
;---------------------------------------------------
;输入：
;    AH：子功能号=0x88 
;输出：
;    CF位：若CF位为0表示调用未出错，为1表示调用出错。
;    AX：以1KB 为单位大小，内存空间1MB之上的连续单位数量，不包括低端1MB内存。
;        故内存大小为AX*1024字节+1MB
;该子功能只能识别最大64MB的内存。即使内存容量大于64MB，也只会显示63MB。
;且此中断只会显示1MB之上的内存，不包括这1MB，在使用的时候需要加上这1MB
.e801_failed_so_try_88:
    mov ah, 0x88

    int 0x15

    jc .error_halt
    and eax, 0x0000ffff

    ;16位乘法，被除数AX，积高16位在DX中，低16位在AX中
    mov cx, 0x400                   ;0x400=1024
    mul cx  
    shl edx, 16                     ;DX移到高16位
    or edx, eax                     ;加上AX中低16位内容
    add edx, 0x00100000             ;0x88子功能只会返回1MB以上的内存，
                                    ;故实际内存大小要加上1MB

.mem_get_ok:
    mov [total_mem_bytes], edx

;;;;;;;;;; 利用BOIS提供的0x15H号功能，获取内存容量结束 ;;;;;;;;;;




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


;出错则挂起
.error_halt:
    hlt
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


;----------------------- 三步开启分页机制 -----------------------;
;1.创建页目录表及页表。
;2.将页表地址写入控制寄存器CR3。 
;3.寄存器CR0的PG位置1。
;
;--------------- 1.创建页目录表及页表 ---------------
    call setup_page

    ;要将描述符表地址及偏移量写入内存GDT_ptr，一会用新地址重新加载
    ;在页表中会将内核放置到3GB以上的地址，因此也把GDT放在内核的地址空间
    ;sgdt指令用于获取GDT(全局描述表)的虚拟地址。
    sgdt [GDT_ptr]                          ;存储到原来的GDT_ptr的位置

    ;修改显存段的段描述符的段基址，因为将来内核运行在3GB以上，打印功
    ;能将来也是在内核中实现，不能让用户进程直接能控制显存。故显存段的
    ;段基址也要改为3GB以上。0xc000_0000 ~ 0xffff_ffff是最高1GB
    ;将gdt描述符中VIDEO段描述符中的段基址+0xc000_0000
    mov ebx, [GDT_ptr + 2]                  ;GDT_ptr前2字节(16bits)是偏移量，
                                            ;后4个字节是基址，此处得到GDT的基址

    or dword [ebx + 0x18 + 4], 0xc0_000000  ;视频段是第4个段描述符，每个段描述符是
                                            ;8字节，故0x18=24。
                                            ;段描述符的高4字节的最高8位是段基址
                                            ;的31~24位(8bit)

    ;将gdt的基址加上0xc0000000使其成为内核所在的高地址
    add dword [GDT_ptr + 2], 0xc0000000

    ;将栈指针同样映射到内核地址，等同于LOADER_STACK_TOP_MEM_ADDR+0xc0000000
    add esp, 0xc0000000    

;--------------- 2.将页表地址写入控制寄存器CR3 ---------------
    mov eax, PAGE_DIR_TABLE_BASE_ADDR
    mov cr3, eax

;--------------- 3.寄存器CR0的PG位置1 ---------------
    mov eax, cr0
    or eax, 0x8000_0000                     ;0x8=1000b
    mov cr0, eax

;----------------------- 三步开启分页机制结束 -----------------------;


;在开启分页后，用gdt指令重新加载GDT新的地址
    lgdt [GDT_ptr]                          ;重新加载

;;;;;;;;;; 视频段段基址已经被更新,用Virtual Addr表示 ;;;;;;;;;;
    mov byte [gs:320], 'V'     
    mov byte [gs:322], 'i'     
    mov byte [gs:324], 'r'     
    mov byte [gs:326], 't'     
    mov byte [gs:328], 'u'     
    mov byte [gs:330], 'a'     
    mov byte [gs:332], 'l'
    mov byte [gs:334], ' '
    mov byte [gs:336], 'A'
    mov byte [gs:338], 'd'
    mov byte [gs:340], 'd'
    mov byte [gs:342], 'r'     
;;;;;;;;;; 视频段段基址已经被更新,用Virtual Addr表示结束 ;;;;;;;;;;


;----------------------- 加载内核 -----------------------;
    mov eax, KERNEL_START_SECTOR_LAB_ADDR       ;kerbel.bin内核文件所在扇区号
    mov ebx, KERNEL_BIN_BASE_MEM_ADDR           ;从磁盘读出后，写入到EBX指定的地址
    mov ecx, KERNEL_CONSUMED_SECTOR_NUM         ;读入的扇区数

    call rd_disk_m_32                           ;读入内核文件

;;;;;;;;;;;;;;;;;;;;;;;;;;;;  此时不刷新流水线也没问题  ;;;;;;;;;;;;;;;;;;;;;;;;
;由于一直处在32位下,原则上不需要强制刷新,经过实际测试没有以下这两句也没问题。
;但以防万一，还是加上，免得将来出来莫句奇妙的问题.
    jmp SELECTOR_CODE:enter_kernel              ;强制刷新流水线，更新gdt
;----------------------- 加载内核结束 -----------------------;
enter_kernel:
    mov byte [gs:640], 'K'
    mov byte [gs:642], 'e'
    mov byte [gs:644], 'r'
    mov byte [gs:646], 'n'
    mov byte [gs:648], 'e'
    mov byte [gs:650], 'l'
    mov byte [gs:652], ' '
    mov byte [gs:654], 'L'
    mov byte [gs:656], 'o'
    mov byte [gs:658], 'a'
    mov byte [gs:660], 'd'
    mov byte [gs:662], 'i'
    mov byte [gs:664], 'n'
    mov byte [gs:666], 'g'
    mov byte [gs:668], '.'
    mov byte [gs:670], '.'
    mov byte [gs:672], '.'

    call kernel_init

    mov esp, KERNEL_STACK_TOP_MEM_ADDR
    jmp KERNEL_ENTRY_POINT      ;进入内核 

;---------------------------- START ------------------------------
;创建页目录及页表
;
setup_page:
;;;;;;;;;; 将页目录占用的空间逐字节清0 ;;;;;;;;;;
    mov ecx, 4096
    mov esi, 0
.clear_page_dir_table:
    mov byte [PAGE_DIR_TABLE_BASE_ADDR + esi], 0
    inc esi
    loop .clear_page_dir_table
;;;;;;;;;; 将页目录占用的空间逐字节清0结束 ;;;;;;;;;;


;;;;;;;;;; 创建页目录项(PDE) ;;;;;;;;;;
.create_pde:
    mov eax, PAGE_DIR_TABLE_BASE_ADDR
    add eax, 0x1000                            ;此时EAX为第一个页表的地址和属性
                                                ;此处仅有地址，属性在之后赋予
                                                ;EAX=0x100000+0x1000=0x101000
                                                ;0x1000 = 4K = 4096

    mov ebx, eax                                ;此处赋值EBX是为.create_pte做
                                                ;准备，EBX为1024个页表的基址

;以下代码将页目录项0和768=0xc00byte/4byte都作为第一个页表的地址目录项，
;一个页表可以表示1024*4KB=4MB内存，4M = 0x40_0000，
;这样物理地址0xc00_0000~0xc03f_ffff的4MB和物理地址0x00_0000~0x003f_ffff
;的4MB都指向相同的页表，这将为地址映射为内核地址做准备
    or eax, PAGE_US_U | PAGE_RW_W | PAGE_P      ;页目录项的属性RW和P位为1，US为1
                                                ;表示用户属性，所有特权级别都可访问

    mov [PAGE_DIR_TABLE_BASE_ADDR + 0x0], eax   ;第1个目录项，在页目录表中的第1个目
                                                ;录项写入第一个页表的位置(0x101000)
                                                ;及属性(0x7=0000_0000_0111b)
                                                
    mov [PAGE_DIR_TABLE_BASE_ADDR + 0xc00], eax ;一个页表项占用4字节，0xc00表示第
                                                ;768个页表占用的目录项，0xc00以上的
                                                ;目录项用于内核空间，也就是页表的
                                                ;0xc000_0000~0xffff_ffff共计1G
                                                ;属于内核，
                                                ;0x0~0xbfff_ffff共计3G属于用户进程。
    
    sub eax, 0x1000                             ;EAX=0x00100_007，属性位之后被覆盖
    mov [PAGE_DIR_TABLE_BASE_ADDR + 4092], eax  ;最后一个目录项指向页目录表自己的地址
;;;;;;;;;; 创建页目录项(PDE)结束 ;;;;;;;;;;


;;;;;;;;;; 创建页表项(PTE) ;;;;;;;;;;
    mov ecx, 256                                ;1M低端内存 / 每页大小4K = 256页
    mov esi, 0
    mov edx, PAGE_US_U | PAGE_RW_W | PAGE_P     ;属性位0x007，US=1,RW=1,P=1
.create_pte:
    mov [ebx + esi*4], edx                      ;之前赋值EBX=0x101000，
                                                ;即第一个页表地址

    add edx, 4096                               ;4096=4K=0x1000
    inc esi
    loop .create_pte
;;;;;;;;;; 创建页表项(PTE)结束 ;;;;;;;;;;


;;;;;;;;;; 创建内核其它页表的PDE ;;;;;;;;;; 
    mov eax, PAGE_DIR_TABLE_BASE_ADDR
    add eax, 0x2000                             ;此时EAX为第二个页表的位置
    or eax, PAGE_US_U | PAGE_RW_W | PAGE_P      ;此页目录项的属性位US=1,RW=1,P=1

    mov ebx, PAGE_DIR_TABLE_BASE_ADDR
    mov ecx, 254                                ;范围为第769~1022的所有目录项数量
    mov esi, 769
.create_kernel_pde:
    mov [ebx + esi*4], eax
    inc esi
    add eax, 0x1000                             ;0x1000=4k，一个页表大小为4KB
    loop .create_kernel_pde 
;;;;;;;;;; 创建内核其它页表的PDE结束 ;;;;;;;;;;

    ret
;----------------------------  END  ------------------------------




;---------------------------- START ------------------------------
;功能：在32位模式下读取硬盘n个扇区
;输入：
;   EAX=LBA扇区号
;   BX=从硬盘读取的数据写入内存的开始地址
;   CX=从硬盘读取的扇区数
;输出：
;   BX指向内存地址
;
rd_disk_m_32:
;备份操作:
    mov esi, eax        ;备份EAX，因为in/out指令会用到AL
    mov di, cx          ;备份CX，因为读取数据，计算读取次数时需要用到CX中扇区数

;读写硬盘:   
;第1步:设置要读取硬盘扇区数
    mov dx, 0x01f2      ;Sector count端口
    mov al, cl          ;传入端口的数据
    out dx, al          ;读取扇区数

    mov eax, esi        ;还原EAX，之后操作都需用到EAX传入的值

;第2步:将LBA地址送入LAB端口(0x01f3 ~ 0x1f6)
    ;LBA地址0 ~ 7位写入端口为0x01f3
    mov dx, 0x01f3
    out dx, al

    mov cl, 8           ;LBA寄存器为8位寄存器
    ;LBA地址8 ~ 15位写入端口为0x01f4
    shr eax, cl
    mov dx, 0x01f4
    out dx, al

    ;LBA地址16 ~ 23位写入端口为0x01f5
    shr eax, cl
    mov dx, 0x01f5
    out dx, al

    ;LBA地址14 ~ 27位写入端口为device(0x01f6)低4位
    shr eax, cl
    and al, 0x0f       ;取AL的低4位写入device端口低4位，
    or al, 0xe0        ;设置device高4位为0xe0(1110)，表示LBA模式
    mov dx, 0x01f6
    out dx, al

;第3步:向Command端口(0x01f7)写入读命令(0x20)
    mov dx, 0x01f7
    mov al, 0x20
    out dx, al

;第4步:检查硬盘状态
    ;Status端口(0x01f7)读时表示读取硬盘状态，写时表示写入命令
.not_ready:
    nop
    in al, dx
    and al, 0x88        ;第4位为1表示硬盘控制器已准备好数据传输
                        ;第7位为1表示硬盘忙
                        ;0x88(1000 1000)
    cmp al, 0x08        ;0x08(0000 1000)
    jnz .not_ready      ;若未准备好，继续等待

;第5步:从Data端口(0x01f0)读取数据，一次读取1个字(2 bytes)
    ;计算读取次数
    mov ax, di          ;DI寄存器中备份有传入的CX中的扇区数
    mov dx, 256         ;一次读入2字节，一个扇区512字节，则一个扇区需要读256次
    mul dx              ;总共需要读取的次数=ax(di) * 256

    mov cx, ax
    mov dx, 0x01f0
.go_on_read:
    ;由于在实模式下偏移地址为16位，所以用BX只会访问到0~FFFFh的偏移。
    ;loader的栈指针为0x900，BX为指向的数据输出缓冲区，且为16位，
    ;超过0xffff后，BX部分会从0开始，所以当要读取的扇区数过大，待写入
    ;的地址超过BX的范围时，从硬盘上读出的数据会把0x0000~0xffff的覆盖，
    ;造成栈被破坏，造成ret返回时，返回地址被破坏了，不是之前正确的地址，
    ;故程序出会错，不知道会跳到哪里去。
    ;
    ;所以改为EBX代替BX指向缓冲区，这样生成的机器码前面会有0x66和0x67
    ;来反转：
    ;   0X66用于反转默认的操作数大小。
    ;   0X67用于反转默认的寻址方式。
    ;cpu处于16位模式时，会理所当然的认为操作数和寻址都是16位，处于32位
    ;模式时，也会认为要执行的指令是32位。当我们在其中任意模式下用了另外模
    ;式的寻址方式或操作数大小(姑且认为16位模式用16位字节操作数，32位模式
    ;下用32字节的操作数)时，编译器会在指令前帮我们加上0x66或0x67，临时
    ;改变当前cpu模式到另外的模式下。
    ;
    ;运行在16位模式，遇到0X66时，操作数大小变为32位。
    ;运行在32位模式，遇到0X66时，操作数大小变为16位。
    ;运行在16位模式，遇到0X67时，寻址方式变为32位寻址。
    ;运行在32位模式，遇到0X67时，寻址方式变为16位寻址。
    in ax, dx
    mov [ebx], ax
    add ebx, 2
    loop .go_on_read

    ret
;----------------------------  END  ------------------------------




;---------------------------- START ------------------------------
;将内核文件(kernel.bin)中的程序段(segment)拷贝到编译时
;指定的内存基址(KERNEL_BIN_BASE_MEM_ADDR=0x70000)中
;
;ELF指的是Executable and Linkable Format，可执行链接格式。
;最初是由UNIX系统实验室(USL)作为应用程序二进制接口(ABI)而开发
;和发行的。工具接口标准委员会(TIS)选择了它作为IA32体系结构上不
;同操作系统之间的可移植二进制文件格式，于是它就发展成为了事实上
;的二进制文件格式标准。其格式定义如下所示
;
;#define EI_NIDENT (16)
;
;typedef struct
;{
;    unsigned char e_ident[EI_NIDENT]; /* Magic number and other info       */
;    Elf32_Half    e_type;             /* Object file type                  */
;    Elf32_Half    e_machine;          /* Architecture                      */
;    Elf32_Word    e_version;          /* Object file version               */
;    Elf32_Addr    e_entry;            /* Entry point virtual address       */
;    Elf32_Off     e_phoff;            /* Program header table file offset  */
;    Elf32_Off     e_shoff;            /* Section header table file offset  */
;    Elf32_Word    e_flags;            /* Processor-specific flags          */
;    Elf32_Half    e_ehsize;           /* ELF header size in bytes          */
;    Elf32_Half    e_phentsize;        /* Program header table entry size   */
;    Elf32_Half    e_phnum;            /* Program header table entry count  */
;    Elf32_Half    e_shentsize;        /* Section header table entry size   */
;    Elf32_Half    e_shnum;            /* Section header table entry count  */
;    Elf32_Half    e_shstrndx;         /* Section header string table index */
;} Elf32_Ehdr;
;
;------------+---------+-----+-------------------
; 数据类型名称 | 字节大小 | 对齐 |        意义
;------------+---------+-----+-------------------
;Elf32_Half  |    2    |  2  | 无符号中等大小的整数  
;------------+---------+-----+-------------------
;Elf32_Word  |    4    |  4  | 无符号大整数
;------------+---------+-----+-------------------
;Elf32_Addr  |    4    |  4  | 无符号程序运行地址
;------------+---------+-----+-------------------
;Elf32_Off   |    4    |  4  | 无符号的文件偏移量
;------------+---------+-----+-------------------
;
;e_ident：是16字节大小的数组，用来表示elf字符等信息，
;   开头的4个字节是固定不变的，是elf文件的魔数，它们分别
;   是0x7f，以及字符串ELF的ASCII码：0x45, 0x4c, 0x46。
;   e_ident[4]：用来标识elf文件的类型
;       值为0表示该文件是不可识别类型
;       值为1表示该文件是32位elf格式的文件
;       值为2表示该文件是64位elf格式的文件
;   e_ident[5]：用来指定编码格式，其实就是指定大端字节序还是小端字节序
;       值为0表示非法编码格式
;       值为1表示小端字节序，即LSB(最低有效字节)
;       值为2表示大端字节序，即MSB(最高有效字节)
;   e_ident[6]：ELF头的版本信息，默认为1
;       值为0表示非法版本
;       值为1表示当前版本
;   e_ident[7~15]：暂且不用，保留，均初始化为0
kernel_init:
    xor eax, eax                                
    xor ebx, ebx                            ;EBX中记录程序头表地址
    xor ecx, ecx                            ;CX中记录程序头表中的
                                            ;program header的数量
    xor edx, edx                            ;DX中记录program header
                                            ;尺寸，即e_phentsize

    ;偏移文件开始42字节处的属性是e_phentsize，表示program header的尺寸
    mov dx, [KERNEL_BIN_BASE_MEM_ADDR + 42]     
    ;偏移文件开始28字节处的属性是e_phoff，表示第一个program header在文件中偏移量
    mov ebx, [KERNEL_BIN_BASE_MEM_ADDR + 28]    
    add ebx, KERNEL_BIN_BASE_MEM_ADDR       ;加上内核基址即为program header的基址

    ;偏移文件开始44字节处的属性是e_phnum，表示文件中program header的数量
    mov cx, [KERNEL_BIN_BASE_MEM_ADDR + 44]

.each_segment:
    ;p_type在程序头开始处，偏移量为0。若p_type等于PT_NULL，说明此程序头未使用。
    cmp byte [ebx + 0], PT_NULL
    je .pt_null

    ;为mem_cpy函数压入参数，参数从右往左依次压入，函数原型类似于mem_cpy(dst, src, size)
    ;压入size
    push dword [ebx + 16]                   ;程序头偏移16个字节是程序段大小p_filesz

    ;压入src
    mov eax, [ebx + 4]                      ;程序头偏移4个字节是程序头在elf文件中偏移
                                            ;量p_offset.
    add eax, KERNEL_BIN_BASE_MEM_ADDR       ;加上elf基址得到其程序头基址(存储位置)
    push eax

    ;压入dst
    push dword [ebx + 8]                    ;程序头偏移8个字节是程序段在内存中的起始
                                            ;虚拟地址p_vaddr(运行时位置)

    call mem_cpy
    add esp, 4*3                            ;清理压入栈中的三个参数

.pt_null:
    ;EDX存储程序头尺寸，即e_phentsize，EBX加上此值后指向下一个程序头
    add ebx, edx
    loop .each_segment

    ret
;----------------------------  END  ------------------------------




;---------------------------- START ------------------------------
;功能：逐个字节拷贝内存
;输入：栈中压入的三个参数(dst, src, size)，从右往左压入，即栈顶为dst
;   dst：目的地址
;   src：源地址
;   size：拷贝内存大小，单位为字节
;输出：
;   无
;
mem_cpy:
;cld相对应的指令是std，二者均是用来操作方向标志位DF(Direction Flag)。
;cld使DF复位，即DF=0
;std使DF置位，即DF=1
;这两个指令用于串操作指令中。通过执行cld或std指令可以控制方向标志DF，
;决定内存地址是增大（DF=0，向高地址增加）还是减小（DF=1，向地地址减小）。
;
;串操作指令寻址方式有点特殊：
;    源操作数和目的操作数分别使用寄存器(e)si和(e)di进行间接寻址。
;    每执行一次串操作，源指针(e)si和目的指针(e)di将自动进行修改：±1、±2、±4，
;    其对应的分别是字节操作、字操作和双字操作。
;    注：intel文档使用MOVSD传送双字，而GNU文档使用MOVSL传送双字。
;
;例如：
;    movsb：字节串传送指令，按字节传送数据。 通过SI和DI这两个寄存器
;           控制字符串的源地址和目标地址。
;    DF=0, SI=SI + 1, DI=DI + 1；
;    DF=1, SI=SI - 1, DI=DI - 1；
    cld                     ;置DF=0，向高地址增加

    ;保存上下文
    push ebp
    mov ebp, esp
    push ecx                ;rep指令使用到了ECX


    mov edi, [ebp + 8]      ;dst
    mov esi, [ebp + 12]     ;src
    mov ecx, [ebp + 16]     ;size

    ;rep指令：重复执行该指令后面的汇编指令，执行次数由寄存器ECX控制，不可单独使用。
    rep movsb               

    ;恢复上下文
    pop ecx
    pop ebp

    ret
;----------------------------  END  ------------------------------