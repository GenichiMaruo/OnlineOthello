#include "room_management.h"

#include <stdio.h>  // snprintf のため

#include "client_management.h"  // クライアント情報更新のため必要

// --- グローバル変数定義 ---
Room rooms[MAX_ROOMS];
pthread_mutex_t rooms_mutex = PTHREAD_MUTEX_INITIALIZER;
int room_id_counter = 0;

// --- 部屋初期化 ---
void initialize_rooms() {
    pthread_mutex_lock(&rooms_mutex);
    for (int i = 0; i < MAX_ROOMS; ++i) {
        rooms[i].roomId = -1;  // -1は未使用の部屋を示す
        rooms[i].status = ROOM_EMPTY;
        rooms[i].player1_sock = -1;
        rooms[i].player2_sock = -1;
        memset(rooms[i].roomName, 0, sizeof(rooms[i].roomName));
        // Mutexは必要になった時に初期化する方が良いかもしれないが、ここでは最初に初期化
        if (pthread_mutex_init(&rooms[i].room_mutex, NULL) != 0) {
            perror("Failed to initialize room mutex");
            // エラー処理: 例えばサーバー起動を中止する
            exit(EXIT_FAILURE);
        }
        rooms[i].player1_rematch_agree = 0;
        rooms[i].player2_rematch_agree = 0;
        rooms[i].last_action_time = 0;

        rooms[i].chat_history_count = 0;
        rooms[i].chat_history_next_idx = 0;
        memset(rooms[i].chat_history, 0, sizeof(rooms[i].chat_history));
    }
    pthread_mutex_unlock(&rooms_mutex);
    printf("Room list initialized.\n");
}

// roomIdから部屋のインデックスを検索 (rooms_mutexで保護)
int find_room_index(int roomId) {
    // rooms_mutex は呼び出し元でロックされている想定
    for (int i = 0; i < MAX_ROOMS; ++i) {
        if (rooms[i].roomId == roomId) {
            return i;
        }
    }
    return -1;  // 見つからない
}

// roomIdからRoomポインタを取得 (rooms_mutexで保護されるべき)
// 注意: この関数はrooms_mutexがロックされているコンテキストで呼ばれる想定
// もしくは、内部でロック/アンロックする。ここでは後者を採用。
Room* get_room_by_id(int roomId) {
    pthread_mutex_lock(&rooms_mutex);
    for (int i = 0; i < MAX_ROOMS; ++i) {
        if (rooms[i].roomId == roomId) {
            pthread_mutex_unlock(&rooms_mutex);
            return &rooms[i];  // ポインタを返す場合は生存期間に注意
        }
    }
    pthread_mutex_unlock(&rooms_mutex);
    return NULL;
}

// 空き部屋のインデックスを検索 (rooms_mutexで保護)
// 注意: この関数はrooms_mutexがロックされているコンテキストで呼ばれる想定
int find_empty_room_index() {
    for (int i = 0; i < MAX_ROOMS; ++i) {
        // roomId == -1 を空き部屋の印とする（status ROOM_EMPTY と併用）
        if (rooms[i].roomId == -1 && rooms[i].status == ROOM_EMPTY) {
            return i;
        }
    }
    return -1;  // 満室
}

int create_new_room(int client_sock, const char* roomName) {
    pthread_mutex_lock(&rooms_mutex);  // 部屋リスト全体をロック

    int room_idx = find_empty_room_index();  // rooms_mutexロック中に呼び出し
    if (room_idx == -1) {
        pthread_mutex_unlock(&rooms_mutex);
        fprintf(stderr, "Failed to create room: no empty slots.\n");
        return -1;  // 満室
    }

    // 部屋固有のミューテックスをロック (リストロック中に取得)
    pthread_mutex_lock(&rooms[room_idx].room_mutex);

    int new_room_id = room_id_counter++;  // 新しいIDを割り当て
    rooms[room_idx].roomId = new_room_id;
    strncpy(rooms[room_idx].roomName, roomName, MAX_ROOM_NAME_LEN - 1);
    rooms[room_idx].roomName[MAX_ROOM_NAME_LEN - 1] = '\0';
    rooms[room_idx].status = ROOM_WAITING;
    rooms[room_idx].player1_sock = client_sock;
    rooms[room_idx].player2_sock = -1;
    rooms[room_idx].last_action_time = time(NULL);
    rooms[room_idx].player1_rematch_agree = 0;
    rooms[room_idx].player2_rematch_agree = 0;

    // ゲーム状態の初期化 (空っぽの状態)
    memset(&rooms[room_idx].gameState, 0, sizeof(GameState));
    rooms[room_idx].gameState.currentTurn = 0;  // まだ始まっていない
    rooms[room_idx].chat_history_count = 0;
    rooms[room_idx].chat_history_next_idx = 0;
    memset(rooms[room_idx].chat_history, 0,
           sizeof(rooms[room_idx].chat_history));

    // 部屋リスト全体のロックを解除 (部屋固有ロックは保持)
    pthread_mutex_unlock(&rooms_mutex);

    // クライアント情報にも部屋IDを記録
    pthread_mutex_lock(&clients_mutex);
    int client_idx = find_client_index(client_sock);
    if (client_idx != -1) {
        clients[client_idx].roomId = new_room_id;
        clients[client_idx].playerColor =
            1;  // 部屋作成者は黒（先手）とする (仮)
    } else {
        // クライアントが見つからないエラー (通常発生しないはず)
        fprintf(stderr,
                "Error: Client sockfd %d not found when creating room %d.\n",
                client_sock, new_room_id);
        // エラー処理: 作成した部屋をキャンセルするなど
        pthread_mutex_unlock(&clients_mutex);
        pthread_mutex_unlock(
            &rooms[room_idx].room_mutex);  // 部屋固有ロックも解除
        // 部屋情報をリセット
        pthread_mutex_lock(&rooms_mutex);
        rooms[room_idx].roomId = -1;
        rooms[room_idx].status = ROOM_EMPTY;
        rooms[room_idx].player1_sock = -1;
        pthread_mutex_unlock(&rooms_mutex);
        return -1;  // エラーを示す
    }
    pthread_mutex_unlock(&clients_mutex);

    printf("Room %d ('%s') created by client sockfd %d (Player 1).\n",
           new_room_id, rooms[room_idx].roomName, client_sock);

    pthread_mutex_unlock(
        &rooms[room_idx].room_mutex);  // 部屋固有のミューテックスをアンロック

    return new_room_id;
}

int join_room(int client_sock, int targetRoomId) {
    pthread_mutex_lock(&rooms_mutex);  // 部屋リスト全体をロック

    int room_idx =
        find_room_index(targetRoomId);  // rooms_mutexロック中に呼び出し

    if (room_idx == -1) {
        pthread_mutex_unlock(&rooms_mutex);
        printf("Client sockfd %d failed to join non-existent room %d\n",
               client_sock, targetRoomId);
        return -1;  // 部屋が見つからない
    }

    Room* current_room = &rooms[room_idx];
    printf("Client sockfd %d is trying to join room %d\n", client_sock,
           targetRoomId);
    // 部屋固有のミューテックスをロック (リストロック中に取得)
    pthread_mutex_lock(&current_room->room_mutex);
    pthread_mutex_unlock(&rooms_mutex);  // rooms_mutex は解放

    if (current_room->status != ROOM_WAITING) {
        pthread_mutex_unlock(&current_room->room_mutex);
        fprintf(stderr,
                "Client sockfd %d failed to join room %d (not waiting, status: "
                "%d)\n",
                client_sock, targetRoomId, current_room->status);
        return (current_room->status == ROOM_PLAYING ||
                current_room->status == ROOM_GAMEOVER ||
                current_room->status == ROOM_REMATCHING)
                   ? -2
                   : -3;
    }

    // プレイヤー2として参加
    current_room->player2_sock = client_sock;
    current_room->last_action_time = time(NULL);

    // クライアント情報にも部屋IDと色を記録
    pthread_mutex_lock(&clients_mutex);
    int client_idx = find_client_index(client_sock);
    if (client_idx != -1) {
        clients[client_idx].roomId = current_room->roomId;
        clients[client_idx].playerColor = 2;
    } else {
        fprintf(stderr,
                "Error: Client sockfd %d not found when joining room %d.\n",
                client_sock, targetRoomId);
        pthread_mutex_unlock(&clients_mutex);
        current_room->player2_sock = -1;  // ロールバック
        pthread_mutex_unlock(&current_room->room_mutex);
        return -1;
    }
    pthread_mutex_unlock(&clients_mutex);
    // --- 参加者にチャット履歴を送信 & 相手に参加を通知 ---
    // 必要な情報を room_mutex ロック中に取得し、アンロック後に送信
    ChatMessageEntry history_copy[MAX_CHAT_HISTORY];
    int history_count_copy = 0;
    int p1_sock_to_notify =
        current_room->player1_sock;  // player1_sock もコピー

    // チャット履歴をコピー (リングバッファ対応)
    if (current_room->chat_history_count > 0) {
        history_count_copy = current_room->chat_history_count;
        int start_idx = (current_room->chat_history_next_idx -
                         current_room->chat_history_count + MAX_CHAT_HISTORY) %
                        MAX_CHAT_HISTORY;
        for (int i = 0; i < history_count_copy; ++i) {
            history_copy[i] =
                current_room->chat_history[(start_idx + i) % MAX_CHAT_HISTORY];
        }
    }

    // 部屋固有のミューテックスをアンロックしてから通知と履歴送信
    pthread_mutex_unlock(&current_room->room_mutex);

    // 相手プレイヤー(Player 1)に参加を通知
    if (p1_sock_to_notify != -1) {
        Message notify_msg;
        notify_msg.type = MSG_PLAYER_JOINED_NOTICE;
        notify_msg.data.playerJoinedNotice.roomId = targetRoomId;
        sendMessage(p1_sock_to_notify, &notify_msg);
        printf(
            "Notified player 1 (sockfd %d) about player 2 joining room %d.\n",
            p1_sock_to_notify, targetRoomId);
    }

    // 新規参加者 (client_sock) にチャット履歴を送信
    if (history_count_copy > 0) {
        printf(
            "Sending %d chat history messages to client sockfd %d in room "
            "%d.\n",
            history_count_copy, client_sock, targetRoomId);
        for (int i = 0; i < history_count_copy; ++i) {
            Message chat_notice_msg;
            chat_notice_msg.type = MSG_CHAT_MESSAGE_BROADCAST_NOTICE;
            ChatMessageBroadcastNoticeData* notice_data =
                &chat_notice_msg.data.chatMessageBroadcastNotice;

            notice_data->roomId = targetRoomId;
            notice_data->timestamp = history_copy[i].timestamp;
            strncpy(notice_data->message_text, history_copy[i].message,
                    MAX_CHAT_MESSAGE_LEN - 1);
            notice_data->message_text[MAX_CHAT_MESSAGE_LEN - 1] = '\0';

            pthread_mutex_lock(&clients_mutex);
            int sender_c_idx = find_client_index(history_copy[i].sender_sock);
            if (sender_c_idx != -1) {
                notice_data->sender_player_color =
                    clients[sender_c_idx].playerColor;
                if (clients[sender_c_idx].playerColor == 1) {
                    snprintf(notice_data->sender_display_name,
                             MAX_ROOM_NAME_LEN, "Player 1");
                } else if (clients[sender_c_idx].playerColor == 2) {
                    snprintf(notice_data->sender_display_name,
                             MAX_ROOM_NAME_LEN, "Player 2");
                } else {  // 念のため
                    snprintf(notice_data->sender_display_name,
                             MAX_ROOM_NAME_LEN, "User %d",
                             history_copy[i].sender_sock);
                }
            } else {
                notice_data->sender_player_color = 0;  // Unknown
                snprintf(notice_data->sender_display_name, MAX_ROOM_NAME_LEN,
                         "Past User");
            }
            pthread_mutex_unlock(&clients_mutex);

            if (sendMessage(client_sock, &chat_notice_msg) == -1) {
                fprintf(
                    stderr,
                    "Error sending chat history message to client sockfd %d\n",
                    client_sock);
                break;  // エラー発生時は以降の送信を中止
            }
        }
    }
    return targetRoomId;  // 成功
}

static void process_and_broadcast_chat_message(int roomId, int sender_sock,
                                               const char* message_text) {
    pthread_mutex_lock(&rooms_mutex);
    int room_idx = find_room_index(roomId);
    if (room_idx == -1) {
        pthread_mutex_unlock(&rooms_mutex);
        fprintf(stderr,
                "Error: Room %d not found for chat message from sock %d.\n",
                roomId, sender_sock);
        // TODO: 送信者にエラーを返すか検討
        return;
    }

    Room* room = &rooms[room_idx];
    pthread_mutex_lock(&room->room_mutex);
    pthread_mutex_unlock(&rooms_mutex);  // rooms_mutex は解放

    // 1. チャット履歴に追加 (リングバッファ)
    ChatMessageEntry* new_entry =
        &room->chat_history[room->chat_history_next_idx];
    new_entry->sender_sock = sender_sock;
    strncpy(new_entry->message, message_text, MAX_CHAT_MESSAGE_LEN - 1);
    new_entry->message[MAX_CHAT_MESSAGE_LEN - 1] = '\0';
    new_entry->timestamp = time(NULL);

    room->chat_history_next_idx =
        (room->chat_history_next_idx + 1) % MAX_CHAT_HISTORY;
    if (room->chat_history_count < MAX_CHAT_HISTORY) {
        room->chat_history_count++;
    }

    // 2. ブロードキャスト用のメッセージ作成
    Message chat_notice_msg;
    chat_notice_msg.type = MSG_CHAT_MESSAGE_BROADCAST_NOTICE;
    ChatMessageBroadcastNoticeData* notice_data =
        &chat_notice_msg.data.chatMessageBroadcastNotice;

    notice_data->roomId = roomId;
    notice_data->timestamp = new_entry->timestamp;
    strncpy(notice_data->message_text, new_entry->message,
            MAX_CHAT_MESSAGE_LEN);  // 既にNULL終端されているはず

    pthread_mutex_lock(&clients_mutex);
    int sender_c_idx = find_client_index(sender_sock);
    if (sender_c_idx != -1) {
        notice_data->sender_player_color = clients[sender_c_idx].playerColor;
        if (clients[sender_c_idx].playerColor == 1) {
            snprintf(notice_data->sender_display_name, MAX_ROOM_NAME_LEN,
                     "Player 1");
        } else if (clients[sender_c_idx].playerColor == 2) {
            snprintf(notice_data->sender_display_name, MAX_ROOM_NAME_LEN,
                     "Player 2");
        } else {
            snprintf(notice_data->sender_display_name, MAX_ROOM_NAME_LEN,
                     "User %d", sender_sock);
        }
    } else {  // 接続が切れた直後など、クライアントリストにない場合
        notice_data->sender_player_color = 0;  // Unknown
        snprintf(notice_data->sender_display_name, MAX_ROOM_NAME_LEN, "User %d",
                 sender_sock);  // SockFDでフォールバック
    }
    pthread_mutex_unlock(&clients_mutex);

    // 3. ルームメンバーにブロードキャスト (送信者自身にも送る)
    int p1_sock = room->player1_sock;
    int p2_sock = room->player2_sock;

    pthread_mutex_unlock(&room->room_mutex);  // sendMessage の前にアンロック

    if (p1_sock != -1) {
        if (sendMessage(p1_sock, &chat_notice_msg) == -1) {
            fprintf(stderr,
                    "Error sending chat broadcast to player 1 (sock %d) in "
                    "room %d.\n",
                    p1_sock, roomId);
        }
    }
    if (p2_sock != -1) {
        if (sendMessage(p2_sock, &chat_notice_msg) == -1) {
            fprintf(stderr,
                    "Error sending chat broadcast to player 2 (sock %d) in "
                    "room %d.\n",
                    p2_sock, roomId);
        }
    }
    printf("Chat from sock %d in room %d ('%s') broadcasted.\n", sender_sock,
           roomId, message_text);
}

// クライアントからのチャットメッセージ要求を処理
void handle_chat_message(int client_sock, int roomId,
                         const char* message_text) {
    // 入力基本的なチェック
    if (message_text == NULL || strlen(message_text) == 0) {
        fprintf(stderr,
                "Received empty chat message from sock %d for room %d.\n",
                client_sock, roomId);
        return;
    }
    // メッセージ長チェックは sendMessage
    // やプロトコルレベルで行われる想定だが、ここでも軽くチェック
    if (strlen(message_text) >= MAX_CHAT_MESSAGE_LEN) {
        fprintf(stderr,
                "Chat message from sock %d for room %d is too long (max %d, "
                "got %zu).\n",
                client_sock, roomId, MAX_CHAT_MESSAGE_LEN,
                strlen(message_text));
        // メッセージを切り詰めるか、エラーを返す。ここでは何もしない。
    }

    // 実際の処理は process_and_broadcast_chat_message に委譲
    process_and_broadcast_chat_message(roomId, client_sock, message_text);
}

// 部屋の全員にメッセージ送信 (exclude_sockを除く)
void broadcast_to_room(int roomId, const Message* msg, int exclude_sock) {
    pthread_mutex_lock(&rooms_mutex);
    int room_idx = find_room_index(roomId);

    if (room_idx == -1) {
        pthread_mutex_unlock(&rooms_mutex);
        fprintf(stderr, "Warning: Cannot broadcast to non-existent room %d.\n",
                roomId);
        return;  // 部屋なし
    }

    pthread_mutex_lock(&rooms[room_idx].room_mutex);
    pthread_mutex_unlock(&rooms_mutex);  // リストロック解除

    int p1_sock = rooms[room_idx].player1_sock;
    int p2_sock = rooms[room_idx].player2_sock;

    // メッセージ送信は room_mutex
    // のロック外で行う方がデッドロックのリスクが低い
    pthread_mutex_unlock(&rooms[room_idx].room_mutex);

    if (p1_sock != -1 && p1_sock != exclude_sock) {
        // printf("Broadcasting msg type %d to P1 (sock %d) in room %d\n",
        // msg->type, p1_sock, roomId);
        if (sendMessage(p1_sock, msg) == -1) {
            fprintf(stderr,
                    "Error sending broadcast message to player 1 (sock %d) in "
                    "room %d.\n",
                    p1_sock, roomId);
            // エラー処理（例: クライアント切断として扱う）が必要な場合がある
        }
    }
    if (p2_sock != -1 && p2_sock != exclude_sock) {
        // printf("Broadcasting msg type %d to P2 (sock %d) in room %d\n",
        // msg->type, p2_sock, roomId);
        if (sendMessage(p2_sock, msg) == -1) {
            fprintf(stderr,
                    "Error sending broadcast message to player 2 (sock %d) in "
                    "room %d.\n",
                    p2_sock, roomId);
            // エラー処理
        }
    }
}

// 部屋を閉鎖し、プレイヤーに通知
void close_room(int roomId, const char* reason) {
    pthread_mutex_lock(&rooms_mutex);
    int room_idx = find_room_index(roomId);

    if (room_idx == -1) {
        pthread_mutex_unlock(&rooms_mutex);
        fprintf(stderr, "Warning: Cannot close non-existent room %d.\n",
                roomId);
        return;  // 部屋なし
    }

    pthread_mutex_lock(&rooms[room_idx].room_mutex);
    // 部屋リスト全体のロックは解除して良い
    pthread_mutex_unlock(&rooms_mutex);

    printf("Closing room %d: %s\n", roomId, reason);

    int p1_sock = rooms[room_idx].player1_sock;
    int p2_sock = rooms[room_idx].player2_sock;

    // 部屋情報をリセット
    // (通知前にリセットしないと、通知中に別のスレッドが入る可能性)
    rooms[room_idx].status = ROOM_EMPTY;
    rooms[room_idx].roomId = -1;  // ID無効化
    rooms[room_idx].player1_sock = -1;
    rooms[room_idx].player2_sock = -1;
    rooms[room_idx].player1_rematch_agree = 0;
    rooms[room_idx].player2_rematch_agree = 0;
    memset(rooms[room_idx].roomName, 0, sizeof(rooms[room_idx].roomName));
    // gameState もクリア
    memset(&rooms[room_idx].gameState, 0, sizeof(GameState));

    pthread_mutex_unlock(&rooms[room_idx].room_mutex);  // 通知前にアンロック

    // 通知メッセージ作成
    Message close_msg;
    close_msg.type = MSG_ROOM_CLOSED_NOTICE;
    close_msg.data.roomClosedNotice.roomId =
        roomId;  // 閉鎖される部屋IDは通知する
    snprintf(close_msg.data.roomClosedNotice.reason,
             sizeof(close_msg.data.roomClosedNotice.reason), "%s", reason);

    // 各プレイヤーに通知し、クライアント側の部屋情報をリセット
    if (p1_sock != -1) {
        sendMessage(p1_sock, &close_msg);
        pthread_mutex_lock(&clients_mutex);
        int idx = find_client_index(p1_sock);
        if (idx != -1) {
            clients[idx].roomId = -1;
            clients[idx].playerColor = 0;
        }
        pthread_mutex_unlock(&clients_mutex);
    }
    if (p2_sock != -1) {
        sendMessage(p2_sock, &close_msg);
        pthread_mutex_lock(&clients_mutex);
        int idx = find_client_index(p2_sock);
        if (idx != -1) {
            clients[idx].roomId = -1;
            clients[idx].playerColor = 0;
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    // room_mutex は再利用するので destroy しない
}

// 相手プレイヤーのソケットを取得 (内部で room lock/unlock)
int get_opponent_sock(int roomId, int self_sock) {
    pthread_mutex_lock(&rooms_mutex);
    int room_idx = find_room_index(roomId);

    if (room_idx == -1) {
        pthread_mutex_unlock(&rooms_mutex);
        return -1;  // 部屋が見つからない
    }

    pthread_mutex_lock(&rooms[room_idx].room_mutex);
    pthread_mutex_unlock(&rooms_mutex);  // リストロック解除

    int opponent_sock = -1;
    if (rooms[room_idx].player1_sock == self_sock) {
        opponent_sock = rooms[room_idx].player2_sock;
    } else if (rooms[room_idx].player2_sock == self_sock) {
        opponent_sock = rooms[room_idx].player1_sock;
    } else {
        // 部屋のプレイヤーではない場合
        fprintf(stderr,
                "Warning: get_opponent_sock called by non-player (sock %d) for "
                "room %d.\n",
                self_sock, roomId);
    }

    pthread_mutex_unlock(&rooms[room_idx].room_mutex);
    return opponent_sock;
}