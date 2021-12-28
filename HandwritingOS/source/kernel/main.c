/*
* @Author: Yooj
* @Date:   2021-12-08 01:25:50
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-28 01:07:06
*/

#include "print.h"
#include "init.h"
#include "debug.h"
#include "string.h"
#include "bitmap.h"
#include "memory.h"
#include "thread.h"




/* 测试用例函数声明 */
void print_test(void);
void intr_test(void);
void assert_test(void);
void string_test(void);
void bitmap_test(void);
void memory_test(void);
void thread_test(void);




// int _start(void) // Linux链接默认程序入口函数名
int main(void)
{   

    // print_test();    

    // intr_test();
    
    // assert_test();

    // string_test();

    // bitmap_test();

    // memory_test();
    
    thread_test();

    while (1);
    return 0;
}





void print_test(void)
{
    /* put_char打印函数测试 */ 
    /*
    put_char('K');
    put_char('e');
    put_char('r');
    put_char('n');
    put_char('e');
    put_char('l');
    put_char('\n'); // 换行
    put_char('1');
    put_char('2');
    put_char('\b'); // 退格
    put_char('3');
    */
    
    /* put_str打印函数测试 */ 
    /*
    put_str("I am the Kernel.\n");
    */
   
    /* put_int打印函数测试 */
    /*
    put_int(0);
    put_char('\n');
    put_int(9);
    put_char('\n');
    put_int(14);
    put_char('\n');
    put_int(0x12345678);
    put_char('\n');
    put_int(0x00e005af);
    put_char('\n');
    put_int(0x00000000);
    */
   
    /* put_char、put_str、put_int打印函数总测试 */
    put_char('C');
    put_char('h');
    put_char('a');
    put_char('r');
    put_char('\n');

    put_str("I am the Kernel.\n");

    put_int(0x00e015af);
}


void intr_test(void)
{
    put_str("I am the Kernel.\n");
    init_all();

    asm volatile ("sti"); // 演示中断处理，临时开中断
}


void assert_test(void)
{
    put_str("I am Kernel\n");
    init_all();

    ASSERT(1==2);

    while (1);
}


void string_test(void)
{
    char* str_a = "string A string";
    char* str_b = "string B string";
    char* str_c = "string C string";
    char* _tmp_ = "               "; // 由于字符串内存连续，此字符串用于占位
    char* str_d = "string D";
    put_str("str_a: ");
    put_int((uint32_t)str_a);
    put_char(':');
    put_str(str_a);
    put_str("\n");

    put_str("str_b: ");
    put_int((uint32_t)str_b);
    put_char(':');
    put_str(str_b);
    put_str("\n");

    put_str("str_c: ");
    put_int((uint32_t)str_c);
    put_char(':');
    put_str(str_c);
    put_str("\n");

    put_str("str_d: ");
    put_int((uint32_t)str_d);
    put_char(':');
    put_str(str_d);
    put_str("\n");
    put_str("\n");

    put_str("--------------- TEST ---------------");
    put_str("\n");

    put_str("TEST strlen(str_a): ");
    put_int(strlen(str_a));
    put_str("\n");

    put_str("TEST strcpy(str_d, str_a): ");
    put_str("str_d=");
    put_str(strcpy(str_d, str_a));
    put_str("\n");

    put_str("TEST strcmp(str_a, str_b): ");
    put_int(strcmp(str_a, str_b));
    put_str("\n");

    put_str("TEST char* p_chr = strchr(str_a, 't'): \n");
    char* p_chr = strchr(str_a, 't');    
    put_str("    put_str(*p_chr):");
    put_char(*p_chr);
    put_str("\n");
    put_str("    put_int((uint32_t)p_chr):");
    put_int((uint32_t)p_chr);
    put_str("\n");
    put_str("    put_int((uint32_t)str_a):");
    put_int((uint32_t)str_a);
    put_str("\n");
    put_str("    put_char((uint8_t)(*(str_a + 1))):");
    put_char((uint8_t)(*(str_a + 1)));
    put_str("\n");

    put_str("TEST char* p_chr = strrchr(str_a, 't'): \n");
    char* p_rchr = strrchr(str_a, 't');    
    put_str("    put_str(*p_chr):");
    put_char(*p_rchr);
    put_str("\n");
    put_str("    put_int((uint32_t)p_rchr):");
    put_int((uint32_t)p_rchr);
    put_str("\n");
    put_str("    put_int((uint32_t)str_a):");
    put_int((uint32_t)str_a);
    put_str("\n");
    put_str("    put_char((uint8_t)(*(str_a + 10))):");
    put_char((uint8_t)(*(str_a + 10)));
    put_str("\n");


    put_str("TEST strcat(str_c, \" Cat (^-^)~\"): ");
    strcat(str_c, " Cat (^-^)~");
    put_str("str_c=");
    put_str(str_c);
    put_str("\n");

    put_str("TEST strchrs(str_d, 's'): \n");
    put_str("     str_d=");
    put_str(str_d);
    put_str("\n");
    put_str("     put_int(strchrs(str_d, 's')):");
    put_int(strchrs(str_d, 's'));
    put_str("\n");
    put_str("\n");

    put_str("--------------- TEST ---------------");
}


void bitmap_test(void)
{
    uint16_t bits_value = 0x0f;
    bitmap btmp;
    btmp.bitmap_bytes_len = 1;
    btmp.bits = &bits_value;
    put_int(*btmp.bits);
    put_str("\n");

    bitmap_init(&btmp);
    put_int(*btmp.bits);
    put_str("\n");

    bitmap_set(&btmp, 4, 1);
    put_int(*btmp.bits);
    put_str("\n");
}


void memory_test(void)
{
    put_str("I am kernel\n");
    init_all();

    void* addr = get_kernel_pages(3);

    put_str("\n get_kernel_pages(3) start vaddr=");
    put_int((uint32_t)addr);
    put_str("\n");

    while(1);
}


void kthread_a(void* args); // thread_test中thread中运行的函数

void thread_test(void)
{
    put_str("I am in kernel\n");

    init_all();

    thread_create("kthread_a", 31, kthread_a, "argA ");

    while(1);
}

void kthread_a(void* args)
{
    char* para = args;
    while (1)
    {
        put_str(para);
    }    
}
