/*
* @Author: Yooj
* @Date:   2021-12-30 00:59:43
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-31 19:36:48
*/

#include "interrupt.h"
#include "list.h"



/**
 * list_init - 初始化双向链表
 * @param plist : 双向链表指针
 */
void list_init(list* plist)
{
    plist->head.prev = NULL;
    plist->head.next = &plist->tail;
    plist->tail.prev = &plist->head;
    plist->tail.next = NULL;
}


/**
 * list_empty - 判断双向链表是否为空
 * @param  plist : 双向链表指针
 * @return       : 双向链表为空返回1(true)，否则返回0(false)
 */
bool list_empty(list* plist)
{
    return (plist->tail.prev == &plist->head);
}


/**
 * list_insert_before - 在before元素之前插入新的元素
 * @param before : 新插入元素待插入位置元素指针
 * @param pelem  : 新的链表元素指针
 */
void list_insert_before(list_elem* before, list_elem* pelem)
{
    intr_status old_status = intr_disable(); // 关中断，保证链表修改操作原子性

    pelem->prev = before->prev;
    pelem->next = before;

    before->prev->next = pelem;
    before->prev = pelem;

    intr_set_status(old_status); // 恢复中断
}


/**
 * list_append_left - 在双向链表头部插入新元素
 * @param plist : 双向链表指针
 * @param pelem : 新的链表元素指针
 */
void list_append_left(list* plist, list_elem* pelem)
{
    list_insert_before(plist->head.next, pelem);
}


/**
 * list_append - 在双向链表尾部追加新元素
 * @param plist : 双向链表指针
 * @param pelem : 新的链表元素指针
 */
void list_append(list* plist, list_elem* pelem)
{
    list_insert_before(&plist->tail, pelem);
}


/**
 * list_remove - 从双向链表中移除元素
 * @param pelem : 双向链表指针
 */
void list_remove(list_elem* pelem)
{
    intr_status old_status = intr_disable(); // 关中断，保证链表修改操作原子性

    pelem->prev->next = pelem->next;
    pelem->next->prev = pelem->prev;

    intr_set_status(old_status); // 恢复中断
}



/**
 * list_pop_left - 从双向链表中移除第一个元素，并返回移除的第一个元素，
 *                 注意，必须保证列表为非空状态，当列表为空会出错
 * @param pelem : 双向链表指针
 * @return      : 从双向链表中移除的第一个元素
 */
list_elem* list_pop_left(list* plist)
{
    list_elem* pelem = plist->head.next;
    list_remove(plist->head.next);
    return pelem;
}


/**
 * list_len - 获取双向链表长度
 * @param  plist : 双向链表指针
 * @return       : 链表长度
 */
uint32_t list_len(list* plist)
{
    list_elem* pelem = plist->head.next;

    uint32_t length = 0;
    while (pelem != &plist->tail)
    {
        ++length;
        pelem = pelem->next;
    }

    return length;
}


/**
 * list_search - 在双向链表中寻找符合func函数条件的元素
 * @param  plist : 双向链表指针
 * @param  func  : 作用于元素的条件判断函数，元素满足条件返回true，否则返回false
 * @param  arg   : 条件判断函数参数
 * @return       : 寻找成功，则返回双向链表中第一个满足条件的元素，
 *                 寻找失败，则返回NULL
 */
list_elem* list_search(list* plist, function func, int arg)
{
    list_elem* pelem = plist->head.next;
    if (list_empty(plist))
    {
        return NULL;
    }

    while (pelem != &plist->tail)
    {
        if (func(pelem, arg))
        {
            return pelem;
        }
        pelem = pelem->next;
    }

    return NULL;
}


/**
 * list_has_elem - 判断双向链表中是否存在obj_elem元素
 * @param  plist    : 双向链表指针
 * @param  obj_elem : 目标元素
 * @return          : 双向链表中存在目标元素返回1(true)，否则返回0(false)
 */
bool list_has_elem(list* plist, list_elem* obj_elem)
{
    list_elem* pelem = plist->head.next;
    if (list_empty(plist))
    {
        return false;
    }

    while (pelem != &plist->tail)
    {
        if (pelem == obj_elem)
        {
            return true;
        }
        pelem = pelem->next;
    }

    return false;
}