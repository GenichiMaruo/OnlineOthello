#include <ctype.h>  // isspace

#include "client_common.h"
#include "json_output.h"
#include "network.h"
#include "receiver.h"
#include "state.h"

// --- プロトタイプ宣言 ---
static void handle_input_commands();
static void process_command(const char* json_command);
static void cleanup();

// --- グローバル変数 ---
// サーバーのIPアドレスとポート番号 (デフォルト値)
char g_server_ip[64] = "127.0.0.1";  // デフォルト値
int g_server_port = 10000;           // デフォルト値

// --- main関数 ---
int main() {
    initialize_state();
    send_log_event(LOG_INFO, "Client starting...");

    pthread_t tid;
    if (pthread_create(&tid, NULL, receive_handler, NULL) != 0) {
        perror("Failed to create receiver thread");  // stderrに出力
        send_error_event(
            "Failed to start receiver thread");  // JSONでNode.jsにも通知
        cleanup();
        return 1;
    }
    set_recv_thread_id(tid);

    // メインスレッドは入力コマンド処理
    handle_input_commands();

    // 終了処理
    cleanup();

    return 0;
}

// --- 入力コマンド処理ループ ---
static void handle_input_commands() {
    char input_buffer[512];

    send_log_event(LOG_INFO, "Waiting for commands from stdin...");

    while (1) {
        ClientState current_state = get_client_state();
        if (current_state == STATE_REMOTE_CLOSED ||
            current_state == STATE_QUITTING) {
            send_log_event(LOG_INFO, "Exiting command loop due to state %s",
                           state_to_string(current_state));
            break;
        }

        if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
            if (feof(stdin)) {
                send_log_event(LOG_INFO, "EOF detected on stdin. Exiting...");
            } else {
                perror("fgets error");
                send_error_event("Error reading command from stdin");
            }
            set_client_state(STATE_QUITTING);
            break;
        }

        char* ptr = input_buffer;
        while (isspace((unsigned char)*ptr)) ptr++;
        if (*ptr == '\0' || *ptr == '\n') {
            continue;
        }
        input_buffer[strcspn(input_buffer, "\n")] = 0;
        process_command(input_buffer);
    }
}

// --- JSONコマンド処理 ---
static void process_command(const char* json_command) {
    send_log_event(LOG_DEBUG, "Received command: %s", json_command);

    char command[64] = {0};
    char roomName[MAX_ROOM_NAME_LEN] = {0};
    int roomId = -1;
    int row = -1, col = -1;
    int agree = -1;           // 0 or 1 for boolean
    char serverIp[64] = {0};  // connect 用
    int serverPort = -1;      // connect 用
    char chatMessage[sizeof(((ChatMessageSendRequestData*)0)->message_text)] = {
        0};  // chat 用

    const char* cmd_ptr = strstr(json_command, "\"command\":\"");
    if (cmd_ptr) {
        sscanf(cmd_ptr + strlen("\"command\":\""), "%63[^\"]", command);
    } else {
        send_error_event("Invalid command JSON: Missing 'command' field.");
        return;
    }
    // Parse arguments based on command
    if (strcmp(command, "create") == 0) {
        const char* name_ptr = strstr(json_command, "\"roomName\":\"");
        if (name_ptr) {
            sscanf(name_ptr + strlen("\"roomName\":\""), "%31[^\"]", roomName);
        } else {
            strncpy(roomName, "DefaultRoom", sizeof(roomName) - 1);
            roomName[sizeof(roomName) - 1] = '\0';  // Ensure null termination
        }
    } else if (strcmp(command, "join") == 0) {
        const char* id_ptr = strstr(json_command, "\"roomId\":");
        if (id_ptr) {
            sscanf(id_ptr + strlen("\"roomId\":"), "%d", &roomId);
        } else {
            send_error_event("Invalid 'join' command: Missing 'roomId'.");
            return;
        }
    } else if (strcmp(command, "start") == 0) {
        const char* id_ptr = strstr(json_command, "\"roomId\":");
        if (id_ptr) {
            sscanf(id_ptr + strlen("\"roomId\":"), "%d", &roomId);
        } else {
            roomId = get_my_room_id();
            if (roomId == -1) {
                send_error_event("Invalid 'start' command: Not in a room.");
                return;
            }
        }
    } else if (strcmp(command, "place") == 0) {
        const char* id_ptr = strstr(json_command, "\"roomId\":");
        const char* row_ptr = strstr(json_command, "\"row\":");
        const char* col_ptr = strstr(json_command, "\"col\":");
        if (id_ptr && row_ptr && col_ptr) {
            sscanf(id_ptr + strlen("\"roomId\":"), "%d", &roomId);
            sscanf(row_ptr + strlen("\"row\":"), "%d", &row);
            sscanf(col_ptr + strlen("\"col\":"), "%d", &col);
        } else {
            send_error_event("Invalid 'place' command: Missing fields.");
            return;
        }
    } else if (strcmp(command, "rematch") == 0) {
        const char* id_ptr = strstr(json_command, "\"roomId\":");
        const char* agree_ptr = strstr(json_command, "\"agree\":");
        if (id_ptr && agree_ptr) {
            sscanf(id_ptr + strlen("\"roomId\":"), "%d", &roomId);
            if (strstr(agree_ptr, "true")) {
                agree = 1;
            } else if (strstr(agree_ptr, "false")) {
                agree = 0;
            } else {
                send_error_event(
                    "Invalid 'rematch' command: 'agree' must be true or "
                    "false.");
                return;
            }
        } else {
            send_error_event("Invalid 'rematch' command: Missing fields.");
            return;
        }
    } else if (strcmp(command, "chat") == 0) {  // New command: chat
        const char* id_ptr = strstr(json_command, "\"roomId\":");
        const char* msg_ptr = strstr(json_command, "\"message\":\"");
        int chat_target_roomId = -1;

        if (msg_ptr) {
            // sscanfで読み取る際、バッファサイズ-1を指定して終端ヌル文字のスペースを確保
            sscanf(msg_ptr + strlen("\"message\":\""), "%255[^\"]",
                   chatMessage);
        } else {
            send_error_event("Invalid 'chat' command: Missing 'message'.");
            return;
        }
        if (id_ptr) {
            sscanf(id_ptr + strlen("\"roomId\":"), "%d", &chat_target_roomId);
        } else {
            chat_target_roomId = get_my_room_id();  // 現在のルームIDを使用
            if (chat_target_roomId == -1) {
                send_error_event(
                    "Invalid 'chat' command: Not in a room and roomId not "
                    "specified.");
                return;
            }
        }

        ClientState current_state_for_chat = get_client_state();
        int current_room_id_for_chat = get_my_room_id();

        if (current_room_id_for_chat != -1 &&
            current_room_id_for_chat == chat_target_roomId) {
            switch (current_state_for_chat) {
                case STATE_WAITING_IN_ROOM:
                case STATE_MY_TURN:
                case STATE_OPPONENT_TURN:
                case STATE_GAME_OVER:
                    // 他、チャットを許可したい状態があれば追加
                    // (例: STATE_STARTING_GAME, STATE_PLACING_PIECE)
                    {  // case 内で変数を宣言するためにブロックを使用
                        Message msg;
                        msg.type = MSG_CHAT_MESSAGE_SEND_REQUEST;
                        msg.data.chatMessageSendReq.roomId =
                            current_room_id_for_chat;
                        strncpy(
                            msg.data.chatMessageSendReq.message_text,
                            chatMessage,
                            sizeof(msg.data.chatMessageSendReq.message_text) -
                                1);
                        msg.data.chatMessageSendReq.message_text
                            [sizeof(msg.data.chatMessageSendReq.message_text) -
                             1] = '\0';  // 終端保証
                        if (!send_message_to_server(&msg)) {
                            send_error_event("Failed to send chat message.");
                        } else {
                            send_log_event(
                                LOG_INFO, "Chat message sent to room %d: %s",
                                current_room_id_for_chat, chatMessage);
                            // 自分の送信したメッセージを即時UIに反映させたい場合は、ここで専用のイベントをフロントに送ることもできる。
                            // 通常はサーバーからのブロードキャストを待つ。
                        }
                    }
                    break;
                case STATE_CONNECTED:  // ロビーにいる場合
                    send_error_event(
                        "Cannot send chat message while in Lobby. Join a room "
                        "first.");
                    break;
                default:
                    send_error_event(
                        "Cannot send chat message in current state (%s).",
                        state_to_string(current_state_for_chat));
                    break;
            }
        } else if (chat_target_roomId == -1 &&
                   current_room_id_for_chat ==
                       -1) {  // roomId未指定でルームにもいない
            send_error_event("Cannot send chat: Not in a room.");
        } else {  // 指定された roomId が現在のルームと異なる
            send_error_event(
                "Cannot send chat to room %d: Not currently in that room "
                "(current room: %d).",
                chat_target_roomId, current_room_id_for_chat);
        }
        return;  // chat コマンド処理完了
    } else if (strcmp(command, "getStatus") == 0) {
        // 特に処理なし (状態は自動的に更新される)
        send_log_event(LOG_INFO, "Status command received.");
        // 現在の状態を強制的にJSONで送信
        pthread_mutex_lock(get_state_mutex());
        send_state_change_event_unsafe();
        if (get_my_room_id_unsafe() != -1 &&
            get_client_state_unsafe() != STATE_QUITTING &&
            get_client_state_unsafe() !=
                STATE_DISCONNECTED) {          // 部屋にいる場合
            send_board_update_event_unsafe();  // 盤面も送る
        }
        pthread_mutex_unlock(get_state_mutex());
        return;
    } else if (strcmp(command, "connect") == 0) {
        const char* ip_ptr = strstr(json_command, "\"serverIp\":\"");
        const char* port_ptr = strstr(json_command, "\"serverPort\":");
        if (ip_ptr) {
            sscanf(ip_ptr + strlen("\"serverIp\":\""), "%63[^\"]", serverIp);
            strncpy(g_server_ip, serverIp, sizeof(g_server_ip) - 1);
            g_server_ip[sizeof(g_server_ip) - 1] =
                '\0';  // Ensure null termination
        }
        if (port_ptr) {
            sscanf(port_ptr + strlen("\"serverPort\":"), "%d", &serverPort);
            if (serverPort > 0 && serverPort <= 65535) {
                g_server_port = serverPort;  // グローバル変数に保存
            } else {
                send_error_event("Invalid server port: %d", serverPort);
                return;
            }
        }
        // ここでconnect処理を呼び出す
        ClientState current_state_before_connect = get_client_state();
        if (current_state_before_connect == STATE_DISCONNECTED ||
            current_state_before_connect == STATE_REMOTE_CLOSED) {
            send_log_event(LOG_INFO, "Attempting to connect to server...");
            int connect_result = connect_to_server();  // network.c の関数を呼ぶ

            if (connect_result >= 0) {
                // 接続成功 -> 受信スレッドを開始
                pthread_t tid;
                if (pthread_create(&tid, NULL, receive_handler, NULL) != 0) {
                    perror("Failed to create receiver thread after connect");
                    send_error_event(
                        "Failed to start receiver thread after connect");
                    // 接続は成功したが受信できない -> 切断処理
                    close_connection();
                    set_client_state(STATE_DISCONNECTED);  // 状態を戻す
                    send_state_change_event();
                } else {
                    set_recv_thread_id(tid);  // スレッドIDを保存
                    // 接続成功の通知は connect_to_server 内で行われる
                }
            } else {
                // 接続失敗の通知は connect_to_server 内で行われる
            }
        } else {
            send_log_event(
                LOG_WARN,
                "Already connected or connecting. Ignoring 'connect' command.");
            send_error_event(
                "Already connected or connecting.");  // フロントにも通知
        }
        return;  // connect コマンドの処理終了
    } else if (strcmp(command, "quit") == 0) {
        set_client_state(STATE_QUITTING);
        send_state_change_event();  // 状態変化通知
        return;
    } else {
        send_error_event("Unknown command: %s", command);
        return;
    }

    ClientState current_state = get_client_state();
    int current_room_id = get_my_room_id();
    uint8_t current_color = get_my_color();
    Message msg;

    switch (current_state) {
        case STATE_CONNECTED:
            if (strcmp(command, "create") == 0) {
                msg.type = MSG_CREATE_ROOM_REQUEST;
                strncpy(msg.data.createRoomReq.roomName, roomName,
                        sizeof(msg.data.createRoomReq.roomName) - 1);
                msg.data.createRoomReq
                    .roomName[sizeof(msg.data.createRoomReq.roomName) - 1] =
                    '\0';
                if (send_message_to_server(&msg)) {
                    set_client_state(STATE_CREATING_ROOM);
                    send_state_change_event();
                }
            } else if (strcmp(command, "join") == 0 && roomId != -1) {
                msg.type = MSG_JOIN_ROOM_REQUEST;
                msg.data.joinRoomReq.roomId = roomId;
                if (send_message_to_server(&msg)) {
                    set_client_state(STATE_JOINING_ROOM);
                    send_state_change_event();
                }
            } else {
                send_error_event("Invalid command '%s' in state Lobby.",
                                 command);
            }
            break;
        case STATE_WAITING_IN_ROOM:
            if (current_color == 1 && strcmp(command, "start") == 0 &&
                current_room_id == roomId) {
                msg.type = MSG_START_GAME_REQUEST;
                msg.data.startGameReq.roomId = current_room_id;
                if (send_message_to_server(&msg)) {
                    // set_client_state(STATE_STARTING_GAME);
                    // send_state_change_event();
                }
            } else {
                send_error_event("Invalid command '%s' in state WaitingInRoom.",
                                 command);
            }
            break;
        case STATE_MY_TURN:
            if (strcmp(command, "place") == 0 && current_room_id == roomId &&
                row != -1 && col != -1) {
                if (row >= 0 && row < BOARD_SIZE && col >= 0 &&
                    col < BOARD_SIZE) {
                    msg.type = MSG_PLACE_PIECE_REQUEST;
                    msg.data.placePieceReq.roomId = current_room_id;
                    msg.data.placePieceReq.row = (uint8_t)row;
                    msg.data.placePieceReq.col = (uint8_t)col;
                    if (send_message_to_server(&msg)) {
                        set_client_state(STATE_PLACING_PIECE);
                        send_state_change_event();
                    }
                } else {
                    send_error_event("Invalid coordinates (%d, %d).", row, col);
                }
            } else {
                send_error_event("Invalid command '%s' in state MyTurn.",
                                 command);
            }
            break;
        case STATE_GAME_OVER:
            if (strcmp(command, "rematch") == 0 && current_room_id == roomId &&
                agree != -1) {
                msg.type = MSG_REMATCH_REQUEST;
                msg.data.rematchReq.roomId = current_room_id;
                msg.data.rematchReq.agree = (uint8_t)agree;
                if (send_message_to_server(&msg)) {
                    set_client_state(STATE_SENDING_REMATCH);
                    send_state_change_event();
                }
            } else {
                send_error_event("Invalid command '%s' in state GameOver.",
                                 command);
            }
            break;
        case STATE_OPPONENT_TURN:
        case STATE_CONNECTING:
        case STATE_CREATING_ROOM:
        case STATE_JOINING_ROOM:
        case STATE_STARTING_GAME:
        case STATE_PLACING_PIECE:
        case STATE_SENDING_REMATCH:
            send_error_event(
                "Cannot execute command '%s' in current state (%s).", command,
                state_to_string(current_state));
            break;
        default:
            send_log_event(LOG_WARN, "Ignoring command '%s' in state %s.",
                           command, state_to_string(current_state));
            break;
    }
}

static void cleanup() {
    send_log_event(LOG_INFO, "Starting cleanup...");
    ClientState state = get_client_state();
    if (state != STATE_QUITTING && state != STATE_REMOTE_CLOSED &&
        state != STATE_DISCONNECTED) {
        set_client_state(STATE_QUITTING);
        send_state_change_event();
    }

    close_connection();

    pthread_t tid = get_recv_thread_id();
    if (tid != 0) {
        if (pthread_join(tid, NULL) != 0) {
            send_log_event(LOG_ERROR, "Failed to join receiver thread");
        } else {
            send_log_event(LOG_INFO, "Receiver thread joined.");
        }
        set_recv_thread_id(0);
    }

    // Mutexの破棄は必要なら行う (プロセス終了時に自動解放されることが多い)
    // pthread_mutex_destroy(get_state_mutex());
    send_log_event(LOG_INFO, "Cleanup complete.");
}