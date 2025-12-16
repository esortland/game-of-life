CC := gcc
NVCC := nvcc
SRC_DIR := src
BUILD_DIR := build
INCLUDE_DIR := include
CFLAGS := -O2 -Wall -I$(INCLUDE_DIR)
NVCCFLAGS = -O2 -std=c++17 -I$(INCLUDE_DIR)

# ------------------------------------------------------------
# Default run parameters (override on the command line)
# Example: make verify GENS=200 INPUT=../docs/initial_worlds/one_glider.txt
# ------------------------------------------------------------
GENS ?= 100
INPUT ?= ../docs/initial_worlds/Gosper_glider_gun.txt
REPEAT ?= 1

# ------------------------------------------------------------
# Test suite definitions (paths relative to life/)
# ------------------------------------------------------------
TESTS_DIR := ../tests
T1 := $(TESTS_DIR)/test_case_1.txt
T2 := $(TESTS_DIR)/test_case_2.txt
T3 := $(TESTS_DIR)/test_case_3.txt
T4 := $(TESTS_DIR)/test_case_4.txt
T5 := $(TESTS_DIR)/test_case_5.txt

T1_GENS := 1 100 101 300 301
T2_GENS := 100 200 500 1000
T3_GENS := 100 200 500 1000
T4_GENS := 100 200 500 1000
T5_GENS := 5 20 50 100

_SRC_FILES := life_test.c life.c life_rules_test.c
SRC_FILES := ${addprefix ${SRC_DIR}/, ${_SRC_FILES}}

TARGETS := life_test rules_test

all: ${TARGETS}

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

life_test: | $(BUILD_DIR)
	${CC} ${CFLAGS} ${SRC_FILES} -o ${BUILD_DIR}/$@ && ./${BUILD_DIR}/$@

rules_test: | $(BUILD_DIR)
	${CC} ${CFLAGS} ${SRC_DIR}/life_rules_test.c ${SRC_DIR}/life.c -o ${BUILD_DIR}/$@ && ./${BUILD_DIR}/$@

# ------------------------------------------------------------
# Tools: CUDA verifier + benches (CPU and CUDA)
# ------------------------------------------------------------

$(BUILD_DIR)/life_cuda.o: ${SRC_DIR}/life_cuda.cu | $(BUILD_DIR)
	${NVCC} ${NVCCFLAGS} -c $< -o $@

$(BUILD_DIR)/verify_cuda_vs_cpu: verify_cuda_vs_cpu.c $(BUILD_DIR)/life_cuda.o ${SRC_DIR}/life.c | $(BUILD_DIR)
	${NVCC} ${NVCCFLAGS} $^ -o $@

$(BUILD_DIR)/bench_cuda: bench_cuda.cu $(BUILD_DIR)/life_cuda.o ${SRC_DIR}/life.c | $(BUILD_DIR)
	${NVCC} ${NVCCFLAGS} $^ -o $@

$(BUILD_DIR)/bench_cpu: bench_cpu.c ${SRC_DIR}/life.c | $(BUILD_DIR)
	${CC} ${CFLAGS} $^ -o $@

# Runner that prints world in test format (for expected-output diffs)
$(BUILD_DIR)/run_serial: run_serial.c ${SRC_DIR}/life.c | $(BUILD_DIR)
	${CC} ${CFLAGS} $^ -o $@

.PHONY: tools
tools: $(BUILD_DIR)/verify_cuda_vs_cpu $(BUILD_DIR)/bench_cuda $(BUILD_DIR)/bench_cpu $(BUILD_DIR)/run_serial
	@echo "Built tools under $(BUILD_DIR)/: verify_cuda_vs_cpu, bench_cuda, bench_cpu"

# ------------------------------------------------------------
# Run targets: verify correctness and benchmark CPU/CUDA
# ------------------------------------------------------------
.PHONY: verify bench-cpu bench-cuda bench

# Verify CUDA vs CPU produce identical final world
verify: tools
	@echo "Verifying CUDA vs CPU (gens=$(GENS), input=$(INPUT))..."
	@$(BUILD_DIR)/verify_cuda_vs_cpu $(GENS) $(INPUT)

# CPU benchmark (prints human summary and CSV line)
bench-cpu: tools
	@echo "CPU bench (gens=$(GENS), input=$(INPUT))..."
	@$(BUILD_DIR)/bench_cpu $(GENS) $(INPUT)

# CUDA benchmark (end-to-end timing)
bench-cuda: tools
	@echo "CUDA bench (gens=$(GENS), input=$(INPUT))..."
	@$(BUILD_DIR)/bench_cuda $(INPUT) $(GENS)

# Run both CPU and CUDA benches (optionally REPEAT=N times)
bench: tools
	@i=1; \
	while [ $$i -le $(REPEAT) ]; do \
	  echo "--- Bench run $$i/$(REPEAT) ---"; \
	  $(MAKE) --no-print-directory bench-cpu GENS=$(GENS) INPUT=$(INPUT); \
	  $(MAKE) --no-print-directory bench-cuda GENS=$(GENS) INPUT=$(INPUT); \
	  i=$$((i+1)); \
	done

# ------------------------------------------------------------
# Expected toroidal outputs validation
# Compares run_serial output against precomputed *_it_toroidal.txt files
# ------------------------------------------------------------
.PHONY: verify-expected verify-expected-t5

verify-expected: tools
	@set -e; \
	pad3() { printf "%03d" "$$1"; }; \
	echo "[expected] Test 1: $(T1) gens: $(T1_GENS)"; \
	for g in $(T1_GENS); do \
	  exp="$(TESTS_DIR)/test_case_1_$$(pad3 $$g)_it_toroidal.txt"; \
	  out="$(BUILD_DIR)/out_t1_$$g.txt"; \
	  $(BUILD_DIR)/run_serial $$g $(T1) > $$out; \
	  diff -q $$out $$exp >/dev/null && echo "PASS t1 g=$$g" || (echo "FAIL t1 g=$$g"; diff -w $$out $$exp | head -n 50; exit 1); \
	done; \
	echo "[expected] Test 2: $(T2) gens: $(T2_GENS)"; \
	for g in $(T2_GENS); do \
	  exp="$(TESTS_DIR)/test_case_2_$$(pad3 $$g)_it_toroidal.txt"; \
	  out="$(BUILD_DIR)/out_t2_$$g.txt"; \
	  $(BUILD_DIR)/run_serial $$g $(T2) > $$out; \
	  diff -q $$out $$exp >/dev/null && echo "PASS t2 g=$$g" || (echo "FAIL t2 g=$$g"; diff -w $$out $$exp | head -n 50; exit 1); \
	done; \
	echo "[expected] Test 3: $(T3) gens: $(T3_GENS)"; \
	for g in $(T3_GENS); do \
	  exp="$(TESTS_DIR)/test_case_3_$$(pad3 $$g)_it_toroidal.txt"; \
	  out="$(BUILD_DIR)/out_t3_$$g.txt"; \
	  $(BUILD_DIR)/run_serial $$g $(T3) > $$out; \
	  diff -q $$out $$exp >/dev/null && echo "PASS t3 g=$$g" || (echo "FAIL t3 g=$$g"; diff -w $$out $$exp | head -n 50; exit 1); \
	done; \
	echo "[expected] Test 4: $(T4) gens: $(T4_GENS)"; \
	for g in $(T4_GENS); do \
	  exp="$(TESTS_DIR)/test_case_4_$$(pad3 $$g)_it_toroidal.txt"; \
	  out="$(BUILD_DIR)/out_t4_$$g.txt"; \
	  $(BUILD_DIR)/run_serial $$g $(T4) > $$out; \
	  diff -q $$out $$exp >/dev/null && echo "PASS t4 g=$$g" || (echo "FAIL t4 g=$$g"; diff -w $$out $$exp | head -n 50; exit 1); \
	done; \
	echo "[expected] Skipping Test 5 by default; use verify-expected-t5 to include."

verify-expected-t5: tools
	@set -e; \
	pad3() { printf "%03d" "$$1"; }; \
	echo "[expected] Test 5: $(T5) gens: $(T5_GENS)"; \
	for g in $(T5_GENS); do \
	  exp="$(TESTS_DIR)/test_case_5_$$(pad3 $$g)_it_toroidal.txt"; \
	  out="$(BUILD_DIR)/out_t5_$$g.txt"; \
	  $(BUILD_DIR)/run_serial $$g $(T5) > $$out; \
	  diff -q $$out $$exp >/dev/null && echo "PASS t5 g=$$g" || (echo "FAIL t5 g=$$g"; diff -w $$out $$exp | head -n 50; exit 1); \
	done

# ------------------------------------------------------------
# Suite targets (iterate over all tests)
# Notes:
#  - verify-all runs CUDA vs CPU parity for Tests 1-4 (T5 is extremely large).
#    Set INCLUDE_T5=1 to include T5 at your own risk.
#  - bench-all-cpu runs CPU benches for Tests 1-4 by default (set INCLUDE_T5_CPU=1 to include T5).
#  - bench-all-cuda runs CUDA benches for Tests 1-5.
#  - bench-all runs both CPU and CUDA benches (skips CPU on T5 unless INCLUDE_T5_CPU=1).
# ------------------------------------------------------------
.PHONY: verify-all bench-all bench-all-cpu bench-all-cuda

verify-all: tools
	@set -e; \
	echo "[verify] Test 1: $(T1) gens: $(T1_GENS)"; \
	for g in $(T1_GENS); do $(BUILD_DIR)/verify_cuda_vs_cpu $$g $(T1); done; \
	echo "[verify] Test 2: $(T2) gens: $(T2_GENS)"; \
	for g in $(T2_GENS); do $(BUILD_DIR)/verify_cuda_vs_cpu $$g $(T2); done; \
	echo "[verify] Test 3: $(T3) gens: $(T3_GENS)"; \
	for g in $(T3_GENS); do $(BUILD_DIR)/verify_cuda_vs_cpu $$g $(T3); done; \
	echo "[verify] Test 4: $(T4) gens: $(T4_GENS)"; \
	for g in $(T4_GENS); do $(BUILD_DIR)/verify_cuda_vs_cpu $$g $(T4); done; \
	if [ "$(INCLUDE_T5)" = "1" ]; then \
	  echo "[verify] Test 5: $(T5) gens: $(T5_GENS) (LARGE!)"; \
	  for g in $(T5_GENS); do $(BUILD_DIR)/verify_cuda_vs_cpu $$g $(T5); done; \
	else \
	  echo "[verify] Skipping Test 5 by default (set INCLUDE_T5=1 to include)"; \
	fi

bench-all-cpu: tools
	@set -e; \
	echo "[bench-cpu] Test 1: $(T1) gens: $(T1_GENS)"; \
	for g in $(T1_GENS); do $(BUILD_DIR)/bench_cpu $$g $(T1); echo; done; \
	echo "[bench-cpu] Test 2: $(T2) gens: $(T2_GENS)"; \
	for g in $(T2_GENS); do $(BUILD_DIR)/bench_cpu $$g $(T2); echo; done; \
	echo "[bench-cpu] Test 3: $(T3) gens: $(T3_GENS)"; \
	for g in $(T3_GENS); do $(BUILD_DIR)/bench_cpu $$g $(T3); echo; done; \
	echo "[bench-cpu] Test 4: $(T4) gens: $(T4_GENS)"; \
	for g in $(T4_GENS); do $(BUILD_DIR)/bench_cpu $$g $(T4); echo; done; \
	if [ "$(INCLUDE_T5_CPU)" = "1" ]; then \
	  echo "[bench-cpu] Test 5: $(T5) gens: $(T5_GENS) (LARGE & SLOW)"; \
	  for g in $(T5_GENS); do $(BUILD_DIR)/bench_cpu $$g $(T5); echo; done; \
	else \
	  echo "[bench-cpu] Skipping Test 5 by default (set INCLUDE_T5_CPU=1 to include)"; \
	fi

bench-all-cuda: tools
	@set -e; \
	echo "[bench-cuda] Test 1: $(T1) gens: $(T1_GENS)"; \
	for g in $(T1_GENS); do $(BUILD_DIR)/bench_cuda $(T1) $$g; done; \
	echo "[bench-cuda] Test 2: $(T2) gens: $(T2_GENS)"; \
	for g in $(T2_GENS); do $(BUILD_DIR)/bench_cuda $(T2) $$g; done; \
	echo "[bench-cuda] Test 3: $(T3) gens: $(T3_GENS)"; \
	for g in $(T3_GENS); do $(BUILD_DIR)/bench_cuda $(T3) $$g; done; \
	echo "[bench-cuda] Test 4: $(T4) gens: $(T4_GENS)"; \
	for g in $(T4_GENS); do $(BUILD_DIR)/bench_cuda $(T4) $$g; done; \
	echo "[bench-cuda] Test 5: $(T5) gens: $(T5_GENS)"; \
	for g in $(T5_GENS); do $(BUILD_DIR)/bench_cuda $(T5) $$g; done

bench-all: tools
	@$(MAKE) --no-print-directory bench-all-cpu; \
	$(MAKE) --no-print-directory bench-all-cuda

.PHONY: clean
clean:
	# Remove all build artifacts, including tools built by `make tools`
	rm -rf $(BUILD_DIR)
	# Remove any ad-hoc binaries created outside of build (e.g., legacy scripts)
	rm -f run_serial bench_cpu bench_cuda verify_cuda_vs_cpu life_cuda.o life_test rules_test

.PHONY: all
