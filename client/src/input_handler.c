#include "input_handler.h"

#include "connection.h"  // 切断処理を呼ぶ可能性
#include "ui.h"          // プロンプト表示のため

// --- ユーザー入力処理 ---
void handle_user_input() {
    char input_buffer[256];
    char command[64];  // コマンド用に少し余裕を持たせる
    char arg_str1[64];
    char arg_str2[64];
    int arg_int1, arg_int2;

    // keep_running フラグが 0 になるまでループ
    while (keep_running) {
        display_prompt();  // UIモジュール呼び出し

        // fgets はブロッキングする
        // サーバー切断時にここから抜け出すために工夫が必要
        // 例: 受信スレッドが pipe に書き込み、select で入力と pipe を監視する
        // 簡単な方法: サーバー切断時に keep_running = 0 にし、ユーザーに Enter
        // を促す
        if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
            // Ctrl+D などで EOF になった場合
            if (keep_running) {  // サーバーがまだ動いているなら
                printf("\nEOF detected, exiting...\n");
                keep_running = 0;  // 終了フラグ
            }
            break;  // ループを抜ける
        }

        // keep_running が fgets の後で 0 になった場合もチェック
        // (受信スレッドからの通知)
        if (!keep_running) {
            printf("\nReceived exit signal, stopping input handler.\n");
            break;
        }

        // 改行文字を削除
        input_buffer[strcspn(input_buffer, "\n")] = 0;

        // 入力が空ならループの先頭へ
        if (strlen(input_buffer) == 0) {
            continue;
        }

        // 状態を取得 (ロックしてコピー)
        pthread_mutex_lock(&state_mutex);
        ClientState state_copy = current_state;
        int room_id_copy = my_room_id;
        uint8_t my_color_copy = my_color;  // ホストかどうかの判定用にコピー
        pthread_mutex_unlock(&state_mutex);

        // 切断状態ならループを抜けるべきだが、ユーザーに Enter
        // を促す実装のためここでは抜けない
        if (state_copy == STATE_REMOTE_CLOSED) {
            printf("Exiting due to disconnection.\n");
            keep_running = 0;
            break;
        }
        if (state_copy == STATE_QUITTING || state_copy == STATE_DISCONNECTED) {
            printf("Input ignored during shutdown or disconnection.\n");
            continue;  // 次の入力待ちへ (通常はすぐループ抜けるはず)
        }

        // 簡単なコマンドパーシング (sscanf は柔軟性に欠けるがシンプル)
        int items =
            sscanf(input_buffer, "%s %s %s", command, arg_str1, arg_str2);

        if (items <= 0) continue;  // 空行やスペースのみは無視

        // "quit" コマンドは常に受け付ける
        if (strcmp(command, "quit") == 0) {
            printf("Quit command received.\n");
            keep_running = 0;  // 終了フラグ
            break;             // ループを抜ける
        }

        Message msg;
        int msg_prepared = 0;  // 送信するメッセージが準備できたか

        switch (state_copy) {
            case STATE_CONNECTED:  // ロビー
                if (strcmp(command, "create") == 0) {
                    msg.type = MSG_CREATE_ROOM_REQUEST;
                    // 部屋名を指定する場合: sscanf の結果 (arg_str1) を使う
                    if (items >= 2) {
                        strncpy(msg.data.createRoomReq.roomName, arg_str1,
                                sizeof(msg.data.createRoomReq.roomName) - 1);
                        msg.data.createRoomReq
                            .roomName[sizeof(msg.data.createRoomReq.roomName) -
                                      1] = '\0';
                    } else {
                        snprintf(msg.data.createRoomReq.roomName,
                                 sizeof(msg.data.createRoomReq.roomName),
                                 "Room_%ld", time(NULL) % 100);  // デフォルト名
                    }
                    msg_prepared = 1;
                    printf("Sending create room request (Name: %s)...\n",
                           msg.data.createRoomReq.roomName);
                } else if (strcmp(command, "join") == 0 && items >= 2) {
                    if (sscanf(arg_str1, "%d", &arg_int1) ==
                        1) {  // 文字列を数値に変換
                        msg.type = MSG_JOIN_ROOM_REQUEST;
                        msg.data.joinRoomReq.roomId = arg_int1;
                        msg_prepared = 1;
                        printf("Sending join room request (ID: %d)...\n",
                               arg_int1);
                    } else {
                        printf(
                            "Invalid room ID format. Please enter a number.\n");
                    }
                } else if (strcmp(command, "list") == 0) {
                    // TODO: MSG_LIST_ROOMS_REQUEST の送信処理
                    printf("List rooms requested (TODO: implement)\n");
                    // msg.type = MSG_LIST_ROOMS_REQUEST;
                    // msg_prepared = 1;
                } else {
                    printf(
                        "Invalid command in Lobby. Available: create [name], "
                        "join [id], list, quit\n");
                }
                break;

            case STATE_WAITING_IN_ROOM:
                // 部屋作成者(my_color == 1) のみ start 可能
                if (my_color_copy == 1 && strcmp(command, "start") == 0) {
                    msg.type = MSG_START_GAME_REQUEST;
                    msg.data.startGameReq.roomId = room_id_copy;
                    msg_prepared = 1;
                    printf("Sending start game request...\n");
                } else {
                    printf(
                        "Invalid command while waiting. Only 'start' (by host) "
                        "or 'quit' allowed.\n");
                }
                break;

            case STATE_MY_TURN:
                // 入力が "row col" 形式かチェック (items==3
                // だと3つ目の単語まで読んでしまう)
                // 厳密には、コマンドが数字かどうかなどをチェックすべき
                if (items >= 2 &&
                    sscanf(input_buffer, "%d %d", &arg_int1, &arg_int2) == 2) {
                    int row = arg_int1;
                    int col = arg_int2;
                    // 簡単な入力値チェック
                    if (row >= 0 && row < BOARD_SIZE && col >= 0 &&
                        col < BOARD_SIZE) {
                        msg.type = MSG_PLACE_PIECE_REQUEST;
                        msg.data.placePieceReq.roomId = room_id_copy;
                        msg.data.placePieceReq.row = (uint8_t)row;
                        msg.data.placePieceReq.col = (uint8_t)col;
                        msg_prepared = 1;
                        printf("Sending move (%d, %d)...\n", row, col);
                        // 送信後、サーバーからの応答を待つので、ここでは状態を変えない
                        // （受信スレッドが YOUR_TURN や UPDATE_BOARD
                        // を処理する）
                        // ただし、ユーザー体験のために仮に相手ターン表示にしておく
                        pthread_mutex_lock(&state_mutex);
                        current_state = STATE_OPPONENT_TURN;
                        pthread_mutex_unlock(&state_mutex);
                    } else {
                        printf(
                            "Invalid coordinates. Row and Col must be between "
                            "0 and %d.\n",
                            BOARD_SIZE - 1);
                    }
                } else {
                    printf(
                        "Invalid input format. Use 'row col' (e.g., '3 4').\n");
                }
                break;

            case STATE_OPPONENT_TURN:
                printf("It's not your turn. Waiting for opponent...\n");
                break;

            case STATE_GAME_OVER:
                if (strcmp(command, "rematch") == 0 && items >= 2) {
                    int agree = -1;
                    if (strcmp(arg_str1, "yes") == 0)
                        agree = 1;
                    else if (strcmp(arg_str1, "no") == 0)
                        agree = 0;

                    if (agree != -1) {
                        msg.type = MSG_REMATCH_REQUEST;
                        msg.data.rematchReq.roomId = room_id_copy;
                        msg.data.rematchReq.agree = (uint8_t)agree;
                        msg_prepared = 1;
                        printf("Sending rematch response (%s)...\n",
                               agree ? "Yes" : "No");
                        // サーバーからの結果通知(REMATCH_RESULT)を待つ
                    } else {
                        printf(
                            "Invalid rematch command. Use 'rematch yes' or "
                            "'rematch no'.\n");
                    }
                } else {
                    printf(
                        "Invalid command in Game Over state. Available: "
                        "rematch yes/no, quit\n");
                }
                break;

            default:  // CONNECTING など
                printf("Cannot process commands in the current state (%d).\n",
                       state_copy);
                break;
        }  // end switch(state_copy)

        // メッセージ送信処理
        if (msg_prepared) {
            if (sockfd != -1) {
                if (sendMessage(sockfd, &msg) <= 0) {
                    // 送信失敗
                    fprintf(stderr,
                            "Failed to send message to server. Connection "
                            "might be lost.\n");
                    // 接続切れとみなして終了処理へ
                    keep_running = 0;
                    pthread_mutex_lock(&state_mutex);
                    current_state = STATE_REMOTE_CLOSED;  // 状態更新
                    pthread_mutex_unlock(&state_mutex);
                    break;  // ループを抜ける
                }
                // 送信成功後は特に何もしない（サーバーからの応答を受信スレッドが待つ）
            } else {
                fprintf(stderr, "Cannot send message: Not connected.\n");
                keep_running = 0;
                pthread_mutex_lock(&state_mutex);
                current_state = STATE_REMOTE_CLOSED;  // 状態更新
                pthread_mutex_unlock(&state_mutex);
                break;  // ループを抜ける
            }
        }

    }  // end while(keep_running)

    printf("Input handler loop finished.\n");
    // ループを抜けたら、メインスレッドに終了を伝える必要がある
    // keep_running = 0; // 既に0になっているはずだが念のため
}