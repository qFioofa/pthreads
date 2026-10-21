#define _POSIX_C_SOURCE 200809L
#include "nbody.h"

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define G 6.67430e-11
#define DT 1.0e-3
#define NBODY_OUTPUT "results/nbody.csv"

typedef struct {
    double m;
    double x, y, z;
    double vx, vy, vz;
    double fx, fy, fz;
} body_t;

typedef struct task_s {
    body_t* bodies;
    int n;
    int start;
    int end;
    int stop;
    double* ax;
    double* ay;
    double* az;
    pthread_barrier_t* bar;
    struct task_s* tasks;
    int nthreads;
} task_t;

static body_t* read_bodies(const char* path, int* n_out) {
    FILE* f = fopen(path, "r");
    if (f == NULL) return NULL;
    int n;
    if (fscanf(f, "%d", &n) != 1 || n <= 0) {
        fclose(f);
        return NULL;
    }
    body_t* bodies = calloc((size_t)n, sizeof(body_t));
    if (bodies == NULL) {
        fclose(f);
        return NULL;
    }
    for (int i = 0; i < n; i++) {
        if (fscanf(f, "%lf %lf %lf %lf %lf %lf %lf", &bodies[i].m, &bodies[i].x,
                   &bodies[i].y, &bodies[i].z, &bodies[i].vx, &bodies[i].vy,
                   &bodies[i].vz) != 7) {
            free(bodies);
            fclose(f);
            return NULL;
        }
    }
    fclose(f);
    *n_out = n;
    return bodies;
}

static void compute_forces(task_t* t) {
    memset(t->ax, 0, (size_t)t->n * sizeof(double));
    memset(t->ay, 0, (size_t)t->n * sizeof(double));
    memset(t->az, 0, (size_t)t->n * sizeof(double));
    for (int i = t->start; i < t->end; i++) {
        for (int j = i + 1; j < t->n; j++) {
            double dx = t->bodies[j].x - t->bodies[i].x;
            double dy = t->bodies[j].y - t->bodies[i].y;
            double dz = t->bodies[j].z - t->bodies[i].z;
            double r2 = dx * dx + dy * dy + dz * dz;
            double r3 = r2 * sqrt(r2);
            if (r3 == 0.0) continue;
            double f = G * t->bodies[i].m * t->bodies[j].m / r3;
            t->ax[i] += f * dx;
            t->ay[i] += f * dy;
            t->az[i] += f * dz;
            t->ax[j] -= f * dx;
            t->ay[j] -= f * dy;
            t->az[j] -= f * dz;
        }
    }
}

static void merge_forces(task_t* t) {
    for (int i = t->start; i < t->end; i++) {
        double fx = 0.0, fy = 0.0, fz = 0.0;
        for (int k = 0; k < t->nthreads; k++) {
            fx += t->tasks[k].ax[i];
            fy += t->tasks[k].ay[i];
            fz += t->tasks[k].az[i];
        }
        t->bodies[i].fx = fx;
        t->bodies[i].fy = fy;
        t->bodies[i].fz = fz;
    }
}

static void update_bodies(task_t* t) {
    for (int i = t->start; i < t->end; i++) {
        double m = t->bodies[i].m;
        t->bodies[i].vx += (t->bodies[i].fx / m) * DT;
        t->bodies[i].vy += (t->bodies[i].fy / m) * DT;
        t->bodies[i].vz += (t->bodies[i].fz / m) * DT;
        t->bodies[i].x += t->bodies[i].vx * DT;
        t->bodies[i].y += t->bodies[i].vy * DT;
        t->bodies[i].z += t->bodies[i].vz * DT;
    }
}

static void* worker(void* arg) {
    task_t* t = (task_t*)arg;
    while (1) {
        pthread_barrier_wait(t->bar);
        if (t->stop) break;
        compute_forces(t);
        pthread_barrier_wait(t->bar);
        merge_forces(t);
        update_bodies(t);
        pthread_barrier_wait(t->bar);
    }
    return NULL;
}

static void write_step(FILE* fout, double t, const body_t* bodies, int n) {
    fprintf(fout, "%.6e", t);
    for (int i = 0; i < n; i++) {
        fprintf(fout, " %.6e %.6e", bodies[i].x, bodies[i].y);
    }
    fprintf(fout, "\n");
}

int nbody_run(const char* input_path, double t_end, int nthreads) {
    if (t_end <= 0.0 || nthreads <= 0) return 1;
    int n;
    body_t* bodies = read_bodies(input_path, &n);
    if (bodies == NULL) return 1;
    if (nthreads > n) nthreads = n;

    pthread_t* th = malloc((size_t)nthreads * sizeof(pthread_t));
    task_t* tasks = calloc((size_t)nthreads, sizeof(task_t));
    if (th == NULL || tasks == NULL) return 1;
    pthread_barrier_t bar;
    pthread_barrier_init(&bar, NULL, (unsigned)nthreads + 1);
    int step = n / nthreads;
    for (int i = 0; i < nthreads; i++) {
        task_t* t = &tasks[i];
        t->bodies = bodies;
        t->n = n;
        t->start = i * step;
        t->end = (i == nthreads - 1) ? n : (i + 1) * step;
        t->stop = 0;
        t->ax = calloc((size_t)n, sizeof(double));
        t->ay = calloc((size_t)n, sizeof(double));
        t->az = calloc((size_t)n, sizeof(double));
        t->bar = &bar;
        t->tasks = tasks;
        t->nthreads = nthreads;
        if (t->ax == NULL || t->ay == NULL || t->az == NULL) return 1;
        pthread_create(&th[i], NULL, worker, t);
    }

    FILE* fout = fopen(NBODY_OUTPUT, "w");
    if (fout == NULL) return 1;
    long steps_total = (long)ceil(t_end / DT);
    write_step(fout, 0.0, bodies, n);
    for (long s = 1; s <= steps_total; s++) {
        pthread_barrier_wait(&bar);
        pthread_barrier_wait(&bar);
        pthread_barrier_wait(&bar);
        write_step(fout, (double)s * DT, bodies, n);
    }
    for (int i = 0; i < nthreads; i++) {
        tasks[i].stop = 1;
    }
    pthread_barrier_wait(&bar);
    for (int i = 0; i < nthreads; i++) {
        pthread_join(th[i], NULL);
    }
    pthread_barrier_destroy(&bar);
    fclose(fout);
    for (int i = 0; i < nthreads; i++) {
        free(tasks[i].ax);
        free(tasks[i].ay);
        free(tasks[i].az);
    }
    free(th);
    free(tasks);
    free(bodies);
    return 0;
}
