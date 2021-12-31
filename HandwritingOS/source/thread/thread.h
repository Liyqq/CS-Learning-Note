#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H

#include "stdint.h"
#include "list.h"


/* 自定义线程通用函数类型thread_func，在线程实现函数中作为形参类型 */
typedef void thread_func(void*);


/* 进程或线程的状态 */
typedef enum task_status
{
    TASK_RUNNING, 
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED,
} task_status;


/**
 * 用于在中断发生时保护程序（进程或线程）的上下文环境：
 * 进程或线程被外部中断或软中断打断时，会按照次结构压入
 * 上下文寄存器（从ss~vec_no，因为ss是高地址，vec_no
 * 是低地址，栈从高地址向低地址的方向生成），中断处理
 * 程序中相应的出栈操作即是此结构的逆操作。
 *
 * 此栈在线程自己的内存栈中位置固定，在页的最顶端。
 */
typedef struct intr_stack
{
    uint32_t vec_no;    // kernel_interrupt.S宏VECTOR中push %1压入的中断号
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy; 
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    /* 以下将由CPU从低特权级转入高特权级时压入 */
    uint32_t err_code;
    void (*eip) (void);
    uint32_t cs;
    uint32_t eflags;
    void* esp;
    uint32_t ss;
} intr_stack;


#define THREAD_STACK_MAGIC 0x77777777

/**
 * 线程栈thread_stack
 * 此栈是线程在运行过程中所使用的栈，区别于内核栈，用于存储线程中待执行的函数。
 * 此结构在线程自己的内核栈中位置不固定，用在switch_to时保存线程的上下文环境，
 * 实际位置取决于实际运行情况。
 */
typedef struct thread_stack
{
    /**
     * 根据C语言的ABI规范EBP、EBX、EDI、ESI和ESP五个寄存器
     * 归主调函数所用，其余的寄存器归被调函数所用。
     * 换句话说，不管被调函数中是否使用了这5个寄存器，
     * 在被调函数执行完后，这5个寄存器的值不该被改变。
     * 
     * 因此被调函数必须为主调函数保护好这5个寄存器的值，
     * 即被调函数必须在自己的栈中存储这些寄存器的值，
     * 保存的位置即为这个栈结构。
     */
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;
    // ESP寄存器的值会由C语言函数调用约定来保证

    /**
     * 线程第一次执行时，EIP指向被调函数kernel_thread，
     * 其余时候EIP指向switch_to的返回地址。
     */
    void (*eip)(thread_func* func, void* func_args);


    /* 以下仅在线程第一次被调度上CPU时使用 */
    void (*unused_ret_addr); // 仅为占位，在返回地址所在的栈帧占个位置
    thread_func* function;   // 由kernel_thread所调用的函数名
    void* func_args;         // 由kernel_thread所调用的函数所需参数

} thread_stack;



/* 进程或线程的PCB(Progress Control Block) */
typedef struct task_struct
{
    uint32_t* self_kstack;       // 各个内核线程都拥有自己的内核栈
    enum task_status status;     // 线程状态
    char name[16];               // 线程名
    uint8_t priority;            // 线程优先级
    uint8_t ticks;               // 线程获取到的CPU时间片长度(以时钟周期计数)
    uint32_t elapsed_ticks;      // 此线程到目前为止总共运行的时钟周期数
    list_elem general_queue_tag; // 标记线程为一般线程队列中的结点
    list_elem all_queue_tag;     // 标记线程为所有线程队列中的结点
    uint32_t* page_vaddr;        // 进程自身页表的虚拟地址，线程此值为NULL
    uint32_t stack_magic;        // 用此魔数做为栈的边界标记，用于检测栈的溢出
} task_struct;




void init_thread_pcb(task_struct* pthread, char* name, int priority);


void 
init_thread_stack(task_struct* pthread, thread_func function, void* func_args);


task_struct* 
thread_create(char* name, int priority, thread_func function, void* func_args);


task_struct* running_thread(void);


void schedule(void);


void thread_init(void);

#endif // __THREAD_THREAD_H