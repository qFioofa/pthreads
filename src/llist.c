#define _POSIX_C_SOURCE 200809L
#include "llist.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LLIST_KEY_RANGE 10000
#define LLIST_PREFILL 100

#ifdef USE_PTHREAD_RWLOCK
typedef pthread_rwlock_t rwlock_t;
#define rwlock_init(x) pthread_rwlock_init((x), NULL)
#define rwlock_destroy(x) pthread_rwlock_destroy(x)
#define rw_rdlock(x) pthread_rwlock_rdlock(x)
#define rw_wrlock(x) pthread_rwlock_wrlock(x)
#define rw_unlock(x) pthread_rwlock_unlock(x)
#else
#include "my_rwlock.h"
typedef my_rwlock_t rwlock_t;
#define rwlock_init(x) my_rwlock_init(x)
#define rwlock_destroy(x) my_rwlock_destroy(x)
#define rw_rdlock(x) my_rwlock_rdlock(x)
#define rw_wrlock(x) my_rwlock_wrlock(x)
#define rw_unlock(x) my_rwlock_unlock(x)
#endif

typedef struct node {
    int key;
    struct node* next;
} node_t;

typedef struct {
    int duration_sec;
    double read_frac;
} task_t;

static node_t* head = NULL;
static rwlock_t rwlock;
static long ops_count = 0;

static node_t* find(int key) {
    node_t* cur = head;
    while (cur != NULL && cur->key != key) {
        cur = cur->next;
    }
    return cur;
}

static void insert_key(int key) {
    node_t* n = malloc(sizeof(node_t));
    if (n == NULL) return;
    n->key = key;
    n->next = head;
    head = n;
}

static void delete_key(int key) {
    node_t* cur = head;
    node_t* prev = NULL;
    while (cur != NULL && cur->key != key) {
        prev = cur;
        cur = cur->next;
    }
    if (cur == NULL) return;
    if (prev == NULL) {
        head = cur->next;
    } else {
        prev->next = cur->next;
    }
    free(cur);
}

static void list_clear(void) {
    while (head != NULL) {
        node_t* tmp = head;
        head = head->next;
        free(tmp);
    }
}

static void* worker(void* arg) {
    task_t* t = (task_t*)arg;
    time_t start = time(NULL);
    while (time(NULL) - start < t->duration_sec) {
        double r = (double)rand() / RAND_MAX;
        int key = rand() % LLIST_KEY_RANGE;
        if (r < t->read_frac) {
            rw_rdlock(&rwlock);
            (void)find(key);
            rw_unlock(&rwlock);
        } else if (rand() % 2 == 0) {
            rw_wrlock(&rwlock);
            insert_key(key);
            rw_unlock(&rwlock);
        } else {
            rw_wrlock(&rwlock);
            delete_key(key);
            rw_unlock(&rwlock);
        }
        __atomic_fetch_add(&ops_count, 1, __ATOMIC_SEQ_CST);
    }
    return NULL;
}

static void prefill_list(void) {
    for (int i = 0; i < LLIST_PREFILL; i++) {
        insert_key(rand() % LLIST_KEY_RANGE);
    }
}

int llist_run(int nthreads, int duration_sec, double read_frac) {
    if (nthreads <= 0 || duration_sec <= 0 || read_frac < 0.0 ||
        read_frac > 1.0)
        return 1;
    srand((unsigned)time(NULL));
    rwlock_init(&rwlock);
    prefill_list();

    task_t task = {duration_sec, read_frac};
    pthread_t* th = malloc((size_t)nthreads * sizeof(pthread_t));
    if (th == NULL) return 1;
    for (int i = 0; i < nthreads; i++) {
        pthread_create(&th[i], NULL, worker, &task);
    }
    for (int i = 0; i < nthreads; i++) {
        pthread_join(th[i], NULL);
    }
    free(th);
    list_clear();
    rwlock_destroy(&rwlock);
    printf("%ld ops in %d s (%ld ops/s)\n", ops_count, duration_sec,
           ops_count / (long)duration_sec);
    return 0;
}
