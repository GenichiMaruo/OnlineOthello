// server_comm.h
#ifndef SERVER_COMM_H
#define SERVER_COMM_H

#include <stdint.h>

#include "server_protocol.h"

// サーバーへ接続し、ソケットを返す
int connect_to_server(const char *addr, uint16_t port);

// PacketHeader＋可変長ペイロードを送信
int server_send_packet(int sock, const PacketHeader *hdr, const void *payload);

// PacketHeader＋ペイロードを受信
int server_recv_packet(int sock, PacketHeader *hdr, void *payload);

#endif  // SERVER_COMM_H
