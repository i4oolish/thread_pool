
#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <pthread.h>
#include <time.h>

struct ThreadWorker
{
    struct ThreadWorker *next;
    
    int wake_up; // 唤醒标志： 1唤醒、0睡眠
    pthread_cond_t  wake;
    pthread_mutex_t mutex;
    
    pthread_t pid;
    time_t sleep_start_time;
    
    void (*func)(void *arg);
    void *arg;
};

struct ThreadPool
{
    struct ThreadWorker *head;
    
    unsigned int min_thread_num;
    unsigned int max_thread_num;
    unsigned int cur_thread_num;
    
    pthread_mutex_t mutex;
    time_t last_empty_time;
};

#endif
