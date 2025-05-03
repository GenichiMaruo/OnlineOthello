#ifndef NETWORK_H
#define NETWORK_H

#include "client_common.h"

// --- 関数プロトタイプ ---

// サーバーに接続する
// 戻り値: 成功ならソケットディスクリプタ、失敗なら -1
int connect_to_server();

// 接続を閉じる
void close_connection();

// サーバーにメッセージを送信する (状態管理付きラッパー)
// 戻り値: 成功なら 1, 失敗なら 0
int send_message_to_server(const Message* msg);

#endif  // NETWORK_H