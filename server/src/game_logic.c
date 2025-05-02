#include "game_logic.h"

#include <stdio.h>  // for printf in debug messages (optional)

// --- 補助関数 ---
// (r, c) が盤面内にあるかチェック
static inline int is_within_bounds(int r, int c) {
    return r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE;
}

// 指定方向に相手の石が連続し、その先に自分の石があるかチェック
// ひっくり返せる数を返す (0ならひっくり返せない)
static int check_direction(const GameState* gs, int playerColor, int r, int c,
                           int dr, int dc) {
    int opponentColor = (playerColor == 1) ? 2 : 1;
    int flipped_count = 0;
    int current_r = r + dr;
    int current_c = c + dc;

    // 1. 隣が相手の石か？
    if (!is_within_bounds(current_r, current_c) ||
        gs->board[current_r][current_c] != opponentColor) {
        return 0;
    }

    // 2. その先に相手の石が続いているか？
    while (is_within_bounds(current_r, current_c)) {
        if (gs->board[current_r][current_c] == opponentColor) {
            flipped_count++;
        } else if (gs->board[current_r][current_c] == playerColor) {
            // 3. 自分の石が見つかったら、この方向は有効
            return flipped_count;
        } else {  // 空きマスが見つかったらダメ
            return 0;
        }
        current_r += dr;
        current_c += dc;
    }

    // 盤面の端まで相手の石が続いた場合もダメ
    return 0;
}

// 指定されたマスに石を置いた場合にひっくり返せる石の総数を返す
// (is_valid_moveの実質的な本体)
static int get_total_flips_for_move(const GameState* gs, int playerColor, int r,
                                    int c) {
    // 方向ベクトル (8方向)
    const int dr[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    const int dc[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int total_flips = 0;

    for (int i = 0; i < 8; ++i) {
        total_flips += check_direction(gs, playerColor, r, c, dr[i], dc[i]);
    }
    return total_flips;
}

// --- メイン関数 ---

// ゲーム状態を初期化する (オセロの初期配置)
void initialize_game_state(GameState* gs) {
    memset(gs->board, 0, sizeof(gs->board));
    gs->board[BOARD_SIZE / 2 - 1][BOARD_SIZE / 2 - 1] = 2;  // 白
    gs->board[BOARD_SIZE / 2][BOARD_SIZE / 2] = 2;          // 白
    gs->board[BOARD_SIZE / 2 - 1][BOARD_SIZE / 2] = 1;      // 黒
    gs->board[BOARD_SIZE / 2][BOARD_SIZE / 2 - 1] = 1;      // 黒
    gs->currentTurn = 1;                                    // 黒番から開始
    // printf("Game state initialized.\n");
}

// (r, c) が playerColor にとって有効な手かチェックする
int is_valid_move(const GameState* gs, int playerColor, int r, int c) {
    // 1. 盤面内で、空いているか？
    if (!is_within_bounds(r, c) || gs->board[r][c] != 0) {
        return 0;
    }

    // 2. 相手の石を1つ以上ひっくり返せるか？
    if (get_total_flips_for_move(gs, playerColor, r, c) > 0) {
        return 1;  // ひっくり返せるので有効
    }

    return 0;  // ひっくり返せないので無効
}

// 盤面を更新する (石を置き、相手の石をひっくり返す)
// 戻り値: ひっくり返した石の数
int update_board(GameState* gs, int playerColor, int r, int c) {
    // 事前に is_valid_move
    // でチェックされている想定だが、念のため基本的なチェック
    if (!is_within_bounds(r, c) || gs->board[r][c] != 0) {
        fprintf(stderr,
                "Error: update_board called for invalid position (%d, %d)\n", r,
                c);
        return 0;
    }

    // 方向ベクトル (8方向)
    const int dr[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    const int dc[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int total_flipped_count = 0;
    int opponentColor = (playerColor == 1) ? 2 : 1;

    // まず石を置く
    gs->board[r][c] = playerColor;
    // printf("GameLogic: Placed piece at (%d, %d) for player %d.\n", r, c,
    // playerColor);

    // 各方向についてひっくり返す処理
    for (int i = 0; i < 8; ++i) {
        int flips_in_direction =
            check_direction(gs, playerColor, r, c, dr[i], dc[i]);
        if (flips_in_direction > 0) {
            // printf("  Flipping %d stones in direction (%d, %d)\n",
            // flips_in_direction, dr[i], dc[i]);
            int current_r = r + dr[i];
            int current_c = c + dc[i];
            // ひっくり返す処理
            for (int j = 0; j < flips_in_direction; ++j) {
                // is_within_bounds チェックは check_direction
                // で担保されているはず
                if (gs->board[current_r][current_c] == opponentColor) {
                    gs->board[current_r][current_c] = playerColor;
                    total_flipped_count++;
                } else {
                    // 予期せぬエラー: 相手の石ではない
                    fprintf(stderr,
                            "Error: Trying to flip non-opponent stone at (%d, "
                            "%d)!\n",
                            current_r, current_c);
                    break;  // この方向のフリップを中止
                }
                current_r += dr[i];
                current_c += dc[i];
            }
        }
    }

    if (total_flipped_count == 0 &&
        get_total_flips_for_move(gs, playerColor, r, c) > 0) {
        // is_valid_move
        // はOKだったのに、ひっくり返せなかった場合のエラーチェック
        fprintf(stderr,
                "Warning: Move at (%d, %d) by player %d resulted in 0 flips, "
                "but should have been valid.\n",
                r, c, playerColor);
    }
    // printf("GameLogic: Flipped %d stones total.\n", total_flipped_count);

    return total_flipped_count;
}

// playerColor が置ける場所があるかチェックする (パス判定用)
int has_valid_moves(const GameState* gs, int playerColor) {
    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            // 空きマスに対してのみチェック
            if (gs->board[r][c] == 0) {
                if (is_valid_move(gs, playerColor, r, c)) {
                    // printf("GameLogic: Player %d has a valid move at (%d,
                    // %d)\n", playerColor, r, c);
                    return 1;  // 置ける場所が見つかった
                }
            }
        }
    }
    // printf("GameLogic: Player %d has no valid moves.\n", playerColor);
    return 0;  // 置ける場所がない
}

// ゲームが終了したかチェックする
// 戻り値: 0:継続, 1:黒勝, 2:白勝, 3:引分
int check_game_over(const GameState* gs) {
    // 1. 両プレイヤーとも置ける場所がないか？
    int black_can_move = has_valid_moves(gs, 1);
    int white_can_move = has_valid_moves(gs, 2);

    if (!black_can_move && !white_can_move) {
        printf(
            "GameLogic: Game over condition - Both players have no valid "
            "moves.\n");
        // ゲーム終了 -> 勝敗判定
    } else {
        // 2. 盤面がすべて埋まっているか？
        int empty_count = 0;
        for (int i = 0; i < BOARD_SIZE; ++i) {
            for (int j = 0; j < BOARD_SIZE; ++j) {
                if (gs->board[i][j] == 0) {
                    empty_count++;
                    break;  // 1つでも空きがあれば盤面は埋まっていない
                }
            }
            if (empty_count > 0) break;
        }

        if (empty_count == 0) {
            printf("GameLogic: Game over condition - Board is full.\n");
            // ゲーム終了 -> 勝敗判定
        } else {
            // どちらかが動けて、かつ盤面に空きがある -> ゲーム継続
            return 0;
        }
    }

    // --- ゲーム終了時の勝敗判定 ---
    int black_score = 0;
    int white_score = 0;
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            if (gs->board[i][j] == 1)
                black_score++;
            else if (gs->board[i][j] == 2)
                white_score++;
        }
    }

    printf("GameLogic: Final score - Black (1): %d, White (2): %d\n",
           black_score, white_score);

    if (black_score > white_score) return 1;  // 黒勝利
    if (white_score > black_score) return 2;  // 白勝利
    return 3;                                 // 引き分け
}