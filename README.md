# Parallelized Conway’s Game of Life Exploration Using OpenMP and CUDA

**CSCI 5451 Course Project**

**Project Members:** Andrea Arifin, Mari Duda, Matt Rajala, Eric Sortland

---

<p align="center">
  <img align="center" src="docs/animations/animation.gif" alt="Main Gif"/>
</p>

---

## Repository Structure

This repository contains multiple implementations of Conway’s Game of Life, organized by branch:

- **`main`**  
  Baseline serial C implementation.

- **`openmp`**  
  OpenMP-parallelized implementation.

- **`sortl005/CUDA`**  
  CUDA-based GPU implementation.

Each branch contains its own build and run instructions.

### Running CUDA tests
Checkout the `cuda_evals` branch:
`git checkout cuda_evals`

Unzip the test directory:
`tar -xzf tests.tar.gz `

Go to the `life` directory:
`cd life`

Run the script:
`./run_tests.sh`

### Running OMP tests
`git checkout omp_evals`

Go to the `life` directory:
`cd life`

Run the script:
`./run_tests.sh`

---

## Attribution

The baseline C implementation is a fork of existing work. We made small modifications to improve correctness and to support larger test sizes. All OpenMP and CUDA parallelization work was implemented by the project team.
