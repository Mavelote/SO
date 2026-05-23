#ifndef COUNTDOWN_H
#define COUNTDOWN_H

#include <pthread.h>

typedef struct {
    int count;               // O contador em si
    pthread_mutex_t mutex;   // O aloquete para proteger o contador
    pthread_cond_t cond;     // O sino para acordar as threads em espera
} countdown_t;

int countdown_init(countdown_t *cd, int initialValue);
int countdown_destroy(countdown_t *cd);
int countdown_wait(countdown_t *cd);
int countdown_down(countdown_t *cd);

#endif