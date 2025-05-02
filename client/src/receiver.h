#ifndef RECEIVER_H
#define RECEIVER_H

#include "client_common.h"

// --- 関数プロトタイプ ---
void* receive_handler(void* arg);
void process_server_message(const Message* msg);

#endif  // RECEIVER_H