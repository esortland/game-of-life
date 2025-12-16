#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "life.h"

int cell_lives(const int submatrix[3][3], const int rule[RULE_SIZE])
{
	/* Conway's Game of Life (B3/S23):
	 * - Any live cell with fewer than two live neighbours dies (underpopulation).
	 * - Any live cell with two or three live neighbours lives on.
	 * - Any live cell with more than three live neighbours dies (overpopulation).
	 * - Any dead cell with exactly three live neighbours becomes a live cell (reproduction).
	 * The provided rule array is expected to be {3, 2, 3} but we compute directly.
	 */
	int center = submatrix[1][1] ? 1 : 0;
	int neighbors = 0;
	for (int r = 0; r < 3; ++r) {
		for (int c = 0; c < 3; ++c) {
			if (r == 1 && c == 1) continue; /* exclude center */
			neighbors += (submatrix[r][c] ? 1 : 0);
		}
	}

	if (center) {
		return (neighbors == 2 || neighbors == 3) ? 1 : 0;
	} else {
		return (neighbors == 3) ? 1 : 0;
	}
}

void clear_world(int world[][MAX_COLS], int rows_count, int cols_count)
{
	if (cols_count > VIRTUAL_MAX_COLS || rows_count < 2 || cols_count < 2)
		return;

	for (size_t row = 1; row < rows_count + 1; row++)
	{
		for (size_t col = 1; col < cols_count + 1; col++)
		{
			world[row][col] = 0;
		}
	}
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
	for (size_t row = 1; row < rows_count + 1; row++)
	{
		for (size_t col = 1; col < cols_count + 1; col++)
		{
			world1[row][col] = world2[row][col];
		}
	}
}

void update_world(
	int world[][MAX_COLS],
	int rows_count, int cols_count,
	int world_aux[][MAX_COLS], const int rule[RULE_SIZE])
{
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

void update_world_n_generations(
	int n, int world[][MAX_COLS],
	int rows_count, int cols_count,
	int world_aux[][MAX_COLS],
	const int rule[RULE_SIZE])
{
	if (n <= 0)
		return;

	for (size_t i = 0; i < (size_t)n; i++)
	{
		update_world(world, rows_count, cols_count, world_aux, rule);
	}
}

void fprint_world(const int world[][MAX_COLS], int rows_count, int cols_count, FILE *__restrict__ __stream)
{
	for (size_t row = 1; row < rows_count + 1; row++)
	{
		for (size_t col = 1; col < cols_count + 1; col++)
		{
			fputs((world[row][col]) ? "X " : ". ", __stream);
		}
		fputc('\n', __stream);
	}
}

void print_world(const int world[][MAX_COLS], int rows_count, int cols_count)
{
	fprint_world(world, rows_count, cols_count, stdout);
}

void write_world(const int world[][MAX_COLS], int rows_count, int cols_count, const char *filename)
{
	FILE *file = fopen(filename, "w");
	if (file == NULL)
	{
		printf("Error!\n");
		exit(-1);
	}

	fprint_world(world, rows_count, cols_count, file);

	fclose(file);
}

void read_world(int world[FILE_MAX_LINES + 2][MAX_COLS], int world_size[2], const char *filename)
{
	FILE *file = fopen(filename, "r");
	if (file == NULL)
	{
		printf("Error!\n");
		exit(1);
	}

	int row = 1;
	int col = 1;

	for (char c = getc(file); c != EOF; c = getc(file))
	{
		switch (c)
		{
		case '\n':
			if (row == 1)
				world_size[1] = col;

			row++;
			col = 1;
			break;
		case '.':
		case 'X':
			set_cell(world, row, col++, c == 'X');
			break;
		default:
			break;
		}
	}

	world_size[0] = row;

	fclose(file);
}

/* ---------------------- Dynamic-size support (non-disruptive) ---------------------- */
#include <errno.h>

static inline int get_cell_flat(const int *world_flat, int pitch, int row, int col)
{
	return world_flat[row * pitch + col];
}

static inline void set_cell_flat(int *world_flat, int pitch, int row, int col, int value)
{
	world_flat[row * pitch + col] = value;
}

DynWorld *alloc_world_dyn(int rows_count, int cols_count)
{
	if (rows_count <= 0 || cols_count <= 0)
		return NULL;
	DynWorld *w = (DynWorld *)malloc(sizeof(DynWorld));
	if (!w)
		return NULL;
	w->rows = rows_count;
	w->cols = cols_count;
	w->pitch = cols_count + 2;
	size_t total = (size_t)(rows_count + 2) * (size_t)w->pitch;
	w->data = (int *)calloc(total, sizeof(int));
	if (!w->data) {
		free(w);
		return NULL;
	}
	return w;
}

void free_world_dyn(DynWorld *w)
{
	if (!w) return;
	free(w->data);
	free(w);
}

/* Robust line reader using getline (POSIX). */
DynWorld *read_world_dyn(const char *filename)
{
	FILE *file = fopen(filename, "r");
	if (!file)
		return NULL;

	char *line = NULL;
	size_t cap = 0;
	ssize_t len;

	int cols = -1;
	int rows = 0;

	/* First pass: determine rows and columns (tokens per line) */
	while ((len = getline(&line, &cap, file)) != -1) {
		int tokens = 0;
		int in_token = 0;
		for (ssize_t i = 0; i < len; ++i) {
			char c = line[i];
			if (c == '.' || c == 'x' || c == 'X') {
				if (!in_token) {
					in_token = 1;
					tokens++;
				}
			} else if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
				in_token = 0;
			} else {
				/* ignore other characters */
				in_token = 0;
			}
		}
		if (tokens > 0) {
			if (cols < 0)
				cols = tokens;
			else if (tokens != cols) {
				/* inconsistent line length */
				free(line);
				fclose(file);
				return NULL;
			}
			rows++;
		}
	}

	if (cols <= 0 || rows <= 0) {
		free(line);
		fclose(file);
		return NULL;
	}

	DynWorld *w = alloc_world_dyn(rows, cols);
	if (!w) {
		free(line);
		fclose(file);
		return NULL;
	}

	/* Second pass: fill data */
	rewind(file);
	int r = 1;
	while ((len = getline(&line, &cap, file)) != -1) {
		int c_idx = 1;
		for (ssize_t i = 0; i < len; ++i) {
			char c = line[i];
			if (c == '.' || c == 'x' || c == 'X') {
				if (c_idx <= w->cols && r <= w->rows) {
					set_cell_flat(w->data, w->pitch, r, c_idx, (c == 'x' || c == 'X') ? 1 : 0);
				}
				c_idx++;
			}
		}
		if (c_idx > 1) {
			r++;
			if (r > w->rows)
				break;
		}
	}

	free(line);
	fclose(file);
	return w;
}

void update_world_flat(int *world_flat, int rows_count, int cols_count, int *world_aux_flat, const int rule[RULE_SIZE], int pitch)
{
	for (int row = 1; row <= rows_count; ++row) {
		for (int col = 1; col <= cols_count; ++col) {
			int pre_row = row - 1; if (pre_row == 0) pre_row = rows_count;
			int pos_row = row + 1; if (pos_row == rows_count + 1) pos_row = 1;
			int pre_col = col - 1; if (pre_col == 0) pre_col = cols_count;
			int pos_col = col + 1; if (pos_col == cols_count + 1) pos_col = 1;

			int live_cells = 0;
			live_cells += get_cell_flat(world_flat, pitch, pre_row, pre_col);
			live_cells += get_cell_flat(world_flat, pitch, pre_row, col);
			live_cells += get_cell_flat(world_flat, pitch, pre_row, pos_col);
			live_cells += get_cell_flat(world_flat, pitch, row, pre_col);
			live_cells += get_cell_flat(world_flat, pitch, row, pos_col);
			live_cells += get_cell_flat(world_flat, pitch, pos_row, pre_col);
			live_cells += get_cell_flat(world_flat, pitch, pos_row, col);
			live_cells += get_cell_flat(world_flat, pitch, pos_row, pos_col);

			int cell = get_cell_flat(world_flat, pitch, row, col);
			int lives = (cell == 1 && (live_cells >= rule[1] && live_cells <= rule[0])) ||
						(cell == 0 && live_cells == rule[2]);
			set_cell_flat(world_aux_flat, pitch, row, col, lives ? 1 : 0);
		}
	}

	for (int r2 = 1; r2 <= rows_count; ++r2) {
		for (int c2 = 1; c2 <= cols_count; ++c2) {
			world_flat[r2 * pitch + c2] = world_aux_flat[r2 * pitch + c2];
		}
	}
}

void update_world_n_generations_flat(int n, int *world_flat, int rows_count, int cols_count, int *world_aux_flat, const int rule[RULE_SIZE], int pitch)
{
	if (n <= 0) return;
	for (int i = 0; i < n; ++i) {
		update_world_flat(world_flat, rows_count, cols_count, world_aux_flat, rule, pitch);
	}
}

/* Dead-border (non-wrapping) update: neighbors outside [1..rows]x[1..cols] count as 0 */
void update_world_flat_dead(int *world_flat, int rows_count, int cols_count, int *world_aux_flat, const int rule[RULE_SIZE], int pitch)
{
	for (int row = 1; row <= rows_count; ++row) {
		for (int col = 1; col <= cols_count; ++col) {
			int live_cells = 0;
			for (int rr = row - 1; rr <= row + 1; ++rr) {
				for (int cc = col - 1; cc <= col + 1; ++cc) {
					if (rr == row && cc == col) continue;
					if (rr >= 1 && rr <= rows_count && cc >= 1 && cc <= cols_count) {
						live_cells += get_cell_flat(world_flat, pitch, rr, cc);
					}
				}
			}
			int cell = get_cell_flat(world_flat, pitch, row, col);
			int lives = (cell == 1 && (live_cells >= rule[1] && live_cells <= rule[0])) ||
						(cell == 0 && live_cells == rule[2]);
			set_cell_flat(world_aux_flat, pitch, row, col, lives ? 1 : 0);
		}
	}
	for (int r2 = 1; r2 <= rows_count; ++r2) {
		for (int c2 = 1; c2 <= cols_count; ++c2) {
			world_flat[r2 * pitch + c2] = world_aux_flat[r2 * pitch + c2];
		}
	}
}

void update_world_n_generations_flat_dead(int n, int *world_flat, int rows_count, int cols_count, int *world_aux_flat, const int rule[RULE_SIZE], int pitch)
{
	if (n <= 0) return;
	for (int i = 0; i < n; ++i) {
		update_world_flat_dead(world_flat, rows_count, cols_count, world_aux_flat, rule, pitch);
	}
}