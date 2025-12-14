#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "life.h"

/* ---------------- CELL UPDATE RULE ---------------- */

int cell_lives(const int submatrix[3][3], const int rule[RULE_SIZE])
{
    int cell = submatrix[1][1];
    int live_cells = 0;

    for (int row = 0; row < 3; row++)
    {
        for (int col = 0; col < 3; col++)
        {
            if (row == 1 && col == 1)
                continue;
            live_cells += submatrix[row][col];
        }
    }

    return (cell == 1 && (live_cells >= rule[1] && live_cells <= rule[0])) ||
           (cell == 0 && live_cells == rule[2]);
}

/* ---------------- WORLD OPS ---------------- */

void clear_world(int world[][MAX_COLS], int rows_count, int cols_count)
{
    if (cols_count > VIRTUAL_MAX_COLS || rows_count < 2 || cols_count < 2)
        return;

    for (int row = 1; row <= rows_count; row++)
        for (int col = 1; col <= cols_count; col++)
            world[row][col] = 0;
}

void set_cell(int world[][MAX_COLS], int row, int col, int value)
{
    world[row][col] = value;
}

int get_cell(const int world[][MAX_COLS], int row, int col)
{
    return world[row][col];
}

void copy_world(
    int world1[][MAX_COLS], int rows_count, int cols_count,
    int world2[][MAX_COLS])
{
    for (int row = 1; row <= rows_count; row++)
        for (int col = 1; col <= cols_count; col++)
            world1[row][col] = world2[row][col];
}

/* ---------------- UPDATE WORLD ---------------- */

void update_world(
    int world[][MAX_COLS],
    int rows_count, int cols_count,
    int world_aux[][MAX_COLS], const int rule[RULE_SIZE])
{
    for (int row = 1; row <= rows_count; row++)
    {
        int pre_row = (row == 1) ? rows_count : row - 1;
        int pos_row = (row == rows_count) ? 1 : row + 1;

        for (int col = 1; col <= cols_count; col++)
        {
            int pre_col = (col == 1) ? cols_count : col - 1;
            int pos_col = (col == cols_count) ? 1 : col + 1;

            int submatrix[3][3];

            submatrix[0][0] = get_cell(world, pre_row, pre_col);
            submatrix[0][1] = get_cell(world, pre_row, col);
            submatrix[0][2] = get_cell(world, pre_row, pos_col);
            submatrix[1][0] = get_cell(world, row, pre_col);
            submatrix[1][1] = get_cell(world, row, col);
            submatrix[1][2] = get_cell(world, row, pos_col);
            submatrix[2][0] = get_cell(world, pos_row, pre_col);
            submatrix[2][1] = get_cell(world, pos_row, col);
            submatrix[2][2] = get_cell(world, pos_row, pos_col);

            set_cell(world_aux, row, col, cell_lives(submatrix, rule));
        }
    }

    copy_world(world, rows_count, cols_count, world_aux);
}

/* NOTE: fix loop condition — should run n times, not n+1 */
void update_world_n_generations(
    int n, int world[][MAX_COLS],
    int rows_count, int cols_count,
    int world_aux[][MAX_COLS],
    const int rule[RULE_SIZE])
{
    if (n <= 0)
        return;

    for (int i = 0; i < n; i++)
        update_world(world, rows_count, cols_count, world_aux, rule);
}

/* ---------------- PRINTING ---------------- */

void fprint_world(const int world[][MAX_COLS], int rows_count, int cols_count,
                  FILE *__restrict__ __stream)
{
    for (int row = 1; row <= rows_count; row++)
    {
        for (int col = 1; col <= cols_count; col++)
            fputs(world[row][col] ? "X " : ". ", __stream);
        fputc('\n', __stream);
    }
}

void print_world(const int world[][MAX_COLS], int rows_count, int cols_count)
{
    fprint_world(world, rows_count, cols_count, stdout);
}

void write_world(const int world[][MAX_COLS], int rows_count, int cols_count,
                 const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        perror("write_world fopen");
        exit(1);
    }

    fprint_world(world, rows_count, cols_count, file);
    fclose(file);
}

/* ---------------- FILE I/O ---------------- */

void read_world_size(int world_size[2], const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("read_world_size fopen");
        exit(1);
    }

    int rows = 0;
    int cols = 0;
    int current_cols = 0;

    for (int c = getc(file); c != EOF; c = getc(file))
    {
        if (c == 'X' || c == 'x' || c == '.')
        {
            current_cols++;
        }
        else if (c == '\n')
        {
            rows++;
            if (current_cols > cols)
                cols = current_cols;
            current_cols = 0;
        }
    }

    if (current_cols > 0)
    {
        rows++;
        if (current_cols > cols)
            cols = current_cols;
    }

    fclose(file);

    world_size[0] = rows;
    world_size[1] = cols;
}

void read_world(int world[][MAX_COLS],
                int world_size[2],
                const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("read_world fopen");
        exit(1);
    }

    int row = 1;
    int col = 1;
    int max_cols = 0;
    int rows_with_data = 0;

    for (int c = getc(file); c != EOF; c = getc(file))
    {
        switch (c)
        {
        case 'X':
        case 'x':
        case '.':
            if (row > FILE_MAX_LINES || col > VIRTUAL_MAX_COLS)
            {
                fprintf(stderr,
                        "Error: world in %s exceeds compile-time limits\n",
                        filename);
                fclose(file);
                exit(1);
            }

            world[row][col] = (c == 'X' || c == 'x');
            col++;

            break;

        case '\n':
            if (col > 1)
            {
                rows_with_data = row;
                if (col - 1 > max_cols)
                    max_cols = col - 1;
            }
            row++;
            col = 1;
            break;

        default:
            /* ignore spaces and other junk */
            break;
        }
    }

    /* Handle case where file doesn't end with newline */
    if (col > 1)
    {
        rows_with_data = row;
        if (col - 1 > max_cols)
            max_cols = col - 1;
    }

    fclose(file);

    world_size[0] = rows_with_data;
    world_size[1] = max_cols;
}
