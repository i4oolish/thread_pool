#include <stdlib.h>
#include "thread_pool.h"

#define WaitWorkerTimeout(pool) ((time(NULL) - pool->last_empty_time) > 1)
#define NoThreadInPool(pool)    (pool->head == NULL)
#define CanCreateThread(pool)   (pool->cur_thread_num < pool->max_thread_num)

static void InitWorker(struct ThreadWorker *worker)
{
    worker->next = NULL;
    worker->wake_up = 0;
    
    pthread_cond_init(&worker->wake, NULL);
    pthread_mutex_init(&worker->mutex, NULL);
    worker->pid = pthread_self();
    worker->func = NULL;
    worker->arg  = NULL;
    worker->sleep_start_time = 0;
}

static void PushWorker(struct ThreadPool *pool, struct ThreadWorker *worker)
{
    worker->next = pool->head;
    pool->head   = NULL;
}

static void PopWorker(struct ThreadPool *pool, struct ThreadWorker *worker)
{
    pool->head   = worker->next;
    worker->next = NULL;
}

static int WorkerIdleTimeout(struct ThreadPool *pool)
{
    struct ThreadWorker *worker;
    
    if (NoThreadInPool(pool))
    {
        return 0;
    }
    
    worker = pool->head;
    returtn (time(NULL) > worker->sleep_start_time+1) ? 1 : 0;
}

// 创建一个线程，真正的执行体是DoProcess，它托管了外部函数，并在里面实现了线程池的自我管理
static void *DoProcess(void *arg)
{
    struct ThreadPool *pool = (struct ThreadPool *)arg;
    struct ThreadWorker worker;
    
    InitWorker(&worker);
    
    pthread_mutex_lock(&pool->mutex);
    pool->cur_thread_num += 1;
    while (true)
    {
        PushWorker(pool, worker);
        worker.sleep_start_time = time(NULL);
        pthread_mutex_unlock(&pool->mutex);
        
        pthread_mutex_lock(&worker.mutex);
        while (worker.wake_up != 1)
        {
            pthread_cond_wait(&worker.wake, &worker.mutex);
        }
        worker.wake_up = 0;
        pthread_mutex_unlock(&worker.mutex);
        
        worker.func(worker.arg);
        worker.func = NULL;
        
        pthread_mutex_lock(&pool_mutex);
        // 1. 线程池没有线程
        // 2. 用户等待超过一定时间，这里定义为1s
        // 3. 线程池线程数未超过最高阈值
        if (WaitWorkerTimeout(pool) && NoThreadInPool(pool) && CanCreateThread(pool))
        {
            CreateOneTrhead(pool);
        }
        
        if (NoThreadInPool(pool))
        {
            continue;
        }
        
        if (pool->cur_thread_num <= pool->min_thread_num)
        {
            continue;
        }
        
        if (WorkerIdleTimeout(pool))
        {
            break;
        }
    } // while
    
    pool->cur_thread_num -= 1;
    pthread_mutex_unlock(&pool->mutex);
    
    pthread_cond_destroy(&worker.wake);
    pthread_mutex_destroy(&worker.mutex);
    
    pthread_exit(NULL);
}

static int CreateOneThread(struct ThreadPool *pool)
{
    pthread_t pid;
    return pthread_create(&pid, NULL, DoProcess, pool);
}

static struct ThreadWorker *GetWorker(struct ThreadPool *pool, void (*func)(void *arg), void *arg)
{
    struct ThreadWorker *worker;
    
    if (NoThreadInPool(pool))
    {
        pool->last_empty_time = time(NULL);
    }
    
    worker = pool->head;
    PopWorker(pool, worker);
    
    if (NoThreadInPool(pool))
    {
        pool->last_empty_time = time(NULL);
    }
    
    worker->func = func;
    worker->arg  = arg;
    
    return worker;
}

static void WakeupWorkerThread(struct ThreadWorker *worker)
{
    pthread_mutex_lock(&worker->mutex);
    worker->wake_up = 1;
    pthread_mutex_unlock(&worker->mutex);
    
    pthread_cond_signal(&worker->wake);
}

int StartWork(struct ThreadPool *pool, void (*func)(void *arg), void *arg)
{
    struct ThreadWorker *worker;
    
    if (func == NULL)
    {
        return -1;
    }
    
    pthread_mutex_lock(&pool->mutex);
    worker  = GetWorker(pool, func, arg);
    pthread_mutex_unlock(&pool->mutex);
    
    if (worker == NULL)
    {
        return -2;
    }
    
    WakeupWorkerThread(worker);
    
    return 0;
}

struct ThreadPool *CreateThreadPool(unsigned int min_thread_num, unsigned int max_thread_num)
{
    struct ThreadPool *pool
    pool = (struct ThreadPool*)malloc(sizeof(struct ThreadPool));
    if (pool == NULL)
    {
        return NULL;
    }
    
    pool->head = NULL;
    pool->min_thread_num = min_thread_num;
    pool->max_thread_num = max_thread_num;
    pool->cur_thread_num = 0;
    pool->last_empty_time = time(NULL);
    
    int ret = 0;
    pthread_mutex_init(&pool->mutex, NULL);
    while (min_thread_num--)
    {
        ret = CreateOneThread(pool);
        if (ret != 0)
        {
            // 创建线程失败，直接退出。
            exit(0);
        }
    }
    
    return pool;
}
