[bits 32]
section .text
global switch_to

;---------------------------- START ------------------------------
;功能：完成线程切换过程中current线程寄存器环境的保存，恢复next线程寄存器环境
;输入：(current, next)
;   current : 当前运行的线程的PCB指针
;   next    : 下一个即将运行的线程的PCB指针
;输出：
;   无
;
switch_to:
;;;;;;;;;; 保存current寄存器环境 ;;;;;;;;;;
    push esi
    push edi
    push ebx
    push ebp

    mov eax, [esp + 20]     ;获取栈中传入参数current，current=[esp + 20]
    mov [eax], esp          ;保存栈顶指针esp，task_struct的self_kstack字段，
                            ;self_kstack字段其在task_struct中的偏移量为0，
                            ;因此直接往current（线程PCB）处存esp(4字节)即可。
;;;;;;;;;; 保存current寄存器环境结束 ;;;;;;;;;;


;;;;;;;;;; 恢复next寄存器环境 ;;;;;;;;;;
    mov eax, [esp + 24]     ;获取栈中传入参数next，next=[esp + 24]
    mov esp, [eax]          ;PCB的第一个字段是self_kstack，用来记录0级栈顶指针。
                            ;用来上CPU时恢复0级栈，0级栈中保存了进程或线程所有信息，
                            ;包括3级栈指针
    pop ebp
    pop ebx
    pop edi
    pop esi
;;;;;;;;;; 恢复next寄存器环境结束 ;;;;;;;;;;

    ret                     
; 若当前线程是第一次运行则ret返回的是kernel_thread的地址，
; 这是由初始化线程PCB函数init_thread_pcb在初始化
; self_kernel时指定的eip为kernel_thread函数地址确定的。
; 
; 若当前线程并非第一次运行，ret返回的是self_kstack中
; switch_to的调用者schedule函数返回地址（schedule函数
; 的调用者为intr_timer_handler函数，执行完
; intr_timer_handler函数则返回到kernel_interrput中
; 的中断入口函数处恢复中断前线程运行的上下文，然后退出中断
; 入口函数。至此，线程恢复到上一次的运行状态）
;  返回链如下所示
;/////////////////////////////////////////////////////////////////////////////// 
; 
;                   +--- (first in CPU) ---> kernel_thread --> function ----+
;                   |                                                       |
;  START ====> ret -+                                                       |
;               ^   |                                                       |
;               |   +--> schedule --> intr_timer_handler --> intr20_entry   |
;               |                                               |           |
;               |                                               |           |
;               |                                               |           |
;           switch_to                                           |           |
;               ^                                               +-----------+
;               |                                               |
;            schedule                                           |
;               ^                                               |
;               |                                               |
;       intr_timer_handler                                      |
;               ^                                               |
;               |                                               |
;             true                                              |
;               |                                               |
;  (-------- schedule --------)                                 |
;  ( other thread->ticks == 0 ) ? ----false----+                |
;  (-------- schedule --------)                |                |
;               ^                              |                |  
;               |                              |                |  
;       intr_timer_handler                     |                |
;               ^                   (other thread->ticks--)     |  
;               |                              |                |  
;          intr20_entry                        |                |
;               ^                              |                |  
;               |                              |                |  
;      other thread running <------------------+                |
;               ^                                               |  
;               |                                               |  
;      switch_to(save context)                                  |
;               ^                                               |  
;               |                                              \|/         
;            schedule                               (next) thread running <--+
;               ^                                               |            |  
;               |                                               |            |  
;             true                                              |            |  
;               |                                               |            |  
;  (-------- schedule --------)                                \|/           |  
;  ((next) thread->ticks == 0 ) ? <-- intr_timer_handler <-- intr20_entry    |  
;  (-------- schedule --------)                                              |  
;               |                                                            |  
;             false                                                          |  
;               |                                                            |  
;               +----------------- ((next) thread->ticks--) -----------------+  
; 
;/////////////////////////////////////////////////////////////////////////////// 
;----------------------------  END  ------------------------------