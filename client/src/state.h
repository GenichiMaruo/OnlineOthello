#ifndef STATE_H
#define STATE_H

#include "client_common.h"

// --- 関数プロトタイプ ---

void initialize_state();
pthread_mutex_t* get_state_mutex();

ClientState get_client_state();
void set_client_state(ClientState new_state);

int get_sockfd();
void set_sockfd(int new_sockfd);

pthread_t get_recv_thread_id();
void set_recv_thread_id(pthread_t tid);

int get_my_room_id();
void set_my_room_id(int room_id);

uint8_t get_my_color();
void set_my_color(uint8_t color);

void get_game_board(uint8_t board_copy[BOARD_SIZE][BOARD_SIZE]);
void set_game_board(const uint8_t new_board[BOARD_SIZE][BOARD_SIZE]);

void reset_room_info();

// 状態enumを文字列に変換する関数を追加
const char* state_to_string(ClientState state);

// --- 内部用 ---
void set_client_state_unsafe(ClientState new_state);
ClientState get_client_state_unsafe();
void set_my_room_id_unsafe(int room_id);
int get_my_room_id_unsafe();
void set_my_color_unsafe(uint8_t color);
uint8_t get_my_color_unsafe();
// ゲーム盤面取得 (mutex保護不要だが _unsafe を追加)
void get_game_board_unsafe(uint8_t board_copy[BOARD_SIZE][BOARD_SIZE]);
void set_game_board_unsafe(const uint8_t new_board[BOARD_SIZE][BOARD_SIZE]);
void reset_room_info_unsafe();

#endif  // STATE_H