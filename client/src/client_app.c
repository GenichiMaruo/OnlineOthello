// client_app.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "front_comm.h"
#include "othello_logic.h"
#include "server_comm.h"
#include "server_protocol.h"  // サーバー用 PacketHeader etc.
#include "utils.h"

#define SERVER_PORT 10000
#define FRONT_PORT 5000

int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s -s server_addr -n user_name\n", argv[0]);
        exit(1);
    }
    char *srv_addr = NULL, *user_name = NULL;
    int opt;
    while ((opt = getopt(argc, argv, "s:n:")) != -1) {
        if (opt == 's') {
            srv_addr = optarg;
            // もしアドレスがlocalhostなら数値に変換
            if (strcmp(srv_addr, "localhost") == 0) {
                srv_addr = strdup("127.0.0.1");
            }
            printf("%s", srv_addr);
        }
        if (opt == 'n') user_name = optarg;
    }

    // 1) Next.js フロントを起動
    system("cd ./src/othello-front && npm run dev &");
    sleep(20);  // 起動待ち

    // 2) サーバーとフロントへ接続
    int srv_sock = connect_to_server(srv_addr, SERVER_PORT);
    int front_sock = connect_to_front("127.0.0.1", FRONT_PORT);

    // 3) ゲーム開始シーケンス (hello/conn_req 等) はここで実装
    //    → server_send_packet / server_recv_packet を使う

    Stone board[ROWS][COLS];
    initialize_board(board);

    fd_set rfds;
    while (1) {
        FD_ZERO(&rfds);
        FD_SET(srv_sock, &rfds);
        FD_SET(front_sock, &rfds);
        int maxfd = srv_sock > front_sock ? srv_sock : front_sock;
        if (select(maxfd + 1, &rfds, NULL, NULL, NULL) < 0) {
            die("select");
        }

        // サーバーからの更新
        if (FD_ISSET(srv_sock, &rfds)) {
            PacketHeader hdr;
            uint8_t payload[512];
            if (server_recv_packet(srv_sock, &hdr, payload) <= 0)
                die("server disconnected");
            switch (hdr.type) {
                case MOVE: {
                    MoveInfo *m = (MoveInfo *)payload;
                    flip((Stone)m->color, m->row, m->col, board);
                    // フロントに盤面更新を通知
                    front_send_board(front_sock, board);
                    break;
                }
                // ... 他ケース (BOARD_STATE, CHAT_MESSAGE など)
                default:
                    break;
            }
        }

        // フロントからの操作
        if (FD_ISSET(front_sock, &rfds)) {
            uint8_t cmd;
            uint8_t data[16];
            int len = front_recv_cmd(front_sock, &cmd, data);
            if (len < 0) die("front disconnected");
            switch (cmd) {
                case 1: {  // FRONT_MOVE
                    int row = ((int *)data)[0];
                    int col = ((int *)data)[1];
                    // サーバーへ送信
                    PacketHeader hdr = {.type = MOVE,
                                        .length = sizeof(MoveInfo)};
                    MoveInfo m = {
                        .room_id = 0, .row = row, .col = col, .color = BLACK};
                    server_send_packet(srv_sock, &hdr, &m);
                    break;
                }
                // ... 他コマンド
                default:
                    break;
            }
        }
    }

    close(srv_sock);
    close(front_sock);
    return 0;
}
