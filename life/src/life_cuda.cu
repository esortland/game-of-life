// CUDA implementation matching CPU toroidal semantics (B3/S23)
#include <cuda_runtime.h>
#include <cstdio>
#include <cstdlib>

extern "C" {
#include "life.h"
}

struct CudaWorld {
	int rows;
	int cols;
	int pitch; // cols + 2
	int rule[3];
	int *d_curr;
	int *d_next;
};

__global__ void gol_step_kernel(const int *curr, int *next, int rows, int cols, int pitch,
								int maxN, int minN, int bornN)
{
	int r = blockIdx.y * blockDim.y + threadIdx.y + 1; // inner indices 1..rows
	int c = blockIdx.x * blockDim.x + threadIdx.x + 1; // inner indices 1..cols
	if (r > rows || c > cols) return;

	int pre_row = r - 1; if (pre_row == 0) pre_row = rows;
	int pos_row = r + 1; if (pos_row == rows + 1) pos_row = 1;
	int pre_col = c - 1; if (pre_col == 0) pre_col = cols;
	int pos_col = c + 1; if (pos_col == cols + 1) pos_col = 1;

	int live = 0;
	live += curr[pre_row * pitch + pre_col];
	live += curr[pre_row * pitch + c];
	live += curr[pre_row * pitch + pos_col];
	live += curr[r * pitch + pre_col];
	live += curr[r * pitch + pos_col];
	live += curr[pos_row * pitch + pre_col];
	live += curr[pos_row * pitch + c];
	live += curr[pos_row * pitch + pos_col];

	int cell = curr[r * pitch + c];
	int lives = (cell == 1 && (live >= minN && live <= maxN)) || (cell == 0 && live == bornN);
	next[r * pitch + c] = lives ? 1 : 0;
}

static void launch_step(const int *d_in, int *d_out, const CudaWorld *cw)
{
	dim3 block(16, 16);
	dim3 grid((cw->cols + block.x - 1) / block.x, (cw->rows + block.y - 1) / block.y);
	gol_step_kernel<<<grid, block>>>(d_in, d_out, cw->rows, cw->cols, cw->pitch,
									 cw->rule[0], cw->rule[1], cw->rule[2]);
	cudaDeviceSynchronize();
}

extern "C" int cuda_init_world(CudaWorld **out, const int *host_world_flat, int rows_count, int cols_count, int pitch, const int rule[3])
{
	if (!out || !host_world_flat || rows_count <= 0 || cols_count <= 0 || pitch <= 0)
		return -1;
	CudaWorld *cw = (CudaWorld*)std::malloc(sizeof(CudaWorld));
	if (!cw) return -2;
	cw->rows = rows_count;
	cw->cols = cols_count;
	cw->pitch = pitch;
	cw->rule[0] = rule[0]; cw->rule[1] = rule[1]; cw->rule[2] = rule[2];
	size_t total_elems = (size_t)(rows_count + 2) * (size_t)pitch;
	if (cudaMalloc(&cw->d_curr, total_elems * sizeof(int)) != cudaSuccess) { std::free(cw); return -3; }
	if (cudaMalloc(&cw->d_next, total_elems * sizeof(int)) != cudaSuccess) { cudaFree(cw->d_curr); std::free(cw); return -4; }
	if (cudaMemcpy(cw->d_curr, host_world_flat, total_elems * sizeof(int), cudaMemcpyHostToDevice) != cudaSuccess) {
		cudaFree(cw->d_curr); cudaFree(cw->d_next); std::free(cw); return -5;
	}
	// Initialize next buffer to zero
	cudaMemset(cw->d_next, 0, total_elems * sizeof(int));
	*out = cw;
	return 0;
}

extern "C" int cuda_step_n(CudaWorld *cw, int n)
{
	if (!cw || n < 0) return -1;
	for (int i = 0; i < n; ++i) {
		launch_step(cw->d_curr, cw->d_next, cw);
		// swap buffers
		int *tmp = cw->d_curr;
		cw->d_curr = cw->d_next;
		cw->d_next = tmp;
	}
	return 0;
}

extern "C" int cuda_get_world(CudaWorld *cw, int *host_world_flat)
{
	if (!cw || !host_world_flat) return -1;
	size_t total_elems = (size_t)(cw->rows + 2) * (size_t)cw->pitch;
	if (cudaMemcpy(host_world_flat, cw->d_curr, total_elems * sizeof(int), cudaMemcpyDeviceToHost) != cudaSuccess)
		return -2;
	return 0;
}

extern "C" void cuda_free_world(CudaWorld *cw)
{
	if (!cw) return;
	cudaFree(cw->d_curr);
	cudaFree(cw->d_next);
	std::free(cw);
}

// Single-step CUDA update that mirrors update_world_flat semantics (host API)
extern "C" void update_world_cuda(int *world_flat, int rows_count, int cols_count, int *world_aux_flat, const int rule[RULE_SIZE], int pitch)
{
	size_t total_elems = (size_t)(rows_count + 2) * (size_t)pitch;
	int *d_in = nullptr, *d_out = nullptr;
	cudaMalloc(&d_in, total_elems * sizeof(int));
	cudaMalloc(&d_out, total_elems * sizeof(int));
	cudaMemcpy(d_in, world_flat, total_elems * sizeof(int), cudaMemcpyHostToDevice);
	cudaMemset(d_out, 0, total_elems * sizeof(int));

	CudaWorld cw{rows_count, cols_count, pitch, {rule[0], rule[1], rule[2]}, d_in, d_out};
	launch_step(d_in, d_out, &cw);

	// Copy result for inner region back to aux
	cudaMemcpy(world_aux_flat, d_out, total_elems * sizeof(int), cudaMemcpyDeviceToHost);

	// Mirror CPU: copy aux back into world for inner cells
	for (int r = 1; r <= rows_count; ++r) {
		for (int c = 1; c <= cols_count; ++c) {
			world_flat[r * pitch + c] = world_aux_flat[r * pitch + c];
		}
	}

	cudaFree(d_in);
	cudaFree(d_out);
}

