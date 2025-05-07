#ifndef SERVER_COMMON_H
#define SERVER_COMMON_H

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "protocol.h"  // 共通プロトコルヘッダー

// --- 定数定義 ---
#define MAX_CLIENTS 100         // 最大同時接続クライアント数 (部屋数*2以上)
#define MAX_ROOMS 50            // 最大部屋数
#define SERVER_PORT 10000       // サーバーポート番号
#define REMATCH_TIMEOUT_SEC 30  // 再戦受付時間（秒）

// --- データ構造定義 ---

// クライアント情報
typedef struct {
    int sockfd;
    struct sockaddr_in addr;
    int roomId;  // 参加中の部屋ID (-1ならロビー)
    pthread_t thread_id;
    int playerColor;  // 1:黒, 2:白, 0:未定
    // 必要ならユーザー名なども追加
} ClientInfo;

// ゲーム盤面状態
typedef struct {
    uint8_t board[BOARD_SIZE][BOARD_SIZE];  // 0:空, 1:黒, 2:白
    uint8_t currentTurn;                    // 1:黒, 2:白
    // ゲームの状態 (手数、パス状況など) を追加
} GameState;

// 部屋の状態
typedef enum {
    ROOM_EMPTY,      // 空室
    ROOM_WAITING,    // 1人待機中
    ROOM_PLAYING,    // 対戦中
    ROOM_GAMEOVER,   // ゲーム終了 (再戦待ち)
    ROOM_REMATCHING  // 再戦同意待ち
} RoomStatus;

// 部屋情報
typedef struct {
    int roomId;
    char roomName[MAX_ROOM_NAME_LEN];
    RoomStatus status;
    int player1_sock;  // プレイヤー1のソケットディスクリプタ (-1なら不在)
    int player2_sock;  // プレイヤー2のソケットディスクリプタ (-1なら不在)
    GameState gameState;
    pthread_mutex_t room_mutex;  // 各部屋ごとのミューテックス
    time_t last_action_time;     // タイムアウト処理用
    int player1_rematch_agree;   // 0:未返答, 1:Yes, 2:No
    int player2_rematch_agree;   // 0:未返答, 1:Yes, 2:No
    // タイマー用変数など
} Room;

#endif  // SERVER_COMMON_H