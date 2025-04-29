// othello_logic.h
#ifndef OTHELLO_LOGIC_H
#define OTHELLO_LOGIC_H

#define ROWS 8
#define COLS 8

typedef enum { EMPTY = 0, BLACK = 1, WHITE = 2 } Stone;

// ボード初期化
void initialize_board(Stone board[ROWS][COLS]);

// 指定位置にひっくり返し＆配置。返り値はひっくり返した数
int flip(Stone turn, int row, int col, Stone board[ROWS][COLS]);

// その位置に置けるかどうか (ひっくり返す数 > 0 か)
int check_position(Stone turn, int row, int col, Stone board[ROWS][COLS]);

#endif  // OTHELLO_LOGIC_H
