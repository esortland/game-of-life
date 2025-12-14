#include <omp.h>
#include "life_omp.h"

void update_world_omp(
    int world[][MAX_COLS],
    int rows_count, int cols_count,
    int world_aux[][MAX_COLS],
    const int rule[RULE_SIZE])
{
    #pragma omp parallel for collapse(2)
    for (size_t row = 1; row < rows_count + 1; row++)
    {
        for (size_t col = 1; col < cols_count + 1; col++)
        {
            int pre_row = row - 1;
            if (pre_row == 0)
                pre_row = rows_count;

            int pos_row = row + 1;
            if (pos_row == rows_count + 1)
                pos_row = 1;

            int pre_col = col - 1;
            if (pre_col == 0)
                pre_col = cols_count;

            int pos_col = col + 1;
            if (pos_col == cols_count + 1)
                pos_col = 1;

            int submatrix[3][3];
            submatrix[0][0] = world[pre_row][pre_col];
            submatrix[0][1] = world[pre_row][col];
            submatrix[0][2] = world[pre_row][pos_col];

            submatrix[1][0] = world[row][pre_col];
            submatrix[1][1] = world[row][col];
            submatrix[1][2] = world[row][pos_col];

            submatrix[2][0] = world[pos_row][pre_col];
            submatrix[2][1] = world[pos_row][col];
            submatrix[2][2] = world[pos_row][pos_col];

            world_aux[row][col] = cell_lives(submatrix, rule);
        }
    }

    // Copy results back — can parallelize but not necessary
    copy_world(world, rows_count, cols_count, world_aux);
}


void update_world_n_generations_omp(
    int n, int world[][MAX_COLS],
    int rows_count, int cols_count,
    int world_aux[][MAX_COLS],
    const int rule[RULE_SIZE])
{
    if (n <= 0)
        return;

    for (int i = 0; i < n; i++)
    {
        update_world_omp(world, rows_count, cols_count, world_aux, rule);
    }
}
