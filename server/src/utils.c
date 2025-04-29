#include "utils.h"

#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// エラー出力して即終了
void die(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

// 安全なメモリ確保（malloc失敗チェック付き）
void *safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        die("safe_malloc failed");
    }
    return ptr;
}

// 安全な送信 (切断やエラーを検出できる)
ssize_t safe_send(int sockfd, const void *buf, size_t len) {
    ssize_t total_sent = 0;
    const char *p = buf;

    while (total_sent < (ssize_t)len) {
        ssize_t sent = send(sockfd, p + total_sent, len - total_sent, 0);
        if (sent <= 0) {
            if (errno == EINTR) continue;  // 割り込みならリトライ
            return -1;                     // その他エラー
        }
        total_sent += sent;
    }
    return total_sent;
}

// 安全な受信 (切断やエラーを検出できる)
ssize_t safe_recv(int sockfd, void *buf, size_t len) {
    ssize_t total_received = 0;
    char *p = buf;

    while (total_received < (ssize_t)len) {
        ssize_t received =
            recv(sockfd, p + total_received, len - total_received, 0);
        if (received <= 0) {
            if (errno == EINTR) continue;  // 割り込みならリトライ
            return -1;                     // その他エラー
        }
        total_received += received;
    }
    return total_received;
}
