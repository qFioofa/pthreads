#include <stdio.h>
#include <stdlib.h>

#include "mandelbrot.h"

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s nthreads npoints\n", argv[0]);
        return 1;
    }
    int nthreads = atoi(argv[1]);
    long npoints = atol(argv[2]);
    return mandelbrot_run(nthreads, npoints);
}
