#include "packet_handler.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "room_manager.h"
#include "server_protocol.h"
#include "utils.h"

// クライアントごとの通信セッションを処理する
int handle_client_session(int client_sock) {
    ProtocolPacket packet;

    while (1) {
        // まずパケットを受信
        ssize_t received =
            safe_recv(client_sock, &packet, sizeof(ProtocolPacket));
        if (received <= 0) {
            printf("クライアント[%d]切断またはエラー\n", client_sock);
            leave_room(client_sock);  // ルームから抜ける
            close(client_sock);
            return -1;
        }

        // 受け取ったパケットを解析
        switch (packet.type) {
            case PACKET_JOIN_ROOM:
                printf("クライアント[%d]がルームに参加要求\n", client_sock);
                join_room(client_sock, packet.data.join_room.room_name,
                          packet.data.join_room.password, 0);
                break;

            case PACKET_SPECTATE_ROOM:
                printf("クライアント[%d]が観戦に参加要求\n", client_sock);
                join_room(client_sock, packet.data.join_room.room_name,
                          packet.data.join_room.password, 1);
                break;

            case PACKET_MOVE:
                printf("クライアント[%d]がコマを置いた\n", client_sock);
                notify_move(packet.data.move.room_id, &packet.data.move);
                break;

            case PACKET_CHAT:
                printf("クライアント[%d]がチャット送信\n", client_sock);
                notify_chat(packet.data.chat.room_id, &packet.data.chat);
                break;

            case PACKET_DISCONNECT:
                printf("クライアント[%d]が手動で切断\n", client_sock);
                leave_room(client_sock);
                close(client_sock);
                return 0;

            default:
                printf("クライアント[%d]から未知のパケット受信: %d\n",
                       client_sock, packet.type);
                break;
        }
    }

    return 0;
}
