#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include "life.h"
}

typedef struct CudaWorld CudaWorld;
extern "C" int cuda_init_world(CudaWorld **out, const int *host_world_flat, int rows_count, int cols_count, int pitch, const int rule[3]);
extern "C" int cuda_step_n(CudaWorld *cw, int n);
extern "C" int cuda_get_world(CudaWorld *cw, int *host_world_flat);
extern "C" void cuda_free_world(CudaWorld *cw);

static void print_world_cells(const int *world_flat, int rows, int cols, int pitch) {
    for (int r = 1; r <= rows; ++r) {
        for (int c = 1; c <= cols; ++c) {
            int v = world_flat[r * pitch + c];
            putchar(v ? 'x' : '.');
            if (c < cols) putchar(' ');
        }
        putchar('\n');
    }
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: run_cuda <generations> <input_file>\n");
        return 2;
    }
    int generations = atoi(argv[1]);
    const char *input = argv[2];

    DynWorld *dw = read_world_dyn(input);
    if (!dw) {
        fprintf(stderr, "Failed to read %s\n", input);
        return 2;
    }

    int rows = dw->rows;
    int cols = dw->cols;
    int pitch = dw->pitch;
    const int rule[RULE_SIZE] = {3, 2, 3};

    CudaWorld *cw = NULL;
    if (cuda_init_world(&cw, dw->data, rows, cols, pitch, rule) != 0) {
        fprintf(stderr, "cuda_init_world failed\n");
        free_world_dyn(dw);
        return 3;
    }

    if (cuda_step_n(cw, generations) != 0) {
        fprintf(stderr, "cuda_step_n failed\n");
        cuda_free_world(cw);
        free_world_dyn(dw);
        return 4;
    }

    // Copy back final world and print in the same format as run_serial
    if (cuda_get_world(cw, dw->data) != 0) {
        fprintf(stderr, "cuda_get_world failed\n");
        cuda_free_world(cw);
        free_world_dyn(dw);
        return 5;
    }

    print_world_cells(dw->data, rows, cols, pitch);

    cuda_free_world(cw);
    free_world_dyn(dw);
    return 0;
}
