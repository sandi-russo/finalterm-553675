#include "thread_pool.h"

// funzione eseguita da ogni worker thread
static void* worker_thread(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;

    while (1) {
        pthread_mutex_lock(&pool->mutex);

        // aspetta fino a quando non ci sono job o viene richiesto lo shutdown
        while (pool->queue_head == NULL && !pool->shutdown) {
            pthread_cond_wait(&pool->cond, &pool->mutex);
        }

        if (pool->shutdown && pool->queue_head == NULL) {
            pthread_mutex_unlock(&pool->mutex);
            break;
        }

        // prendo il primo job dalla coda
        JobNode* job = pool->queue_head;
        if (job != NULL) {
            pool->queue_head = job->next;
            if (pool->queue_head == NULL) {
                pool->queue_tail = NULL;
            }
        }

        pthread_mutex_unlock(&pool->mutex);

        // eseguo il job
        if (job != NULL) {
            job->function(job->arg);
            free(job);
        }
    }

    return NULL;
}

// creo il thread pool
ThreadPool* threadpool_create(int num_threads) {
    ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
    if (pool == NULL) {
        return NULL;
    }

    pool->num_threads = num_threads;
    pool->shutdown = 0;
    pool->queue_head = NULL;
    pool->queue_tail = NULL;

    // inizializza mutex e condition variable
    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->cond, NULL);

    // creo i thread
    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * num_threads);
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0) {
            // Gestione errore (semplificata)
            fprintf(stderr, "Errore nella creazione del thread %d\n", i);
            threadpool_destroy(pool);
            return NULL;
        }
    }

    printf("Thread pool creato con %d workers.\n", num_threads);
    return pool;
}

// aggiungo un job al pool
int threadpool_add_job(ThreadPool* pool, void (*function)(void*), void* arg) {
    if (pool == NULL || function == NULL) {
        return -1;
    }

    // creo il nuovo nodo
    JobNode* new_job = (JobNode*)malloc(sizeof(JobNode));
    if (new_job == NULL) {
        return -1;
    }

    new_job->function = function;
    new_job->arg = arg;
    new_job->next = NULL;

    pthread_mutex_lock(&pool->mutex);

    if (pool->queue_tail == NULL) {
        pool->queue_head = new_job;
        pool->queue_tail = new_job;
    } else {
        pool->queue_tail->next = new_job;
        pool->queue_tail = new_job;
    }

    pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);

    return 0;
}

// elimino il thread pool
void threadpool_destroy(ThreadPool* pool) {
    if (pool == NULL) {
        return;
    }

    pthread_mutex_lock(&pool->mutex);
    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);

    for (int i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    JobNode* current = pool->queue_head;
    while (current != NULL) {
        JobNode* next = current->next;
        free(current);
        current = next;
    }

    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond);
    free(pool->threads);
    free(pool);
}