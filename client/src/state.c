#include "state.h"

#include <string.h>  // for memset

// --- グローバル変数定義 (変更なし) ---
static int g_sockfd = -1;
static pthread_t g_recv_thread_id = 0;
static pthread_mutex_t g_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static ClientState g_current_state = STATE_DISCONNECTED;
static int g_my_room_id = -1;
static uint8_t g_my_color = 0;
static uint8_t g_game_board[BOARD_SIZE][BOARD_SIZE];

// --- 初期化 (変更なし) ---
void initialize_state() {
    pthread_mutex_lock(&g_state_mutex);
    g_sockfd = -1;
    g_recv_thread_id = 0;
    g_current_state = STATE_DISCONNECTED;
    g_my_room_id = -1;
    g_my_color = 0;
    memset(g_game_board, 0, sizeof(g_game_board));
    pthread_mutex_unlock(&g_state_mutex);
    // printf は削除 (ログは json_output 経由で)
}

// --- Mutex (変更なし) ---
pthread_mutex_t* get_state_mutex() { return &g_state_mutex; }

// --- Client State (変更なし) ---
ClientState get_client_state() {
    pthread_mutex_lock(&g_state_mutex);
    ClientState state = g_current_state;
    pthread_mutex_unlock(&g_state_mutex);
    return state;
}
void set_client_state(ClientState new_state) {
    pthread_mutex_lock(&g_state_mutex);
    g_current_state = new_state;
    pthread_mutex_unlock(&g_state_mutex);
}
void set_client_state_unsafe(ClientState new_state) {
    g_current_state = new_state;
}
ClientState get_client_state_unsafe() { return g_current_state; }

// --- Socket FD (変更なし) ---
int get_sockfd() {
    pthread_mutex_lock(&g_state_mutex);
    int fd = g_sockfd;
    pthread_mutex_unlock(&g_state_mutex);
    return fd;
}
void set_sockfd(int new_sockfd) {
    pthread_mutex_lock(&g_state_mutex);
    g_sockfd = new_sockfd;
    pthread_mutex_unlock(&g_state_mutex);
}

// --- Thread ID (変更なし) ---
pthread_t get_recv_thread_id() {
    pthread_mutex_lock(&g_state_mutex);
    pthread_t tid = g_recv_thread_id;
    pthread_mutex_unlock(&g_state_mutex);
    return tid;
}
void set_recv_thread_id(pthread_t tid) {
    pthread_mutex_lock(&g_state_mutex);
    g_recv_thread_id = tid;
    pthread_mutex_unlock(&g_state_mutex);
}

// --- Room ID (変更なし) ---
int get_my_room_id() {
    pthread_mutex_lock(&g_state_mutex);
    int id = g_my_room_id;
    pthread_mutex_unlock(&g_state_mutex);
    return id;
}
void set_my_room_id(int room_id) {
    pthread_mutex_lock(&g_state_mutex);
    g_my_room_id = room_id;
    pthread_mutex_unlock(&g_state_mutex);
}
void set_my_room_id_unsafe(int room_id) { g_my_room_id = room_id; }
int get_my_room_id_unsafe() { return g_my_room_id; }

// --- My Color (変更なし) ---
uint8_t get_my_color() {
    pthread_mutex_lock(&g_state_mutex);
    uint8_t color = g_my_color;
    pthread_mutex_unlock(&g_state_mutex);
    return color;
}
void set_my_color(uint8_t color) {
    pthread_mutex_lock(&g_state_mutex);
    g_my_color = color;
    pthread_mutex_unlock(&g_state_mutex);
}
void set_my_color_unsafe(uint8_t color) { g_my_color = color; }
uint8_t get_my_color_unsafe() { return g_my_color; }

// --- Game Board (get_game_board_unsafe を追加) ---
void get_game_board(uint8_t board_copy[BOARD_SIZE][BOARD_SIZE]) {
    pthread_mutex_lock(&g_state_mutex);
    memcpy(board_copy, g_game_board, sizeof(g_game_board));
    pthread_mutex_unlock(&g_state_mutex);
}
void set_game_board(const uint8_t new_board[BOARD_SIZE][BOARD_SIZE]) {
    pthread_mutex_lock(&g_state_mutex);
    memcpy(g_game_board, new_board, sizeof(g_game_board));
    pthread_mutex_unlock(&g_state_mutex);
}
// ゲーム盤面取得 (_unsafe version)
void get_game_board_unsafe(uint8_t board_copy[BOARD_SIZE][BOARD_SIZE]) {
    // 呼び出し元でロックされている前提
    memcpy(board_copy, g_game_board, sizeof(g_game_board));
}
void set_game_board_unsafe(const uint8_t new_board[BOARD_SIZE][BOARD_SIZE]) {
    memcpy(g_game_board, new_board, sizeof(g_game_board));
}

// --- Reset Room Info (変更なし) ---
void reset_room_info() {
    pthread_mutex_lock(&g_state_mutex);
    g_my_room_id = -1;
    g_my_color = 0;
    memset(g_game_board, 0, sizeof(g_game_board));
    pthread_mutex_unlock(&g_state_mutex);
}
void reset_room_info_unsafe() {
    g_my_room_id = -1;
    g_my_color = 0;
    memset(g_game_board, 0, sizeof(g_game_board));
}

// --- 状態enumを文字列に変換 (json_output.c から移動) ---
const char* state_to_string(ClientState state) {
    switch (state) {
        case STATE_DISCONNECTED:
            return "Disconnected";
        case STATE_CONNECTING:
            return "Connecting";
        case STATE_CONNECTED:
            return "Lobby";
        case STATE_CREATING_ROOM:
            return "CreatingRoom";
        case STATE_JOINING_ROOM:
            return "JoiningRoom";
        case STATE_WAITING_IN_ROOM:
            return "WaitingInRoom";
        case STATE_STARTING_GAME:
            return "StartingGame";
        case STATE_MY_TURN:
            return "MyTurn";
        case STATE_OPPONENT_TURN:
            return "OpponentTurn";
        case STATE_PLACING_PIECE:
            return "PlacingPiece";
        case STATE_GAME_OVER:
            return "GameOver";
        case STATE_SENDING_REMATCH:
            return "SendingRematch";
        case STATE_QUITTING:
            return "Quitting";
        case STATE_REMOTE_CLOSED:
            return "ConnectionClosed";
        default:
            return "Unknown";
    }
}