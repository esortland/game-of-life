#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include "life.h"
#include "life_omp.h"

//timing function
double now() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(void)
{
    int rows = 1000;
    int cols = 1000;
    int generations = 200;

    int rule[RULE_SIZE] = {3, 2, 3};

    printf("Benchmarking Game of Life...\n");
    printf("Grid: %d x %d\n", rows, cols);
    printf("Generations: %d\n", generations);
    printf("OMP threads: %d\n\n", omp_get_max_threads());
    
    // Allocate using the SAME FORMAT the original project expects:
    // int world[][MAX_COLS]

    int (*world)[MAX_COLS]     = malloc((rows + 2) * sizeof *world);
    int (*world_aux)[MAX_COLS] = malloc((rows + 2) * sizeof *world_aux);
    int (*world_copy)[MAX_COLS]= malloc((rows + 2) * sizeof *world_copy);

    if (!world || !world_aux || !world_copy) {
        printf("ERROR: malloc failed\n");
        return 1;
    }

    srand(0);

    // initialize world randomly
    for (int r = 1; r <= rows; r++)
        for (int c = 1; c <= cols; c++)
            world[r][c] = rand() % 2;

    // copy for OMP version
    for (int r = 1; r <= rows; r++)
        for (int c = 1; c <= cols; c++)
            world_copy[r][c] = world[r][c];

    // ---------
    // SERIAL
    // ----------
    double t0 = now();
    update_world_n_generations(generations, world, rows, cols, world_aux, rule);
    double serial_time = now() - t0;

    // restore world for OMP test
    for (int r = 1; r <= rows; r++)
        for (int c = 1; c <= cols; c++)
            world[r][c] = world_copy[r][c];

    // ----
    // OMP
    // ----
    double t1 = now();
    update_world_n_generations_omp(generations, world, rows, cols, world_aux, rule);
    double omp_time = now() - t1;

    // ---
    // RESULTS
    // ----
    printf("Serial: %.4f seconds\n", serial_time);
    printf("OMP:    %.4f seconds\n", omp_time);
    printf("Speedup: %.2fx\n", serial_time / omp_time);

    free(world);
    free(world_aux);
    free(world_copy);

    return 0;
}
