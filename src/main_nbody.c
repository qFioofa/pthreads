#include <stdio.h>
#include <stdlib.h>

#include "nbody.h"

#ifndef NBODY_NTHREADS
#define NBODY_NTHREADS 4
#endif

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s t_end input_file\n", argv[0]);
        return 1;
    }
    double t_end = atof(argv[1]);
    return nbody_run(argv[2], t_end, NBODY_NTHREADS);
}
