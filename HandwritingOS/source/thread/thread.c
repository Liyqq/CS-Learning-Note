/*
* @Author: Yooj
* @Date:   2021-12-27 01:35:38
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-31 23:49:19
*/

#include "stdint.h"
#include "string.h"
#include "global.h"
#include "debug.h"
#include "interrupt.h"
#include "print.h"
#include "memory.h"
#include "list.h"
#include "thread.h"


/* 页框大小，单位字节 */
#define PAGE_SIZE 4096


/* 主线程（进程）的PCB */
task_struct* main_thread;


/* 管理线程的队列 */
list ready_thread_queue;      // 就绪队列
list all_thread_queue;        // 所有线程队列
static list_elem* thread_tag; // 用于保存队列中的线程节点


/* 完成线程的切换 */
extern void switch_to(task_struct* current, task_struct* next);



/* 由kernel_thread函数去执行function(func_arg) */
static void kernel_thread(thread_func* function, void* func_args);

/* 调用此函数创建主线程 */
static void make_main_thread(void);




/**
 * kernel_thread - 由该函数调用执行线程中的函数
 * @param function  : 线程中待执行函数的函数名
 * @param func_args : 线程中待执行函数所需参数
 */
static void kernel_thread(thread_func* function, void* func_args)
{
    /** 
     * 执行function函数前应该开中断，
     * 避免后面的时钟中断被屏蔽，而无法调度其它的线程。
     */
    intr_enable(); 
    function(func_args);
}


task_struct* running_thread(void)
{
    uint32_t esp;
    asm ("mov %%esp, %0" : "=g"(esp));
    return (task_struct*)(esp & 0xfffff000);
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

    if (pthread == main_thread)
    {
        pthread->status = TASK_RUNNING; // main函数则直接指定线程为运行态
    }
    else
    {
        pthread->status = TASK_READY; // 除main函数外，其余新创建的线程指定为就绪态   
    }

    /* self_kstack是线程自己在内核态使用栈的栈顶地址 */
    pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PAGE_SIZE);
    pthread->priority = priority;
    pthread->ticks = priority;
    pthread->elapsed_ticks = 0;
    pthread->page_vaddr = NULL;
    pthread->stack_magic = THREAD_STACK_MAGIC; // 自定义的栈边界魔数
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

    /* 将线程加入就绪队列 */
    ASSERT(!list_has_elem(&ready_thread_queue, &pthread->general_queue_tag));
    list_append(&ready_thread_queue, &pthread->general_queue_tag);

    /* 将线程加入所有线程队列 */
    ASSERT(!list_has_elem(&all_thread_queue, &pthread->all_queue_tag));
    list_append(&all_thread_queue, &pthread->all_queue_tag);

    return pthread;   
}


/**
 * make_main_thread - 创建主线程
 */
static void make_main_thread(void)
{
    main_thread = running_thread();
    init_thread_pcb(main_thread, "main", 31);

    ASSERT(!list_has_elem(&all_thread_queue, &main_thread->all_queue_tag));
    list_append(&all_thread_queue, &main_thread->all_queue_tag);
}


/**
 * schedule - 完成队列中线程的调度
 */
void schedule(void)
{
    ASSERT(intr_get_status() == INTR_OFF);

    task_struct* current = running_thread();
    /* 若当前线程仅因为时间片用完，则将其放入就绪队列等待下一次调度 */
    if (current->status == TASK_RUNNING)
    {
        ASSERT(!list_has_elem(&ready_thread_queue, &current->general_queue_tag));
        list_append(&ready_thread_queue, &current->general_queue_tag);
        current->ticks = current->priority; // 重设当前线程的时间片，待下次调度使用
        current->status = TASK_READY;
    }
    else
    {
        // TODO: 将线程加入事件触发队列
    }

    ASSERT(!list_empty(&ready_thread_queue)); // 保证有可调动的线程存在

    thread_tag = NULL;
    thread_tag = list_pop_left(&ready_thread_queue);
    task_struct* next = elem2entry(task_struct, general_queue_tag, thread_tag);
    next->status = TASK_RUNNING;
    switch_to(current, next);
}


/**
 * thread_init - 初始化内存线程环境
 */
void thread_init(void)
{
    put_str("thread_init start\n");

    list_init(&ready_thread_queue);
    list_init(&all_thread_queue);
    make_main_thread(); // 将当前main函数创建为主线程

    put_str("thread_init done\n");
}