#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "life.h"

int main(int argc, char **argv)
{
    int generations = 100;
    const char *preferred = "../initial_worlds/Gosper_glider_gun.txt";
    const char *fallback = "../tests/test_case_1.txt";
    const char *input = preferred;

    if (argc > 1)
        generations = atoi(argv[1]);
    if (argc > 2 && argv[2] != NULL) {
        input = argv[2];
    } else {
        if (access(preferred, F_OK) != 0) {
            input = fallback;
        }
    }

    /* Try to read input; if not found, try common relative fallbacks used in the repo */
    DynWorld *dw = read_world_dyn(input);
    if (!dw) {
        const char *base = strrchr(input, '/');
        base = base ? base + 1 : input;
        char alt1[4096];
        char alt2[4096];
        snprintf(alt1, sizeof(alt1), "../initial_worlds/%s", base);
        snprintf(alt2, sizeof(alt2), "../tests/%s", base);
        dw = read_world_dyn(alt1);
        if (!dw) dw = read_world_dyn(alt2);
        if (!dw) {
            fprintf(stderr, "Failed to read input file %s (tried fallbacks)\n", input);
            return 2;
        }
    }

    int rows = dw->rows;
    int cols = dw->cols;
    int pitch = dw->pitch;
    int rule[RULE_SIZE] = {3, 2, 3};

    size_t total_rows = (size_t)rows + 2;
    size_t total_elems = total_rows * (size_t)pitch;
    int *aux_flat = (int *)calloc(total_elems, sizeof(int));
    if (!aux_flat) {
        fprintf(stderr, "OOM: cannot allocate aux buffer (%zu elems)\n", total_elems);
        free_world_dyn(dw);
        return 2;
    }

    struct timespec t0, t1;
    if (clock_gettime(CLOCK_MONOTONIC, &t0) != 0) {
        perror("clock_gettime");
        free(aux_flat);
        free_world_dyn(dw);
        return 1;
    }

    update_world_n_generations_flat(generations, dw->data, rows, cols, aux_flat, rule, pitch);

    if (clock_gettime(CLOCK_MONOTONIC, &t1) != 0) {
        perror("clock_gettime");
        free(aux_flat);
        free_world_dyn(dw);
        return 1;
    }

    double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
    double sec_per_gen = elapsed / (double)generations;

    // Human-friendly summary
    printf("CPU bench: gens=%d rows=%d cols=%d elapsed=%.6f s (%.6f s/gen)\n", generations, rows, cols, elapsed, sec_per_gen);
    // blank line to separate human output from machine CSV
    printf("\n");
    // Machine-friendly CSV (one line, easy to parse). Fields:
    // impl,generations,rows,cols,seconds,sec_per_gen
    printf("cpu,%d,%d,%d,%.6f,%.9f\n", generations, rows, cols, elapsed, sec_per_gen);

    free(aux_flat);
    free_world_dyn(dw);
    return 0;
}
