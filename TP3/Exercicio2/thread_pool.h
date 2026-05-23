#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>

typedef struct thread_pool_t thread_pool_t; 

// A estrutura do trabalho que vai para a fila
typedef struct {
    void (*function)(void *); // A função que a thread vai correr
    void *argument;           // O argumento dessa função
} thread_pool_work_t;

// Inicia a piscina com um número específico de threads. Devolve o ponteiro para o pool.
thread_pool_t* thread_pool_create(int num_threads);

// Submete um trabalho para a fila.
int thread_pool_submit(thread_pool_t *pool, void (*function)(void *), void *argument);

// Encerra a piscina graciosamente
int thread_pool_destroy(thread_pool_t *pool);

#endif