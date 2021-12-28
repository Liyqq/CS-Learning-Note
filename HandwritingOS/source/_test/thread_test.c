/*
* @Author: Yooj
* @Date:   2021-12-27 00:20:42
* @Last Modified by:   Yooj
* @Last Modified time: 2021-12-27 00:36:40
*/
#include <stdio.h>
#include <pthread.h>


void* thread_func(void* _arg)
{
    unsigned int * arg = _arg;
    printf(" new thread: my tid is %u\n", *arg);
}

int main(void)
{
    pthread_t new_thread_id;
    pthread_create(&new_thread_id, NULL, thread_func, &new_thread_id);
    printf("main thread: my tid is %lu\n", pthread_self());
    usleep(100);

    return 0;
}