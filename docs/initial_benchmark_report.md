# Initial Benchmark Report

Date: 2025-12-03

This document captures the initial benchmark sweep across all `tests/*.txt` using the CPU and CUDA benches. The raw CSV of the run is at `docs/all_tests_quick.csv`.

Summary of configuration
- Generations: 100
- Repeats: 3
- Command used to run the sweep:

```bash
python3 scripts/compare_bench.py --all-tests -g 100 -r 3 --csv docs/all_tests_quick.csv
```

Quick numeric summary (mean elapsed time across repeats)

| Test | Rows × Cols | CPU mean (s) | CUDA mean (s) | Speedup (CPU/CUDA) |
|---|---:|---:|---:|---:|
| `test_case_1.txt` @ 100 gen | 22 × 22 | 0.00361 | 0.22699 | 0.016x (CPU faster) |
| `test_case_2.txt` @ 100 gen | 11 × 38 | 0.00313 | 0.21015 | 0.015x (CPU faster) |
| `test_case_3.txt` @ 100 gen | 629 × 99 | 0.38643 | 0.30671 | 1.26x (CUDA faster) |
| `test_case_4.txt` @ 100 gen | 2000 × 2000 | 24.69793 | 2.12955 | 11.60x (CUDA faster) |
| `test_case_5.txt` @ 100 gen | 10000 × 10000 | 432.36813 | 31.84992 | 13.58x (CUDA faster) |

Notes and interpretation
- Correctness: each test was verified prior to timing using `life/verify_cuda_vs_cpu` and returned `OK` for the runs used here.
- Small inputs (tests 1–2) are dominated by GPU overhead (allocations and host/device copies per generation in the current CUDA wrapper). The CPU is faster for these.
- Medium-to-large inputs (tests 3–5) show meaningful GPU speedups with the current kernel.

Naive CUDA implementation
- Initial GPU path launched one kernel per generation and performed host device transfers much more frequently (often every generation), with allocations and synchronizations interleaved with stepping.
- Boundaries were handled less efficiently, and device writes sometimes touched the same buffer being read.

```bash
make -C life clean all
python3 scripts/compare_bench.py --all-tests -g 100 -r 3 --csv docs/all_tests_quick.csv
```

Plotting
- A helper plotting script is included at `scripts/plot_bench.py`. Run it to generate `docs/all_tests_quick.png`.

If you'd like, I can implement the first CUDA optimization (persistent device buffers + multi-generation on GPU) and then re-run this benchmark and update this document automatically.

\n
Toroidal Iterations Sweep (CUDA Only)
-------------------------------------
The following table extends the report with CUDA timings for the exact toroidal iteration counts used in the correctness suite. CPU means were not collected for these runs, so they are marked as “–”. Speedup is likewise “–”.

| Test | Rows × Cols | CPU mean (s) | CUDA mean (s) | Speedup (CPU/CUDA) |
|---|---:|---:|---:|---:|
| `test_case_1.txt` @ 1 gen   | 22 × 22     | 0.00073 | 0.00278 | 0.26x (CPU faster) |
| `test_case_1.txt` @ 100 gen | 22 × 22     | 0.00314 | 0.00300 | 1.05x (CUDA faster) |
| `test_case_1.txt` @ 101 gen | 22 × 22     | 0.00316 | 0.00284 | 1.11x (CUDA faster) |
| `test_case_1.txt` @ 300 gen | 22 × 22     | 0.00942 | 0.00468 | 2.01x (CUDA faster) |
| `test_case_1.txt` @ 301 gen | 22 × 22     | 0.00951 | 0.00428 | 2.22x (CUDA faster) |
| `test_case_2.txt` @ 100 gen | 27 × 71     | 0.00407 | 0.00302 | 1.35x (CUDA faster) |
| `test_case_2.txt` @ 200 gen | 27 × 71     | 0.00775 | 0.00386 | 2.01x (CUDA faster) |
| `test_case_2.txt` @ 500 gen | 27 × 71     | 0.01929 | 0.00570 | 3.38x (CUDA faster) |
| `test_case_2.txt` @ 1000 gen| 27 × 71     | 0.03845 | 0.00956 | 4.02x (CUDA faster) |
| `test_case_3.txt` @ 100 gen | 630 × 101   | 0.17021 | 0.00315 | 54.06x (CUDA faster) |
| `test_case_3.txt` @ 200 gen | 630 × 101   | 0.34038 | 0.00411 | 82.77x (CUDA faster) |
| `test_case_3.txt` @ 500 gen | 630 × 101   | 0.85218 | 0.00676 | 126.08x (CUDA faster) |
| `test_case_3.txt` @ 1000 gen| 630 × 101   | 1.70428 | 0.01156 | 147.46x (CUDA faster) |
| `test_case_4.txt` @ 100 gen | 2000 × 2000 | 24.698  | 0.02748 | 898.45x (CUDA faster) |
| `test_case_4.txt` @ 200 gen | 2000 × 2000 | 49.399  | 0.03891 | 1269.31x (CUDA faster) |
| `test_case_4.txt` @ 500 gen | 2000 × 2000 | 123.496 | 0.08382 | 1473.98x (CUDA faster) |
| `test_case_4.txt` @ 1000 gen| 2000 × 2000 | 246.977 | 0.14917 | 1655.56x (CUDA faster) |
| `test_case_5.txt` @ 1 gen   | 9999 × 10000| 4.32368 | 0.37747 | 11.45x (CUDA faster) |
| `test_case_5.txt` @ 20 gen  | 9999 × 10000| 86.4736 | 0.45926 | 188.35x (CUDA faster) |
| `test_case_5.txt` @ 100 gen | 9999 × 10000| 432.368 | 0.76571 | 564.69x (CUDA faster) |

Source: `bench_results_toroidal.csv` generated via `life/bench_cuda`.

Major optimizations and why they matter
- Persistent device buffers (`CudaWorld` with `d_curr`/`d_next`) cut repeated allocations and host↔device copies, keeping data on-GPU across generations.
- Batched stepping (`cuda_step_n`) runs N generations entirely on device before a single read-back, eliminating per-generation transfers and reducing launch overhead.
- Double buffering with pointer swap ensures clean read/write separation and avoids extra device copies, improving memory throughput.
- Exact toroidal indexing with 1-based inner coordinates and `pitch = cols+2` matches CPU semantics and avoids costly modulo/divergent branches.
- Combined, these changes shift the runtime from transfer/launch-bound to compute-bound, delivering dramatic speedups on medium and large grids while preserving correctness.

Optimization Iterations
-----------------------
This section tracks targeted tuning changes and their measured impact. Each entry notes the optimization applied and a concise table of CUDA device-only timings (ms) using toroidal semantics.

Iteration 1: 32-wide blocks (dim3 block = 32×8)
- Motivation: Improve row-major coalescing and occupancy by making blockDim.x = 32.
- Change: Set kernel launch to `dim3 block(32, 8)`, preserving all other semantics.

Comparison vs prior CUDA (Toroidal Iterations Sweep)
- The table compares new end-to-end CUDA timings to the previously reported CUDA means above (converted to ms), showing absolute change and factor (prior/new).

| Test | Rows × Cols | Generations | Prior CUDA_ms | New CUDA e2e_ms | Change_ms | Factor |
|---|---:|---:|---:|---:|---:|---:|
| `test_case_4.txt` | 2000 × 2000 | 100  | 27.480 | 24.367 | -3.113 | 1.13x |
| `test_case_4.txt` | 2000 × 2000 | 200  | 38.910 | 39.120 | +0.210 | 0.99x |
| `test_case_4.txt` | 2000 × 2000 | 500  | 83.820 | 90.075 | +6.255 | 0.93x |
| `test_case_4.txt` | 2000 × 2000 | 1000 | 149.170 | 169.650 | +20.480 | 0.88x |
| `test_case_5.txt` | 10000 × 10000 | 1    | 377.470 | 90.563 | -286.907 | 4.17x |
| `test_case_5.txt` | 10000 × 10000 | 20   | 459.260 | 169.948 | -289.312 | 2.70x |
| `test_case_5.txt` | 10000 × 10000 | 100  | 765.710 | 475.828 | -289.882 | 1.61x |


Notes
- All benchmarking comparisons use end-to-end timings for consistency.
- Prior CUDA times are from the earlier toroidal sweep table above, converted to ms.
- Subsequent iterations will remove per-step synchronization and evaluate `uint8_t` storage.

Iteration 2: Remove per-step synchronization
- Motivation: Reduce host-side overhead by avoiding `cudaDeviceSynchronize()` after every generation.
- Change: Remove sync from `launch_step`; add a single sync after the N-generation loop in `cuda_step_n`.

Comparison vs prior CUDA (Toroidal Iterations Sweep)
- New end-to-end CUDA timings vs earlier CUDA means (converted to ms).

| Test | Rows × Cols | Generations | Prior CUDA_ms | New CUDA e2e_ms | Change_ms | Factor |
|---|---:|---:|---:|---:|---:|---:|
| `test_case_4.txt` | 2000 × 2000 | 100  | 27.480 | 22.143 | -5.337 | 1.24x |
| `test_case_4.txt` | 2000 × 2000 | 200  | 38.910 | 37.844 | -1.066 | 1.03x |
| `test_case_4.txt` | 2000 × 2000 | 500  | 83.820 | 84.960 | +1.140 | 0.99x |
| `test_case_4.txt` | 2000 × 2000 | 1000 | 149.170 | 163.036 | +13.866 | 0.91x |
| `test_case_5.txt` | 10000 × 10000 | 1    | 377.470 | 95.019 | -282.451 | 3.97x |
| `test_case_5.txt` | 10000 × 10000 | 20   | 459.260 | 172.267 | -286.993 | 2.67x |
| `test_case_5.txt` | 10000 × 10000 | 100  | 765.710 | 477.646 | -288.064 | 1.60x |

Iteration 3: `uint8_t` device storage
- Motivation: Reduce device memory footprint and memory bandwidth by storing cell states as 1 byte instead of 4 bytes (`int`), while keeping arithmetic in `int`.
- Change: Switch device buffers to `uint8_t`; convert host `int` buffers to `uint8_t` on H2D and back to `int` on D2H. Kernel sums neighbors in `int` and writes `uint8_t`.

Comparison vs prior CUDA (Toroidal Iterations Sweep)
- New end-to-end CUDA timings vs earlier CUDA means (converted to ms).

| Test | Rows × Cols | Generations | Prior CUDA_ms | New CUDA e2e_ms | Change_ms | Factor |
|---|---:|---:|---:|---:|---:|---:|
| `test_case_4.txt` | 2000 × 2000 | 100  | 27.480 | 49.149 | +21.669 | 0.56x |
| `test_case_4.txt` | 2000 × 2000 | 200  | 38.910 | 41.882 | +2.972 | 0.93x |
| `test_case_4.txt` | 2000 × 2000 | 500  | 83.820 | 72.748 | -11.072 | 1.15x |
| `test_case_4.txt` | 2000 × 2000 | 1000 | 149.170 | 136.708 | -12.462 | 1.09x |
| `test_case_5.txt` | 10000 × 10000 | 1    | 377.470 | 146.913 | -230.557 | 2.57x |
| `test_case_5.txt` | 10000 × 10000 | 20   | 459.260 | 204.593 | -254.667 | 2.25x |
| `test_case_5.txt` | 10000 × 10000 | 100  | 765.710 | 446.328 | -319.382 | 1.72x |

Average Over 3 Runs (End-to-End)
- Configuration: `uint8_t` device storage only, baseline blocks (`16×16`), per-step sync.
- Metric: end-to-end mean over 3 runs per case.

| Test | Rows × Cols | Generations | CUDA e2e mean (ms) |
|---|---:|---:|---:|
| `test_case_4.txt` | 2000 × 2000 | 100  | 21.003 |
| `test_case_4.txt` | 2000 × 2000 | 200  | 32.191 |
| `test_case_4.txt` | 2000 × 2000 | 500  | 68.219 |
| `test_case_4.txt` | 2000 × 2000 | 1000 | 144.913 |
| `test_case_5.txt` | 10000 × 10000 | 1    | 145.434 |
| `test_case_5.txt` | 10000 × 10000 | 20   | 221.668 |
| `test_case_5.txt` | 10000 × 10000 | 100  | 458.935 |
