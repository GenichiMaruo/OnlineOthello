#include "receiver.h"

#include "json_output.h"
#include "network.h"
#include "state.h"

// --- サーバーからのメッセージ受信スレッド (変更なし) ---
void* receive_handler(void* arg) {
    // ... (内容は変更なし) ...
    Message msg;
    int read_size;
    int current_sockfd = get_sockfd();  // スレッド開始時の sockfd を取得

    // スレッド開始時に sockfd が無効なら何もせず終了
    if (current_sockfd < 0) {
        send_error_event("Receiver thread started with invalid sockfd.");
        return NULL;
    }
    send_log_event(LOG_INFO, "Receiver thread started.");

    while (1) {
        // 現在の sockfd を再確認 (接続が切断されている可能性)
        current_sockfd = get_sockfd();
        if (current_sockfd < 0) {
            send_log_event(LOG_INFO, "Receiver thread detected disconnection.");
            break;  // sockfd が無効になったらループ終了
        }

        read_size = receiveMessage(current_sockfd, &msg);

        if (read_size <= 0) {
            // 接続が切れた場合の処理
            pthread_mutex_lock(get_state_mutex());
            ClientState current = get_client_state_unsafe();  // mutexロック済み
            if (current != STATE_QUITTING && current != STATE_DISCONNECTED &&
                current != STATE_REMOTE_CLOSED) {
                // 自分で切断開始した場合や既に切断状態でない場合のみ更新
                send_log_event_unsafe(LOG_INFO,
                                      "Connection closed by server or error.");
                set_client_state_unsafe(STATE_REMOTE_CLOSED);
                send_state_change_event_unsafe();  // JSON出力 (ロック中)
            }
            pthread_mutex_unlock(get_state_mutex());

            // 接続を閉じる (既に閉じているかもしれないが念のため)
            // close_connection(); // ここで呼ぶとデッドロックの可能性?
            // cleanupに任せる

            break;  // ループを抜けてスレッド終了
        }

        // 受信したメッセージを処理
        process_server_message(&msg);

        // UI更新トリガーは process_server_message 内で行う
    }

    send_log_event(LOG_INFO, "Receiver thread finished.");
    return NULL;
}

// --- サーバーメッセージ処理 (unused variable 警告修正) ---
void process_server_message(const Message* msg) {
    pthread_mutex_lock(get_state_mutex());

    ClientState current = get_client_state_unsafe();
    int current_room_id = get_my_room_id_unsafe();
    uint8_t current_my_color = get_my_color_unsafe();  // <<< この変数を後で使用

    // send_log_event_unsafe(LOG_DEBUG, "Processing server msg type %d",
    // msg->type);

    switch (msg->type) {
        // ...(他の case は変更なし)...
        case MSG_CREATE_ROOM_RESPONSE:
            if (current == STATE_CREATING_ROOM) {
                if (msg->data.createRoomResp.success) {
                    set_my_room_id_unsafe(msg->data.createRoomResp.roomId);
                    set_client_state_unsafe(STATE_WAITING_IN_ROOM);
                    set_my_color_unsafe(1);  // 部屋作成者は黒 (サーバー仕様)
                    send_server_message_event_unsafe(
                        "Room created (ID: %d). Waiting for opponent...",
                        get_my_room_id_unsafe());      // JSON出力
                    send_state_change_event_unsafe();  // 状態変化をJSON出力
                } else {
                    send_server_message_event_unsafe(
                        "Failed to create room: %s",
                        msg->data.createRoomResp.message);
                    set_client_state_unsafe(STATE_CONNECTED);  // ロビーに戻る
                    send_state_change_event_unsafe();
                }
            } else {
                send_log_event_unsafe(
                    LOG_WARN,
                    "Received CREATE_ROOM_RESPONSE in unexpected state: %s",
                    state_to_string(current));
            }
            break;
        case MSG_JOIN_ROOM_RESPONSE:
            if (current == STATE_JOINING_ROOM) {
                if (msg->data.joinRoomResp.success) {
                    set_my_room_id_unsafe(msg->data.joinRoomResp.roomId);
                    set_client_state_unsafe(
                        STATE_WAITING_IN_ROOM);  // 相手(ホスト)の開始待ち
                    set_my_color_unsafe(2);      // 参加者は白 (サーバー仕様)
                    send_server_message_event_unsafe(
                        "Joined room %d. Waiting for host...",
                        get_my_room_id_unsafe());
                    send_state_change_event_unsafe();
                } else {
                    send_server_message_event_unsafe(
                        "Failed to join room: %s",
                        msg->data.joinRoomResp.message);
                    set_client_state_unsafe(STATE_CONNECTED);  // ロビーに戻る
                    send_state_change_event_unsafe();
                }
            } else {
                send_log_event_unsafe(
                    LOG_WARN,
                    "Received JOIN_ROOM_RESPONSE in unexpected state: %s",
                    state_to_string(current));
            }
            break;
        case MSG_PLAYER_JOINED_NOTICE:
            if (current == STATE_WAITING_IN_ROOM &&
                msg->data.playerJoinedNotice.roomId == current_room_id) {
                send_server_message_event_unsafe("Opponent joined room %d.",
                                                 current_room_id);
                // 状態は変わらないが、UI更新のため stateChange
                // を送っても良いかも send_state_change_event_unsafe();
            }
            break;
        case MSG_GAME_START_NOTICE:
            if (msg->data.gameStartNotice.roomId == current_room_id) {
                // 自分がホストで開始待ちだったか、参加者で開始待ちだった場合に状態遷移
                if (current == STATE_WAITING_IN_ROOM ||
                    current == STATE_STARTING_GAME) {
                    set_my_color_unsafe(msg->data.gameStartNotice.yourColor);
                    set_game_board_unsafe(
                        msg->data.gameStartNotice.board);  // state.cで実装

                    send_server_message_event_unsafe(
                        "Game Start! You are %s.", (get_my_color_unsafe() == 1)
                                                       ? "Black (X)"
                                                       : "White (O)");
                    send_board_update_event_unsafe();  // 盤面をJSON出力

                    if (get_my_color_unsafe() == 1) {  // 自分が黒番(先手)の場合
                        // サーバー仕様として、先手は必ず最初に YOUR_TURN
                        // が来るはず set_client_state_unsafe(STATE_MY_TURN); //
                        // YOUR_TURN を待つ
                        send_log_event_unsafe(
                            LOG_INFO, "Waiting for YOUR_TURN notice...");
                    } else {  // 自分が白番(後手)の場合
                        set_client_state_unsafe(STATE_OPPONENT_TURN);
                    }
                    send_state_change_event_unsafe();  // 状態変化をJSON出力
                } else {
                    send_log_event_unsafe(
                        LOG_WARN,
                        "Received GAME_START_NOTICE in unexpected state: %s",
                        state_to_string(current));
                }
            }
            break;
        case MSG_UPDATE_BOARD_NOTICE:
            if (msg->data.updateBoardNotice.roomId == current_room_id) {
                // 盤面は常に更新
                set_game_board_unsafe(msg->data.updateBoardNotice.board);
                send_server_message_event_unsafe(
                    "Board updated by player %d at (%d, %d).",
                    msg->data.updateBoardNotice.playerColor,
                    msg->data.updateBoardNotice.row,
                    msg->data.updateBoardNotice.col);
                send_board_update_event_unsafe();  // 盤面をJSON出力
                // 状態遷移は YOUR_TURN_NOTICE で行う
            }
            break;
        case MSG_YOUR_TURN_NOTICE:
            if (msg->data.yourTurnNotice.roomId == current_room_id) {
                // 相手ターンだった場合、またはゲーム開始直後(先手)に自分のターンになる
                if (current == STATE_OPPONENT_TURN ||
                    current == STATE_WAITING_IN_ROOM ||
                    current == STATE_STARTING_GAME ||
                    current == STATE_PLACING_PIECE) {
                    set_client_state_unsafe(STATE_MY_TURN);
                    send_your_turn_event_unsafe();  // 自分のターン通知をJSON出力
                    send_state_change_event_unsafe();  // 状態変化をJSON出力
                } else {
                    send_log_event_unsafe(
                        LOG_WARN,
                        "Received YOUR_TURN_NOTICE in unexpected state: %s",
                        state_to_string(current));
                }
            }
            break;
        case MSG_INVALID_MOVE_NOTICE:
            if (msg->data.invalidMoveNotice.roomId == current_room_id) {
                // 自分の手を送信中(PLACING_PIECE)だった場合に発生するはず
                if (current == STATE_PLACING_PIECE) {
                    send_server_message_event_unsafe(
                        "Server: Invalid move! %s",
                        msg->data.invalidMoveNotice.message);
                    set_client_state_unsafe(
                        STATE_MY_TURN);  // 再度自分のターンに戻る
                    send_state_change_event_unsafe();
                } else {
                    send_log_event_unsafe(
                        LOG_WARN,
                        "Received INVALID_MOVE_NOTICE in unexpected state: %s",
                        state_to_string(current));
                }
            }
            break;
        case MSG_GAME_OVER_NOTICE:
            if (msg->data.gameOverNotice.roomId == current_room_id) {
                if (current == STATE_MY_TURN ||
                    current == STATE_OPPONENT_TURN ||
                    current == STATE_PLACING_PIECE) {
                    set_client_state_unsafe(STATE_GAME_OVER);

                    // result_message バッファのサイズを増やす (例: 256)
                    // MAX_MESSAGE_LEN (128) + 固定文字列の最大長 + 終端文字
                    // を考慮
                    char result_message[256];  // <<< サイズを増やす
                    uint8_t winner = msg->data.gameOverNotice.winner;
                    const char* base_message = msg->data.gameOverNotice.message;

                    // 安全のため、ベースメッセージが長すぎる場合は切り詰める
                    char safe_base_message[MAX_MESSAGE_LEN];
                    strncpy(safe_base_message, base_message,
                            MAX_MESSAGE_LEN - 1);
                    safe_base_message[MAX_MESSAGE_LEN - 1] = '\0';

                    if (winner == current_my_color) {
                        snprintf(result_message, sizeof(result_message),
                                 "%s You Win!", safe_base_message);
                    } else if (winner == 0) {  // Draw
                        snprintf(result_message, sizeof(result_message),
                                 "%s It's a Draw!", safe_base_message);
                    } else if (winner == 3) {  // Opponent disconnect / error
                        snprintf(result_message, sizeof(result_message),
                                 "%s Opponent disconnected or error.",
                                 safe_base_message);
                    } else {  // Lost
                        snprintf(result_message, sizeof(result_message),
                                 "%s You Lose.", safe_base_message);
                    }
                    // snprintf
                    // は常にヌル終端するので、バッファオーバーフローは防がれるが、切り詰めは起こりうる
                    result_message[sizeof(result_message) - 1] =
                        '\0';  // 念のため終端保証

                    send_game_over_event_unsafe(winner, result_message);
                    send_state_change_event_unsafe();
                } else {
                    send_log_event_unsafe(
                        LOG_WARN,
                        "Received GAME_OVER_NOTICE in unexpected state: %s",
                        state_to_string(current));
                }
            }
            break;
        case MSG_REMATCH_OFFER_NOTICE:
            if (msg->data.rematchOfferNotice.roomId == current_room_id) {
                if (current == STATE_GAME_OVER) {
                    send_rematch_offer_event_unsafe();  // 再戦要求通知をJSON出力
                    // 状態は GAME_OVER のまま
                } else {
                    send_log_event_unsafe(
                        LOG_WARN,
                        "Received REMATCH_OFFER_NOTICE in unexpected state: %s",
                        state_to_string(current));
                }
            }
            break;
        case MSG_REMATCH_RESULT_NOTICE:
            if (msg->data.rematchResultNotice.roomId == current_room_id) {
                // 再戦要求を送信中(SENDING_REMATCH)またはゲームオーバー状態
                if (current == STATE_GAME_OVER ||
                    current == STATE_SENDING_REMATCH) {
                    uint8_t result = msg->data.rematchResultNotice.result;
                    send_rematch_result_event_unsafe(
                        result);        // 再戦結果をJSON出力
                    if (result == 1) {  // 成立した場合
                        // GAME_START を待つ
                        set_client_state_unsafe(
                            STATE_WAITING_IN_ROOM);  // 仮に待機状態へ
                    } else {                         // 不成立の場合
                        // ROOM_CLOSEDを待つか、ここでロビーに戻っても良い
                        set_client_state_unsafe(
                            STATE_GAME_OVER);  // ゲームオーバー状態に戻る
                    }
                    send_state_change_event_unsafe();  // 状態変化を送信
                } else {
                    send_log_event_unsafe(LOG_WARN,
                                          "Received REMATCH_RESULT_NOTICE in "
                                          "unexpected state: %s",
                                          state_to_string(current));
                }
            }
            break;

        case MSG_ROOM_CLOSED_NOTICE:
            if (msg->data.roomClosedNotice.roomId == current_room_id) {
                send_server_message_event_unsafe(
                    "Room %d closed: %s", msg->data.roomClosedNotice.roomId,
                    msg->data.roomClosedNotice.reason);
                reset_room_info_unsafe();
                if (current != STATE_QUITTING &&
                    current != STATE_REMOTE_CLOSED) {
                    set_client_state_unsafe(STATE_CONNECTED);
                    send_state_change_event_unsafe();
                }
            }
            break;
        case MSG_ERROR_NOTICE:
            send_error_event_unsafe("Server Error: %s",
                                    msg->data.errorNotice.message);
            // 状態遷移はエラー内容による
            break;

        default:
            send_log_event_unsafe(
                LOG_WARN, "Received unhandled message type: %d", msg->type);
            break;
    }

    pthread_mutex_unlock(get_state_mutex());
}