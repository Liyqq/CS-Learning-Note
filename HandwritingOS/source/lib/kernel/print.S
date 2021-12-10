;实现打印函数
;
;1.实现打印单个字符的函数put_char
;
;---------------------------------------------------------------------------
;
%include "print.inc"

section .data

[bits 32]
section .text

global put_char



;把每一个寄存器分组视为一个寄存器数组，提供一个寄存器用于指定数组下标，
;再提供一个寄存器用于对索引所指向的数组元素（也就是寄存器）进行输入输
;出操作。这样用这两个寄存器就能够定位寄存器数组中的任何寄存器。
;
;这两个寄存器就是各组中的Address Register和Data Register。
;Address Register：作为数组的索引（下标）。
;Data Register：作为寄存器数组中该索引对应的寄存器，
;            它相当于所对应的寄存器的窗口，往此窗口
;            读写的数据都作用在索引所对应的寄存器上。
;
;所以，对这类分组的寄存器操作方法是：
;    先在Address Register中指定寄存器的索引值，用来确定
;    所操作的寄存器是哪个；
;    然后在Data Register寄存器中对所索引的寄存器进行读写操作。
;
;           CRT Controller Data Registers           
;+-------+-----------------------------------------+
;| Index |                Registers                |
;+-------+-----------------------------------------+
;|  00h  |   Horizontal Total Register             |
;|  01h  |   End Horizontal Display Register       |
;|  02h  |   Start Horizontal Blanking Register    |
;|  03h  |   End Horizontal Blanking Register      |
;|  04h  |   Start Horizontal Retrace Register     |
;|  05h  |   End Horizontal Retrace Register       |
;|  06h  |   Vertical Total Register               |
;|  07h  |   Overflow Register                     |
;|  08h  |   Preset Row Scan Register              |
;|  09h  |   Maximum Scan Line Register            |
;|  0Ah  |   Cursor Start Register                 |
;|  0Bh  |   Cursor End Register                   |
;|  0Ch  |   Start Address High Register           |
;|  0Dh  |   Start Address Low Register            |
;|  0Eh  |   Cursor Location High Register         |
;|  0Fh  |   Cursor Location Low Register          |
;|  10h  |   Vertical Retrace Start Register       |
;|  11h  |   Vertical Retrace End Register         |
;|  12h  |   Vertical Display End Register         |
;|  13h  |   Offset Register                       |
;|  14h  |   Underline Location Register           |
;|  15h  |   Start Vertical Blanking Register      |
;|  16h  |   End Vertical Blanking                 |
;|  17h  |   CRTC Mode Control Register            |
;|  18h  |   Line Compare Register                 |
;+-------+-----------------------------------------+ 



;---------------------------- START ------------------------------
;功能：将单个ASCII字符写入光标所在处
;输入：(char)
;   char：需要向屏幕打印的单个ASCII字符
;输出：
;   无
;
put_char:
;;;;;;;;;; 备份寄存器现场，配置视频段选择子 ;;;;;;;;;;
    pushad                      ;备份32位寄存器环境
    ;需要保证GS中为正确的视频段选择子，为保险起见，每次打印时都为GS赋值
    mov ax, SELECTOR_VIDEO      
    mov gs, ax
;;;;;;;;;; 备份寄存器现场，配置视频段选择子结束 ;;;;;;;;;;


;;;;;;;;;; 获取当前光标位置 ;;;;;;;;;;
;
;光标是字符的坐标，只不过该坐标不是二维的，而是一维的线性
;坐标，是屏幕上所有字符以0为起始的顺序。
; 
;在默认的80*25模式下，每行80个字符共25行，屏幕上可以容纳
;2000个字符，故该坐标值的范围是0~1999。第0行的所有字符坐
;标是0~24，第1行的所有字符坐标是25~49，以此类推，最后一
;行的所有字符是1975~1999。
;
;由于一个字符占用2字节，所以光标乘以 2 后才是字符在显存中的地址。
;
;--------------------
;
;CRT Controller Data Registers寄存器组中
;索引为0Eh的Cursor Location High Register寄存器和
;索引为0Fh的Cursor Location Low Register寄存器，
;都是8位长度，分别用来存储光标坐标的低8位和高8位地址。
;
;访问CRT寄存器组的寄存器，需要先往端口地址为0x3D4的
;Address Register寄存器中写入寄存器的索引，再从
;端口地址为0x3D5的Data Register寄存器读、写数据。
; 
    ;获取光标高8位
    mov dx, 0x03d4
    mov al, 0x0e 
    out dx, al

    mov dx, 0x03d5
    in al, dx
    mov ah, al

    ;获取光标低8位
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al

    mov dx, 0x03d5
    in al, dx

    ;将光标位置存入BX
    mov bx, ax
;;;;;;;;;; 获取当前光标位置结束 ;;;;;;;;;;


;;;;;;;;;; 从栈中获取打印的字符 ;;;;;;;;;;
    ;pushad压入4×8＝32字节，加上主调函数的返回地址4字节，故ESP+36字节
    mov ecx, [esp + 4*8 + 4]
;;;;;;;;;; 从栈中获取打印的字符结束 ;;;;;;;;;;


;;;;;;;;;; 判断字符是否为控制字符(回车、换行和退格)或可见字符 ;;;;;;;;;;
    ;对于回车符CR(carriage_return)、换行符LF(line_feed)和
    ;退格键BS(backspace)这样的不可见字符，按其实际控制意义处理。
    ;CR=0x0d, LF=0x0a，BS=0x08
    cmp cl, 0x0d
    jz .is_carriage_return

    cmp cl, 0x0a
    jz .is_line_feed

    cmp cl, 0x08
    jz .is_backspace

    jmp .put_other
;;;;;;;;;; 判断字符是否为控制字符(回车、换行和退格)或可见字符结束 ;;;;;;;;;;


;;;;;;;;;; 控制字符退格处理流程 ;;;;;;;;;;
.is_backspace:
    ;在处理退格符，也就是将光标回撤到行首的方法是
    ;将光标向回移动1位，将该处的字符用空格覆盖。
    ;
    ;本质上只要将光标移向前一个显存位置即，后面再
    ;输入的字符自然会覆盖此处的字符，但有可能在键入
    ;backspace后并不再键入新的字符，这时在光标已
    ;经向前移动到待删除的字符位置，但字符还在原处，
    ;这就显得好怪异，所以此处添加了空格或空字符0
    dec bx                      ;回退一格
    shl bx, 1                   ;乘2转换为(字符, 属性)格式坐标

    mov byte [gs:bx], 0x20      ;将待删除的字节补为空格或0都可
    inc bx
    mov byte [gs:bx], 0x07      ;设置字符属性为黑底白字

    shr bx, 1                   ;除2还原BX的光标坐标值

    jmp .set_cursor
;;;;;;;;;; 控制字符退格处理流程结束 ;;;;;;;;;;


;;;;;;;;;; 打印字符处理流程 ;;;;;;;;;;
.put_other:
    shl bx, 1                   ;乘2转换为(字符, 属性)格式坐标
    
    mov [gs:bx], cl             ;设置打印的字符
    inc bx
    mov byte [gs:bx], 0x07      ;设置字符属性为黑底白字

    shr bx, 1                   ;除2还原BX的光标坐标值，余数丢弃
    inc bx                      ;打印字符后，新的光标值
    
    ;在单独的打印字符处理流程中也要判断在打印字符后的新光标位置
    ;是否超出了屏幕范围而需要滚屏。即在最后一行中的任意一个位置
    ;打印字符后，新光标超出屏幕范围。
    cmp bx, 2000                
    jl .set_cursor              ;如果小于2000说明不用滚屏，
                                ;直接设置光标即可
;;;;;;;;;; 打印字符处理流程结束 ;;;;;;;;;;


;;;;;;;;;; 控制字符回车和换行处理流程 ;;;;;;;;;;
;不管参数是回车符，还是换行符，一律按我们平时所理解的
;回车换行符(CRLF)处理，Linux中就把换行符处理成回车+换行，
;即这两个动作的合成：光标回撤到行首+换到下一行。
.is_carriage_return:
.is_line_feed:
    ;在处理回车符，也就是将光标回撤到行首的方法是
    ;将光标坐标值BX对80求模，再用坐标值BX减去余
    ;数就是行首字符的位置。
    xor dx, dx                  ;DX是32位被除数的高16位，清0。
    mov ax, bx                  ;AX是32位被除数的低16位，
    mov si, 80                  ;SI作为16位除数
    div si

    sub bx, dx                  ;DX中保存的是32位除法的余数，
                                ;BX中光标坐标减去余数便是行首坐标

    ;在处理换行符，也就是将光标切换到下一行的方法是
    ;将当前光标坐标值加上每行的字符数80。
    add bx, 80

    ;在单独的回车换行处理流程中也要判断在将光标更新为下一行后
    ;是否超出了屏幕范围而需要滚屏。即在最后一行中的任意一个位置
    ;有回车或换行符都将导致滚屏。
    cmp bx, 2000
    jl .set_cursor              ;如果小于2000说明不用滚屏，
                                ;直接设置光标即可
;;;;;;;;;; 控制字符回车和换行处理流程结束 ;;;;;;;;;;


;;;;;;;;;; 滚屏操作处理流程 ;;;;;;;;;;
;屏幕行范围是0~24,滚屏的原理是将屏幕的
;1~24行搬运到0~23行,再将第24行用空格填充
.roll_screen:
    cld           
    mov ecx, 960                ;一共有2000-80=1920个字符需要搬运，
                                ;共1920*2=3840个字节，一次搬运4个
                                ;字节则需要搬运3840/4=960次
    mov esi, 0xb80a0            ;第1行行首显存映射地址
    mov edi, 0xb8000            ;第0行行首显存映射地址
    rep movsd                   ;一次搬运4个字节

    ;最后一行填充空格
    mov ebx, 3840
    mov ecx, 80                 ;一次清空2字节，共80次
.cls:
    mov word [gs:ebx], 0x0720   ;黑底白字的空格
    add ebx, 2
    loop .cls

    mov bx, 1920                ;将光标位置设置为最后一行的行首
;;;;;;;;;; 滚屏操作处理流程结束 ;;;;;;;;;;


;;;;;;;;;; 设置新的光标位置 ;;;;;;;;;;
;同获取光标位置一样使用CRT索引寄存器(0x03d4)和数据寄存器(0x03d5)
;对光标高8位寄存器(0x0e)和光标低8位寄存器(0x0f)输出BX中光标坐标
.set_cursor:
    ;设置高8位
    mov dx, 0x03d4              ;索引寄存器
    mov al, 0x0e                ;光标高8位寄存器
    out dx, al

    mov dx, 0x03d5              ;数据寄存器
    mov al, bh                  ;写入高8位
    out dx, al                  

    ;设置低8位
    mov dx, 0x03d4              ;索引寄存器
    mov al, 0x0f                ;光标低8位寄存器
    out dx, al

    mov dx, 0x03d5              ;数据寄存器
    mov al, bl                  ;写入低8位
    out dx, al
;;;;;;;;;; 设置新的光标位置结束 ;;;;;;;;;;

.put_char_done:
    popad
    ret

;----------------------------  END  ------------------------------
