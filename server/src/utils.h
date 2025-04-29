#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>

// エラー時終了
void die(const char *message);

// 安全なメモリ確保
void *safe_malloc(size_t size);

// ソケット送信の安全版
ssize_t safe_send(int sockfd, const void *buf, size_t len);

// ソケット受信の安全版
ssize_t safe_recv(int sockfd, void *buf, size_t len);

#endif  // UTILS_H
