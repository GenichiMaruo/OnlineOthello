// othello_logic.c
#include "othello_logic.h"

static int count_flip_stone(Stone turn, int row, int col, int d_row, int d_col,
                            Stone b[ROWS][COLS]) {
    int r = row + d_row, c = col + d_col, cnt = 0;
    while (r >= 0 && r < ROWS && c >= 0 && c < COLS && b[r][c] != EMPTY &&
           b[r][c] != turn) {
        cnt++;
        r += d_row;
        c += d_col;
    }
    if (r < 0 || r >= ROWS || c < 0 || c >= COLS || b[r][c] != turn) return 0;
    return cnt;
}

void initialize_board(Stone b[ROWS][COLS]) {
    for (int i = 0; i < ROWS; i++)
        for (int j = 0; j < COLS; j++) b[i][j] = EMPTY;
    b[3][3] = b[4][4] = BLACK;
    b[3][4] = b[4][3] = WHITE;
}

int flip(Stone turn, int row, int col, Stone b[ROWS][COLS]) {
    int total = 0;
    b[row][col] = turn;
    for (int dr = -1; dr <= 1; dr++)
        for (int dc = -1; dc <= 1; dc++) {
            int cnt = count_flip_stone(turn, row, col, dr, dc, b);
            if (cnt > 0) {
                // flip
                int r = row + dr, c = col + dc;
                for (int k = 0; k < cnt; k++) {
                    b[r][c] = turn;
                    r += dr;
                    c += dc;
                }
                total += cnt;
            }
        }
    return total;
}

int check_position(Stone turn, int row, int col, Stone b[ROWS][COLS]) {
    if (b[row][col] != EMPTY) return 0;
    for (int dr = -1; dr <= 1; dr++)
        for (int dc = -1; dc <= 1; dc++) {
            if (count_flip_stone(turn, row, col, dr, dc, b) > 0) return 1;
        }
    return 0;
}
