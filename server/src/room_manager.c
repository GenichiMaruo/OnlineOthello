#include "room_manager.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

typedef struct {
    int room_id;
    char room_name[64];
    char password[32];
    int player_socks[2];  // プレイヤーのソケット
    int player_count;
    int spectator_socks[MAX_SPECTATORS];
    int spectator_count;
} Room;

static Room rooms[MAX_ROOMS];

void init_rooms() { memset(rooms, 0, sizeof(rooms)); }

// 空いてる部屋を探して作成する
int create_room(const char *room_name, const char *password) {
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].player_count == 0) {
            rooms[i].room_id = i;
            strncpy(rooms[i].room_name, room_name,
                    sizeof(rooms[i].room_name) - 1);
            strncpy(rooms[i].password, password, sizeof(rooms[i].password) - 1);
            rooms[i].player_socks[0] = -1;
            rooms[i].player_socks[1] = -1;
            rooms[i].spectator_count = 0;
            printf("ルーム[%s]を作成 (ID:%d)\n", room_name, i);
            return i;
        }
    }
    printf("ルーム作成失敗：上限到達\n");
    return -1;
}

// ルームに参加
int join_room(int client_sock, const char *room_name, const char *password,
              int is_spectator) {
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (strcmp(rooms[i].room_name, room_name) == 0) {
            if (strcmp(rooms[i].password, password) != 0) {
                printf("パスワード不一致\n");
                return -1;
            }

            if (is_spectator) {
                if (rooms[i].spectator_count >= MAX_SPECTATORS) {
                    printf("観戦者上限\n");
                    return -1;
                }
                rooms[i].spectator_socks[rooms[i].spectator_count++] =
                    client_sock;
                printf("クライアント[%d]が観戦者としてルーム[%s]に参加\n",
                       client_sock, room_name);
                return i;
            } else {
                if (rooms[i].player_count >= 2) {
                    printf("プレイヤー上限\n");
                    return -1;
                }
                rooms[i].player_socks[rooms[i].player_count++] = client_sock;
                printf("クライアント[%d]がプレイヤーとしてルーム[%s]に参加\n",
                       client_sock, room_name);
                return i;
            }
        }
    }

    // ルームが存在しなかったら新規作成
    int new_room_id = create_room(room_name, password);
    if (new_room_id >= 0) {
        rooms[new_room_id].player_socks[rooms[new_room_id].player_count++] =
            client_sock;
        printf("クライアント[%d]が新規ルーム[%s]作成して参加\n", client_sock,
               room_name);
    }
    return new_room_id;
}

// クライアントが退出
void leave_room(int client_sock) {
    for (int i = 0; i < MAX_ROOMS; i++) {
        for (int p = 0; p < 2; p++) {
            if (rooms[i].player_socks[p] == client_sock) {
                rooms[i].player_socks[p] = -1;
                rooms[i].player_count--;
                printf("クライアント[%d]がルーム[%s]からプレイヤー退出\n",
                       client_sock, rooms[i].room_name);
                return;
            }
        }
        for (int s = 0; s < rooms[i].spectator_count; s++) {
            if (rooms[i].spectator_socks[s] == client_sock) {
                // 観戦者リストから削除
                for (int k = s; k < rooms[i].spectator_count - 1; k++) {
                    rooms[i].spectator_socks[k] =
                        rooms[i].spectator_socks[k + 1];
                }
                rooms[i].spectator_count--;
                printf("クライアント[%d]がルーム[%s]から観戦者退出\n",
                       client_sock, rooms[i].room_name);
                return;
            }
        }
    }
}

// プレイヤーがコマを置いたら通知（未完成版、あとで完成させる）
void notify_move(int room_id, const MoveInfo *move) {
    if (room_id < 0 || room_id >= MAX_ROOMS) return;

    // 仮実装: 本当はここで相手にsend()する
    printf("ルーム[%d]でコマが置かれた (x=%d, y=%d)\n", room_id, move->row,
           move->col);
}

// チャットメッセージを通知（未完成版、あとで完成させる）
void notify_chat(int room_id, const ChatMessage *msg) {
    if (room_id < 0 || room_id >= MAX_ROOMS) return;

    // 仮実装: 本当はここで全員にsend()する
    printf("ルーム[%d]でチャットメッセージ: %s\n", room_id, msg->message);
}

// オンライン状態を管理する（未使用だけど後で使うかも）
void set_player_online_status(int client_sock, int online) {
    // 今のところ特に何もしてない
}
