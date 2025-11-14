#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "common.h"

typedef struct JobNode {
    void (*function)(void* arg);
    void* arg;
    struct JobNode* next;
} JobNode;

typedef struct {
    pthread_t* threads;
    int num_threads;
    int shutdown;
    JobNode* queue_head;
    JobNode* queue_tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} ThreadPool;


ThreadPool* threadpool_create(int num_threads);
int threadpool_add_job(ThreadPool* pool, void (*function)(void*), void* arg);
void threadpool_destroy(ThreadPool* pool);

#endif