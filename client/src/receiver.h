#ifndef RECEIVER_H
#define RECEIVER_H

#include "client_common.h"

// --- 関数プロトタイプ ---

// サーバーからのメッセージ受信スレッド関数
void* receive_handler(void* arg);

// 受信したサーバーメッセージを処理する
void process_server_message(const Message* msg);

#endif  // RECEIVER_H