#include "ui.h"

// --- 盤面表示 ---
void display_board() {
    // 状態を読むだけなので、ロックは必須ではないかもしれないが、
    // 他スレッドによる game_board/my_color の変更と表示のズレを防ぐためロック
    // pthread_mutex_lock(&state_mutex);
    printf("\n== Board (Room: %d) ==\n", my_room_id);
    printf("  ");
    for (int j = 0; j < BOARD_SIZE; ++j) printf(" %d", j);  // Align columns
    printf("\n");
    printf(" +");
    for (int j = 0; j < BOARD_SIZE; ++j) printf("--");
    printf("-+\n");
    for (int i = 0; i < BOARD_SIZE; ++i) {
        printf("%d|", i);
        for (int j = 0; j < BOARD_SIZE; ++j) {
            char piece = '.';  // 空き
            if (game_board[i][j] == 1)
                piece = 'X';  // 黒
            else if (game_board[i][j] == 2)
                piece = 'O';  // 白
            printf(" %c", piece);
        }
        printf(" |\n");
    }
    printf(" +");
    for (int j = 0; j < BOARD_SIZE; ++j) printf("--");
    printf("-+\n");
    printf("Your color: %s\n", my_color == 1
                                   ? "Black (X)"
                                   : (my_color == 2 ? "White (O)" : "None"));
    printf("=====================\n");
    // pthread_mutex_unlock(&state_mutex);
}

// --- プロンプト表示 ---
void display_prompt() {
    pthread_mutex_lock(&state_mutex);  // 状態読み取り中にロック

    // 現在の状態に基づいてプロンプト文字列を決定
    const char* state_str = "Unknown";
    const char* prompt_msg = "Enter command (quit): ";

    switch (current_state) {
        case STATE_DISCONNECTED:
            state_str = "Disconnected";
            prompt_msg = "Exiting...";
            break;
        case STATE_CONNECTING:
            state_str = "Connecting";
            prompt_msg = "Connecting to server...";
            break;
        case STATE_CONNECTED:
            state_str = "Lobby";
            prompt_msg = "Enter command (create, join [id], list, quit): ";
            break;
        case STATE_WAITING_IN_ROOM:
            state_str = "Waiting";
            if (my_color == 1)
                prompt_msg = "Enter command (start, quit): ";  // Host
            else
                prompt_msg =
                    "Waiting for host to start... (quit): ";  // Player 2
            break;
        case STATE_MY_TURN:
            state_str = "Your Turn";
            prompt_msg = "Enter move (row col) or quit: ";
            break;
        case STATE_OPPONENT_TURN:
            state_str = "Opponent Turn";
            prompt_msg = "Waiting for opponent... (quit): ";
            break;
        case STATE_GAME_OVER:
            state_str = "Game Over";
            prompt_msg = "Enter command (rematch yes/no, quit): ";
            break;
        case STATE_REMOTE_CLOSED:
            state_str = "Disconnected";
            prompt_msg = "Connection closed. Press Enter to exit: ";
            break;
        case STATE_QUITTING:
            state_str = "Quitting";
            prompt_msg = "Shutting down...";
            break;
    }

    // 部屋IDも表示（ロビー以外）
    if (my_room_id != -1 && current_state != STATE_CONNECTED &&
        current_state != STATE_DISCONNECTED &&
        current_state != STATE_CONNECTING) {
        printf("\n[%s - Room %d] %s", state_str, my_room_id, prompt_msg);
    } else {
        printf("\n[%s] %s", state_str, prompt_msg);
    }

    pthread_mutex_unlock(&state_mutex);
    fflush(stdout);  // プロンプトを強制表示
}