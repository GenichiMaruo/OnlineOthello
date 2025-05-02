#include "receiver.h"

#include "connection.h"  // disconnect_from_serverを呼ぶ可能性
#include "ui.h"          // プロンプト再表示や盤面表示のため

// --- サーバーからのメッセージ受信スレッド ---
void* receive_handler(void* arg) {
    Message msg;
    int read_size;

    printf("Receiver thread started.\n");

    // keep_running フラグが 0 になるまでループ
    while (keep_running) {
        // sockfd が有効かチェック (メインスレッドで切断される可能性)
        if (sockfd < 0) {
            printf("Receiver: Socket closed, exiting loop.\n");
            break;
        }

        // receiveMessage はブロッキングする可能性がある
        // タイムアウト付きの受信や select/poll
        // を使うと、より応答性の高い停止が可能
        read_size = receiveMessage(sockfd, &msg);

        // keep_running がループ中に 0 になった場合も抜ける
        if (!keep_running) {
            printf("Receiver: keep_running is false, exiting loop.\n");
            break;
        }

        if (read_size <= 0) {
            if (read_size == 0) {
                printf(
                    "\n[Receiver] Server closed the connection gracefully.\n");
            } else {  // read_size < 0
                // ECONNRESET や ETIMEDOUT などのエラーをチェック
                perror("\n[Receiver] Receive error");
            }

            pthread_mutex_lock(&state_mutex);
            // 自分で切断処理中でなければ、リモートクローズとする
            if (current_state != STATE_QUITTING &&
                current_state != STATE_DISCONNECTED) {
                current_state = STATE_REMOTE_CLOSED;
            }
            pthread_mutex_unlock(&state_mutex);

            keep_running = 0;  // 他のスレッドにも終了を通知
            printf("[Receiver] Signaling main thread to exit (press Enter).\n");
            // TODO: メインスレッドの入力待ちを解除するより良い方法 (pipe,
            // signalなど)
            break;  // ループを抜けてスレッド終了
        }

        // 受信したメッセージを処理
        process_server_message(&msg);

        // プロンプト再表示 (非同期表示更新) - UIモジュールに依頼
        // process_server_message内で表示更新しても良いが、一貫性のためここで呼ぶ例
        display_prompt();
        fflush(stdout);  // プロンプトを強制表示
    }

    printf("Receiver thread finished.\n");
    return NULL;
}

// --- サーバーメッセージ処理 ---
void process_server_message(const Message* msg) {
    pthread_mutex_lock(&state_mutex);  // 状態変更前にロック

    // メッセージ処理中はユーザー入力による状態変更はないはずだが念のためロック
    printf("\n[Server Message Received] Type: %d\n", msg->type);  // デバッグ用

    // 現在の状態を確認（表示用など）
    ClientState state_before_processing = current_state;
    // 終了処理中は新しいメッセージを基本的に無視
    if (state_before_processing == STATE_QUITTING ||
        state_before_processing == STATE_REMOTE_CLOSED ||
        state_before_processing == STATE_DISCONNECTED) {
        printf(
            "[Server Message Ignored] Client is shutting down or "
            "disconnected.\n");
        pthread_mutex_unlock(&state_mutex);
        return;
    }

    switch (msg->type) {
        case MSG_CREATE_ROOM_RESPONSE:
            if (current_state == STATE_CONNECTED) {  // ロビーにいるはず
                if (msg->data.createRoomResp.success) {
                    my_room_id = msg->data.createRoomResp.roomId;
                    current_state = STATE_WAITING_IN_ROOM;
                    my_color = 1;  // 部屋作成者は黒 (サーバー仕様)
                    printf(
                        "Room created successfully! Room ID: %d. Waiting for "
                        "opponent...\n",
                        my_room_id);
                } else {
                    printf("Failed to create room: %s\n",
                           msg->data.createRoomResp.message);
                    // current_state は STATE_CONNECTED のまま
                }
            } else {
                fprintf(stderr,
                        "Warning: Received CREATE_ROOM_RESPONSE in unexpected "
                        "state %d\n",
                        current_state);
            }
            break;

        case MSG_JOIN_ROOM_RESPONSE:                 // 参加応答処理を追加
            if (current_state == STATE_CONNECTED) {  // ロビーにいるはず
                if (msg->data.joinRoomResp.success) {
                    my_room_id = msg->data.joinRoomResp.roomId;
                    current_state = STATE_WAITING_IN_ROOM;
                    my_color = 2;  // 部屋参加者は白 (サーバー仕様)
                    printf(
                        "Successfully joined room %d. Waiting for host to "
                        "start...\n",
                        my_room_id);
                    // 盤面情報は GAME_START_NOTICE で来るはず
                } else {
                    printf("Failed to join room %d: %s\n",
                           msg->data.joinRoomResp.roomId,
                           msg->data.joinRoomResp.message);
                    // current_state は STATE_CONNECTED のまま
                }
            } else {
                fprintf(stderr,
                        "Warning: Received JOIN_ROOM_RESPONSE in unexpected "
                        "state %d\n",
                        current_state);
            }
            break;

        case MSG_PLAYER_JOINED_NOTICE:
            if (current_state == STATE_WAITING_IN_ROOM &&
                msg->data.playerJoinedNotice.roomId == my_room_id) {
                printf(
                    "Opponent joined the room! You (%s) can now start the "
                    "game.\n",
                    (my_color == 1 ? "Host" : "Player 2"));
                // 状態はまだ WAITING_IN_ROOM (ホストの start 待ち)
            }
            break;

        case MSG_GAME_START_NOTICE:
            // 部屋にいて、ゲーム開始を待っている状態のはず
            if ((current_state == STATE_WAITING_IN_ROOM ||
                 current_state == STATE_GAME_OVER /* 再戦時 */) &&
                msg->data.gameStartNotice.roomId == my_room_id) {
                // 自分の色と初期盤面を設定
                my_color = msg->data.gameStartNotice.yourColor;
                memcpy(game_board, msg->data.gameStartNotice.board,
                       sizeof(game_board));
                printf("Game Start! You are %s.\n",
                       (my_color == 1) ? "Black (X)" : "White (O)");
                display_board();  // UIモジュール呼び出し

                // サーバーは初手プレイヤーに YOUR_TURN_NOTICE
                // を送るはずなので、
                // ここでターン状態を決めずに、YOUR_TURN_NOTICE
                // を待つ方が確実かもしれない。 もし YOUR_TURN_NOTICE
                // が来ない仕様ならここで設定。 今回は YOUR_TURN_NOTICE
                // が来ると仮定し、一旦相手ターンにしておく。
                current_state =
                    STATE_OPPONENT_TURN;  // 一旦相手ターンに（YOUR_TURNが来れば変わる）
                printf("Waiting for turn notification...\n");
            } else {
                fprintf(stderr,
                        "Warning: Received GAME_START_NOTICE in unexpected "
                        "state %d or for wrong room %d (my room %d)\n",
                        current_state, msg->data.gameStartNotice.roomId,
                        my_room_id);
            }
            break;

        case MSG_UPDATE_BOARD_NOTICE:
            // 相手のターン中、または自分のターン中に相手の手の情報が来た場合
            if ((current_state == STATE_OPPONENT_TURN ||
                 current_state == STATE_MY_TURN) &&
                msg->data.updateBoardNotice.roomId == my_room_id) {
                // 送信者が相手の場合のみ表示（自分の手に対する更新通知は内部処理のみ）
                if (msg->data.updateBoardNotice.playerColor != my_color) {
                    printf("Opponent (Player %d) placed a piece at (%d, %d).\n",
                           msg->data.updateBoardNotice.playerColor,
                           msg->data.updateBoardNotice.row,
                           msg->data.updateBoardNotice.col);
                }
                // 盤面を更新
                memcpy(game_board, msg->data.updateBoardNotice.board,
                       sizeof(game_board));
                display_board();  // UIモジュール呼び出し

                // ターン状態は YOUR_TURN_NOTICE で変更されるのを待つ
            } else {
                fprintf(stderr,
                        "Warning: Received UPDATE_BOARD_NOTICE in unexpected "
                        "state %d or for wrong room %d (my room %d)\n",
                        current_state, msg->data.updateBoardNotice.roomId,
                        my_room_id);
            }
            break;

        case MSG_YOUR_TURN_NOTICE:
            // 相手のターンだった場合、自分のターンに変更
            if (current_state == STATE_OPPONENT_TURN &&
                msg->data.yourTurnNotice.roomId == my_room_id) {
                current_state = STATE_MY_TURN;
                printf("It's your turn.\n");
            } else {
                // 既に自分のターンだった場合などは無視しても良い
                fprintf(stderr,
                        "Warning: Received YOUR_TURN_NOTICE in unexpected "
                        "state %d or for wrong room %d (my room %d)\n",
                        current_state, msg->data.yourTurnNotice.roomId,
                        my_room_id);
            }
            break;

        case MSG_INVALID_MOVE_NOTICE:
            // 自分のターン中に無効手通知が来た場合
            if (current_state == STATE_MY_TURN &&
                msg->data.invalidMoveNotice.roomId == my_room_id) {
                printf("Server: Invalid move! %s\n",
                       msg->data.invalidMoveNotice.message);
                // 状態は MY_TURN のまま、再入力を促す
            }
            break;

        case MSG_GAME_OVER_NOTICE:
            // ゲーム中のターンのはず
            if ((current_state == STATE_MY_TURN ||
                 current_state == STATE_OPPONENT_TURN) &&
                msg->data.gameOverNotice.roomId == my_room_id) {
                current_state = STATE_GAME_OVER;
                // 盤面最終状態を表示しても良い
                display_board();
                printf("Game Over! %s\n", msg->data.gameOverNotice.message);
                uint8_t winner = msg->data.gameOverNotice.winner;
                if (winner == my_color) {
                    printf("You Win!\n");
                } else if (winner == 0) {  // 引き分け
                    printf("It's a Draw!\n");
                } else if (winner == 3) {  // 相手切断など
                    printf("Opponent disconnected or game aborted.\n");
                } else {  // 相手の色が勝利
                    printf("You Lose.\n");
                }
                // 再戦のプロンプト表示は REMATCH_OFFER_NOTICE を待つ
            } else {
                fprintf(stderr,
                        "Warning: Received GAME_OVER_NOTICE in unexpected "
                        "state %d or for wrong room %d (my room %d)\n",
                        current_state, msg->data.gameOverNotice.roomId,
                        my_room_id);
            }
            break;

        case MSG_REMATCH_OFFER_NOTICE:
            // ゲーム終了状態のはず
            if (current_state == STATE_GAME_OVER &&
                msg->data.rematchOfferNotice.roomId == my_room_id) {
                printf(
                    "Do you want a rematch? (Enter 'rematch yes' or 'rematch "
                    "no')\n");
            }
            break;

        case MSG_REMATCH_RESULT_NOTICE:
            // ゲーム終了状態のはず
            if (current_state == STATE_GAME_OVER &&
                msg->data.rematchResultNotice.roomId == my_room_id) {
                uint8_t result = msg->data.rematchResultNotice.result;
                if (result == 1) {  // Agreed
                    printf("Rematch agreed! Starting new game soon...\n");
                    // 状態は GAME_START_NOTICE で更新されるのを待つ
                } else if (result == 0) {  // Disagreed
                    printf(
                        "Rematch declined by opponent. Room will be closed.\n");
                    // 状態は ROOM_CLOSED_NOTICE で更新されるのを待つ
                    // または、ここでロビーに戻るように状態変更しても良い
                    // current_state = STATE_CONNECTED;
                    // my_room_id = -1; my_color = 0; ...
                } else {  // result == 2 (Timeout)
                    printf("Rematch timed out. Room will be closed.\n");
                    // 状態は ROOM_CLOSED_NOTICE で更新されるのを待つ
                }
            }
            break;

        case MSG_ROOM_CLOSED_NOTICE:
            printf("Notification: Room %d closed: %s\n",
                   msg->data.roomClosedNotice.roomId,
                   msg->data.roomClosedNotice.reason);
            // 自分がその部屋にいた場合、状態を更新してロビーに戻る
            if (msg->data.roomClosedNotice.roomId == my_room_id) {
                my_room_id = -1;
                my_color = 0;
                memset(game_board, 0, sizeof(game_board));
                // 切断状態でなければロビーへ
                if (current_state != STATE_REMOTE_CLOSED &&
                    current_state != STATE_QUITTING) {
                    current_state = STATE_CONNECTED;
                    printf("Returned to lobby.\n");
                }
            }
            break;

        case MSG_ERROR_NOTICE:
            // サーバーからの汎用エラー
            printf("[Server Error] %s\n", msg->data.errorNotice.message);
            // エラー内容によっては状態を変更する必要があるかもしれない
            break;

            // 他のメッセージタイプに対する処理を追加
            // case MSG_LIST_ROOMS_RESPONSE:
            // handle_list_rooms_response(&msg->data.listRoomsResp);
            // break;
            // case MSG_PONG:
            // handle_pong();
            // break;

        default:
            printf("Received unhandled message type: %d\n", msg->type);
            break;
    }

    pthread_mutex_unlock(&state_mutex);  // ロック解除
}