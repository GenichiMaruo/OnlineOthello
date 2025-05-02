#include "client_common.h"
#include "connection.h"
#include "input_handler.h"
#include "receiver.h"
#include "ui.h"  // 終了時のメッセージ表示など

// --- グローバル変数定義 ---
int sockfd = -1;
pthread_t recv_thread_id = 0;  // 初期化
pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;
ClientState current_state = STATE_DISCONNECTED;
int my_room_id = -1;
uint8_t my_color = 0;
uint8_t game_board[BOARD_SIZE][BOARD_SIZE];
volatile int keep_running = 1;  // ループ制御フラグ (初期値1)

// --- プロトタイプ宣言 (このファイル内) ---
void initialize_client();
void cleanup();

// --- main関数 ---
int main() {
    initialize_client();

    // サーバー接続
    if (connect_to_server() < 0) {  // connection.c の関数
        fprintf(stderr, "Failed to connect to server. Exiting.\n");
        cleanup();  // ミューテックス破棄など
        return 1;
    }

    // 受信スレッド開始
    if (pthread_create(&recv_thread_id, NULL, receive_handler, NULL) !=
        0) {  // receiver.c の関数
        perror("Failed to create receiver thread");
        disconnect_from_server();  // connection.c
        cleanup();
        return 1;
    }

    // メインスレッドはユーザー入力処理
    handle_user_input();  // input_handler.c の関数

    // ユーザー入力ループが終了したら、クリーンアップ処理
    cleanup();

    return 0;
}

// --- クライアント初期化 ---
void initialize_client() {
    printf("Initializing client...\n");
    // グローバル変数の初期化 (既に定義時に初期化されているものもあるが念のため)
    sockfd = -1;
    recv_thread_id = 0;
    current_state = STATE_DISCONNECTED;
    my_room_id = -1;
    my_color = 0;
    memset(game_board, 0, sizeof(game_board));
    keep_running = 1;
    // ミューテックスは初期化済み (PTHREAD_MUTEX_INITIALIZER)
    // 必要なら pthread_mutex_init を使う
}

// --- 終了処理 ---
void cleanup() {
    printf("Starting cleanup...\n");

    // keep_running フラグを確実に0にする
    keep_running = 0;

    // サーバーから切断 (既に切断済み/処理中でも内部でチェックされる)
    disconnect_from_server();  // connection.c

    // 受信スレッドの終了を待つ
    if (recv_thread_id != 0) {
        printf("Waiting for receiver thread to join...\n");
        // スレッドが recv でブロックしている場合、shutdown/close
        // で解除されることを期待
        int join_ret = pthread_join(recv_thread_id, NULL);
        if (join_ret != 0) {
            fprintf(stderr, "Error joining receiver thread: %s\n",
                    strerror(join_ret));
            // キャンセルを試みる (最終手段)
            // pthread_cancel(recv_thread_id);
        } else {
            printf("Receiver thread joined successfully.\n");
        }
        recv_thread_id = 0;  // スレッドIDをクリア
    } else {
        printf("Receiver thread was not running or already joined.\n");
    }

    // ミューテックスの破棄
    printf("Destroying mutex...\n");
    pthread_mutex_destroy(&state_mutex);

    printf("Cleanup complete.\n");
}