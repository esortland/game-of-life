#pragma once

#include "life.h"

// OpenMP version of update_world().
/**
 * Parallel version of update_world() using OpenMP.
 * Computes the next generation of the Game of Life grid.
 *
 * world      : current grid
 * rows_count : number of rows (without ghost borders)
 * cols_count : number of cols (without ghost borders)
 * world_aux  : auxiliary grid where updates are written
 * rule       : Conway rule {maxNeighbors, minNeighbors, neighborsToBorn}
 */

// Same parameters as the sequential version in life.c.
void update_world_omp(
    int world[][MAX_COLS],
    int rows_count, int cols_count,
    int world_aux[][MAX_COLS],
    const int rule[RULE_SIZE]
);

void update_world_n_generations_omp(
    int n, int world[][MAX_COLS],
    int rows_count, int cols_count,
    int world_aux[][MAX_COLS],
    const int rule[RULE_SIZE]);
