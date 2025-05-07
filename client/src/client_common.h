#ifndef CLIENT_COMMON_H
#define CLIENT_COMMON_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>  // for uint8_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "protocol.h"

// --- グローバル変数 ---
extern char g_server_ip[64];
extern int g_server_port;

// --- クライアントの状態 ---
typedef enum {
    STATE_DISCONNECTED,
    STATE_CONNECTING,       // 接続試行中
    STATE_CONNECTED,        // サーバーには接続したがロビーにいる状態
    STATE_CREATING_ROOM,    // 部屋作成要求中
    STATE_JOINING_ROOM,     // 部屋参加要求中
    STATE_WAITING_IN_ROOM,  // 部屋で相手待ち
    STATE_STARTING_GAME,    // ゲーム開始要求中
    STATE_MY_TURN,          // 自分のターン
    STATE_OPPONENT_TURN,    // 相手のターン
    STATE_PLACING_PIECE,    // コマ配置要求中
    STATE_GAME_OVER,        // ゲーム終了 (再戦待ち)
    STATE_SENDING_REMATCH,  // 再戦要求送信中
    STATE_QUITTING,         // 終了処理中
    STATE_REMOTE_CLOSED     // サーバー/相手によって切断された
} ClientState;

#endif  // CLIENT_COMMON_H