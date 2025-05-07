#include "client_handler.h"

#include <time.h>  // time() 関数を使うために必要

#include "client_management.h"
#include "game_logic.h"  // ゲームロジック関数を使用
#include "room_management.h"

// --- メッセージハンドラ ---

void handle_create_room_request(int client_sock, const Message* msg) {
    printf("Received CREATE_ROOM request from client sockfd %d\n", client_sock);

    // 既に部屋に入っている場合は作成できない
    pthread_mutex_lock(&clients_mutex);
    int client_idx = find_client_index(client_sock);
    int current_room_id = -1;
    if (client_idx != -1) {
        current_room_id = clients[client_idx].roomId;
    }
    pthread_mutex_unlock(&clients_mutex);

    if (current_room_id != -1) {
        fprintf(
            stderr,
            "Client sockfd %d tried to create room while already in room %d.\n",
            client_sock, current_room_id);
        Message response;
        response.type = MSG_CREATE_ROOM_RESPONSE;
        response.data.createRoomResp.success = 0;
        response.data.createRoomResp.roomId = -1;
        snprintf(response.data.createRoomResp.message,
                 sizeof(response.data.createRoomResp.message),
                 "You are already in a room (%d).", current_room_id);
        sendMessage(client_sock, &response);
        return;
    }

    int new_room_id =
        create_new_room(client_sock, msg->data.createRoomReq.roomName);

    Message response;
    response.type = MSG_CREATE_ROOM_RESPONSE;
    if (new_room_id >= 0) {
        response.data.createRoomResp.success = 1;
        response.data.createRoomResp.roomId = new_room_id;
        snprintf(response.data.createRoomResp.message,
                 sizeof(response.data.createRoomResp.message),
                 "Room '%s' created successfully (ID: %d).",
                 msg->data.createRoomReq.roomName, new_room_id);
    } else {
        response.data.createRoomResp.success = 0;
        response.data.createRoomResp.roomId = -1;
        snprintf(response.data.createRoomResp.message,
                 sizeof(response.data.createRoomResp.message),
                 "Failed to create room (server full or error?).");
    }
    sendMessage(client_sock, &response);
}

// TODO: 部屋参加リクエスト処理
void handle_join_room_request(int client_sock, const Message* msg) {
    printf("Received JOIN_ROOM request from client sockfd %d for room %d\n",
           client_sock,
           msg->data.joinRoomReq
               .roomId);  // joinRoomReq は protocol.h で定義が必要

    // 既に部屋に入っている場合は参加できない
    pthread_mutex_lock(&clients_mutex);
    int client_idx = find_client_index(client_sock);
    int current_room_id = -1;
    if (client_idx != -1) {
        current_room_id = clients[client_idx].roomId;
    }
    pthread_mutex_unlock(&clients_mutex);

    if (current_room_id != -1) {
        fprintf(
            stderr,
            "Client sockfd %d tried to join room while already in room %d.\n",
            client_sock, current_room_id);
        // エラー応答
        Message response;
        response.type = MSG_JOIN_ROOM_RESPONSE;
        response.data.joinRoomResp.success =
            0;  // joinRoomResp が使えるようになる
        response.data.joinRoomResp.roomId = msg->data.joinRoomReq.roomId;
        snprintf(response.data.joinRoomResp.message,
                 sizeof(response.data.joinRoomResp.message),
                 "You are already in a room (%d).", current_room_id);
        sendMessage(client_sock, &response);
        return;
    }

    int targetRoomId = msg->data.joinRoomReq.roomId;
    int result = join_room(client_sock, targetRoomId);

    Message response;
    response.type = MSG_JOIN_ROOM_RESPONSE;
    response.data.joinRoomResp.roomId = targetRoomId;

    if (result == targetRoomId) {  // 成功
        response.data.joinRoomResp.success = 1;
        snprintf(response.data.joinRoomResp.message,
                 sizeof(response.data.joinRoomResp.message),
                 "Successfully joined room %d.", targetRoomId);
    } else {
        response.data.joinRoomResp.success = 0;
        if (result == -1) {
            snprintf(response.data.joinRoomResp.message,
                     sizeof(response.data.joinRoomResp.message),
                     "Room %d not found.", targetRoomId);
        } else if (result == -2) {
            snprintf(response.data.joinRoomResp.message,
                     sizeof(response.data.joinRoomResp.message),
                     "Room %d is full or already playing.", targetRoomId);
        } else {
            snprintf(response.data.joinRoomResp.message,
                     sizeof(response.data.joinRoomResp.message),
                     "Failed to join room %d (error code %d).", targetRoomId,
                     result);
        }
    }
    sendMessage(client_sock, &response);

    // 参加成功した場合、参加者自身にも PlayerJoinedNotice を送る (任意)
    // または、参加成功応答に相手の情報を載せるなど
    if (result == targetRoomId) {
        // 必要なら追加処理
    }
}

void handle_start_game_request(int client_sock, const Message* msg) {
    int roomId = msg->data.startGameReq.roomId;
    printf("Received START_GAME request for room %d from client sockfd %d\n",
           roomId, client_sock);

    pthread_mutex_lock(&rooms_mutex);
    int room_idx = find_room_index(roomId);
    if (room_idx == -1) {
        pthread_mutex_unlock(&rooms_mutex);
        fprintf(stderr, "Error: Room %d not found for start game request.\n",
                roomId);
        Message err_msg;
        err_msg.type = MSG_ERROR_NOTICE;
        snprintf(err_msg.data.errorNotice.message,
                 sizeof(err_msg.data.errorNotice.message), "Room %d not found.",
                 roomId);
        sendMessage(client_sock, &err_msg);
        return;
    }

    pthread_mutex_lock(&rooms[room_idx].room_mutex);
    pthread_mutex_unlock(&rooms_mutex);  // リストロック解除

    Room* room = &rooms[room_idx];  // ポインタ取得

    // ゲーム開始条件のチェック
    // 1. リクエスト者がプレイヤー1であること
    // 2. 部屋にプレイヤー2が存在すること
    // 3. 部屋の状態が待機中(ROOM_WAITING)であること
    if (room->player1_sock != client_sock) {
        pthread_mutex_unlock(&room->room_mutex);
        fprintf(stderr, "Error: Client sockfd %d is not player 1 in room %d.\n",
                client_sock, roomId);
        Message err_msg;
        err_msg.type = MSG_ERROR_NOTICE;
        snprintf(err_msg.data.errorNotice.message,
                 sizeof(err_msg.data.errorNotice.message),
                 "Only the room creator (Player 1) can start the game.");
        sendMessage(client_sock, &err_msg);
        return;
    }
    if (room->player2_sock == -1) {
        pthread_mutex_unlock(&room->room_mutex);
        fprintf(stderr, "Error: Player 2 has not joined room %d yet.\n",
                roomId);
        Message err_msg;
        err_msg.type = MSG_ERROR_NOTICE;
        snprintf(err_msg.data.errorNotice.message,
                 sizeof(err_msg.data.errorNotice.message),
                 "Waiting for opponent to join.");
        sendMessage(client_sock, &err_msg);
        return;
    }
    if (room->status != ROOM_WAITING) {
        pthread_mutex_unlock(&room->room_mutex);
        fprintf(stderr,
                "Error: Room %d is not in WAITING state (current: %d).\n",
                roomId, room->status);
        Message err_msg;
        err_msg.type = MSG_ERROR_NOTICE;
        snprintf(err_msg.data.errorNotice.message,
                 sizeof(err_msg.data.errorNotice.message),
                 "Cannot start game in current room state (%d). Game might be "
                 "ongoing or over.",
                 room->status);
        sendMessage(client_sock, &err_msg);
        return;
    }

    // ゲーム状態を初期化し、部屋の状態をPLAYINGに変更
    room->status = ROOM_PLAYING;
    initialize_game_state(&room->gameState);  // game_logic.c の関数を使用
    room->last_action_time = time(NULL);

    // 両プレイヤーにゲーム開始を通知
    Message start_notice;
    start_notice.type = MSG_GAME_START_NOTICE;
    start_notice.data.gameStartNotice.roomId = roomId;
    memcpy(start_notice.data.gameStartNotice.board, room->gameState.board,
           sizeof(room->gameState.board));

    // プレイヤー1 (黒) への通知
    start_notice.data.gameStartNotice.yourColor = 1;  // あなたは黒
    sendMessage(room->player1_sock, &start_notice);

    // プレイヤー2 (白) への通知
    start_notice.data.gameStartNotice.yourColor = 2;  // あなたは白
    sendMessage(room->player2_sock, &start_notice);

    // 最初のプレイヤー(黒番)に手番通知
    Message turn_notice;
    turn_notice.type = MSG_YOUR_TURN_NOTICE;
    turn_notice.data.yourTurnNotice.roomId = roomId;
    if (room->gameState.currentTurn == 1) {
        sendMessage(room->player1_sock, &turn_notice);
    } else {  // 通常は黒番(1)から始まるはずだが念のため
        sendMessage(room->player2_sock, &turn_notice);
    }

    printf("Game started in room %d.\n", roomId);

    pthread_mutex_unlock(&room->room_mutex);
}

void handle_place_piece_request(int client_sock, const Message* msg) {
    int roomId = msg->data.placePieceReq.roomId;
    uint8_t row = msg->data.placePieceReq.row;
    uint8_t col = msg->data.placePieceReq.col;

    printf(
        "Received PLACE_PIECE request for room %d from client sockfd %d at "
        "(%d, %d)\n",
        roomId, client_sock, row, col);

    pthread_mutex_lock(&rooms_mutex);
    int room_idx = find_room_index(roomId);
    if (room_idx == -1) {
        pthread_mutex_unlock(&rooms_mutex);
        fprintf(stderr, "Error: Room %d not found for place piece request.\n",
                roomId);
        return;
    }

    pthread_mutex_lock(&rooms[room_idx].room_mutex);
    pthread_mutex_unlock(&rooms_mutex);

    Room* room = &rooms[room_idx];

    // 1. Check if playing
    if (room->status != ROOM_PLAYING) {
        pthread_mutex_unlock(&room->room_mutex);
        fprintf(stderr, "Error: Room %d is not in playing state (%d).\n",
                roomId, room->status);
        Message err_msg;
        err_msg.type = MSG_INVALID_MOVE_NOTICE;
        err_msg.data.invalidMoveNotice.roomId = roomId;
        snprintf(err_msg.data.invalidMoveNotice.message,
                 sizeof(err_msg.data.invalidMoveNotice.message),
                 "Game is not currently playing in this room.");
        sendMessage(client_sock, &err_msg);
        return;
    }

    // 2. Check if it's sender's turn
    pthread_mutex_lock(&clients_mutex);
    int client_idx = find_client_index(client_sock);
    int playerColor = (client_idx != -1) ? clients[client_idx].playerColor : 0;
    pthread_mutex_unlock(&clients_mutex);

    if (playerColor == 0 || playerColor != room->gameState.currentTurn) {
        pthread_mutex_unlock(&room->room_mutex);
        fprintf(stderr,
                "Error: Not client sockfd %d's turn in room %d (turn=%d, "
                "clientColor=%d).\n",
                client_sock, roomId, room->gameState.currentTurn, playerColor);
        Message err_msg;
        err_msg.type = MSG_INVALID_MOVE_NOTICE;
        err_msg.data.invalidMoveNotice.roomId = roomId;
        snprintf(err_msg.data.invalidMoveNotice.message,
                 sizeof(err_msg.data.invalidMoveNotice.message),
                 "It's not your turn.");
        sendMessage(client_sock, &err_msg);
        return;
    }

    // 3. Check if move is valid
    if (!is_valid_move(&room->gameState, playerColor, row, col)) {
        pthread_mutex_unlock(&room->room_mutex);
        fprintf(
            stderr,
            "Error: Invalid move by client sockfd %d in room %d at (%d, %d).\n",
            client_sock, roomId, row, col);
        Message err_msg;
        err_msg.type = MSG_INVALID_MOVE_NOTICE;
        err_msg.data.invalidMoveNotice.roomId = roomId;
        snprintf(err_msg.data.invalidMoveNotice.message,
                 sizeof(err_msg.data.invalidMoveNotice.message),
                 "Invalid move at (%d, %d).", row, col);
        sendMessage(client_sock, &err_msg);
        return;
    }

    // 4. Update board
    update_board(&room->gameState, playerColor, row, col);
    printf("Board updated in room %d after move by player %d at (%d,%d).\n",
           roomId, playerColor, row, col);

    // 5. Broadcast board update
    Message update_msg;
    update_msg.type = MSG_UPDATE_BOARD_NOTICE;
    update_msg.data.updateBoardNotice.roomId = roomId;
    update_msg.data.updateBoardNotice.playerColor = playerColor;
    update_msg.data.updateBoardNotice.row = row;
    update_msg.data.updateBoardNotice.col = col;
    memcpy(update_msg.data.updateBoardNotice.board, room->gameState.board,
           sizeof(room->gameState.board));

    int p1_sock_temp = room->player1_sock;
    int p2_sock_temp = room->player2_sock;
    pthread_mutex_unlock(&room->room_mutex);  // Unlock before sending

    printf("Broadcasting board update to room %d.\n", roomId);
    if (p1_sock_temp != -1) sendMessage(p1_sock_temp, &update_msg);
    if (p2_sock_temp != -1) sendMessage(p2_sock_temp, &update_msg);

    pthread_mutex_lock(&room->room_mutex);  // Re-lock

    // 6. Check game over
    int winner = check_game_over(&room->gameState);
    if (winner != 0) {
        room->status = ROOM_GAMEOVER;
        room->last_action_time = time(NULL);
        room->player1_rematch_agree = 0;
        room->player2_rematch_agree = 0;

        printf("Game over in room %d. Winner code: %d\n", roomId, winner);

        Message gameover_msg;
        gameover_msg.type = MSG_GAME_OVER_NOTICE;
        gameover_msg.data.gameOverNotice.roomId = roomId;
        gameover_msg.data.gameOverNotice.winner = winner;
        if (winner == 1)
            snprintf(gameover_msg.data.gameOverNotice.message,
                     sizeof(gameover_msg.data.gameOverNotice.message),
                     "Game Over! Black wins.");
        else if (winner == 2)
            snprintf(gameover_msg.data.gameOverNotice.message,
                     sizeof(gameover_msg.data.gameOverNotice.message),
                     "Game Over! White wins.");
        else
            snprintf(gameover_msg.data.gameOverNotice.message,
                     sizeof(gameover_msg.data.gameOverNotice.message),
                     "Game Over! It's a draw.");

        Message rematch_offer_msg;
        rematch_offer_msg.type = MSG_REMATCH_OFFER_NOTICE;
        rematch_offer_msg.data.rematchOfferNotice.roomId = roomId;

        p1_sock_temp = room->player1_sock;
        p2_sock_temp = room->player2_sock;
        pthread_mutex_unlock(&room->room_mutex);  // Unlock before sending

        if (p1_sock_temp != -1) {
            sendMessage(p1_sock_temp, &gameover_msg);
            sendMessage(p1_sock_temp, &rematch_offer_msg);
        }
        if (p2_sock_temp != -1) {
            sendMessage(p2_sock_temp, &gameover_msg);
            sendMessage(p2_sock_temp, &rematch_offer_msg);
        }
        printf("Sent game over and rematch offer notices for room %d.\n",
               roomId);
        return;  // Game over, exit handler

    } else {
        // --- Turn change logic ---
        int nextTurnPlayer = (playerColor == 1) ? 2 : 1;
        int currentTurnPlayer = playerColor;  // 手を打ったプレイヤー

        // 8. Check if next player must pass
        if (!has_valid_moves(&room->gameState, nextTurnPlayer)) {
            printf("Player %d has no valid moves in room %d. Passing turn.\n",
                   nextTurnPlayer, roomId);

            // ターンを元のプレイヤー(currentTurnPlayer)に戻す
            // gameState.currentTurn は変更しない（まだ nextTurnPlayer のパス）
            // 元のプレイヤーも打てるかチェック
            if (!has_valid_moves(&room->gameState, currentTurnPlayer)) {
                // 両者打てないのでゲーム終了へ
                // (check_game_overで検知済みのはず)
                printf(
                    "Player %d also has no valid moves. Game should end "
                    "(detected after pass).\n",
                    currentTurnPlayer);
                // このケースは通常、check_game_over で既に検出されているはず
                // 万が一のためのフォールバックとして再度チェック＆終了処理も可能
                pthread_mutex_unlock(
                    &room->room_mutex);  // ★★★ Fallback game over needs unlock
                                         // before potential send/close_room ★★★
                winner = check_game_over(&room->gameState);  // 再チェック
                if (winner != 0) {
                    // ゲームオーバー処理（6.
                    // とほぼ同じだが、ロック状態が異なるので注意）
                    room->status = ROOM_GAMEOVER;  // 状態だけ更新
                                                   // (ロックの外からなので注意)
                    printf(
                        "Force Game over in room %d after double pass. Winner "
                        "code: %d\n",
                        roomId, winner);
                    // 通知などは省略するか、別途実装
                    // close_room(roomId, "Game ended due to double pass."); //
                    // 必要なら
                } else {
                    // ここに来るのは想定外
                    fprintf(stderr,
                            "Error: Double pass detected but check_game_over "
                            "returned 0.\n");
                }
                return;  // ダブルパス or フォールバック終了

            } else {
                // 元のプレイヤー(currentTurnPlayer)にターンが戻る
                room->gameState.currentTurn =
                    currentTurnPlayer;  // ターンを正式に戻す
                printf(
                    "Returning turn to player %d (sockfd %d) in room %d after "
                    "opponent pass.\n",
                    currentTurnPlayer, client_sock, roomId);

                Message turn_notice;
                turn_notice.type = MSG_YOUR_TURN_NOTICE;
                turn_notice.data.yourTurnNotice.roomId = roomId;

                pthread_mutex_unlock(
                    &room->room_mutex);  // Unlock before sending
                sendMessage(client_sock,
                            &turn_notice);  // 自分自身(打った人)に通知
                pthread_mutex_lock(&room->room_mutex);  // Re-lock
            }
        } else {
            // 9. 通常のターン交代: 次のプレイヤー(nextTurnPlayer)に通知
            room->gameState.currentTurn = nextTurnPlayer;  // ターンを交代
            int target_sock = -1;
            if (nextTurnPlayer == 1 && room->player1_sock != -1) {
                target_sock = room->player1_sock;
            } else if (nextTurnPlayer == 2 && room->player2_sock != -1) {
                target_sock = room->player2_sock;
            }

            if (target_sock != -1) {
                Message turn_notice;
                turn_notice.type = MSG_YOUR_TURN_NOTICE;
                turn_notice.data.yourTurnNotice.roomId = roomId;

                printf(
                    "Sent YOUR_TURN notice to player %d (sockfd %d) in room "
                    "%d.\n",
                    nextTurnPlayer, target_sock, roomId);

                pthread_mutex_unlock(
                    &room->room_mutex);  // Unlock before sending
                sendMessage(target_sock, &turn_notice);
                pthread_mutex_lock(&room->room_mutex);  // Re-lock
            } else {
                fprintf(stderr,
                        "Error: Could not find socket for next turn player %d "
                        "in room %d.\n",
                        nextTurnPlayer, roomId);
                // 相手がいない？致命的なエラーの可能性
            }
        }
    }

    room->last_action_time = time(NULL);
    pthread_mutex_unlock(&room->room_mutex);  // Function end unlock
}

void handle_rematch_request(int client_sock, const Message* msg) {
    int roomId = msg->data.rematchReq.roomId;
    int agree =
        msg->data.rematchReq
            .agree;  // 1: Yes, 0: No (protocol.hに合わせる: 1=Yes, 0=No)

    printf(
        "Received REMATCH request from client sockfd %d for room %d (Agree: "
        "%d)\n",
        client_sock, roomId, agree);

    pthread_mutex_lock(&rooms_mutex);
    int room_idx = find_room_index(roomId);
    if (room_idx == -1) {
        pthread_mutex_unlock(&rooms_mutex);
        fprintf(stderr, "Error: Room %d not found for rematch request.\n",
                roomId);
        return;
    }

    pthread_mutex_lock(&rooms[room_idx].room_mutex);
    pthread_mutex_unlock(&rooms_mutex);  // リストロック解除

    Room* room = &rooms[room_idx];

    if (room->status != ROOM_GAMEOVER && room->status != ROOM_REMATCHING) {
        pthread_mutex_unlock(&room->room_mutex);
        fprintf(stderr,
                "Error: Room %d is not in game over/rematching state for "
                "rematch (current: %d).\n",
                roomId, room->status);
        // エラー通知を送っても良い
        return;
    }

    // どちらのプレイヤーからのリクエストか判断し、同意状況を記録
    int player_slot = 0;  // 1 or 2
    if (room->player1_sock == client_sock) {
        // player1_rematch_agree: 0:未返答, 1:Yes, 2:No
        if (room->player1_rematch_agree == 0) {  // まだ返答してない場合のみ更新
            room->player1_rematch_agree = agree ? 1 : 2;
            player_slot = 1;
        }
    } else if (room->player2_sock == client_sock) {
        if (room->player2_rematch_agree == 0) {
            room->player2_rematch_agree = agree ? 1 : 2;
            player_slot = 2;
        }
    } else {
        // 部屋のプレイヤーではない
        pthread_mutex_unlock(&room->room_mutex);
        fprintf(stderr, "Error: Client sockfd %d is not a player in room %d.\n",
                client_sock, roomId);
        return;
    }

    if (player_slot == 0) {  // 既に返答済みだった場合
        pthread_mutex_unlock(&room->room_mutex);
        printf("Client sockfd %d already responded for rematch in room %d.\n",
               client_sock, roomId);
        return;
    }

    room->status = ROOM_REMATCHING;       // 状態を更新
    room->last_action_time = time(NULL);  // タイムアウト用時間更新

    // 両者の同意状況を確認
    int p1_agree = room->player1_rematch_agree;
    int p2_agree = room->player2_rematch_agree;

    int p1_sock = room->player1_sock;  // 通知用に保持
    int p2_sock = room->player2_sock;  // 通知用に保持

    // 結果通知メッセージの準備
    Message result_msg;
    result_msg.type = MSG_REMATCH_RESULT_NOTICE;
    result_msg.data.rematchResultNotice.roomId = roomId;

    if (p1_agree == 2 || p2_agree == 2) {  // どちらかが No (値が2)
        result_msg.data.rematchResultNotice.result = 0;  // Disagreed
        printf("Rematch disagreed in room %d.\n", roomId);
        pthread_mutex_unlock(&room->room_mutex);  // close_room の前にアンロック

        // 両者に通知
        if (p1_sock != -1) sendMessage(p1_sock, &result_msg);
        if (p2_sock != -1) sendMessage(p2_sock, &result_msg);

        close_room(roomId, "Rematch declined by a player.");  // 部屋を閉じる

    } else if (p1_agree == 1 && p2_agree == 1) {         // 両者が Yes (値が1)
        result_msg.data.rematchResultNotice.result = 1;  // Agreed
        printf("Rematch agreed in room %d. Starting new game.\n", roomId);

        // --- ゲームを再開する処理 ---
        room->status = ROOM_PLAYING;
        // ゲーム状態を再初期化
        initialize_game_state(&room->gameState);
        // TODO: 先手後手交代が必要な場合は gameState.currentTurn を設定
        // 例: room->gameState.currentTurn = ( previous_first_player == 1 ) ? 2
        // : 1; 今回は常に黒番から開始とする
        room->gameState.currentTurn = 1;

        room->last_action_time = time(NULL);
        room->player1_rematch_agree = 0;  // リセット
        room->player2_rematch_agree = 0;

        // 再戦結果通知を送信
        if (p1_sock != -1) sendMessage(p1_sock, &result_msg);
        if (p2_sock != -1) sendMessage(p2_sock, &result_msg);

        // 新しいゲーム開始通知を送信
        Message start_notice;
        start_notice.type = MSG_GAME_START_NOTICE;
        start_notice.data.gameStartNotice.roomId = roomId;
        memcpy(start_notice.data.gameStartNotice.board, room->gameState.board,
               sizeof(room->gameState.board));

        // Player1 (黒と仮定) への通知
        start_notice.data.gameStartNotice.yourColor = 1;
        if (p1_sock != -1) sendMessage(p1_sock, &start_notice);
        // Player2 (白と仮定) への通知
        start_notice.data.gameStartNotice.yourColor = 2;
        if (p2_sock != -1) sendMessage(p2_sock, &start_notice);

        // 最初のプレイヤーに手番通知
        Message turn_notice;
        turn_notice.type = MSG_YOUR_TURN_NOTICE;
        turn_notice.data.yourTurnNotice.roomId = roomId;
        if (room->gameState.currentTurn == 1 && p1_sock != -1) {
            sendMessage(p1_sock, &turn_notice);
        } else if (room->gameState.currentTurn == 2 && p2_sock != -1) {
            sendMessage(p2_sock, &turn_notice);
        }

        pthread_mutex_unlock(&room->room_mutex);

    } else {
        // まだ片方しか返答していない -> 何もしない (タイムアウト待ち)
        printf(
            "Waiting for opponent's rematch response in room %d (P1:%d, "
            "P2:%d).\n",
            roomId, p1_agree, p2_agree);
        pthread_mutex_unlock(&room->room_mutex);
        // TODO: タイムアウト処理の実装が必要
        // (別スレッドで定期的にチェック or イベントドリブン)
    }
}

// --- クライアント切断処理 ---
void handle_disconnect(int client_sock) {
    printf("Client sockfd %d disconnected.\n", client_sock);

    // クライアントがどの部屋にいたか確認
    pthread_mutex_lock(&clients_mutex);
    int client_idx = find_client_index(client_sock);
    int roomId = -1;
    if (client_idx != -1) {
        roomId = clients[client_idx].roomId;
    }
    pthread_mutex_unlock(&clients_mutex);

    // クライアントリストから削除 (ソケットはまだ閉じない)
    remove_client(client_sock);

    // もし部屋に参加していたら、部屋の処理を行う
    if (roomId != -1) {
        pthread_mutex_lock(&rooms_mutex);
        int room_idx = find_room_index(roomId);

        if (room_idx != -1) {
            pthread_mutex_lock(&rooms[room_idx].room_mutex);
            pthread_mutex_unlock(&rooms_mutex);  // リストロック解除

            Room* room = &rooms[room_idx];
            int opponent_sock = -1;
            int disconnected_player_slot = 0;  // 1 or 2

            if (room->player1_sock == client_sock) {
                opponent_sock = room->player2_sock;
                room->player1_sock = -1;  // 切断されたプレイヤーを無効化
                disconnected_player_slot = 1;
            } else if (room->player2_sock == client_sock) {
                opponent_sock = room->player1_sock;
                room->player2_sock = -1;
                disconnected_player_slot = 2;
            }

            printf("Client sockfd %d (Player %d) disconnected from room %d.\n",
                   client_sock, disconnected_player_slot, roomId);

            // 部屋の状態に応じて処理
            if (room->status == ROOM_WAITING || room->status == ROOM_PLAYING ||
                room->status == ROOM_GAMEOVER ||
                room->status == ROOM_REMATCHING) {
                // 相手がいたら部屋閉鎖通知を送る
                if (opponent_sock != -1) {
                    printf(
                        "Notifying opponent sockfd %d in room %d about "
                        "disconnect.\n",
                        opponent_sock, roomId);
                    pthread_mutex_unlock(
                        &room->room_mutex);  // 通知前にアンロック

                    Message close_msg;
                    close_msg.type = MSG_ROOM_CLOSED_NOTICE;
                    close_msg.data.roomClosedNotice.roomId = roomId;
                    snprintf(close_msg.data.roomClosedNotice.reason,
                             sizeof(close_msg.data.roomClosedNotice.reason),
                             "Opponent disconnected.");
                    sendMessage(opponent_sock, &close_msg);

                    // 相手クライアントの roomId もリセット
                    pthread_mutex_lock(&clients_mutex);
                    int opp_client_idx = find_client_index(opponent_sock);
                    if (opp_client_idx != -1) {
                        clients[opp_client_idx].roomId = -1;
                        clients[opp_client_idx].playerColor = 0;
                    }
                    pthread_mutex_unlock(&clients_mutex);

                    // 部屋を閉じる
                    close_room(
                        roomId,
                        "A player disconnected.");  // close_room内で最終的なリセット

                } else {
                    // 相手がいなかった場合 (WAITING状態だったなど)
                    pthread_mutex_unlock(&room->room_mutex);
                    printf("Closing empty room %d after player disconnect.\n",
                           roomId);
                    close_room(roomId,
                               "Player disconnected.");  // 部屋を閉じるだけ
                }
            } else {
                // ROOM_EMPTY のはずだが、念のため
                pthread_mutex_unlock(&room->room_mutex);
                printf("Room %d status was %d during disconnect handling.\n",
                       roomId, room->status);
                // 必要なら close_room を呼ぶ
            }

        } else {
            pthread_mutex_unlock(&rooms_mutex);  // 部屋が見つからなかった場合
            fprintf(stderr,
                    "Warning: Disconnected client sockfd %d was associated "
                    "with room %d, but room not found in list.\n",
                    client_sock, roomId);
        }
    }

    // ソケットを閉じる
    close(client_sock);
    printf("Socket for client sockfd %d closed.\n", client_sock);

    // スレッド終了
    // pthread_exit(NULL); // handle_client ループの終了で自動的に終了する
}

// --- クライアント処理スレッド ---
void* handle_client(void* arg) {
    // ClientInfo* client_info = (ClientInfo*)arg; //
    // add_clientで渡されたポインタ ただし、client_info
    // は共有メモリclients[]の一部なので、直接使うのは危険 sockfd
    // を引数として受け取る方が安全
    int client_sock = *(int*)arg;  // main から sockfd のポインタを受け取る
    free(arg);                     // main で malloc したメモリを解放

    // スレッドをデタッチする場合（メインスレッドで join しない場合）
    pthread_detach(pthread_self());

    Message msg;
    int read_size;

    // 受信ループ
    while ((read_size = receiveMessage(client_sock, &msg)) > 0) {
        // 受信成功
        printf("Received message type %d from client sockfd %d\n", msg.type,
               client_sock);

        // メッセージタイプに基づいて処理を分岐
        switch (msg.type) {
            case MSG_CREATE_ROOM_REQUEST:
                handle_create_room_request(client_sock, &msg);
                break;
            case MSG_JOIN_ROOM_REQUEST:
                handle_join_room_request(client_sock, &msg);
                break;
            case MSG_START_GAME_REQUEST:
                handle_start_game_request(client_sock, &msg);
                break;
            case MSG_PLACE_PIECE_REQUEST:
                handle_place_piece_request(client_sock, &msg);
                break;
            case MSG_REMATCH_REQUEST:
                handle_rematch_request(client_sock, &msg);
                break;
            // 他のクライアントからのリクエストタイプもここに追加
            // case MSG_LIST_ROOMS_REQUEST:
            //     handle_list_rooms_request(client_sock, &msg); // 要実装
            //     break;
            // case MSG_PING:
            //     handle_ping(client_sock, &msg); // 要実装 (PONGを返す)
            //     break;
            default:
                fprintf(
                    stderr,
                    "Unknown message type %d received from client sockfd %d\n",
                    msg.type, client_sock);
                // 不明なメッセージに対するエラー応答など (任意)
                Message err_msg;
                err_msg.type = MSG_ERROR_NOTICE;
                snprintf(err_msg.data.errorNotice.message,
                         sizeof(err_msg.data.errorNotice.message),
                         "Unknown message type: %d", msg.type);
                sendMessage(client_sock, &err_msg);
                break;
        }
    }

    // receiveMessage が 0 以下を返した場合 (接続断またはエラー)
    if (read_size == 0) {
        printf("Client sockfd %d connection closed gracefully.\n", client_sock);
    } else {  // read_size < 0
        perror("recv error in handler thread");
        fprintf(stderr, "Receive error from client sockfd %d.\n", client_sock);
    }

    // 切断処理
    handle_disconnect(client_sock);

    printf("Client handler thread for sockfd %d exiting.\n", client_sock);
    return NULL;  // スレッド終了
}