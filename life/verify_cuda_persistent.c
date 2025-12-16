#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include "life.h"
}

extern "C" int cuda_init_world(CudaWorld **out, const int *host_world_flat, int rows_count, int cols_count, int pitch, const int rule[3]);
extern "C" int cuda_step_n(CudaWorld *cw, int n);
extern "C" int cuda_get_world(CudaWorld *cw, int *host_world_flat);
extern "C" void cuda_free_world(CudaWorld *cw);

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: verify_cuda_persistent <generations> <input_file>\n");
        return 2;
    }
    int gens = atoi(argv[1]);
    const char *input = argv[2];

    DynWorld *dw = read_world_dyn(input);
    if (!dw) {
        fprintf(stderr, "Failed to read %s\n", input);
        return 2;
    }

    int rows = dw->rows;
    int cols = dw->cols;
    int pitch = dw->pitch;
    int total_rows = rows + 2;
    size_t total_elems = (size_t)total_rows * (size_t)pitch;

    int *cpu_buf = (int *)malloc(total_elems * sizeof(int));
    int *aux_buf = (int *)malloc(total_elems * sizeof(int));
    if (!cpu_buf || !aux_buf) {
        fprintf(stderr, "OOM allocating temp buffers\n");
        free_world_dyn(dw);
        return 2;
    }

    memcpy(cpu_buf, dw->data, total_elems * sizeof(int));

    const int rule[3] = {3,2,3};

    // run CPU
    update_world_n_generations_flat(gens, cpu_buf, rows, cols, aux_buf, rule, pitch);

    // run persistent CUDA
    CudaWorld *cw = NULL;
    if (cuda_init_world(&cw, dw->data, rows, cols, pitch, (const int*)rule) != 0) {
        fprintf(stderr, "cuda_init_world failed\n");
        free(cpu_buf); free(aux_buf); free_world_dyn(dw);
        return 3;
    }
    if (cuda_step_n(cw, gens) != 0) {
        fprintf(stderr, "cuda_step_n failed\n");
        cuda_free_world(cw); free(cpu_buf); free(aux_buf); free_world_dyn(dw);
        return 4;
    }
    int *gpu_out = (int *)malloc(total_elems * sizeof(int));
    if (!gpu_out) {
        fprintf(stderr, "OOM allocating gpu_out\n");
        cuda_free_world(cw); free(cpu_buf); free(aux_buf); free_world_dyn(dw);
        return 2;
    }
    if (cuda_get_world(cw, gpu_out) != 0) {
        fprintf(stderr, "cuda_get_world failed\n");
        cuda_free_world(cw); free(cpu_buf); free(aux_buf); free(gpu_out); free_world_dyn(dw);
        return 5;
    }

    // Compare inner region
    for (int r = 1; r <= rows; ++r) {
        for (int c = 1; c <= cols; ++c) {
            int idx = r * pitch + c;
            if (cpu_buf[idx] != gpu_out[idx]) {
                fprintf(stderr, "MISMATCH at r=%d c=%d: cpu=%d gpu=%d\n", r, c, cpu_buf[idx], gpu_out[idx]);
                cuda_free_world(cw); free(cpu_buf); free(aux_buf); free(gpu_out); free_world_dyn(dw);
                return 6;
            }
        }
    }

    printf("verify_persistent: OK (generations=%d, rows=%d, cols=%d)\n", gens, rows, cols);

    cuda_free_world(cw);
    free(cpu_buf); free(aux_buf); free(gpu_out); free_world_dyn(dw);
    return 0;
}
