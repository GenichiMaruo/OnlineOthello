// front_comm.c
#include "front_comm.h"

#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

// 簡易：ヘッダーは (1byte type, 2byte length)
typedef struct {
    uint8_t type;
    uint16_t length;
} FrontHeader;

int connect_to_front(const char *addr, uint16_t port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) die("socket");
    struct sockaddr_in fr = {0};
    fr.sin_family = AF_INET;
    fr.sin_port = htons(port);
    if (!inet_aton(addr, &fr.sin_addr)) die("invalid front address");
    if (connect(sock, (struct sockaddr *)&fr, sizeof(fr)) < 0)
        die("connect_to_front");
    return sock;
}

int front_send_move(int sock, int row, int col) {
    FrontHeader hdr = {.type = 1, .length = sizeof(int) * 2};
    if (safe_send(sock, &hdr, sizeof(hdr)) < 0) return -1;
    int payload[2] = {row, col};
    return safe_send(sock, payload, sizeof(payload)) > 0 ? 0 : -1;
}

int front_send_board(int sock, const Stone board[ROWS][COLS]) {
    FrontHeader hdr = {.type = 2, .length = ROWS * COLS};
    if (safe_send(sock, &hdr, sizeof(hdr)) < 0) return -1;
    // flatten board
    uint8_t flat[ROWS * COLS];
    for (int i = 0; i < ROWS; i++)
        for (int j = 0; j < COLS; j++) flat[i * COLS + j] = board[i][j];
    return safe_send(sock, flat, sizeof(flat)) > 0 ? 0 : -1;
}

int front_recv_cmd(int sock, uint8_t *cmd_type, void *payload) {
    FrontHeader hdr;
    if (safe_recv(sock, &hdr, sizeof(hdr)) <= 0) return -1;
    *cmd_type = hdr.type;
    if (hdr.length > 0 && safe_recv(sock, payload, hdr.length) <= 0) return -1;
    return hdr.length;
}
