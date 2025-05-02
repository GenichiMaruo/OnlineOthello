#ifndef CONNECTION_H
#define CONNECTION_H

#include "client_common.h"  // 共通ヘッダー

// --- 関数プロトタイプ ---
int connect_to_server();  // 接続成功ならsockfd, 失敗なら-1を返すように変更
void disconnect_from_server();  // サーバーからの切断処理

#endif  // CONNECTION_H