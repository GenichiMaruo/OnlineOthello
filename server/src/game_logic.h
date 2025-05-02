#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "server_common.h"

// --- ゲームロジック関数プロトタイプ ---

// ゲーム状態を初期化する (オセロの初期配置)
void initialize_game_state(GameState* gs);

// (r, c) が playerColor にとって有効な手かチェックする
// (今回はスタブ実装)
int is_valid_move(const GameState* gs, int playerColor, int r, int c);

// 盤面を更新する (石を置き、相手の石をひっくり返す)
// (今回はスタブ実装 - 石を置くだけ)
// 戻り値: ひっくり返した石の数など (必要に応じて)
int update_board(GameState* gs, int playerColor, int r, int c);

// ゲームが終了したかチェックする
// 戻り値: 0:継続, 1:黒勝, 2:白勝, 3:引分
// (今回はスタブ実装 - 常に継続)
int check_game_over(const GameState* gs);

// playerColor が置ける場所があるかチェックする (パス判定用)
// (今回はスタブ実装 - 常に置けると仮定)
int has_valid_moves(const GameState* gs, int playerColor);

#endif  // GAME_LOGIC_H