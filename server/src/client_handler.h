#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "server_common.h"  // 共通定義をインクルード

// --- 関数プロトタイプ ---

// クライアント処理スレッド関数
void* handle_client(void* arg);

// メッセージハンドラ関数
void handle_create_room_request(int client_sock, const Message* msg);
void handle_join_room_request(int client_sock,
                              const Message* msg);  // 追加 (実装は未定)
void handle_start_game_request(int client_sock, const Message* msg);
void handle_place_piece_request(int client_sock, const Message* msg);
void handle_rematch_request(int client_sock, const Message* msg);
void handle_disconnect(int client_sock);
// 他に必要なメッセージハンドラがあれば追加

#endif  // CLIENT_HANDLER_H