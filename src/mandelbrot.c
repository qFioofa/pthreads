#define _POSIX_C_SOURCE 200112L
#include "mandelbrot.h"

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_ITER 1000
#define MANDELBROT_OUTPUT "results/mandelbrot.csv"

static const double XMIN = -2.5;
static const double XMAX = 1.0;
static const double YMIN = -1.25;
static const double YMAX = 1.25;

typedef struct {
    int start;
    int end;
    int width;
    int height;
    double* xbuf;
    double* ybuf;
    long* count;
} task_t;

static int point_in_set(double cx, double cy) {
    double zx = 0.0, zy = 0.0, zx2 = 0.0, zy2 = 0.0;
    int iter = 0;
    while (iter < MAX_ITER && zx2 + zy2 <= 4.0) {
        zy = 2.0 * zx * zy + cy;
        zx = zx2 - zy2 + cx;
        zx2 = zx * zx;
        zy2 = zy * zy;
        iter++;
    }
    return iter == MAX_ITER;
}

static void* worker(void* arg) {
    task_t* t = (task_t*)arg;
    for (int i = t->start; i < t->end; i++) {
        int px = i % t->width;
        int py = i / t->width;
        double cx = XMIN + (XMAX - XMIN) * px / (t->width - 1);
        double cy = YMIN + (YMAX - YMIN) * py / (t->height - 1);
        if (point_in_set(cx, cy)) {
            long c = __atomic_fetch_add(t->count, 1, __ATOMIC_SEQ_CST);
            t->xbuf[c] = cx;
            t->ybuf[c] = cy;
        }
    }
    return NULL;
}

static int write_csv(const double* xbuf, const double* ybuf, long count) {
    FILE* f = fopen(MANDELBROT_OUTPUT, "w");
    if (f == NULL) return -1;
    for (long i = 0; i < count; i++) {
        fprintf(f, "%.15e,%.15e\n", xbuf[i], ybuf[i]);
    }
    fclose(f);
    return 0;
}

int mandelbrot_run(int nthreads, long npoints) {
    if (nthreads <= 0 || npoints <= 0) return 1;
    int side = (int)ceil(sqrt((double)npoints));
    if (side < 2) side = 2;
    int width = side, height = side;
    if ((long)width * height < npoints) width++;

    double* xbuf = malloc((size_t)npoints * sizeof(double));
    double* ybuf = malloc((size_t)npoints * sizeof(double));
    pthread_t* th = malloc((size_t)nthreads * sizeof(pthread_t));
    task_t* tasks = malloc((size_t)nthreads * sizeof(task_t));
    if (xbuf == NULL || ybuf == NULL || th == NULL || tasks == NULL) return 1;
    long count = 0;

    long total = (long)width * height;
    long step = total / nthreads;
    for (int i = 0; i < nthreads; i++) {
        tasks[i].start = (int)(i * step);
        tasks[i].end = (i == nthreads - 1) ? (int)total : (int)((i + 1) * step);
        tasks[i].width = width;
        tasks[i].height = height;
        tasks[i].xbuf = xbuf;
        tasks[i].ybuf = ybuf;
        tasks[i].count = &count;
        pthread_create(&th[i], NULL, worker, &tasks[i]);
    }
    for (int i = 0; i < nthreads; i++) {
        pthread_join(th[i], NULL);
    }
    free(th);
    free(tasks);

    int rc = write_csv(xbuf, ybuf, count);
    free(xbuf);
    free(ybuf);
    return rc == 0 ? 0 : 1;
}
