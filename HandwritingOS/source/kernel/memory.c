/*
* @Author: Yooj
* @Date:   2021-12-19 21:25:07
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-26 17:57:00
*/

#include "stdint.h"
#include "print.h"
#include "bitmap.h"
#include "debug.h"
#include "string.h"
#include "memory.h"


/* 页框大小，单位字节 */
#define PAGE_SIZE 4096

/**
 * 0xc009f000为内核主线程栈顶，0xc009e000是内核主线程的PCB基址.
 * 一个页框大小(4KB)的位图可表示128M/4KB=4K*8内存，
 * 因此位图位置安排在地址0xc009a000处，
 * 这样本系统最大支持4个页框的位图，即512M。
 */
#define MEM_BITMAP_BASE_MEM_ADDR 0xc009a000

/** 
 * 0xc0000000=3G是内核的虚拟地址的基址. 
 * 0x100000意指跨过低端1M内存，使虚拟地址在逻辑上连续。
 */
#define KERNEL_HEAP_BASE_MEM_ADDR 0xc0100000

/* 获取高10位页目录项索引 */
#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
/* 获取次10位页表项索引 */
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)




/* 物理内存池结构，用于管理物理内存，用于内核内存池和用户内存池 */
typedef struct pmem_pool
{
    struct bitmap pmem_bitmap;  // 管理物理内存池的位图
    uint32_t base_paddr;        // 管理的物理内存的起始地址
    uint32_t size;              // 内存池的容量(字节)
} pmem_pool;

/* 内核物理内存池和用户物理内存池 */
struct pmem_pool kernel_pmem_pool, user_pmem_pool;

/* 内核虚拟内存地址池 */
struct vaddr_pool kernel_vaddr_pool;




/* 静态函数声明 */
static void mem_pool_init(uint32_t all_mem);
static void print_mem_pool_info(void);
static void* vaddr_pool_get(pool_flags pf, uint32_t page_count);
static void* palloc(pmem_pool* m_pool);
static void mapping_vaddr_to_pmem(void* _vaddr, void* _page_paddr);


/**
 * mem_pool_init - 初始化内核和用户的物理内存池，同时初始化内核虚拟内存地址池。
 * @param all_mem : 计算机总共的内存容量(字节)
 */
static void mem_pool_init(uint32_t all_mem)
{
    put_str("   pmem_pool_init start\n");
    
    /** 
     *页表大小 = 1页的页目录表 + 第0和第768个页目录项指向同一个页表 
     *          + 第769~1022个页目录项共指向254个页表 = 共256个页框
     */
    uint32_t page_table_size = PAGE_SIZE * 256;

    uint32_t used_mem = page_table_size + 0x100000; // 0x100000指低端1M内存
    uint32_t free_mem = all_mem - used_mem;
    
    /** 
     * 1页大小为4k，不管剩余总内存是不是4k的倍数。
     * 因为对于以页为单位的内存分配策略，不足1页的内存不用考虑了。
     */
    uint16_t all_free_pages = free_mem / PAGE_SIZE; 
    uint16_t k_free_pages = all_free_pages / 2; // 内核内存和用户内存各一半
    uint16_t u_free_pages = all_free_pages - k_free_pages;

    /**
     * 为简化位图操作，余数不做处理。
     * 这样做的坏处是会丢内存；
     * 好处是不用做内存的越界检查，因为位图表示的内存少于实际物理内存。
     */
    uint32_t k_bitmap_len = k_free_pages / 8; 
    uint32_t u_bitmap_len = u_free_pages / 8;

    uint32_t k_base_paddr = used_mem;
    uint32_t u_base_paddr = k_base_paddr + k_free_pages*PAGE_SIZE;


    /******************** 初始化内核与用户物理内存池 **********************/
    kernel_pmem_pool.base_paddr = k_base_paddr;
    user_pmem_pool.base_paddr = u_base_paddr;

    kernel_pmem_pool.size = k_free_pages * PAGE_SIZE;
    user_pmem_pool.size = u_free_pages * PAGE_SIZE;

    kernel_pmem_pool.pmem_bitmap.bitmap_bytes_len = k_bitmap_len;
    user_pmem_pool.pmem_bitmap.bitmap_bytes_len = u_bitmap_len;
    /**
     * 内核内存池和用户内存池位图
     * 
     * 位图是全局的数据，长度不固定。全局或静态的数组需要在编译时知道其长度。
     * 需要根据总内存大小算出需要多少字节，所以改为指定一块内存来生成位图。
     */
    /* 内核使用的最高地址是0xc009f000，这是主线程的栈地址(内核的大小预计为70K左右)
     * 32M内存占用的位图是2k，内核内存池的位图先定在0xc009a000处。
     */
    kernel_pmem_pool.pmem_bitmap.bits = (void*)MEM_BITMAP_BASE_MEM_ADDR;
    /* 用户内存池的位图紧跟在内核内存池位图之后 */
    user_pmem_pool.pmem_bitmap.bits = (void*)(MEM_BITMAP_BASE_MEM_ADDR 
                                              + k_bitmap_len);
    /* 初始化位图 */
    bitmap_init(&kernel_pmem_pool.pmem_bitmap);
    bitmap_init(&user_pmem_pool.pmem_bitmap);
    /******************** 初始化内核与用户物理内存池结束 **********************/


    /******************** 初始化内核虚拟内存地址池 **********************/
    /** 
     * 初始化内核虚拟地址的位图，按实际物理内存大小生成数组。
     * 用于维护内核堆的虚拟地址，所以要和内核内存池大小一致。
     */
    kernel_vaddr_pool.vaddr_bitmap.bitmap_bytes_len = k_bitmap_len;
    /**
     * 位图的数组指向一块未使用的内存，目前定位在内核物理内存池和用户物理内存池之外
     * 即不受内存管理系统的约束。
     */
    kernel_vaddr_pool.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE_MEM_ADDR 
                                                  + k_bitmap_len + u_bitmap_len);
    kernel_vaddr_pool.base_vaddr = KERNEL_HEAP_BASE_MEM_ADDR;
    /* 初始化位图 */
    bitmap_init(&kernel_vaddr_pool.vaddr_bitmap);
    /******************** 初始化内核虚拟内存地址池 **********************/

    /* 打印内存池信息 */
    print_mem_pool_info();

    put_str("   pmem_pool_init done\n");
}


/**
 * print_mem_pool_info - 打印内存池信息
 */
static void print_mem_pool_info(void)
{
    put_str("       kernel_pmem_pool_bitmap start from 0x");
    put_int(kernel_pmem_pool.pmem_bitmap.bits);
    put_str(" to 0x");
    put_int(kernel_pmem_pool.pmem_bitmap.bits 
            + kernel_pmem_pool.pmem_bitmap.bitmap_bytes_len);
    put_str("\n");

    put_str("       kernel_pmem_pool_phy_addr start from 0x");
    put_int(kernel_pmem_pool.base_paddr);
    put_str(" to 0x");
    put_int(kernel_pmem_pool.base_paddr + kernel_pmem_pool.size);
    put_str("\n");
    put_str("\n");


    put_str("       user_pmem_pool_bitmap start from 0x");
    put_int(user_pmem_pool.pmem_bitmap.bits);
    put_str(" to 0x");
    put_int(user_pmem_pool.pmem_bitmap.bits 
            + user_pmem_pool.pmem_bitmap.bitmap_bytes_len);
    put_str("\n");

    put_str("       user_pmem_pool_phy_addr start from 0x");
    put_int(user_pmem_pool.base_paddr);
    put_str(" to 0x");
    put_int(user_pmem_pool.base_paddr + user_pmem_pool.size);
    put_str("\n");
    put_str("\n");


    put_str("       kernel_vaddr_pool_bitmap start from 0x");
    put_int(kernel_vaddr_pool.vaddr_bitmap.bits);
    put_str(" to 0x");
    put_int(kernel_vaddr_pool.vaddr_bitmap.bits 
            + kernel_vaddr_pool.vaddr_bitmap.bitmap_bytes_len);
    put_str("\n");
}


/**
 * vaddr_pool_get - 在pf表示的虚拟内存池中申请page_count个虚拟页,
 * @param pf         : 内核内存池(PF_KERNEL)或用户内存池(PF_USER)
 * @param page_count : 申请的虚拟页个数
 * @return           : 成功则返回虚拟页的起始地址，
 *                     失败则返回NULL
 */
static void* vaddr_pool_get(pool_flags pf, uint32_t page_count)
{
    int vaddr_start = 0, bit_start_index = -1;
    uint32_t count = 0;
    if (pf == PF_KERNEL)
    {
        bit_start_index = bitmap_scan(&kernel_vaddr_pool.vaddr_bitmap, 
                                      page_count);
        if (bit_start_index == -1)
        {
            return NULL;
        }
        while (count < page_count)
        {
            // 设置内核虚拟内存地址池位图对应位为1，即表示申请对应page
            bitmap_set(&kernel_vaddr_pool.vaddr_bitmap, 
                       (bit_start_index + count), 1);
            ++count;
        }
        // 计算虚拟内存地址的起始位置
        vaddr_start = kernel_vaddr_pool.base_vaddr + bit_start_index*PAGE_SIZE;
    }
    else
    {
        // TODO: 用户内存池，实现用户进程时补充
    }

    return (void*)vaddr_start;
}


/**
 * palloc - 在m_pool指向的物理内存地址池中分配一个物理页面
 * @param m_pool : 物理内存池指针
 * @return       : 成功则返回页框的物理地址，
 *                 失败则返回NULL
 */
static void* palloc(pmem_pool* m_pool)
{
    /* 在物理内存池中申请一个物理页面，应保证位图扫描和置位操作的原子性 */
    int bit_index = bitmap_scan(&m_pool->pmem_bitmap, 1);
    if (bit_index == -1)
    {
        return NULL;
    }
    bitmap_set(&m_pool->pmem_bitmap, bit_index, 1);
    uint32_t page_paddr = (m_pool->base_paddr + (bit_index * PAGE_SIZE));

    return (void*)page_paddr;
}


/**
 * pte_pointer - 获取虚拟地址vaddr所属的PTE的指针（指针的值也是虚拟地址），
 *               最终经过CPU的页访问机制，通过该指针能够访问vaddr所属的PTE
 * @param  vaddr : 32位虚拟地址
 * @return       : 指向vaddr所属的PTE的指针，即可通过该指针访问PTE
 */
uint32_t* pte_pointer(uint32_t vaddr)
{
    /**
     * 1.访问页目录项，由于页目录表的最后一项(1023)在loader.S中被设置为
     *   指向页目录表自身，因此通过直接指定地址高10位(PTE)为1023=1111_1111_11
     *   可将页目录表当做页表来访问。
     *  page dir table   
     *   +----------+ <---+
     *   |          |     |
     *   |          |     |
     *   |          |     |
     *   |          |     |
     *   |          |     |
     *   |          |     |
     *   +----------+     |
     *   |   1023   |-----+
     *   +----------+
     * 2.找到页表项，因为vaddr中高10位原本指定的是页目录表中vaddr的
     *   页目录项，而此时实际访问的是页目录表，通过将vaddr中高10位
     *   指定为32位地址中的次10位，访问得到的便是vaddr对应的页表项。
     *  page dir table   
     *   +----------+
     *   |          |
     *   |          |
     *   |          |
     *   +----------+
     *   |   PDE    | <--- vadrr high 10 bits
     *   +----------+
     *   |          |
     *   |          |
     *   |          |
     *   +----------+
     * 3.在页表中找到PTE，因为vaddr中次10位原本指定的是页表中vaddr
     *   的页表项，而此时实际访问的是页表，通过将vaddr中次10位乘4
     *   （一个页表项为4字节）后指定为32位地址中的低12位，访问得到
     *   的便是页表中vaddr对应的页表项(PTE)的虚拟地址，
     *   即PTE的指针指向的值。
     *    page table   
     *   +----------+
     *   |          |
     *   |          |
     *   |          |
     *   +----------+
     *   |   PTE    | <--- vadrr sub 10 bits * 4
     *   +----------+
     *   |          |
     *   |          |
     *   |          |
     *   +----------+
     *  
     */
    uint32_t* pte = (uint32_t*)(0xffc00000 
                                + ((vaddr & 0xffc00000) >> 10)
                                + (PTE_IDX(vaddr) << 2));
    return pte;
}


/**
 * pde_pointer - 获取虚拟地址vaddr所属的PDE的指针（指针的值也是虚拟地址），
 *               最终经过CPU的页访问机制，通过该指针能够访问vaddr所属的PDE
 * @param  vaddr : 32位虚拟地址
 * @return       : 指向vaddr所属的PDE的指针，即可通过该指针访问PDE
 */
uint32_t* pde_pointer(uint32_t vaddr)
{
    /**
     * 通过两次访问（32位地址高10位和32位地址次10位）页目录表
     * 最后一项(1023)，使得CPU在访问物理页阶段（32位地址低12位）
     * 时实际访问的是页目录表，再通过指定32位虚拟地址的低12位为
     * vaddr的高10位乘以4，便可以得到vaddr对应的页目录项(PDE)
     * 的虚拟地址，即PDE的指针指向的值。
     * 
     * 1. 32位地址高10位
     *  page dir table   
     *   +----------+ <---+
     *   |          |     |
     *   |          |     |
     *   |          |     |
     *   |          |     |
     *   |          |     |
     *   |          |     |
     *   +----------+     |
     *   |   1023   |-----+
     *   +----------+
     *
     * 2. 32位地址次10位
     *  page dir table   
     *   +----------+ <---+
     *   |          |     |
     *   |          |     |
     *   |          |     |
     *   |          |     |
     *   |          |     |
     *   |          |     |
     *   +----------+     |
     *   |   1023   |-----+
     *   +----------+
     *
     * 3. 32位地址低12位
     *  page dir table   
     *   +----------+
     *   |          |
     *   |          |
     *   |          |
     *   |          |
     *   +----------+
     *   |   PDE    | <--- vadrr high 10 bits * 4
     *   +----------+
     *   |          |
     *   |          |
     *   +----------+
     */
    uint32_t* pde = (uint32_t*)(0xfffff000 + (PDE_IDX(vaddr) << 2));
    return pde;
}


/**
 * mapping_vaddr_to_pmem - 在页表中添加虚拟地址_vaddr到物理地址_page_paddr之间的映射
 * @param _vaddr      : 虚拟内存地址
 * @param _page_paddr : 物理内存地址
 */
static void mapping_vaddr_to_pmem(void* _vaddr, void* _page_paddr)
{
    uint32_t vaddr = (uint32_t)_vaddr;
    uint32_t page_paddr = (uint32_t)_page_paddr;
    uint32_t* pde = pde_pointer(vaddr);
    uint32_t* pte = pte_pointer(vaddr);


    /* 判断页目录表中是否存在vaddr对应的页目录项 */
    if (*pde & 0x00000001) // 存在页目录项
    {
        /* 断言页表中不存在vaddr对应的页表项，仅在DEBUG模式下有用*/
        ASSERT(!(*pte & 0x00000001));
        if (!(*pte & 0x00000001)) // 同上一行断言，非DEUBG模式也适用
        {
            *pte = (page_paddr | PG_US_U | PG_RW_W | PG_P_1);
        }
        else // 一般不会执行此分支，此处为保证逻辑完整性
        {
            PANIC("PTE repeat!");
            *pte = (page_paddr | PG_US_U | PG_RW_W | PG_P_1);
        }
    }
    else // 不存在页目录项
    {
        /**
         * 由于在页目录表中vaddr对应的页目录项不存在，首先应该创建vaddr
         * 对应的页目录项，页目录项中对应的页表的物理页一律从内核物理内存
         * 中分配。
         * 当页目录表中vaddr对应的页目录项不存在，执行*pte会访问到空的PDE
         * 所以应确保PDE想完成创建后才执行*pte，否则会引发页错误(page fault)
         */
        uint32_t page_table_paddr = (uint32_t)palloc(&kernel_pmem_pool);
        *pde = (page_table_paddr | PG_US_U | PG_RW_W | PG_P_1);

        /**
         * 清空物理页中内容，防止原有内容对页表内容产出干扰。
         * 访问页表对应的物理页，用PTE取高20位即可，因为PTE
         * 是基于PDE中指定页表的物理地址为起始进行偏移寻址的，
         * 将PTE把低12位置0，即PDE对应页表的物理页的起始地址
         */
        memset((void*)((int)pte & 0xffffff000), 0, PAGE_SIZE);
        /* 页目录项不存在，页表项也应保证不存在，因此无需再判断页表项的存在与否 */
        *pte = (page_table_paddr | PG_US_U | PG_RW_W | PG_P_1);
    }
}


/**
 * malloc_page - 分配page_count个页的内存空间
 * @param pf         : 内核内存池(PF_KERNEL)或用户内存池(PF_USER)
 * @param page_count : 申请的页个数
 * @return           : 成功则返回页的起始地址(虚拟地址), 
 *                     失败则返回NULL
 */
void* malloc_page(pool_flags pf, uint32_t page_count)
{
    /**
     * 第一步：通过vaddr_pool_get函数在虚拟内存池中申请连续的虚拟地址
     * 第二步：通过palloc函数在物理内存池中申请一个物理内存页
     * 第三步：通过mapping_vaddr_to_pmem函数将以上得到的虚拟地址和
     *        物理地址在页表中完成映射
     */

    /**
     * 内核和用户空间各约为16MB，保守起见用15MB来限制一次申请的内存页数
     * 15MB对应的内存最大页数=15*1024*1024/4096 = 3840页
     */
    ASSERT((page_count > 0) && (page_count < 3840));

    void* vaddr_start = vaddr_pool_get(pf, page_count);
    if (vaddr_start == NULL)
    {
        return NULL;
    }

    uint32_t vaddr = (uint32_t)vaddr_start;
    uint32_t count = page_count;
    pmem_pool* m_pool = (pf == PF_KERNEL) ? 
                        (&kernel_pmem_pool) : (&user_pmem_pool);
    /**
     * 由于虚拟地址是连续的，而对应的物理内存则不一定是连续的。
     * 因此逐个页面做映射
     */
    while (count > 0)
    {
        void* page_paddr = palloc(m_pool); // 申请物理内存页
        if (page_paddr == NULL)
        {
            // TODO:失败时需要将已经申请的虚拟内存地址和物理页面释放，内存回收时再补充实现
            return NULL;
        }
        mapping_vaddr_to_pmem((void*)vaddr, page_paddr); // 映射虚拟地址到物理内存
        vaddr += PAGE_SIZE;
        --count;
    }

    return vaddr_start;
}


/**
 * get_kernel_pages - 从内核物理内存池中申请pg_count页的内存
 * @param page_count : 申请的页个数
 * @return           : 成功则返回页的起始地址(虚拟地址), 
 *                     失败则返回NULL
 */
void* get_kernel_pages(uint32_t page_count)
{
    void* vaddr = malloc_page(PF_KERNEL, page_count);
    if (vaddr != NULL) // 分配成功后将页框清0再后返回
    {
        memset(vaddr, 0, page_count*PAGE_SIZE);
    }

    return vaddr;
}


/**
 * mem_init - 内存的初始化
 */
void mem_init(void)
{
    put_str("mem_init start\n");

    /* 0xb00处存放着在loader中读取内存容量 */
    uint32_t mem_bytes_total = *((uint32_t*)(0xb00)); 
    mem_pool_init(mem_bytes_total); // 初始化内存池



    put_str("mem_init done\n");
}