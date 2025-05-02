#ifndef CLIENT_COMMON_H
#define CLIENT_COMMON_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>  // For uint8_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "protocol.h"  // protocol.h は BOARD_SIZE を定義している前提

// --- 定数定義 ---
#define SERVER_IP "127.0.0.1"  // サーバーのIPアドレス
#define SERVER_PORT 10000      // サーバーのポート番号

// --- グローバル変数 (状態管理) ---

// クライアントの状態
typedef enum {
    STATE_DISCONNECTED,
    STATE_CONNECTING,       // 接続試行中 (追加)
    STATE_CONNECTED,        // サーバーには接続したがロビーにいる状態
    STATE_WAITING_IN_ROOM,  // 部屋で相手待ち
    STATE_MY_TURN,          // 自分のターン
    STATE_OPPONENT_TURN,    // 相手のターン
    STATE_GAME_OVER,        // ゲーム終了 (再戦待ち)
    STATE_REMOTE_CLOSED,    // サーバー/相手によって切断された
    STATE_QUITTING          // 自主的な終了処理中 (追加)
} ClientState;

// extern宣言: 実体は他の.cファイル(例: client.c)で定義する
extern int sockfd;
extern pthread_t recv_thread_id;
extern pthread_mutex_t state_mutex;
extern ClientState current_state;
extern int my_room_id;
extern uint8_t my_color;  // 1: 黒, 2: 白
extern uint8_t game_board[BOARD_SIZE][BOARD_SIZE];
extern volatile int keep_running;  // メインループ/受信ループ制御用フラグ (追加)

#endif  // CLIENT_COMMON_H