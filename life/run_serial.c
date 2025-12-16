#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "life.h"

/* run_serial <generations> <input_file> [--dead-border] [--probe <row> <col>] [--probe-list]
 * Reads a world (dynamic), runs N generations using toroidal logic from life.c,
 * and prints in tests' exact format: '.' for dead, 'x' for live, single spaces,
 * no trailing spaces at line ends.
 */
int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <generations> <input_file> [--dead-border] [--probe <row> <col>] [--probe-list]\n", argv[0]);
        return 1;
    }

    int generations = atoi(argv[1]);
    const char *input = argv[2];
    int use_dead = 0;
    int do_probe = 0;
    int probe_list = 0;
    int probe_row = -1, probe_col = -1; // 1-based indices

    // Parse optional flags from argv[3..]
    for (int i = 3; i < argc; ++i) {
        if (strcmp(argv[i], "--dead-border") == 0) {
            use_dead = 1;
        } else if (strcmp(argv[i], "--probe") == 0) {
            if (i + 2 < argc) {
                probe_row = atoi(argv[i + 1]);
                probe_col = atoi(argv[i + 2]);
                do_probe = 1;
                i += 2; // skip consumed args
            } else {
                fprintf(stderr, "--probe requires <row> <col> (1-based)\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--probe-list") == 0) {
            probe_list = 1;
        }
    }

    if (access(input, F_OK) != 0) {
        fprintf(stderr, "Input file not found: %s\n", input);
        return 2;
    }

    DynWorld *dw = read_world_dyn(input);
    if (!dw) {
        fprintf(stderr, "Failed to read input file %s\n", input);
        return 3;
    }

    int rows = dw->rows;
    int cols = dw->cols;
    int pitch = dw->pitch;
    int rule[RULE_SIZE] = {3, 2, 3};

    size_t total = (size_t)(rows + 2) * (size_t)pitch;
    int *aux = (int *)calloc(total, sizeof(int));
    if (!aux) {
        fprintf(stderr, "Allocation failed\n");
        free_world_dyn(dw);
        return 4;
    }

    // Helper functions for neighbor counting and next-state on a given buffer
    int count_neighbors_toroidal(const int *buf, int r, int c, int rows_local, int cols_local, int pitch_local) {
        int n = 0;
        int r_up = (r == 1) ? rows_local : (r - 1);
        int r_dn = (r == rows_local) ? 1 : (r + 1);
        int c_lt = (c == 1) ? cols_local : (c - 1);
        int c_rt = (c == cols_local) ? 1 : (c + 1);
        n += buf[r_up * pitch_local + c_lt];
        n += buf[r_up * pitch_local + c];
        n += buf[r_up * pitch_local + c_rt];
        n += buf[r * pitch_local + c_lt];
        n += buf[r * pitch_local + c_rt];
        n += buf[r_dn * pitch_local + c_lt];
        n += buf[r_dn * pitch_local + c];
        n += buf[r_dn * pitch_local + c_rt];
        return n;
    }
    int count_neighbors_dead(const int *buf, int r, int c, int rows_local, int cols_local, int pitch_local) {
        int n = 0;
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                if (dr == 0 && dc == 0) continue;
                int rr = r + dr;
                int cc = c + dc;
                if (rr < 1 || rr > rows_local || cc < 1 || cc > cols_local) continue;
                n += buf[rr * pitch_local + cc];
            }
        }
        return n;
    }
    int next_state(int cur, int neighbors) {
        if (cur) {
            return (neighbors == 2 || neighbors == 3) ? 1 : 0;
        } else {
            return (neighbors == 3) ? 1 : 0;
        }
    }

    // If probing, compute before-update diagnostics
    if (do_probe) {
        if (probe_row < 1 || probe_row > rows || probe_col < 1 || probe_col > cols) {
            fprintf(stderr, "Probe coordinates out of range: r=%d c=%d (rows=%d cols=%d)\n",
                    probe_row, probe_col, rows, cols);
        } else {
            int cur = dw->data[probe_row * pitch + probe_col];
            int nb = use_dead ? count_neighbors_dead(dw->data, probe_row, probe_col, rows, cols, pitch)
                              : count_neighbors_toroidal(dw->data, probe_row, probe_col, rows, cols, pitch);
            int nxt = next_state(cur, nb);
            fprintf(stderr, "Probe before: r=%d c=%d cur=%d neighbors=%d next=%d mode=%s\n",
                    probe_row, probe_col, cur, nb, nxt, use_dead ? "dead" : "toroidal");
            if (probe_list) {
                if (use_dead) {
                    for (int dr = -1; dr <= 1; ++dr) {
                        for (int dc = -1; dc <= 1; ++dc) {
                            if (dr == 0 && dc == 0) continue;
                            int rr = probe_row + dr;
                            int cc = probe_col + dc;
                            if (rr < 1 || rr > rows || cc < 1 || cc > cols) {
                                fprintf(stderr, "  neighbor r=%d c=%d (out) val=0\n", rr, cc);
                            } else {
                                int v = dw->data[rr * pitch + cc];
                                fprintf(stderr, "  neighbor r=%d c=%d val=%d\n", rr, cc, v);
                            }
                        }
                    }
                } else {
                    int r_up = (probe_row == 1) ? rows : (probe_row - 1);
                    int r_dn = (probe_row == rows) ? 1 : (probe_row + 1);
                    int c_lt = (probe_col == 1) ? cols : (probe_col - 1);
                    int c_rt = (probe_col == cols) ? 1 : (probe_col + 1);
                    int coords[8][2] = {
                        {r_up, c_lt}, {r_up, probe_col}, {r_up, c_rt},
                        {probe_row, c_lt}, {probe_row, c_rt},
                        {r_dn, c_lt}, {r_dn, probe_col}, {r_dn, c_rt}
                    };
                    for (int k = 0; k < 8; ++k) {
                        int rr = coords[k][0];
                        int cc = coords[k][1];
                        int v = dw->data[rr * pitch + cc];
                        fprintf(stderr, "  neighbor r=%d c=%d val=%d\n", rr, cc, v);
                    }
                }
            }
        }
    }

    // Run generations, but if probing, also capture after first generation
    if (generations > 0) {
        if (use_dead) {
            // First generation
            update_world_flat_dead(dw->data, rows, cols, aux, rule, pitch);
        } else {
            update_world_flat(dw->data, rows, cols, aux, rule, pitch);
        }

        // After first gen probe
        if (do_probe) {
            int cur1 = aux[probe_row * pitch + probe_col];
            int nb1 = use_dead ? count_neighbors_dead(aux, probe_row, probe_col, rows, cols, pitch)
                               : count_neighbors_toroidal(aux, probe_row, probe_col, rows, cols, pitch);
            int nxt1 = next_state(cur1, nb1);
            fprintf(stderr, "Probe after g=1: r=%d c=%d cur=%d neighbors=%d next=%d mode=%s\n",
                    probe_row, probe_col, cur1, nb1, nxt1, use_dead ? "dead" : "toroidal");
            if (probe_list) {
                if (use_dead) {
                    for (int dr = -1; dr <= 1; ++dr) {
                        for (int dc = -1; dc <= 1; ++dc) {
                            if (dr == 0 && dc == 0) continue;
                            int rr = probe_row + dr;
                            int cc = probe_col + dc;
                            if (rr < 1 || rr > rows || cc < 1 || cc > cols) {
                                fprintf(stderr, "  neighbor r=%d c=%d (out) val=0\n", rr, cc);
                            } else {
                                int v = aux[rr * pitch + cc];
                                fprintf(stderr, "  neighbor r=%d c=%d val=%d\n", rr, cc, v);
                            }
                        }
                    }
                } else {
                    int r_up = (probe_row == 1) ? rows : (probe_row - 1);
                    int r_dn = (probe_row == rows) ? 1 : (probe_row + 1);
                    int c_lt = (probe_col == 1) ? cols : (probe_col - 1);
                    int c_rt = (probe_col == cols) ? 1 : (probe_col + 1);
                    int coords[8][2] = {
                        {r_up, c_lt}, {r_up, probe_col}, {r_up, c_rt},
                        {probe_row, c_lt}, {probe_row, c_rt},
                        {r_dn, c_lt}, {r_dn, probe_col}, {r_dn, c_rt}
                    };
                    for (int k = 0; k < 8; ++k) {
                        int rr = coords[k][0];
                        int cc = coords[k][1];
                        int v = aux[rr * pitch + cc];
                        fprintf(stderr, "  neighbor r=%d c=%d val=%d\n", rr, cc, v);
                    }
                }
            }
        }

        // If more generations requested, continue from aux for generations-1
        if (generations > 1) {
            if (use_dead) {
                update_world_n_generations_flat_dead(generations - 1, aux, rows, cols, dw->data, rule, pitch);
            } else {
                update_world_n_generations_flat(generations - 1, aux, rows, cols, dw->data, rule, pitch);
            }
        } else {
            // generations == 1 → copy aux into dw->data for printing
            // Swap buffers by copying minimal necessary region
            for (int r = 0; r < rows + 2; ++r) {
                memcpy(&dw->data[r * pitch], &aux[r * pitch], sizeof(int) * pitch);
            }
        }
    }

    /* Print exactly like tests: '.'/'x' with single spaces, no trailing space */
    for (int r = 1; r <= rows; ++r) {
        for (int c = 1; c <= cols; ++c) {
            int val = dw->data[r * pitch + c];
            fputc(val ? 'x' : '.', stdout);
            if (c < cols) fputc(' ', stdout);
        }
        fputc('\n', stdout);
    }

    free(aux);
    free_world_dyn(dw);
    return 0;
}
