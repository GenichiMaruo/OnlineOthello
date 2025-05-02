#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// メッセージ送信関数
int sendMessage(int sockfd, const Message* msg) {
    ssize_t totalSent = 0;
    ssize_t sentBytes;
    size_t msgSize = sizeof(Message);  // 送信するメッセージ全体のサイズ
    const char* msgPtr = (const char*)msg;

    while (totalSent < msgSize) {
        sentBytes = send(sockfd, msgPtr + totalSent, msgSize - totalSent, 0);
        if (sentBytes <= 0) {
            // エラーまたは接続断
            perror("send failed");
            return -1;
        }
        totalSent += sentBytes;
    }
    // printf("DEBUG: Sent %ld bytes, type: %d\n", totalSent, msg->type);
    return totalSent;
}

// メッセージ受信関数
int receiveMessage(int sockfd, Message* msg) {
    ssize_t totalReceived = 0;
    ssize_t receivedBytes;
    size_t msgSize = sizeof(Message);  // 受信するメッセージ全体のサイズ
    char* msgPtr = (char*)msg;

    while (totalReceived < msgSize) {
        receivedBytes =
            recv(sockfd, msgPtr + totalReceived, msgSize - totalReceived, 0);
        if (receivedBytes < 0) {
            // エラー
            perror("recv failed");
            return -1;
        } else if (receivedBytes == 0) {
            // 接続が正常に閉じられた
            // printf("DEBUG: Connection closed by peer.\n");
            return 0;
        }
        totalReceived += receivedBytes;
    }
    // printf("DEBUG: Received %ld bytes, type: %d\n", totalReceived,
    // msg->type);
    return totalReceived;
}