#include <stdlib.h>
#include <stdio.h>
#include "thread_pool.h"

typedef struct work_node {
    thread_pool_work_t work;
    struct work_node *next;
} work_node_t;

struct thread_pool_t {
    pthread_t *threads;          // Array que guarda as threads (os trabalhadores)
    int num_threads;             // Quantas threads temos

    work_node_t *queue_head;     // Início da fila de trabalhos
    work_node_t *queue_tail;     // Fim da fila de trabalhos
    
    pthread_mutex_t mutex;       // Protege a fila e a flag de encerramento
    pthread_cond_t cond;         // Acorda as threads quando há trabalho novo

    int shutdown;                // Flag: 0 = Aberto, 1 = Encerrar
};

void internal_queue_push(thread_pool_t *pool, thread_pool_work_t work) {
    // 1. Criar a nova "caixa" (nó)
    work_node_t *new_node = malloc(sizeof(work_node_t));
    new_node->work = work;
    new_node->next = NULL;

    // 2. Ligar a caixa ao fim da fila
    if (pool->queue_tail == NULL) {
        // A fila estava vazia
        pool->queue_head = new_node;
        pool->queue_tail = new_node;
    } else {
        // Já havia gente na fila
        pool->queue_tail->next = new_node;
        pool->queue_tail = new_node;
    }
}

thread_pool_work_t internal_queue_pop(thread_pool_t *pool) {
    // 1. Guardar o trabalho e o nó antigo
    work_node_t *old_head = pool->queue_head;
    thread_pool_work_t work = old_head->work;

    // 2. Avançar o início da fila para o próximo
    pool->queue_head = pool->queue_head->next;
    
    // Se a fila ficou vazia, o tail também tem de ficar NULL
    if (pool->queue_head == NULL) {
        pool->queue_tail = NULL;
    }

    // 3. Deitar o nó antigo ao lixo (libertar memória!)
    free(old_head);

    return work;
}

int thread_pool_destroy(thread_pool_t *pool) {
    if (pool == NULL) return -1;

    // 1. Sinalizar para as threads que devem encerrar
    pthread_mutex_lock(&pool->mutex);
    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->mutex);
        return -1; // Já está a encerrar!
    }
    pool->shutdown = 1;
    pthread_mutex_unlock(&pool->mutex);

    // 2. Esperar que todas as threads terminem
    for (int i = 0; i < pool->num_threads; i++) {
        // Criamos um trabalho onde a função é NULL
        thread_pool_work_t poison_pill;
        poison_pill.function = NULL;
        poison_pill.argument = NULL;
        
        // Colocamos o trabalho falso na fila partilhada
        // (Assumindo que tens uma função interna para forçar a entrada na fila)
        internal_queue_push(pool, poison_pill);
    }

    pthread_cond_broadcast(&pool->cond);

    for (int i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond);
    free(pool->threads);

    return 0;
}

void *worker_thread(void *arg) {
    thread_pool_t *pool = (thread_pool_t *)arg;

    while (1) {

        // Tira o trabalho da fila
        thread_pool_work_t work = internal_queue_pop(pool);


        if (work.function == NULL) {
            // É um trabalho especial de terminação! A thread percebe que é 
            // hora de ir embora e quebra o ciclo infinito.
            break; 
        }

        // Se não for NULL, é trabalho a sério
        work.function(work.argument);
    }
    
    return NULL;
}