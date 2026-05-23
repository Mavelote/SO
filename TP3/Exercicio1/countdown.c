#include "countdown.h"

int countdown_init(countdown_t *cd, int initialValue) {
    if (cd == NULL || initialValue <= 0) return -1;

    cd->count = initialValue;

    if (pthread_mutex_init(&cd->mutex, NULL) != 0) return -1;
    if (pthread_cond_init(&cd->cond, NULL) != 0) {
        pthread_mutex_destroy(&cd->mutex);
        return -1;
    }

    return 0;
}

int countdown_destroy(countdown_t *cd) {
    if (cd == NULL) return -1;

    pthread_mutex_destroy(&cd->mutex);
    pthread_cond_destroy(&cd->cond);

    return 0;
}

int countdown_down(countdown_t *cd) {
    if (cd == NULL) return -1;

    pthread_mutex_lock(&cd->mutex);
    
    if (cd->count > 0) {
        cd->count--;

        if (cd->count == 0) {
            pthread_cond_broadcast(&cd->cond);
        }
    }
    
    pthread_mutex_unlock(&cd->mutex);

    return 0;
}

int countdown_wait(countdown_t *cd) {
    if (cd == NULL) return -1;

    pthread_mutex_lock(&cd->mutex);

    if (cd->count == 0) {
        pthread_mutex_unlock(&cd->mutex);
        return -1;
    }

    while (cd->count > 0) {
        pthread_cond_wait(&cd->cond, &cd->mutex);
    }

    pthread_mutex_unlock(&cd->mutex);

    return 0;
}