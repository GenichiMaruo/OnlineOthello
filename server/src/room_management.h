#ifndef ROOM_MANAGEMENT_H
#define ROOM_MANAGEMENT_H

#include "server_common.h"

// --- グローバル変数 (extern宣言) ---
extern Room rooms[MAX_ROOMS];
extern pthread_mutex_t rooms_mutex;
extern int room_id_counter;

// --- 関数プロトタイプ ---
void initialize_rooms();
int find_room_index(
    int roomId);              // roomIdからインデックスを探すヘルパー関数(追加)
int find_empty_room_index();  // 空き部屋のインデックスを探すヘルパー関数(名前変更)
int create_new_room(int client_sock, const char* roomName);
int join_room(int client_sock, int targetRoomId);
void close_room(int roomId, const char* reason);
void broadcast_to_room(int roomId, const Message* msg, int exclude_sock);
int get_opponent_sock(int roomId, int self_sock);
Room* get_room_by_id(
    int roomId);  // roomIdからRoomポインタを取得するヘルパー関数 (追加)

#endif  // ROOM_MANAGEMENT_H