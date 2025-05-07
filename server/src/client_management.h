#ifndef CLIENT_MANAGEMENT_H
#define CLIENT_MANAGEMENT_H

#include "server_common.h"  // 共通定義をインクルード

// --- グローバル変数 (extern宣言) ---
extern ClientInfo clients[MAX_CLIENTS];
extern pthread_mutex_t clients_mutex;

// --- 関数プロトタイプ ---
void initialize_clients();  // クライアントリスト初期化関数名を変更
int find_client_index(int sockfd);
int add_client(int sockfd, struct sockaddr_in addr);
void remove_client(int sockfd);
ClientInfo* get_client_info(
    int sockfd);  // sockfdからClientInfoポインタを取得するヘルパー関数 (追加)

#endif  // CLIENT_MANAGEMENT_H