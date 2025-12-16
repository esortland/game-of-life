// CUDA benchmark runner (end-to-end timing) for toroidal Game of Life
#include <cuda_runtime.h>
#include <cstdio>
#include <cstdlib>
#include <chrono>

extern "C" {
#include "include/life.h"
}

struct CudaWorld; // opaque type from life_cuda.cu
extern "C" int cuda_init_world(CudaWorld **out, const int *host_world_flat,
								int rows_count, int cols_count, int pitch, const int rule[3]);
extern "C" int cuda_step_n(CudaWorld *cw, int n);
extern "C" int cuda_get_world(CudaWorld *cw, int *host_world_flat);
extern "C" void cuda_free_world(CudaWorld *cw);

static void usage(const char *p) { std::fprintf(stderr, "Usage: %s <input_file> <generations>\n", p); }

int main(int argc, char **argv) {
	if (argc < 3) { usage(argv[0]); return 2; }
	const char *file = argv[1];
	int generations = std::atoi(argv[2]);
	if (generations < 0) generations = 0;

	DynWorld *dw = read_world_dyn(file);
	if (!dw) { std::fprintf(stderr, "Failed to read %s\n", file); return 3; }

	const int rule[3] = {3,2,3};
	CudaWorld *cw = nullptr;
	if (cuda_init_world(&cw, dw->data, dw->rows, dw->cols, dw->pitch, rule) != 0) {
		std::fprintf(stderr, "cuda_init_world failed\n");
		free_world_dyn(dw);
		return 4;
	}

	auto t0 = std::chrono::high_resolution_clock::now();
	if (cuda_step_n(cw, generations) != 0) {
		std::fprintf(stderr, "cuda_step_n failed\n");
		cuda_free_world(cw);
		free_world_dyn(dw);
		return 5;
	}
	if (cuda_get_world(cw, dw->data) != 0) {
		std::fprintf(stderr, "cuda_get_world failed\n");
		cuda_free_world(cw);
		free_world_dyn(dw);
		return 6;
	}
	auto t1 = std::chrono::high_resolution_clock::now();
	double e2e_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

	std::printf("rows=%d cols=%d gens=%d e2e_ms=%.3f\n", dw->rows, dw->cols, generations, e2e_ms);

	cuda_free_world(cw);
	free_world_dyn(dw);
	return 0;
}
