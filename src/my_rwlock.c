#include "my_rwlock.h"

int my_rwlock_init(my_rwlock_t* rw) {
    if (rw == NULL) return -1;
    rw->readers_active = 0;
    rw->readers_waiting = 0;
    rw->writers_waiting = 0;
    rw->writer_active = 0;
    if (pthread_mutex_init(&rw->mutex, NULL) != 0) return -1;
    if (pthread_cond_init(&rw->cond_readers, NULL) != 0) {
        pthread_mutex_destroy(&rw->mutex);
        return -1;
    }
    if (pthread_cond_init(&rw->cond_writers, NULL) != 0) {
        pthread_cond_destroy(&rw->cond_readers);
        pthread_mutex_destroy(&rw->mutex);
        return -1;
    }
    return 0;
}

int my_rwlock_destroy(my_rwlock_t* rw) {
    if (rw == NULL) return -1;
    pthread_cond_destroy(&rw->cond_writers);
    pthread_cond_destroy(&rw->cond_readers);
    pthread_mutex_destroy(&rw->mutex);
    return 0;
}

int my_rwlock_rdlock(my_rwlock_t* rw) {
    if (rw == NULL) return -1;
    pthread_mutex_lock(&rw->mutex);
    rw->readers_waiting++;
    while (rw->writer_active == 1 || rw->writers_waiting > 0) {
        pthread_cond_wait(&rw->cond_readers, &rw->mutex);
    }
    rw->readers_waiting--;
    rw->readers_active++;
    pthread_mutex_unlock(&rw->mutex);
    return 0;
}

int my_rwlock_wrlock(my_rwlock_t* rw) {
    if (rw == NULL) return -1;
    pthread_mutex_lock(&rw->mutex);
    rw->writers_waiting++;
    while (rw->writer_active == 1 || rw->readers_active > 0) {
        pthread_cond_wait(&rw->cond_writers, &rw->mutex);
    }
    rw->writers_waiting--;
    rw->writer_active = 1;
    pthread_mutex_unlock(&rw->mutex);
    return 0;
}

int my_rwlock_unlock(my_rwlock_t* rw) {
    if (rw == NULL) return -1;
    pthread_mutex_lock(&rw->mutex);
    if (rw->writer_active == 1) {
        rw->writer_active = 0;
        if (rw->writers_waiting > 0) {
            pthread_cond_signal(&rw->cond_writers);
        } else if (rw->readers_waiting > 0) {
            pthread_cond_broadcast(&rw->cond_readers);
        }
    } else {
        rw->readers_active--;
        if (rw->readers_active == 0 && rw->writers_waiting > 0) {
            pthread_cond_signal(&rw->cond_writers);
        }
    }
    pthread_mutex_unlock(&rw->mutex);
    return 0;
}
