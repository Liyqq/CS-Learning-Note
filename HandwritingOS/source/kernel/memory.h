#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H

#include "stdint.h"
#include "bitmap.h"


extern struct pmem_pool kernel_pmem_pool, user_pmem_pool;

/* 虚拟内存地址池，用于虚拟地址的管理 */
typedef struct vaddr_pool
{
    bitmap vaddr_bitmap;    // 管理虚拟地址的位图
    uint32_t base_vaddr;    // 管理的虚拟地址的起始地址
} vaddr_pool;


/* 内存池标记，用于标记内核内存池和用户内存池 */
typedef enum pool_flags
{
    PF_KERNEL = 0,  // 内核内存池
    PF_USER = 3,    // 用户内存池
} pool_flags;


#define PG_P_1  1   // 页表项或页目录项存在(Present)属性位
#define PG_P_0  0   // 页表项或页目录项存在(Present)属性位

#define PG_RW_R 0   // R/W属性位值, 读/执行
#define PG_RW_W 2   // R/W属性位值, 读/写/执行

#define PG_US_S 0   // U/S属性位值, 系统级
#define PG_US_U 4   // U/S属性位值, 用户级



void mem_init(void);

void* get_kernel_pages(uint32_t page_count);

void* malloc_page(pool_flags pf, uint32_t page_count);


#endif // __KERNEL_MEMORY_H