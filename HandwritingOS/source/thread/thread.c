/*
* @Author: Yooj
* @Date:   2021-12-27 01:35:38
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-28 18:32:30
*/

#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"
#include "thread.h"


/* 页框大小，单位字节 */
#define PAGE_SIZE 4096


/* 由kernel_thread函数去执行function(func_arg) */
static void kernel_thread(thread_func* function, void* func_args);




/**
 * kernel_thread - 由该函数调用执行线程中的函数
 * @param function  : 线程中待执行函数的函数名
 * @param func_args : 线程中待执行函数所需参数
 */
static void kernel_thread(thread_func* function, void* func_args)
{
    function(func_args);
}



/**
 * init_thread_pcb - 初始化线程的基本信息，即初始线程PCB
 * @param pthread  : 线程的PCB
 * @param name     : 线程属性——线程名
 * @param priority : 线程属性——优先级
 */
void init_thread_pcb(task_struct* pthread, char* name, int priority)
{
    memset(pthread, 0, sizeof(*pthread)); // 清空PCB所在物理内存页
    strcpy(pthread->name, name);
    pthread->status = TASK_RUNNING; // 基础版本：此处直接指定线程为运行态
    pthread->priority = priority;
    /* self_kstack是线程自己在内核态使用栈的栈顶地址 */
    pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PAGE_SIZE);
    pthread->stack_magic = 0x77777777; // 自定义的栈边界魔数
}


/**
 * init_thread_stack - 初始化线程栈thread_stack，将待执行的函数和参数放入
 *                     到thread_stack对应位置。
 * @param pthread   : 线程的PCB
 * @param function  : 线程中待执行函数的函数名
 * @param func_args : 线程中待执行函数所需参数
 */
void 
init_thread_stack(task_struct* pthread, thread_func function, void* func_args)
{
    /* 预留中断使用的中断栈，此栈由kernel_interrput.S中中断程序使用 */
    pthread->self_kstack -= sizeof(struct intr_stack);
    
    /**
     * 预留线程栈空间
     * 因为C语言中结构体是从小到大编址，即前面的成员为低地址，后面成员为高地址，
     * 而栈是从高地址向低地址生长的，因此，为了初始化thread_stack中成员，
     * 必须将self_kstack栈指针移动到预留了thread_stack大小内存的低地址处，
     * 相当于self_kstack指向了thread_stack的首地址（低地址处），然后再通过
     * self_kstack栈指针初始化thread_stack中成员
     */
    pthread->self_kstack -= sizeof(struct thread_stack);
    /* 并非按栈的操作逻辑压入，而是仿照调用时的压栈顺序赋值 */
    thread_stack* kthread_stack = (thread_stack*)pthread->self_kstack;
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_args = func_args;
    kthread_stack->ebp = 0;
    kthread_stack->ebx = 0;
    kthread_stack->esi = 0;
    kthread_stack->edi = 0;
}


/**
 * thread_create - 创建一个线程，其线程名为name，优先级为priority，
 *                 线程中所执行的函数为function(func_args)
 * @param  name      : 线程属性——线程名
 * @param  priority  : 线程属性——优先级
 * @param  function  : 线程中待执行函数的函数名
 * @param  func_args : 线程中待执行函数所需参数
 * @return           : task_struct类型的指针，指向创建完成线程的程序控制块PCB
 */
task_struct* 
thread_create(char* name, int priority, thread_func function, void* func_args)
{
    /* 无论内核线程还是用户线程，其PCB都位于内核空间 */
    task_struct* pthread = get_kernel_pages(1);
    init_thread_pcb(pthread, name, priority);
    init_thread_stack(pthread, function, func_args);

    /**
     * 指定pthread->self_kstack的值为栈顶，依次弹出保存的寄存器，
     * 然后使用ret调用（ret会从栈顶弹入返回地址到EIP中，从而改变了
     * 执行流，若在栈顶指定一个函数的开始执行地址便可实现使用ret调
     * 用函数），此处栈顶值是由init_thread_stack函数在
     * self_kstack中指定的函数kernel_thread地址（即初始化
     * thread_stack中的
     * void (*eip)(thread_func* func, void* func_args);处
     * 定义的函数指针eip。
     *
     * pop操作是为了弹出栈中保存的值
     */
    asm volatile ("movl %0, %%esp;              \
                  pop %%ebp;                    \
                  pop %%ebx;                    \
                  pop %%edi;                    \
                  pop %%esi;                    \
                  ret" // 使用ret调用函数function
                  : 
                  : "g"(pthread->self_kstack) // pthread->self_kstack为栈顶
                  : "memory");

    return pthread;   
}