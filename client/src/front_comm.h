// front_comm.h
#ifndef FRONT_COMM_H
#define FRONT_COMM_H

#include <stdint.h>

#include "othello_logic.h"

// フロントエンドへ接続 (localhost, port)
int connect_to_front(const char *addr, uint16_t port);

// フロントへの「自分が打った石」を送信
int front_send_move(int sock, int row, int col);

// フロントへの「盤面全体」を送信
int front_send_board(int sock, const Stone board[ROWS][COLS]);

// フロントからのコマンドを受信
int front_recv_cmd(int sock, uint8_t *cmd_type, void *payload);

#endif  // FRONT_COMM_H
