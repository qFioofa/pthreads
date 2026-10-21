#include <stdio.h>
#include <stdlib.h>

#include "llist.h"

int main(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s nthreads duration_sec read_fraction\n",
                argv[0]);
        return 1;
    }
    int nthreads = atoi(argv[1]);
    int duration_sec = atoi(argv[2]);
    double read_frac = atof(argv[3]);
    return llist_run(nthreads, duration_sec, read_frac);
}
