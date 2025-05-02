#include "connection.h"

#include <errno.h>  // errno 用

// グローバル変数の実体 (connection.c で管理)
// int sockfd = -1; // -> client.c で定義することにする

// --- サーバー接続 ---
// 戻り値: 成功なら sockfd, 失敗なら -1
int connect_to_server() {
    struct sockaddr_in server_addr;
    int temp_sockfd;  // ローカル変数を使用

    pthread_mutex_lock(&state_mutex);
    if (current_state != STATE_DISCONNECTED) {
        pthread_mutex_unlock(&state_mutex);
        fprintf(stderr, "Already connected or connecting.\n");
        return sockfd;  // 既存の sockfd を返すか、エラーを示す
    }
    current_state = STATE_CONNECTING;  // 接続試行中に設定
    pthread_mutex_unlock(&state_mutex);

    temp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (temp_sockfd < 0) {
        perror("socket creation failed");
        pthread_mutex_lock(&state_mutex);
        current_state = STATE_DISCONNECTED;
        pthread_mutex_unlock(&state_mutex);
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        close(temp_sockfd);
        pthread_mutex_lock(&state_mutex);
        current_state = STATE_DISCONNECTED;
        pthread_mutex_unlock(&state_mutex);
        return -1;
    }

    if (connect(temp_sockfd, (struct sockaddr*)&server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect failed");
        close(temp_sockfd);
        pthread_mutex_lock(&state_mutex);
        current_state = STATE_DISCONNECTED;
        pthread_mutex_unlock(&state_mutex);
        return -1;
    }

    printf("Connected to server %s:%d\n", SERVER_IP, SERVER_PORT);
    sockfd = temp_sockfd;  // グローバル変数に設定 (client.c で定義)
    pthread_mutex_lock(&state_mutex);
    current_state = STATE_CONNECTED;  // 接続完了 (ロビー状態)
    pthread_mutex_unlock(&state_mutex);

    return sockfd;  // 成功したので sockfd を返す
}

// --- サーバーからの切断 ---
// これは受信スレッドやメインスレッドが呼ぶ想定
void disconnect_from_server() {
    pthread_mutex_lock(&state_mutex);
    // 既に切断済み、または自分で終了処理中なら何もしない
    if (current_state == STATE_DISCONNECTED ||
        current_state == STATE_QUITTING ||
        current_state == STATE_REMOTE_CLOSED) {
        pthread_mutex_unlock(&state_mutex);
        return;
    }
    // 終了処理中にマーク
    current_state = STATE_QUITTING;
    pthread_mutex_unlock(&state_mutex);

    keep_running = 0;  // ループ停止フラグを立てる

    if (sockfd != -1) {
        printf("Disconnecting from server...\n");
        // shutdown()
        // は相手にFINを送る。recvでブロック中のスレッドを解除する効果も期待できる。
        if (shutdown(sockfd, SHUT_RDWR) == -1) {
            // EPIPE や ENOTCONN
            // は既に切断されている可能性があるので無視してもよい
            if (errno != EPIPE && errno != ENOTCONN) {
                perror("shutdown failed");
            }
        }
        // close() でソケットを閉じる
        if (close(sockfd) == -1) {
            perror("close failed");
        }
        sockfd = -1;  // sockfd を無効化
        printf("Socket closed.\n");
    }

    // 最終的な状態をDISCONNECTEDにする (cleanupで行う方が良いかも)
    // pthread_mutex_lock(&state_mutex);
    // current_state = STATE_DISCONNECTED;
    // pthread_mutex_unlock(&state_mutex);
}