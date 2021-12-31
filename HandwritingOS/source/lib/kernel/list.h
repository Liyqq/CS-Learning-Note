#ifndef __LIB_KERNEL_LIST_H
#define __LIB_KERNEL_LIST_H
#include "global.h"

#define offset(struct_type, member) (int)(&((struct_type*)0)->member)
#define elem2entry(struct_type, struct_member_name, elem_ptr) \
        (struct_type*)((int)elem_ptr - offset(struct_type, struct_member_name))



/* 链表节点结构体 */
typedef struct list_elem
{
    struct list_elem* prev; // 指向前驱结点
    struct list_elem* next; // 指向后继结点
} list_elem;


/* 链表结构体 */
typedef struct list 
{
    struct list_elem head; // 链表头，并非第一个元素
    struct list_elem tail; // 链表尾，并非最后一个元素
} list;


typedef bool (function)(list_elem*, int);





void list_init(list* plist);

bool list_empty(list* plist);

void list_insert_before(list_elem* before, list_elem* pelem);

void list_append_left(list* plist, list_elem* pelem);

void list_append(list* plist, list_elem* pelem);

void list_remove(list_elem* pelem);

list_elem* list_pop_left(list* plist);

uint32_t list_len(list* plist);

list_elem* list_search(list* plist, function func, int arg);

bool list_has_elem(list* plist, list_elem* obj_elem);


#endif // __LIB_KERNEL_LIST_H