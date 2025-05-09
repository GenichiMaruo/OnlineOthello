#include "network.h"

#include "json_output.h"  // JSON出力用
#include "state.h"

// --- サーバー接続 ---
int connect_to_server() {
    struct addrinfo hints, *res, *rp;
    int temp_sockfd;
    char* server_ip = g_server_ip;
    int server_port = g_server_port;

    if (server_port <= 0 || server_port > 65535) {
        send_error_event("Invalid server port: %d", server_port);
        return -1;
    }
    if (strlen(server_ip) == 0) {
        send_error_event("Invalid server IP address: '%s'", server_ip);
        return -1;
    }

    // localhostをループバックアドレスに変換
    if (strcmp(server_ip, "localhost") == 0) {
        server_ip = "127.0.0.1";
    }

    // ログ表示
    send_log_event(LOG_INFO, "Connecting to server at %s:%d", server_ip,
                   server_port);
    set_client_state(STATE_CONNECTING);
    send_state_change_event();

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;        // IPv4のみ
    hints.ai_socktype = SOCK_STREAM;  // TCP

    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%d", server_port);

    int ret = getaddrinfo(server_ip, port_str, &hints, &res);
    if (ret != 0) {
        send_error_event("getaddrinfo failed: %s", gai_strerror(ret));
        set_client_state(STATE_DISCONNECTED);
        send_state_change_event();
        return -1;
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        temp_sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (temp_sockfd < 0) continue;

        if (connect(temp_sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;  // 成功
        }

        close(temp_sockfd);
    }

    freeaddrinfo(res);

    if (rp == NULL) {
        send_error_event("Failed to connect to the server");
        set_client_state(STATE_DISCONNECTED);
        send_state_change_event();
        return -1;
    }

    send_log_event(LOG_INFO, "Connected to server");
    set_sockfd(temp_sockfd);
    set_client_state(STATE_CONNECTED);
    send_state_change_event();
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