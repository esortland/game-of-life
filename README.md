# Game of Life (CPU + CUDA)


# FOR GRADERS HERES HOW TO RUN OUR TEST SUBSET FOR CUDA:
### Running CUDA tests
Checkout the `cuda_evals` branch:
`git checkout cuda_evals`

Unzip the test directory:
`tar -xzf tests.tar.gz `

Go to the `life` directory:
`cd life`

Run the script:
`./run_tests.sh`

# Code Overview

This repo contains a Conway’s Game of Life implementation with:
- A CPU reference (dynamic world size, toroidal wrapping, B3/S23).
- A CUDA implementation that mirrors CPU semantics, plus small bench/verify tools.

Below are quick instructions to verify CUDA correctness ("completeness") and to benchmark CPU vs CUDA using Make targets.

Prerequisites
- Linux with CUDA toolkit (`nvcc`) for building CUDA parts
- GCC/Clang for C builds
	(Python is not required if you use the Make targets below.)

Directory map
- `life/` – sources and small tools (CPU/CUDA)
- `tests/` – test inputs (txt) — may be compressed as `tests.tar.gz`
- `docs/` – benchmark report(s)
- `scripts/` – helper automation scripts

Build and Run (Make)
```bash
cd life
# Build tools (verifier + benches)
make tools

# Quick correctness check (defaults: 100 gens on Gosper glider gun)
make verify

# Quick benches (defaults: 100 gens on Gosper glider gun)
make bench-cpu
make bench-cuda
```

Tests Archive
- To reduce repo size, the `tests/` folder may be compressed as `tests.tar.gz` at the repo root.
- Extract before running suite targets (they expect `tests/` to exist at the repo root):
```bash
cd <repo-root>
tar -xzf tests.tar.gz
# Verify the folder now exists
ls tests | head
```
- After extraction, all Make targets that reference `../tests/...` from inside `life/` will work as documented below.

Completeness testing (CUDA vs CPU)
- `make verify` runs the CPU flat update for N generations and the CUDA update for N generations and compares final inner cells. Prints `verify: OK` on match.
- Use `GENS` and `INPUT` to override defaults.

Examples
```bash
cd life
# Verify 100 generations on test 4
make verify GENS=100 INPUT=../tests/test_case_4.txt

# Verify 1, 20, 100 generations on test 5
for g in 1 20 100; do make verify GENS=$g INPUT=../tests/test_case_5.txt; done
```

Suite runs (all tests)
- Verify the full suite (Tests 1–4 by default): `make verify-all`
- Include Test 5 for verify (very large): `make verify-all INCLUDE_T5=1`
- CUDA benches across all tests: `make bench-all-cuda`
- CPU benches (Tests 1–4 by default): `make bench-all-cpu`
- Include Test 5 CPU bench (slow): `make bench-all-cpu INCLUDE_T5_CPU=1`
- Run both CPU and CUDA benches: `make bench-all`

Manual benchmarking (quick)
```bash
cd life
# CPU (prints summary + CSV line)
make bench-cpu GENS=100 INPUT=../tests/test_case_4.txt

# CUDA (end-to-end timing including final copy)
make bench-cuda GENS=100 INPUT=../tests/test_case_4.txt
```

Notes
- All timings above are end-to-end for fair comparison across CPU/CUDA.
- Tests use toroidal wrapping (edges wrap) and the B3/S23 rule.
- The CPU code in `life/src/life.c` serves as the ground truth.

Troubleshooting
- If `verify_cuda_vs_cpu` can’t find an input file, ensure you run from `life/` or pass an absolute path.
- If `nvcc` is missing, install the CUDA toolkit and ensure it’s on `PATH`.
- If suite targets fail with missing test files, make sure you have extracted `tests.tar.gz` at the repo root.

 Where to look
- CPU implementation: `life/src/life.c`
- CUDA implementation: `life/src/life_cuda.cu`
- Verifier: `life/verify_cuda_vs_cpu.c`
- Benches: `life/bench_cpu.c`, `life/bench_cuda.cu`
 - Make targets provide all verification and benchmarking; the `scripts/` directory has been removed.

