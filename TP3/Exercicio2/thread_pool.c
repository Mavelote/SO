#include <stdlib.h>
#include <stdio.h>
#include "thread_pool.h"

typedef struct work_node {
    thread_pool_work_t work;
    struct work_node *next;
} work_node_t;

struct thread_pool_t {
    pthread_t *threads;          
    int num_threads;             

    work_node_t *queue_head;     
    work_node_t *queue_tail;     
    
    pthread_mutex_t mutex;       
    pthread_cond_t cond;         

    int shutdown;                
};

void internal_queue_push(thread_pool_t *pool, thread_pool_work_t work) {
    work_node_t *new_node = malloc(sizeof(work_node_t));
    new_node->work = work;
    new_node->next = NULL;

    if (pool->queue_tail == NULL) {
        pool->queue_head = new_node;
        pool->queue_tail = new_node;
    } else {
        pool->queue_tail->next = new_node;
        pool->queue_tail = new_node;
    }
}

thread_pool_work_t internal_queue_pop(thread_pool_t *pool) {
    work_node_t *old_head = pool->queue_head;
    thread_pool_work_t work = old_head->work;

    pool->queue_head = pool->queue_head->next;
    
    if (pool->queue_head == NULL) {
        pool->queue_tail = NULL;
    }

    free(old_head);
    return work;
}

void *worker_thread(void *arg) {
    thread_pool_t *pool = (thread_pool_t *)arg;

    while (1) {
        pthread_mutex_lock(&pool->mutex);
        
        while (pool->queue_head == NULL && !pool->shutdown) {
            pthread_cond_wait(&pool->cond, &pool->mutex);
        }
        
        if (pool->shutdown && pool->queue_head == NULL) {
            pthread_mutex_unlock(&pool->mutex);
            break;
        }
        
        thread_pool_work_t work = internal_queue_pop(pool);
        pthread_mutex_unlock(&pool->mutex);

        if (work.function == NULL) {
            break; 
        }
        
        work.function(work.argument);
    }
    
    return NULL;
}

thread_pool_t* thread_pool_create(int num_threads) {
    thread_pool_t *pool = malloc(sizeof(struct thread_pool_t));
    pool->num_threads = num_threads;
    pool->queue_head = NULL;
    pool->queue_tail = NULL;
    pool->shutdown = 0;
    
    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->cond, NULL);
    
    pool->threads = malloc(num_threads * sizeof(pthread_t));
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&pool->threads[i], NULL, worker_thread, pool);
    }
    return pool;
}

int thread_pool_submit(thread_pool_t *pool, void (*function)(void *), void *argument) {
    if (pool == NULL || function == NULL) return -1;
    
    pthread_mutex_lock(&pool->mutex);
    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->mutex);
        return -1; 
    }
    
    thread_pool_work_t work;
    work.function = function;
    work.argument = argument;
    
    internal_queue_push(pool, work);
    pthread_cond_signal(&pool->cond); 
    pthread_mutex_unlock(&pool->mutex);
    
    return 0;
}

int thread_pool_destroy(thread_pool_t *pool) {
    if (pool == NULL) return -1;

    pthread_mutex_lock(&pool->mutex);
    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->mutex);
        return -1; 
    }
    pool->shutdown = 1;

    for (int i = 0; i < pool->num_threads; i++) {
        thread_pool_work_t poison_pill;
        poison_pill.function = NULL;
        poison_pill.argument = NULL;
        internal_queue_push(pool, poison_pill);
    }

    pthread_cond_broadcast(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);

    for (int i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond);
    free(pool->threads);
    free(pool); 

    return 0;
}