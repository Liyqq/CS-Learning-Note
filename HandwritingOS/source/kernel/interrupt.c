/*
* @Author: Yooj
* @Date:   2021-12-13 01:04:47
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-17 17:56:31
*/

#include "stdint.h"
#include "global.h"
#include "interrupt.h"
#include "io.h"
#include "print.h"


#define IDT_DESC_COUNT 0x21 // 目前总共支持的中断数


/**
 * ----------------------------------- PIC编程 -----------------------------------
 * 
 * 在8259A可以正常工作之前，必须首先设置初始化命令字ICW(Initialization Command Words)
 * 寄存器组的内容。而在其工作过程中，则可以使用写入操作命令字OCW(Operation Command Words)
 * 寄存器组来随时设置和管理8259A的工作方式。
 *
 * 
 * A0线用于选择操作的寄存器,在PC/AT微机系统中，
 *     当A0=0时芯片的端口地址是0x20(主芯片)和0xA0(从芯片)
 *     当A0=1时端口就是0x21(主芯片)和0xA1(从芯片)
 *
 * ------------------------------------ ICW ------------------------------------
 * 对ICW1和ICW2的设置是必需的。而只有当系统中包含多片8259A芯片并且是级联的情况下才需要对ICW3
 * 进行设置。这需要在ICW1的设置中明确指出。 另外，是否需要对ICW4进行设置也需要在ICW1中指明。
 * ------------------------------------ ICW ------------------------------------
 * 
 * ------------------------------ ICW1 ------------------------------
 * 当发送的字节第5比特位(D4)=1，并且地址线A0=0时，表示是对ICW1编程。
 * 此时对于PC/AT微机系统的多片级联情况下，8259A主芯片的端口地址是0x20，
 * 从芯片的端口地址是0xA0。
 * 
 *  A0      D7   D6   D5   D4   D3   D2   D1   D0
 * +---+  +----+----+----+----+----+----+----+----+
 * | 0 |  | A7 | A6 | A5 | 1  |LTIM|ADI |SNGL|ICW4|
 * +---+  +----+----+----+----+----+----+----+----+
 * A7~A5：在MCS80/85中表示中断服务过程的页面起始地址，与ICW2中
 *        的A15~A8共同组成，对8086/88系列机无用，置0即可。
 * D4：ICW1标记，恒为1
 * LTIM：level、edge triggered mode，中断检测触发方式，
 *         LTIM=0表示边沿触发
 *         LTIM=1表示电平触发
 * ADI：call address interval，MCS80/85中用于call指令的调用
 *      间隔，对8086/88系列机无用，置0即可。
 * SNGL：single，表示使用单片还是多片。
 *      SNGL=1，表示单片
 *      SNGL=0，表示级联(cascade)，此模式下要涉及到主片(1个)和
 *              从片(多个)用哪个IRQ接口互相连接的问题，此时主片
 *              和从片也是需要ICW3的。
 * ICMW4：是否要写入ICW4，并不是所有的ICW初始化控制字都需要用到。
 *       ICW4=1表示需要在后面写入ICW4
 *       ICW4=0表示不需要。
 *       注意，x86系统ICW4 必须为1。
 *
 * 
 * ------------------------------ ICW2 ------------------------------
 * ICW2用来设置起始中断向量号的高5位，即硬件IRQ接口到逻辑中断向量号的映射。
 * 在设置了ICW1之后，当A0=1时表示对ICW2进行设置。此时对于PC/AT微机系统的
 * 多片级联情况下，8259A主芯片的端口地址是0x21，从芯片的端口地址是0xA1。
 * 
 *  A0      D7   D6   D5   D4   D3   D2   D1   D0
 * +---+  +----+----+----+----+----+----+----+----+
 * | 1 |  | T7 | T6 | T5 | T4 | T3 |A10 | A9 | A8 |
 * +---+  +----+----+----+----+----+----+----+----+
 * T7~T3：在使用8086/88处理器的系统或兼容系统中的中断号
 *        的高5位，与8259A芯片自动设置的低3位（8259A按
 *        IR0~IR7三位编码值自动填入）组成一个8位的中断号。
 *        8259A在收到第2个中断响应脉冲INTA时会把此中断号
 *        送到数据总线上，以供CPU读取。
 *
 *        Linux-0.11系统把主片的ICW2设置为0x20，表示主片
 *        中断请求0~7级对应的中断号是0x20~0x27；把从片的
 *        ICW2设置成0x28，表示从片中断请求8~15级对应的中断
 *        号是0x28~0x2F。
 * 注意：D7 ~ D0对应MCS80/85中表示中断服务过程的页面起始地址的A15~A8
 * 
 *
 *
 * ------------------------------ ICW3 ------------------------------
 * ICW3仅在级联的方式下才需要（如果ICW1中的SNGL为0），用来设置主片和从片用
 * 哪个IRQ接口互连。由于主片和从片的级联方式不一样，主片和从片的ICW3都有自己
 * 不同的结构。主芯片的端口地址是0x21，从芯片的端口地址是0xA1。
 *
 * 主片
 *  A0      D7   D6   D5   D4   D3   D2   D1   D0
 * +---+  +----+----+----+----+----+----+----+----+
 * | 1 |  | S7 | S6 | S5 | S4 | S3 | S2 | S1 | S0 |
 * +---+  +----+----+----+----+----+----+----+----+
 * 对于主片，ICW3中Si的值：
 *     Si=1表示那一位对应的IRQ接口用于连接从片
 *     Si=0 表示接外部设备。
 *     比如，若主片IRQ2和IRQ5接有从片，则主片的ICW3=0010_0100，
 *
 * 从片
 *  A0      D7   D6   D5   D4   D3   D2   D1   D0
 * +---+  +----+----+----+----+----+----+----+----+
 * | 1 |  | 0  | 0  | 0  | 0  | 0  |ID2 |ID1 |ID0 |
 * +---+  +----+----+----+----+----+----+----+----+
 * 对于从片，ID2~ID0三个比特位对应各从片的标识号，即连接到主片的中断级。
 * 当某个从片接收到级联线(CAS2—CAS0)输入的值与自己的ID2~ID0相等时，
 * 表示此从片被选中。此时该从片应该向数据总线发送自己当前被选中的中断请求的中断号。
 *
 * Linux-0.11内核把8259A主片的ICW3设置为0x04，即S2=1，其余各位为0。
 * 表示主芯片的IR2引脚连接一个从芯片。从芯片的ICW3被设置为0x02，即其标
 * 识号为2。表示此从片连接到主片的IR2引脚。
 * 因此，中断优先级的排列次序为：0级最高，1级次之，接下来是从片上的8~15级，
 * 最后是主片的3~7级。
 *
 * ------------------------------ ICW4 ------------------------------
 * 当ICW1的位0(IC4)置位时，表示需要ICW4。
 * 地址线A0=1，主芯片的端口地址是0x21，从芯片的端口地址是0xA1。
 * 
 *  A0      D7   D6   D5   D4   D3   D2   D1   D0
 * +---+  +----+----+----+----+----+----+----+----+
 * | 1 |  | 0  | 0  | 0  |SFNM|BUF |M/S |AEOI|μPM |
 * +---+  +----+----+----+----+----+----+----+----+
 * ICW4的D7~D5位未定义，置为0即可
 * SFNM；表示特殊全嵌套模式(Special Fully Nested Mode)，
 *       SFNM=0表示全嵌套模式
 *       SFNM=1表示特殊全嵌套模式
 * BUF：表示本8259A芯片是否工作在缓冲模式。
 *      BUF=0表示工作在非缓冲模式
 *      BUF=1表示工作在缓冲模式
 * M/S：当多个8259A级联时，如果工作在缓冲模式下，M/S用来规定本
 *      8259A是主片，还是从片。
 *      M/S=1表示是主片
 *      M/S=0表示是从片。
 *      注意，若芯片工作在非缓冲模式(BUF=0)，M/S位无效。
 * AEOI：表示自动结束中断(Auto End Of Interrupt)，8259A在
 *       收到中断结束信号时才能继续处理下一个中断，此项用来设置
 *       是否要让8259A 自动把中断结束。
 *       AEOI=0，表示非自动，即手动结束中断，可以在中断处理程序中
 *             或主函数中手动向8259A的主、从片发送EOI信号。
 *       AEOI=1，表示自动结束中断。
 * μPM：表示微处理器类型(microprocessor)，此项是为了兼容老处理器。
 *      μPM=0，表示8080或8085处理器
 *      μPM=1，表示x86处理器
 *
 * Linux-0.11内核送往8259A主芯片和从芯片的ICW4命令字的值均为0x01。
 * 表示8259A芯片被设置成普通全嵌套、非缓冲、非自动结束中断方式，并且
 * 用于8086及其兼容系统。
 *
 * ------------------------------------ OCW ------------------------------------
 * 在对8259A设置了初始化命令字后，芯片就已准备好接收设备的中断请求信号了。但在8259A工作期间，
 * 也可以利用操作命令字OCW1~OCW3来监测8259A的工作状况，或者随时改变初始化时设定的8259A的工
 * 作方式。需要说明的是，与初始化命令字ICW1～ICW4需要按规定的顺序进行设置不同，操作命令字
 * OCW1～OCW3的设置没有规定其先后顺序，使用时可根据需要灵活选择不同的操作命令字写入到8259A中。
 * ------------------------------------ OCW ------------------------------------
 * 
 * ------------------------------ OCW1 ------------------------------
 * OCW1用于对8259A的中断屏蔽寄存器IMR进行读/写操作。地址线A0需为1。
 * 主芯片的端口地址是0x21，从芯片的端口地址是0xA1。
 *
 *  A0      D7   D6   D5   D4   D3   D2   D1   D0
 * +---+  +----+----+----+----+----+----+----+----+
 * | 1 |  | M7 | M6 | M5 | M4 | M3 | M2 | M1 | M0 |
 * +---+  +----+----+----+----+----+----+----+----+
 * Mi=1，表示屏蔽对应中断请求级IRi，屏蔽高优先级并不会影响其他低优先级的中断请求。
 * Mi=0，表示允许IRi
 * 
 * 在Linux-0.11内核初始化过程中，代码在设置好相关的设备驱动程序后就会利用该操作
 * 命令字来修改相关中断请求屏蔽位。例如在软盘驱动程序初始化结束时，为了允许软驱设
 * 备发出中断请求，就会读端口0x21以取得8259A芯片的当前屏蔽字，然后and ~0x40
 * 来复位M6（软盘控制器连接到了中断请求IR6上），最后再写回中断屏蔽寄存器中。
 *
 * 
 * ------------------------------ OCW2 ------------------------------
 * OCW2用来设置中断结束方式和优先级模式。地址线A0需为0。
 * 主芯片的端口地址是0x20，从芯片的端口地址是0xA0。
 *
 *  A0      D7   D6   D5   D4   D3   D2   D1   D0
 * +---+  +----+----+----+----+----+----+----+----+
 * | 0 |  | R  | SL |EOI | 0  | 0  | L2 | L1 | L0 |
 * +---+  +----+----+----+----+----+----+----+----+
 * R：Rotation，表示中断优先级是否按循环方式设置。
 *    R＝1表示采用循环方式，这样优先级会在0～7内循环。如果SL为0，
 *       初始的优先级次序为IR0>IR1>IR2>IR3>IR4>IR5>IR6>IR7。
 *       当某级别的中断被处理完成后，它的优先级别将变成最低，将最
 *       高优先级传给之前较之低一级别的中断请求，其他依次类推。
 *       所以，可循环方式多用于各中断源优先级相同的情况，优先级
 *       通过这种循环可以实现轮询处理。
 *    R＝0表示采用非循环方式，即IRQ接口号越低，优先级越高。
 * SL：Specific Level，指定OCW2中的L2~L0是否有效。可以针对某个
 *     特定优先级的中断进行操作，优先级模式设置和中断结束都可以基于
 *     此开关做更细粒度的控制。
 *    SL＝1表示有效
 *    SL＝0表示无效
 * EOI：表示中断结束命令位。
 *     EOI＝1表示当前ISR寄存器的相应位清0。当ICW4中的AEOI为0时，
 *     ISR中的相应置位就要由该命令位来清除。
 * D4~D3：恒为0，是OCW2的标识。
 * L2~L0：在SL=1时配合R、SL、EOI的设置，用来确定一个中断优先级的编码。
 *        (L2,L1,L0)的8种编码000~111分别与IR0～IR7相对应。
 *
 * Linux-0.11内核仅使用该操作命令字在中断处理过程结束之前向8259A发送结束中断
 * EOI命令。所使用的OCW2值为0x20=0010_000，表示固定优先级、一般EOI。
 *
 * 
 * ------------------------------ OCW3 ------------------------------
 * OCW3用于设置或清除特殊屏蔽方式和读取寄存器状态(IRR和ISR)。地址线A0需为0。
 * 主芯片的端口地址是0x20，从芯片的端口地址是0xA0。
 *
 *  A0      D7   D6   D5   D4   D3   D2   D1   D0
 * +---+  +----+----+----+----+----+----+----+----+
 * | 0 |  | 0  |ESMM|SMM | 0  | 1  | P  | PR |RIS |
 * +---+  +----+----+----+----+----+----+----+----+
 * D7：未用的，置0即可。
 * ESMM与SMM：Enable Special Mask Mode和Special Mask Mode
 *           两者是组合在一起用的，用来启用或禁用特殊屏蔽模式。
 *           
 *           ESMM：是特殊屏蔽模式允许位，是个开关。
 *                ESMM=0，表示SMM无效。
 *                ESMM=1，表示SMM无效。
 *           SMM：是特殊屏蔽模式位。
 *                SMM=0，表示未工作在特殊屏蔽模式。
 *                SMM=1，并且ESMM=1表示正式工作在特殊屏蔽模式下。
 *
 *          注意：只有在启用特殊屏蔽模式时，特殊屏蔽模式才有效。
 * D4~D3：恒为01，是OCW3的标识，8259A通过这两位判断是哪个控制字。
 * P：Poll command，查询命令，
 *   P=1，表示设置8259A为中断查询方式，这样就可以通过读取寄存器，
 *        如IRS来查看当前的中断处理情况。
 *   P=0，表示不设置8259A为中断查询方式。
 * RR：Read Register，读取寄存器命令。它和RIS位是配合在一起使用的。
 *    RR=1，表示可以读取寄存器。
 *    RR=1，表示禁止读取寄存器。
 * RIS：Read Interrupt register Select，读取中断寄存器选择位，
 *     用此位选择待读取的寄存器。类似显卡寄存器中的索引。
 *     RIS=1，表示选择ISR寄存器。
 *     RIS=0，表示选择IRR寄存器。
 *     这两个寄存器能否读取，前提是RR=1。
 *
 * 在Linux-0.11 内核中并没有用到该操作命令字。
 *
 * ------------------------------------------------------------------
 *
 * 8259A就2个端口地址，它是如何识别4个ICW和3个OCW的？
 *     ICW1、OCW2和OCW3是用偶地址端口0x20（主片）或 0xA0（从片）写入。
 *     ICW2~ICW4和OCW1是用奇地址端口0x21（主片）或 0xA1（从片）写入。
 * 4个ICW要保证一定的次序写入，所以8259A就知道写入端口的数据是什么了。
 *
 * OCW的写入与顺序无关，并且ICW1和OCW2、OCW3的写入端口是一致的，8259A通过辨识
 * 各控制字中的第D4~D3标识位，通过这两位的组合来唯一确定某个控制字，如下表：
 * +----------+----------+----------+
 * | Control  |    D4    |    D3    |
 * | command  |          |          |
 * +----------+----------+----------+
 * |   ICW1   |    1     |    x     |
 * +----------+----------+----------+
 * |   OCW2   |    0     |    0     |
 * +----------+----------+----------+
 * |   OCW3   |    0     |    1     |
 * +----------+----------+----------+
 *
 * ------------------------------------------------------------------
 *
 * 
 * 8259A的编程就是写入ICW和OCW，下面总结下写入的步骤：
 *     对于8259A的初始化必须最先完成，步骤是：
 *         1.无论8259A是否级联，ICW1和ICW2是必须要有的，并且要顺序写入。
 *         2.只有当ICW1中的SNGL位为0时表示级联，级联就需要设置主片和从片，
 *           这才需要在主片和从片中各写入ICW3。
 *           注意，ICW3的格式在主片和从片中是不同的。
 *         3.只能当ICW1中的ICW4为1时，才需要写入ICW4。不过，x86系统ICW4必须为1。
 *     在以上初始化8259A之后可以用OCW对它操作。
 * 
 */
/********** PIC端口号 **********/ 
#define PIC_M_CTRL_PORT 0x20    // 主片的控制端口号为0x20
#define PIC_M_DATA_PORT 0x21    // 主片的数据端口号为0x21

#define PIC_S_CTRL_PORT 0xa0    // 从片的控制端口号为0xA0
#define PIC_S_DATA_PORT 0xa1    // 从片的数据端口号为0xA1




/*
高32位
 31                            16 15 14 13 12 11       8  7  6  5  4        0
+--------------------------------+--+--+--+--+--+--+--+--+--+--+--+----------+
|      interrupt func offset     |  |     |  |           |  |  |  |          |   
|        in target segment       |P | DPL |S |  T Y P E  |0 |0 |0 | Reserved |
|          31 ~ 16 bits          |  |     |  |           |  |  |  |          |
+--------------------------------+--+--+--+--+--+--+--+--+--+--+--+----------+
                                 |<- attribute(8 bits) ->|

低32位
 31                            16 15                             0 
+--------------------------------+--------------------------------+
|      segment description       |      interrupt func offset     |
|          selector of           |        in target segment       |
|    interrupt func(16 bits)     |          31 ~ 16 bits          |
+--------------------------------+--------------------------------+
*/
/* 中断门描述符结构体 */
struct gate_desc
{
    uint16_t    func_offset_low_word;
    uint16_t    selector;
    uint8_t     dcount;                 // 双字计数字段，门描述符中第四字节，固定值
    uint8_t     attribute;
    uint16_t    func_offset_high_word;
};


/* IDT是中断描述符表，本质上是个中断门描述符数组 */
static struct gate_desc IDT[IDT_DESC_COUNT]; 

/* 声明引用定义在kernel.S中的中断处理函数入口数组 */
extern intr_handler intr_entry_table[IDT_DESC_COUNT];

/* 用于保存异常的名字 */
char* intr_name[IDT_DESC_COUNT];

/**
 * 定义中断处理程序数组，在kernel.s中定义的intrXX_entry仅是中断处理程序入口，
 * 最终调用的是IDT_table中的C语言中断处理程序。
 * 相当于intr_entry_table[i]调用IDT_table[i]中的中断处理程序
 * 可见kernel/kernel_interrupt.S中的 call [IDT_table + %1*4]
 */
intr_handler IDT_table[IDT_DESC_COUNT];



/**
 * pic_init - 初始化可编程中断控制器8259A
 */
static void pic_init(void)
{
    /* 初始化主片 */
    outb(PIC_M_CTRL_PORT, 0x11); // ICW1(0x11=0001_0001)指定边沿触发，级联8259A，需要ICW4
    outb(PIC_M_DATA_PORT, 0x20); // ICW2指定中断向量起始号0x20，即IR[0~7]=0x20~0x27
    outb(PIC_M_DATA_PORT, 0x04); // ICW3(0x04=0000_0100)表示IR2引脚接从片
    outb(PIC_M_DATA_PORT, 0x01); // ICW4(0x01=0000_0001)表示8086模式，正常EOI

    /* 初始化从片 */
    outb(PIC_S_CTRL_PORT, 0x11); // ICW1(0x11=0001_0001)指定边沿触发，级联8259A，需要ICW4
    outb(PIC_S_DATA_PORT, 0x28); // ICW2指定中断向量起始号0x28，即IR[8~15]=0x28~0x2F
    outb(PIC_S_DATA_PORT, 0x02); // ICW3(0x02=0000_0010)设置从片连接到主片的IR2引脚
    outb(PIC_S_DATA_PORT, 0x01); // ICW4(0x01=0000_0001)表示8086模式，正常EOI

    /* 打开主片上IR0，也就是目前只接受时钟产生的中断 */
    outb(PIC_M_DATA_PORT, 0xfe); // 0xFE = 1111_1110
    outb(PIC_S_DATA_PORT, 0xff); // 0xFF = 1111_1111

    put_str("   pic_init done\n");
}


/**
 * make_idt_desc - 创建中断门描述符
 * @param p_gdesc : 中断门描述符指针，指向内存中一个中断描述符实体
 * @param attr    : 中断门描述符中属性字段(8bits)
 * @param func    : 中断门描述符中对应的中断服务程序在内存中的地址
 */
static 
void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler func)
{
    p_gdesc->func_offset_low_word = ((uint32_t)func & 0x0000ffff);
    p_gdesc->selector = SELECTOR_K_CODE;
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_high_word = ((uint32_t)func & 0xffff0000) >> 16;
}


/**
 * idt_desc_init - 初始化中断描述符表
 */
static void idt_desc_init(void)
{
    int i;
    for (i = 0; i < IDT_DESC_COUNT; ++i)
    {
        make_idt_desc(&IDT[i], IDT_DESC_ATTR_DPL_0, intr_entry_table[i]);
    }

    put_str("   idt_desc_init done\n");
}


/**
 * default_intr_handler - 默认的中断处理函数，用于异常出现时的处理
 * @param vector_num : 中断向量号，访问为0~255
 */
static void default_intr_handler(uint8_t vector_num)
{
    /** 
     * IRQ7(0x27)和IRQ15(0x2f)会产生伪中断，无法通过IMR屏蔽
     * 此处直接不处理。
     * 且0x2f是8259A从片上最后一个irq引脚，Intel保留。 
     * 
     */
    if ((vector_num == 0x27) || (vector_num == 0x2f))
    {
        return;
    }

    put_str("interrupt vector number: 0x");
    put_int(vector_num);
    put_char('\n');
}


/**
 * exception_init - 一般中断处理函数注册以及异常名称注册
 */
static void exception_init(void)
{
    int i;
    for (i = 0; i < IDT_DESC_COUNT; ++i)
    {
        /**
         * IDT数组中函数是进入中断后根据中断向量号调用的中断处理函数。
         * 默认为default_intr_handler，之后由register_handler来
         * 注册具体的中断处理函数
         */
        IDT_table[i] = default_intr_handler;
        intr_name[i] = "Unknown"; // 中断名默认为Unknown
    }

    intr_name[0] = "#DE Divide Error";
    intr_name[1] = "#DB Debug Exception";

    intr_name[2] = "NMI Interrupt";
    
    intr_name[3] = "#BP Break Point";
    intr_name[4] = "#OF Overflow Exception";
    intr_name[5] = "#BR BOUND Range Exceeded Exception";
    intr_name[6] = "#UD Invalid Opcode(UnDefined Opcode) Exception";
    intr_name[7] = "#NM Device Not Available(No Math Corprocessor) Exception";
    intr_name[8] = "#DF Double Fault Exception";
    intr_name[9] = "#MF Corprocessor Segment Overrun(reserved)";
    intr_name[10] = "#TS Invalid TSS Exception";
    intr_name[11] = "#NP Segment Not Present";
    intr_name[12] = "#SS Stack Fault Exception";
    intr_name[13] = "#GP General Protection Exception";
    intr_name[14] = "#PF Page-Fault Exception";
    // intr_name[15] IRQ15是Intel保留项，未使用。
    intr_name[16] = "#MF X87 FPU Floating-Point Error(Math Fault)";
    intr_name[17] = "#AC Alignment Check Exception";
    intr_name[18] = "#MC Machine-Check Exception";
    intr_name[19] = "#XF SIMD Floating-Point Exception";
    // intr_name[20~31] Intel保留项，未使用
    // intr_name[32~255] 可屏蔽中断
}






/**
 * idt_init - 初始化中断描述符表
 */
void idt_init(void) 
{
    put_str("idt_init start\n");
    
    idt_desc_init();    // 初始化中断描述符表
    exception_init();   // 异常名初始化并注册默认的中断处理函数
    pic_init();         // 初始化8259A可编程中断控制器

    /* 加载IDT */
    uint64_t idt_operand = (sizeof(IDT) - 1) | ((uint64_t)(uint32_t)IDT << 16);
    asm volatile("lidt %0" : : "m"(idt_operand));

    put_str("idt_init done\n");
}

