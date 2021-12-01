;内核加载器 Kernel Loader
;
;测试用加载器
;---------------------------------------------------------------------------
%include "boot.inc"
SECTION LOADER vstart=LOADER_BASE_MEM_ADDR
    
;;;;;;;;;; 直接向显卡内存文本模式区域写入字符 ;;;;;;;;;;
;显卡的文本模式中使用两个字节来表示一个屏幕上的字符。
;其中低字节为标准ASCII码，高字节用于控制字符的显示属性。
;
;low                                          high
; 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 
;+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
;|    Standard ASCII     |B  G  R  I  B  G  R  K |
;+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
;高字节中低4位用于控制字符前景色，高4位用于控制字符背景色。
;I：表示字符显示的亮度,1表示高亮，相当于颜色变浅。
;K：表示字符是否闪烁，1表示闪烁。
;控制输出字符串"Hello MBR"
    mov byte [gs:0x00], 'H'
    mov byte [gs:0x01], 0xa4;(1010 0100)：a表示绿色背景并且字符闪烁，4表示前景色为红色

    mov byte [gs:0x02], 'e'
    mov byte [gs:0x03], 0xa4

    mov byte [gs:0x04], 'l'
    mov byte [gs:0x05], 0xa4

    mov byte [gs:0x06], 'l'
    mov byte [gs:0x07], 0xa4

    mov byte [gs:0x08], 'o'
    mov byte [gs:0x09], 0xa4

    mov byte [gs:0x0A], ' '
    mov byte [gs:0x0B], 0x7f;(0111 1111)：f表示白色背景字符不闪烁，0表示前景色为亮白色

    mov byte [gs:0x0C], 'L'
    mov byte [gs:0x0D], 0x9a;(1001 1010)：9表示蓝色背景并且字符闪烁，a表示前景色为浅渌色

    mov byte [gs:0x0E], 'O'
    mov byte [gs:0x0F], 0x9a

    mov byte [gs:0x10], 'A'
    mov byte [gs:0x11], 0x9a

    mov byte [gs:0x12], 'D'
    mov byte [gs:0x13], 0x9a

    mov byte [gs:0x14], 'E'
    mov byte [gs:0x15], 0x9a

    mov byte [gs:0x16], 'R'
    mov byte [gs:0x17], 0x9a

    mov byte [gs:0x18], '!'
    mov byte [gs:0x19], 0x9a

;;;;;;;;;; 直接向显卡内存文本模式区域写入字符结束 ;;;;;;;;;;
    
    jmp $                   ;程序悬停于此
