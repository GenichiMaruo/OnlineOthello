#include "network.h"

#include "json_output.h"  // JSON出力用
#include "state.h"

// --- サーバー接続 ---
int connect_to_server() {
    struct sockaddr_in server_addr;
    int temp_sockfd;
    char* server_ip = g_server_ip;    // グローバル変数からIPアドレスを取得
    int server_port = g_server_port;  // グローバル変数からポート番号を取得
    if (server_port <= 0 || server_port > 65535) {
        send_error_event("Invalid server port: %d", server_port);
        return -1;
    }
    if (strlen(server_ip) == 0) {
        send_error_event("Invalid server IP address: '%s'", server_ip);
        return -1;
    } else if (strcmp(server_ip, "localhost") == 0) {
        strcpy(server_ip, "127.0.0.1");  // localhost
    }

    set_client_state(STATE_CONNECTING);  // 接続試行中状態へ
    send_state_change_event();           // Node.jsに状態変化を通知

    temp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (temp_sockfd < 0) {
        perror("socket creation failed");
        send_error_event("Socket creation failed");  // Node.jsにエラー通知
        set_client_state(STATE_DISCONNECTED);
        send_state_change_event();
        return -1;
    }

    // (中略 - アドレス設定は同じ)
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);  // ポート番号を設定
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        send_error_event("Invalid server IP address");
        close(temp_sockfd);
        set_client_state(STATE_DISCONNECTED);
        send_state_change_event();
        return -1;
    }

    if (connect(temp_sockfd, (struct sockaddr*)&server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect failed");
        send_error_event("Failed to connect to the server");
        close(temp_sockfd);
        set_client_state(STATE_DISCONNECTED);
        send_state_change_event();
        return -1;
    }

    send_log_event(LOG_INFO, "Connected to server");  // ログ通知
    set_sockfd(temp_sockfd);            // 状態モジュールに sockfd を設定
    set_client_state(STATE_CONNECTED);  // 接続完了状態へ
    send_state_change_event();          // 状態変化通知
    return temp_sockfd;
}

// --- 接続終了 ---
void close_connection() {
    int current_sockfd = get_sockfd();
    if (current_sockfd != -1) {
        send_log_event(LOG_INFO, "Shutting down connection...");
        if (shutdown(current_sockfd, SHUT_RDWR) < 0) {
            // perror("shutdown failed"); // Node.js側でエラー検知推奨
            send_log_event(LOG_WARN, "Socket shutdown failed");
        }
        if (close(current_sockfd) < 0) {
            // perror("close socket failed");
            send_log_event(LOG_WARN, "Socket close failed");
        }
        set_sockfd(-1);  // sockfd を無効化
        send_log_event(LOG_INFO, "Socket closed.");
    }
    // 状態は呼び出し元で設定
}

// --- メッセージ送信ラッパー ---
int send_message_to_server(const Message* msg) {
    int current_sockfd = get_sockfd();
    if (current_sockfd == -1) {
        send_error_event("Cannot send message: Not connected");
        if (get_client_state() != STATE_QUITTING) {
            set_client_state(STATE_REMOTE_CLOSED);
            send_state_change_event();
        }
        return 0;
    }

    if (sendMessage(current_sockfd, msg) <= 0) {
        send_error_event("Failed to send message to server");
        if (get_client_state() != STATE_QUITTING) {
            set_client_state(STATE_REMOTE_CLOSED);
            send_state_change_event();
        }
        close_connection();  // 送信失敗時は接続を切断
        return 0;
    }
    // send_log_event(LOG_DEBUG, "Message sent successfully"); // デバッグ用
    return 1;
}