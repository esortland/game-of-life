#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#include "../include/life.h"
#include "../include/life_omp.h"

/* Compare two worlds */
int worlds_equal(int (*a)[MAX_COLS], int (*b)[MAX_COLS], int rows, int cols)
{
    for (int r = 1; r <= rows; r++) {
        for (int c = 1; c <= cols; c++) {
            if (a[r][c] != b[r][c])
                return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[])
{
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <test_case.txt> <ground_truth.txt> <iterations> <num_threads>\n", argv[0]);
        fprintf(stderr, "Example: %s test_case_1.txt test_case_1_100_it.txt 100 4\n", argv[0]);
        return 1;
    }

    const char *test_file = argv[1];
    const char *truth_file = argv[2];
    int iterations = atoi(argv[3]);
    int num_threads = atoi(argv[4]);

    if (iterations <= 0) {
        fprintf(stderr, "Error: iterations must be > 0\n");
        return 1;
    }

    if (num_threads <= 0) {
        fprintf(stderr, "Error: num_threads must be > 0\n");
        return 1;
    }

    omp_set_num_threads(num_threads);

    int rule[RULE_SIZE] = {3, 2, 3};

    printf("Correctness Test\n");
    printf("================\n");
    printf("Test file:      %s\n", test_file);
    printf("Ground truth:   %s\n", truth_file);
    printf("Iterations:     %d\n", iterations);
    printf("Threads:        %d\n\n", num_threads);

    /* Read test case dimensions */
    int test_size[2];
    read_world_size(test_size, test_file);
    int rows = test_size[0];
    int cols = test_size[1];

    if (rows <= 0 || cols <= 0) {
        fprintf(stderr, "Error: invalid dimensions for %s\n", test_file);
        return 1;
    }

    if (rows > FILE_MAX_LINES || cols > VIRTUAL_MAX_COLS) {
        fprintf(stderr, "Error: %s exceeds compile-time limits (%d > %d or %d > %d)\n",
                test_file, rows, FILE_MAX_LINES, cols, VIRTUAL_MAX_COLS);
        return 1;
    }

    printf("Grid size: %d × %d\n\n", rows, cols);

    /* Allocate worlds */
    int (*world_test)[MAX_COLS] = malloc((rows + 2) * sizeof *world_test);
    int (*world_truth)[MAX_COLS] = malloc((rows + 2) * sizeof *world_truth);
    int (*world_aux)[MAX_COLS] = malloc((rows + 2) * sizeof *world_aux);

    if (!world_test || !world_truth || !world_aux) {
        fprintf(stderr, "Error: malloc failed\n");
        return 1;
    }

    /* Read test case */
    int dummy_size[2];
    read_world(world_test, dummy_size, test_file);

    /* Read ground truth */
    read_world(world_truth, dummy_size, truth_file);

    /* Run OMP version on test case */
    printf("Running OMP implementation for %d iterations...\n", iterations);
    update_world_n_generations_omp(iterations, world_test, rows, cols, world_aux, rule);
    printf("Done.\n\n");

    /* Compare results */
    printf("Comparing with ground truth...\n");
    if (worlds_equal(world_test, world_truth, rows, cols)) {
        printf("✅ SUCCESS: OMP output matches ground truth!\n");
        free(world_test);
        free(world_truth);
        free(world_aux);
        return 0;
    } else {
        printf("‼️ FAILURE: OMP output does NOT match ground truth!\n");
        printf("Finding first mismatch...\n");
        for (int r = 1; r <= rows; r++) {
            for (int c = 1; c <= cols; c++) {
                if (world_test[r][c] != world_truth[r][c]) {
                    printf("First mismatch at row %d, col %d: got %d, expected %d\n",
                           r, c, world_test[r][c], world_truth[r][c]);
                    free(world_test);
                    free(world_truth);
                    free(world_aux);
                    return 1;
                }
            }
        }
    }

    free(world_test);
    free(world_truth);
    free(world_aux);

    return 0;
}
