#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "life.h"

/* Verify that the CUDA implementation produces the same final world
 * as the CPU serial implementation given the same input and number
 * of generations. Exits 0 on success, non-zero on mismatch.
 */

extern void update_world_cuda(int *world_flat, int rows_count, int cols_count, int *world_aux_flat, const int rule[RULE_SIZE], int pitch);

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <generations> [input_file]\n", argv[0]);
        return 2;
    }

    int generations = atoi(argv[1]);
    const char *preferred = "../initial_worlds/Gosper_glider_gun.txt";
    const char *fallback = "../tests/test_case_1.txt";
    const char *input = (argc > 2) ? argv[2] : preferred;
    if (access(input, F_OK) != 0) {
        input = fallback;
    }

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

    int *cpu_flat = (int *)malloc(total_elems * sizeof(int));
    int *aux_cpu = (int *)calloc(total_elems, sizeof(int));
    int *cuda_flat = (int *)malloc(total_elems * sizeof(int));
    int *aux_cuda = (int *)calloc(total_elems, sizeof(int));
    if (!cpu_flat || !aux_cpu || !cuda_flat || !aux_cuda) {
        fprintf(stderr, "OOM allocating verification buffers\n");
        free(cpu_flat); free(aux_cpu); free(cuda_flat); free(aux_cuda);
        free_world_dyn(dw);
        return 2;
    }

    memcpy(cpu_flat, dw->data, total_elems * sizeof(int));
    memcpy(cuda_flat, dw->data, total_elems * sizeof(int));

    // Run CPU flat implementation
    update_world_n_generations_flat(generations, cpu_flat, rows, cols, aux_cpu, rule, pitch);

    // Run CUDA implementation
    for (int i = 0; i < generations; ++i) {
        update_world_cuda(cuda_flat, rows, cols, aux_cuda, rule, pitch);
    }

    // Compare inner region
    for (int r = 1; r <= rows; ++r) {
        for (int c = 1; c <= cols; ++c) {
            int a = cpu_flat[r * pitch + c];
            int b = cuda_flat[r * pitch + c];
            if (a != b) {
                printf("mismatch at row=%d col=%d: cpu=%d cuda=%d\n", r, c, a, b);
                free(cpu_flat); free(aux_cpu); free(cuda_flat); free(aux_cuda);
                free_world_dyn(dw);
                return 1;
            }
        }
    }

    printf("verify: OK (generations=%d, rows=%d, cols=%d)\n", generations, rows, cols);
    free(cpu_flat); free(aux_cpu); free(cuda_flat); free(aux_cuda);
    free_world_dyn(dw);
    return 0;
}
