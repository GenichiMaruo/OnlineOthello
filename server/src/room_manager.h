#ifndef ROOM_MANAGER_H
#define ROOM_MANAGER_H

#include "server_protocol.h"

#define MAX_ROOMS 50
#define MAX_SPECTATORS 20

// ルーム情報初期化
void init_rooms();

// ルーム作成
int create_room(const char *room_name, const char *password);

// ルームに参加（プレイヤーor観戦者）
int join_room(int client_sock, const char *room_name, const char *password,
              int is_spectator);

// クライアント退出
void leave_room(int client_sock);

// コマを置いた通知
void notify_move(int room_id, const MoveInfo *move);

// チャット送信通知
void notify_chat(int room_id, const ChatMessage *msg);

// プレイヤーのオンライン/オフライン管理
void set_player_online_status(int client_sock, int online);

#endif  // ROOM_MANAGER_H
