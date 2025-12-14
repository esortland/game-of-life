#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/life.h"
#include "../include/life_omp.h"

#define MAX_TESTS   30
#define GENERATIONS 200
#define N_RUNS      5

/* ---------------- Timing helper ---------------- */
double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* ---------------- Compare two worlds ---------------- */
int worlds_equal(int (*a)[MAX_COLS], int (*b)[MAX_COLS], int rows, int cols) {
    for (int r = 1; r <= rows; r++) {
        for (int c = 1; c <= cols; c++) {
            if (a[r][c] != b[r][c])
                return 0;
        }
    }
    return 1;
}

/* ---------------- NEW: Parse iteration count from filename ----------------
   Examples:
     test_case_1_001_it.txt  -> 1
     test_case_3_1000_it.txt -> 1000
     test_case_2.txt         -> default_gens
*/
int parse_iterations(const char *filename, int default_gens)
{
    const char *it_pos = strstr(filename, "_it");
    if (!it_pos)
        return default_gens;

    /* Walk backwards to find the number before "_it" */
    const char *p = it_pos - 1;
    while (p > filename && (*p >= '0' && *p <= '9'))
        p--;

    if (!(*p >= '0' && *p <= '9'))
        p++;  // step forward to first digit

    int gens = atoi(p);
    return (gens > 0) ? gens : default_gens;
}

int main(void)
{
    char filenames[MAX_TESTS][64] = {
        "tests/test_case_1.txt",
        "tests/test_case_1_001_it.txt",
        "tests/test_case_1_100_it.txt",
        "tests/test_case_1_101_it.txt",
        "tests/test_case_1_300_it.txt",
        "tests/test_case_1_301_it.txt",

        "tests/test_case_2.txt",
        "tests/test_case_2_100_it.txt",
        "tests/test_case_2_200_it.txt",
        "tests/test_case_2_500_it.txt",
        "tests/test_case_2_1000_it.txt",

        "tests/test_case_3.txt",
        "tests/test_case_3_100_it.txt",
        "tests/test_case_3_200_it.txt",
        "tests/test_case_3_500_it.txt",
        "tests/test_case_3_1000_it.txt",

        "tests/test_case_4.txt",
        "tests/test_case_4_100_it.txt",
        "tests/test_case_4_200_it.txt",
        "tests/test_case_4_500_it.txt",
        "tests/test_case_4_1000_it.txt",

        "tests/test_case_5.txt",
        "tests/test_case_5_005_it.txt",
        "tests/test_case_5_020_it.txt",
        "tests/test_case_5_050_it.txt",
        "tests/test_case_5_100_it.txt"
    };

    printf("| Test | Rows × Cols | Gens | Serial mean (s) | OMP mean (s) | Speedup |\n");
    printf("|------|--------------|------|------------------|--------------|----------|\n");

    int rule[RULE_SIZE] = {3, 2, 3};

    for (int t = 0; t < MAX_TESTS; t++) {

        /* ---- PASS 1: Read ONLY size ---- */
        int size[2];
        read_world_size(size, filenames[t]);
        int rows = size[0];
        int cols = size[1];

        /* ---- Parse generations from filename ---- */
        int gens = parse_iterations(filenames[t], GENERATIONS);

        printf("DEBUG: %s → rows=%d, cols=%d, gens=%d\n",
               filenames[t], rows, cols, gens);

        if (rows <= 0 || cols <= 0) {
            printf("ERROR: invalid size for %s\n", filenames[t]);
            continue;
        }

        if (rows > FILE_MAX_LINES || cols > VIRTUAL_MAX_COLS) {
            printf("ERROR: %s exceeds compile-time limits\n", filenames[t]);
            continue;
        }

        /* ---- Skip SERIAL for huge grids ---- */
        int skip_serial = 0;
        if (rows > 1000 || cols > 1000) {
            printf("Large grid detected (%d×%d). Skipping SERIAL but running OMP...\n",
                   rows, cols);
            skip_serial = 1;
        }

        /* ---- Allocate worlds ---- */
        int (*world_serial)[MAX_COLS] = NULL;
        if (!skip_serial)
            world_serial = malloc((rows + 2) * sizeof *world_serial);

        int (*world_omp)[MAX_COLS] = malloc((rows + 2) * sizeof *world_omp);
        int (*aux_s)[MAX_COLS]      = malloc((rows + 2) * sizeof *aux_s);
        int (*aux_o)[MAX_COLS]      = malloc((rows + 2) * sizeof *aux_o);

        if ((!skip_serial && !world_serial) || !world_omp || !aux_s || !aux_o) {
            printf("❌ malloc failed for %s\n", filenames[t]);
            exit(1);
        }

        double serial_times[N_RUNS];
        double omp_times[N_RUNS];

        for (int run = 0; run < N_RUNS; run++) {

            int dummy_size[2];

            if (!skip_serial)
                read_world(world_serial, dummy_size, filenames[t]);

            read_world(world_omp, dummy_size, filenames[t]);

            if (!skip_serial) {
                double t0 = now();
                update_world_n_generations(gens, world_serial,
                                           rows, cols, aux_s, rule);
                double t1 = now();
                serial_times[run] = t1 - t0;
            } else {
                serial_times[run] = -1.0;
            }

            double t2 = now();
            update_world_n_generations_omp(gens, world_omp,
                                           rows, cols, aux_o, rule);
            double t3 = now();
            omp_times[run] = t3 - t2;

            if (!skip_serial &&
                !worlds_equal(world_serial, world_omp, rows, cols)) {
                printf("❌ ERROR: Serial/OMP mismatch in %s (run %d)\n",
                       filenames[t], run + 1);
            }
        }

        double serial_mean = 0.0, omp_mean = 0.0;
        for (int i = 0; i < N_RUNS; i++) {
            omp_mean += omp_times[i];
            if (!skip_serial)
                serial_mean += serial_times[i];
        }

        omp_mean /= N_RUNS;
        if (!skip_serial)
            serial_mean /= N_RUNS;

        if (skip_serial) {
            printf("| `%s` | %d × %d | %4d | SKIPPED | %.5f | N/A |\n",
                   filenames[t], rows, cols, gens, omp_mean);
        } else {
            printf("| `%s` | %d × %d | %4d | %.5f | %.5f | %.2fx |\n",
                   filenames[t], rows, cols, gens,
                   serial_mean, omp_mean, serial_mean / omp_mean);
        }

        free(world_serial);
        free(world_omp);
        free(aux_s);
        free(aux_o);
    }

    return 0;
}
