/*
* @Author: Yooj
* @Date:   2021-12-17 22:17:19
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-19 14:28:50
*/
#include "timer.h"
#include "io.h"
#include "print.h"


/**
 * 在8253内部有3个独立的计数器，分别是计数器0~计数器2，它们的端口分别是0x40~0x42。
 * 计数器又称为通道，每个计数器都完全相同，都是16位大小。各个计数器都有自己的一套
 * 寄存器资源，工作时自己用自己的，互不干涉。
 * 
 * 寄存器资源包括一个16位的计数初值寄存器、一个计数器执行部件和一个输出锁存器。
 *
 * 每个计数器都有三个引脚：CLK，GATE，OUT。
 * CLK：表示时钟输入信号，即计数器自己工作的节拍，也就是计数器自己的时钟频率。每当此
 *      引脚收到一个时钟信号，减法计数器就将计数值减1。连接到此引脚的脉冲频率最高为
 *      10MHz，8253为2MHz。
 * GATE：表示门控输入信号，在某些工作方式下用于控制计数器是否可以开始计数，在不同工
 *       作方式下GATE的作用不同。
 * OUT：表示计数器输出信号。当定时工作结束，也就是计数值为0时，根据计数器的工作方式，
 *      会在OUT引脚上输出相应的信号。
 *      此信号用来通知处理器或某个设备：定时完成。这样处理器或外部设备便可以执行相
 *      应的行为动作
 *
 * 计数初值寄存器：用来保存计数器的初始值，它是16位宽度，对8253初始化时写入的计数
 *              初始值就保存在计数初值寄存器。它的作用是为计数器执行部件准备初始
 *              计数值，之后的计数过程与它无关。
 * 计数器执行部件：是计数器中真正“计数”的部件，计数的工作是由计数器执行部件完成的，
 *              所以它才是真正实际的计数器。8283是个倒计时计数器，原因是计数器
 *              执行部件是个16位的减法计数器，它从初值寄存器中拿到起始值，载入
 *              到自己的寄存器后便开始递减计数。注意，计数过程中不断变化的值称为
 *              当前计数值，它保存在执行部件自己的寄存器中，初值寄存器中的值不受影响。
 * 输出锁存器：也称为当前计数值锁存器，用于把当前减法计数器中的计数值保存下来，其目
 *           的就是为了让外界可以随时获取当前计数值。计数器中的计数值是不断变化的，
 *           处理器无法直接从计数器中获取当前计数值。
 *           原因：计数器的使命就是通过计数的方式实现定时功能，必须要求精准，所以
 *           绝不能为了获取计数值而计数器停止计数。为了获取任意时刻的计数值，8253
 *           只有将它送到输出锁存器，此锁存器起到暂存寄存器的作用。这样处理器便能
 *           够从输出锁存器中获取瞬间计数值。
 * 
 * 计数初值寄存器、计数器执行部件和输出锁存器都是16位宽度的寄存器，所以高8位和低8位
 * 都可以单独访问。我们之后为其赋予初始计数值时就分为高8位和低8位分别操作。
 *
 * 三个计数器都有自己的用途，它们的作用见下表
 * +----------+------+------------------------------------------------------+
 * |Timer Name| Port |         Function                                     |
 * +----------+------+------------------------------------------------------+
 * |          |      |In PC, timer0 is used exclusively to generate the     |
 * |  Timer0  | 0x40 |real-time clock signal. It works in mode 3 and the    |
 * |          |      |maximum count value is 65536 when 0 is written to this|
 * |          |      |timer.                                                |
 * +----------+------+------------------------------------------------------+
 * |          |      |In PC, timer1 is dedicated to DRAM timing refresh     |
 * |  Timer1  | 0x41 |control. 128 refreshes in 2ms for PC/XT and 256       |
 * |          |      |refreshes in 4ms for PC/AT.                           |
 * |          |      |                                                      |
 * +----------+------+------------------------------------------------------+
 * |          |      |In PC, timer2 is dedicated to the internal speaker to |
 * |  Timer2  | 0x42 |produce different tones of sound, the principle is to |
 * |          |      |feed the speaker with different frequencies of square |
 * |          |      |waves.                                                |
 * +----------+------+------------------------------------------------------+
 *
 *
 * 控制字寄存器，其操作端口是0x43，它是8位大小的寄存器。控制字寄存器也称为模式控制寄存器，
 * 在控制字寄存器中保存的内容称为控制字，控制字用来设置所指定的计数器（通道）的工作方式、
 * 读写格式及数制。
 * 三个计数器是独立工作的，每个计数器都必须明确自己的控制模式才知道该怎样去工作（计数），
 * 它们各自的控制模式都要以控制字的形式在同一个控制字寄存器中设定，所以，控制字中有相关的
 * 字段来选定操作哪个计数器。
 * 控制字的结构如下所示
 *    7      6      5      4      3      2      1      0    
 * +------+------+------+------+------+------+------+------+
 * | SC1  | SC0  | RW1  | RW0  |  M2  |  M1  |  M0  | BCD  | 
 * +------+------+------+------+------+------+------+------+
 * SC1~SC0：是选择计数器位，即Select Counter，或者叫选择通道位，
 *          即Select Channel。在8253内部有3个独立的计数器，每
 *          个计数器都有自己的控制模式，但是这三个计数器的控制字
 *          是共用同一个控制字寄存器写入的，所以此处用SC1和SC0
 *          这两位去选择待操作的计数器，也就是此控制字用来设置
 *          哪个计数器。
 *          00：计数器0
 *          01：计数器1
 *          10：计数器2
 *          11：未定义
 * RW1~RW0：是读/写/锁存操作位，即Read/Write/Latch，用来设置
 *          待操作计数器（通道）的读写及锁存方式。计数器是16位宽度，
 *          当我们往计数器中写入计数初值时，或者读取计数器中的数值
 *          时，可以指定读写低8位，还是高8位。RW1和RW0这两位组合
 *          成4种读写方式。
 *          00：锁存数据供CPU读取
 *          01：只读写低8位
 *          10：只读写高8位
 *          11：先读写低8位，后读写高8位
 * M2~M0：工作方式（模式）选择位，即Method或Mode。每个计数器有
 *        6种不同的工作方式，即方式0~方式5。
 *        000：方式0，计数结束中断方式(Interrupt on Terminal Count)
 *        001：方式1，硬件可重触发单稳方式(Hardware Retriggerable One-Shot)
 *        x10：方式2，比率发生器(Rate Generator)
 *        x11：方式3，方波发生器(Square Wave Generator)
 *        100：方式4，软件触发选通(Software Triggered Strobe)
 *        101：方式5，硬件触发选通(Hardware Triggered Strobe)
 * BCD：数制位，用来指示计数器的计数方式是BCD码，还是二进制数。
 *     8253的各个计数器都有两种计数方式：二进制方式和十进制方式，
 *     其中十进制方式就是用BCD码来表示。即Binary-Coded Decimal ，
 *     十进制最大数为9，也就是需要用4位二进制数来表示1位十进制数。
 *     BCD=1，表示用BCD码来计数，如0x1234，则表示十进制数1234。
 *            BCD码的初始值范围是0~0x9999，0值则表示十进制10000。
 *     BCD=0，表示用二进制数来计数，如0x1234，则表示十进制4660。
 *            二进制数的初始值范围是0~0xFFFF，即十进制范围是
 *            0~65535，0值表示65536。
 *
 * 8253初始化步骤
 * 1.往控制字寄存器端口0x43中写入控制字用控制字为指定使用的计数器设置控制模式，
 * 控制模式包括该计数器工作时采用的工作方式、读写格式及数制。
 * 2.在所指定使用的计数器端口中写入计数初值计数初值要写入所使用的计数器所在的端口，
 * 即若使用计数器0，就要把计数初值往0x40端口写入，若使用的是计数器1，就要把计数初
 * 值往0x41端口写入，依次类推。计数初值寄存器是16位，高8位和低8位可单独使用，所以
 * 初值是8位或16位皆可。若初值是8位，直接往计数器端口写入即可。若初值为16位，必须分
 * 两次来写入，先写低8位，再写高8位。
 */

#define INPUT_FREQUENCY     1193180
#define PIT_CTRL_PORT       0x43


/* 3个计时器编号及端口号 */
#define TIMER0_NO       0
#define TIMER0_PORT     0x40

#define TIMER1_NO       1
#define TIMER1_PORT     0x41

#define TIMER2_NO       2
#define TIMER2_PORT     0x42


/* 控制字读/写/锁存操作位，即Read/Write/Latch */
#define RWL_LATCH       0
#define RWL_RW_LOW8     1
#define RWL_RW_HIGH8    2
#define RWL_RW          3


/* 工作模式选择位，即Method或Mode */
#define MODE_0    0
#define MODE_1    1
#define MODE_2    2
#define MODE_3    3
#define MODE_4    4
#define MODE_5    5

/* 数制位 */
#define BCD     1
#define BIN     0

/**
 * frequency_set - 是把操作的计时器timer_no、读写锁属性rwl、计数器工作模式timer_mode
 *                 写入模式控制寄存器并赋予计时器的计数初值为counter_value。
 * @param timer_port  : 计时器端口号，用于指定初值timer_value的目的端口号
 * @param timer_no    : 用于在控制字中指定所使用的计时器号码，对应控制字中SC1~SC2位
 * @param rwl         : 用来设置计数器的读/写/锁存方式，对应于控制字中的RW1~RW0位
 * @param timer_mode  : 用来设置计数器的工作模式，对应于控制字中的M2~M0位
 * @param timer_value : 用来设置计数器的计数初值，此值为16位
 */
static void frequency_set(uint8_t timer_port, 
                          uint8_t timer_no, 
                          uint8_t rwl, 
                          uint8_t timer_mode, 
                          uint16_t timer_value)
{
    /* 向控制字寄存器0x43中写入控制字 */
    outb(PIT_CTRL_PORT, (uint8_t)((timer_no<<6) | (rwl<<4) | (timer_mode << 1)));
    
    /* 先写入timer_value低8位 */
    outb(timer_port, (uint8_t)(timer_value));
    
    /* 先写入timer_value高8位 */
    outb(timer_port, (uint8_t)(timer_value >> 8));
}


/**
 * timer_init - 初始化PIT8253
 */
void timer_init(void)
{
    put_str("timer_init start\n");

    /* timer_value = 1193180 / IRQ0中断信号频率 */
    uint16_t timer_value = INPUT_FREQUENCY / 1000;

    /* 设置8253的定时周期,也就是发中断的周期 */
    frequency_set(TIMER0_PORT, TIMER0_NO, RWL_RW, MODE_2, timer_value);
    
    put_str("timer_init done\n");
}