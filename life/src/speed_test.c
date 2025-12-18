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
// ./build/speed_test rows cols iterations threads

int main(int argc, char *argv[])
{
    int rows = 1000;
    int cols = 1000;
    int generations = 200;
    int num_threads = omp_get_max_threads();

    // Parse command line arguments: ./exe rows cols threads
    if (argc > 1) {
        rows = atoi(argv[1]);
        if (rows <= 0) rows = 1000;
    }
    if (argc > 2) {
        cols = atoi(argv[2]);
        if (cols <= 0) cols = 1000;
    }
    if (argc > 3) {
        generations = atoi(argv[3]);
        if (generations <= 0) generations = 200;
    }
    if (argc > 4) {
        num_threads = atoi(argv[4]);
        if (num_threads <= 0) {
            fprintf(stderr, "Invalid thread count. Using default (%d)\n", omp_get_max_threads());
            num_threads = omp_get_max_threads();
        }
    }

    omp_set_num_threads(num_threads);

    int rule[RULE_SIZE] = {3, 2, 3};

    printf("Benchmarking Game of Life...\n");
    printf("Grid: %d x %d\n", rows, cols);
    printf("Generations: %d\n", generations);
    printf("OMP threads: %d\n\n", num_threads);
    
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
    double serial_time;
    if (rows < 1000 && cols < 1000){
        printf("Running SERIAL version...\n");
                update_world_n_generations(generations, world, rows, cols, world_aux, rule);
        

        // restore world for OMP test
        for (int r = 1; r <= rows; r++)
            for (int c = 1; c <= cols; c++)
                world[r][c] = world_copy[r][c];
        serial_time = now() - t0;
    }
    else{
        printf(" skipping SERIAL version (this will take too long)...\n");
        serial_time = -1;
    }
    
    

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
