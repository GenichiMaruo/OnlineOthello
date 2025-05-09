#include "json_output.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "state.h"

// --- ヘルパー関数 ---

// JSON文字列のエスケープ
static void escape_json_string(const char* input, char* output,
                               size_t output_size) {
    size_t i = 0, j = 0;
    while (input[i] != '\0' && j < output_size - 1) {
        if (input[i] == '"' || input[i] == '\\') {
            if (j < output_size - 2) {
                output[j++] = '\\';
                output[j++] = input[i];
            } else {
                break;  // Buffer overflow prevention
            }
        } else {
            output[j++] = input[i];
        }
        i++;
    }
    output[j] = '\0';
}

// vsnprintf のラッパー
static void format_message(char* buffer, size_t size, const char* format,
                           va_list args) {
    vsnprintf(buffer, size, format, args);
}

// state_to_string は state.c/h に移動

// JSONイベントをstdoutに出力
static void send_json_event(const char* json_string) {
    // ... (内容は変更なし) ...
    fprintf(stdout, "%s\n", json_string);
    fflush(stdout);
}

// --- va_listを受け取る内部ヘルパー関数の前方宣言 (static) ---
static void send_server_message_event_unsafe_va(const char* format,
                                                va_list args);
static void send_error_event_unsafe_va(const char* format, va_list args);
static void send_log_event_unsafe_va(LogLevel level, const char* format,
                                     va_list args);

// --- イベント送信関数 (公開関数) ---

void send_state_change_event() {
    pthread_mutex_lock(get_state_mutex());
    send_state_change_event_unsafe();
    pthread_mutex_unlock(get_state_mutex());
}
void send_board_update_event() {
    pthread_mutex_lock(get_state_mutex());
    send_board_update_event_unsafe();
    pthread_mutex_unlock(get_state_mutex());
}
void send_server_message_event(const char* format, ...) {
    pthread_mutex_lock(
        get_state_mutex());  // メッセージ内容が状態依存の場合のため
    va_list args;
    va_start(args, format);
    send_server_message_event_unsafe_va(format, args);  // ヘルパー呼び出し
    va_end(args);
    pthread_mutex_unlock(get_state_mutex());
}
void send_error_event(const char* format, ...) {
    pthread_mutex_lock(get_state_mutex());  // 状態依存メッセージのため
    va_list args;
    va_start(args, format);
    send_error_event_unsafe_va(format, args);  // ヘルパー呼び出し
    va_end(args);
    pthread_mutex_unlock(get_state_mutex());
}
void send_log_event(LogLevel level, const char* format, ...) {
    pthread_mutex_lock(get_state_mutex());  // 状態依存メッセージのため
    va_list args;
    va_start(args, format);
    send_log_event_unsafe_va(level, format, args);  // ヘルパー呼び出し
    va_end(args);
    pthread_mutex_unlock(get_state_mutex());
}
void send_your_turn_event() {
    pthread_mutex_lock(get_state_mutex());
    send_your_turn_event_unsafe();
    pthread_mutex_unlock(get_state_mutex());
}
void send_game_over_event(uint8_t winner, const char* message) {
    pthread_mutex_lock(get_state_mutex());
    send_game_over_event_unsafe(winner, message);
    pthread_mutex_unlock(get_state_mutex());
}
void send_rematch_offer_event() {
    pthread_mutex_lock(get_state_mutex());
    send_rematch_offer_event_unsafe();
    pthread_mutex_unlock(get_state_mutex());
}
void send_rematch_result_event(uint8_t result) {
    pthread_mutex_lock(get_state_mutex());
    send_rematch_result_event_unsafe(result);
    pthread_mutex_unlock(get_state_mutex());
}

static void json_escape_string(char* dest, const char* src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) {
        if (dest && dest_size > 0) dest[0] = '\0';
        return;
    }

    size_t i = 0, j = 0;
    while (src[i] != '\0' && j < dest_size - 1) {
        switch (src[i]) {
            case '\"':
                if (j + 2 < dest_size) {  // \" と \0
                    dest[j++] = '\\';
                    dest[j++] = '\"';
                } else
                    goto end_loop;  // バッファ不足
                break;
            case '\\':
                if (j + 2 < dest_size) {  // \\ と \0
                    dest[j++] = '\\';
                    dest[j++] = '\\';
                } else
                    goto end_loop;
                break;
            case '\b':
                if (j + 2 < dest_size) {  // \b と \0
                    dest[j++] = '\\';
                    dest[j++] = 'b';
                } else
                    goto end_loop;
                break;
            case '\f':
                if (j + 2 < dest_size) {  // \f と \0
                    dest[j++] = '\\';
                    dest[j++] = 'f';
                } else
                    goto end_loop;
                break;
            case '\n':
                if (j + 2 < dest_size) {  // \n と \0
                    dest[j++] = '\\';
                    dest[j++] = 'n';
                } else
                    goto end_loop;
                break;
            case '\r':
                if (j + 2 < dest_size) {  // \r と \0
                    dest[j++] = '\\';
                    dest[j++] = 'r';
                } else
                    goto end_loop;
                break;
            case '\t':
                if (j + 2 < dest_size) {  // \t と \0
                    dest[j++] = '\\';
                    dest[j++] = 't';
                } else
                    goto end_loop;
                break;
            // 他の制御文字 (U+0000 から U+001F) も \uXXXX
            // 形式でエスケープするのが望ましいが、ここでは省略
            default:
                dest[j++] = src[i];
                break;
        }
        i++;
    }
end_loop:
    dest[j] = '\0';  // 終端ヌル文字
}

// --- イベント送信関数 (内部 _unsafe) ---

void send_state_change_event_unsafe() {
    char json_buffer[512];
    ClientState state = get_client_state_unsafe();
    int room_id = get_my_room_id_unsafe();
    uint8_t color = get_my_color_unsafe();

    // state_to_string は state.h/c に移動
    snprintf(json_buffer, sizeof(json_buffer),
             "{\"type\":\"stateChange\",\"state\":\"%s\",\"roomId\":%d,"
             "\"color\":%d}",
             state_to_string(state), room_id, color);  // state.h の関数を呼ぶ
    send_json_event(json_buffer);
}

void send_board_update_event_unsafe() {
    // ... (内容は変更なし) ...
    char json_buffer[1024];    // Buffer for the whole JSON string
    char board_str[512] = "";  // Buffer for the board array part
    char row_str[64];
    uint8_t board[BOARD_SIZE][BOARD_SIZE];
    int room_id = get_my_room_id_unsafe();
    // Get board state safely (though already in unsafe context)
    get_game_board_unsafe(board);  // Use the _unsafe getter for consistency

    strcat(board_str, "[");
    for (int i = 0; i < BOARD_SIZE; ++i) {
        // Ensure row_str is properly formatted
        snprintf(row_str, sizeof(row_str), "[%d,%d,%d,%d,%d,%d,%d,%d]",
                 board[i][0], board[i][1], board[i][2], board[i][3],
                 board[i][4], board[i][5], board[i][6], board[i][7]);
        // Check if strcat will overflow board_str before appending
        if (strlen(board_str) + strlen(row_str) + 2 <
            sizeof(board_str)) {  // +2 for comma and closing bracket
            strcat(board_str, row_str);
            if (i < BOARD_SIZE - 1) {
                strcat(board_str, ",");
            }
        } else {
            // Handle potential overflow, e.g., log an error or truncate
            fprintf(stderr,
                    "Warning: Board string buffer might overflow in "
                    "send_board_update_event_unsafe\n");
            break;  // Stop appending rows
        }
    }
    // Check if strcat will overflow before appending the closing bracket
    if (strlen(board_str) + 1 < sizeof(board_str)) {
        strcat(board_str, "]");
    } else {
        fprintf(stderr,
                "Warning: Board string buffer overflow on closing bracket in "
                "send_board_update_event_unsafe\n");
        // Ensure null termination even if overflowed
        board_str[sizeof(board_str) - 1] = '\0';
    }

    snprintf(json_buffer, sizeof(json_buffer),
             "{\"type\":\"boardUpdate\",\"roomId\":%d,\"board\":%s}", room_id,
             board_str);
    send_json_event(json_buffer);
}

// va_list を受け取るヘルパー関数の実装 (static)
static void send_server_message_event_unsafe_va(const char* format,
                                                va_list args) {
    char message_buffer[MAX_MESSAGE_LEN * 2];
    char escaped_message[MAX_MESSAGE_LEN * 2];
    char json_buffer[MAX_MESSAGE_LEN * 2 + 64];

    format_message(message_buffer, sizeof(message_buffer), format, args);
    escape_json_string(message_buffer, escaped_message,
                       sizeof(escaped_message));

    snprintf(json_buffer, sizeof(json_buffer),
             "{\"type\":\"serverMessage\",\"message\":\"%s\"}",
             escaped_message);
    send_json_event(json_buffer);
}
// 通常のunsafe版
void send_server_message_event_unsafe(const char* format, ...) {
    va_list args;
    va_start(args, format);
    send_server_message_event_unsafe_va(format, args);  // ヘルパー呼び出し
    va_end(args);
}

// va_list を受け取るヘルパー関数の実装 (static)
static void send_error_event_unsafe_va(const char* format, va_list args) {
    char message_buffer[MAX_MESSAGE_LEN * 2];
    char escaped_message[MAX_MESSAGE_LEN * 2];
    char json_buffer[MAX_MESSAGE_LEN * 2 + 64];

    format_message(message_buffer, sizeof(message_buffer), format, args);
    escape_json_string(message_buffer, escaped_message,
                       sizeof(escaped_message));

    snprintf(json_buffer, sizeof(json_buffer),
             "{\"type\":\"error\",\"message\":\"%s\"}", escaped_message);
    send_json_event(json_buffer);
}
void send_error_event_unsafe(const char* format, ...) {
    va_list args;
    va_start(args, format);
    send_error_event_unsafe_va(format, args);  // ヘルパー呼び出し
    va_end(args);
}

// va_list を受け取るヘルパー関数の実装 (static)
static void send_log_event_unsafe_va(LogLevel level, const char* format,
                                     va_list args) {
    char message_buffer[MAX_MESSAGE_LEN * 2];
    char escaped_message[MAX_MESSAGE_LEN * 2];
    char json_buffer[MAX_MESSAGE_LEN * 2 + 64];
    const char* level_str = "DEBUG";
    if (level == LOG_INFO)
        level_str = "INFO";
    else if (level == LOG_WARN)
        level_str = "WARN";
    else if (level == LOG_ERROR)
        level_str = "ERROR";

    format_message(message_buffer, sizeof(message_buffer), format, args);
    escape_json_string(message_buffer, escaped_message,
                       sizeof(escaped_message));

    snprintf(json_buffer, sizeof(json_buffer),
             "{\"type\":\"log\",\"level\":\"%s\",\"message\":\"%s\"}",
             level_str, escaped_message);
    send_json_event(json_buffer);
}
void send_log_event_unsafe(LogLevel level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    send_log_event_unsafe_va(level, format, args);  // ヘルパー呼び出し
    va_end(args);
}

void send_your_turn_event_unsafe() {
    char json_buffer[128];
    int room_id = get_my_room_id_unsafe();
    snprintf(json_buffer, sizeof(json_buffer),
             "{\"type\":\"yourTurn\",\"roomId\":%d}", room_id);
    send_json_event(json_buffer);
}

void send_game_over_event_unsafe(uint8_t winner, const char* message) {
    char json_buffer[MAX_MESSAGE_LEN + 128];
    char escaped_message[MAX_MESSAGE_LEN];
    int room_id = get_my_room_id_unsafe();
    escape_json_string(message, escaped_message, sizeof(escaped_message));
    snprintf(json_buffer, sizeof(json_buffer),
             "{\"type\":\"gameOver\",\"roomId\":%d,\"winner\":%d,\"message\":"
             "\"%s\"}",
             room_id, winner, escaped_message);
    send_json_event(json_buffer);
}

void send_rematch_offer_event_unsafe() {
    char json_buffer[128];
    int room_id = get_my_room_id_unsafe();
    snprintf(json_buffer, sizeof(json_buffer),
             "{\"type\":\"rematchOffer\",\"roomId\":%d}", room_id);
    send_json_event(json_buffer);
}

void send_rematch_result_event_unsafe(uint8_t result) {
    char json_buffer[128];
    int room_id = get_my_room_id_unsafe();
    const char* result_str = "declined";
    if (result == 1)
        result_str = "agreed";
    else if (result == 2)
        result_str = "timeout";

    snprintf(
        json_buffer, sizeof(json_buffer),
        "{\"type\":\"rematchResult\",\"roomId\":%d,\"result\":\"%s\"}",  // resultを文字列で送る例
        room_id, result_str);
    send_json_event(json_buffer);
}

void send_chat_message_received_event_unsafe(
    int roomId, int senderColor, const char* senderName, const char* message,
    time_t timestamp) {  // エスケープ後の文字列を格納するバッファ
    // senderName は MAX_ROOM_NAME_LEN (32) -> エスケープ後最大 約64
    // message は 256 (protocol.h) -> エスケープ後最大 約512
    char escaped_sender_name[MAX_ROOM_NAME_LEN * 2 + 1];
    char escaped_message[256 * 2 +
                         1];  // ChatMessageBroadcastNoticeData.message_text
                              // のサイズに合わせる

    // senderName と message をエスケープ
    json_escape_string(escaped_sender_name, senderName ? senderName : "System",
                       sizeof(escaped_sender_name));
    json_escape_string(escaped_message, message ? message : "",
                       sizeof(escaped_message));

    // JSON文字列を構築するための十分なサイズのバッファ
    // 固定文字列長 + 数値の桁数 + エスケープ後文字列長
    // "{\"type\":\"chatMessage\",\"payload\":{\"roomId\":%d,\"senderColor\":%d,\"senderDisplayName\":\"%s\",\"message\":\"%s\",\"timestamp\":%ld}}"
    // 固定部分: 約100文字
    // roomId: int (最大10桁)
    // senderColor: int (最大3桁)
    // timestamp: long (最大20桁)
    // escaped_sender_name: MAX_ROOM_NAME_LEN * 2
    // escaped_message: 256 * 2
    // 余裕を見て 100 + 10 + 3 + 20 + 64 + 512 + α = 約709 + α
    char json_buffer[1024];  // 十分なサイズを確保 (必要に応じて調整)

    // snprintf で安全にJSON文字列を構築
    // time_t を long int にキャストして %ld で出力
    int written = snprintf(
        json_buffer, sizeof(json_buffer),
        "{\"type\":\"chatMessage\",\"payload\":{\"roomId\":%d,\"senderColor\":%"
        "d,\"senderDisplayName\":\"%s\",\"message\":\"%s\",\"timestamp\":%ld}}",
        roomId, senderColor, escaped_sender_name, escaped_message,
        (long)timestamp);  // time_tをlongにキャスト

    if (written < 0 || (size_t)written >= sizeof(json_buffer)) {
        // エラー処理: バッファが小さすぎたか、エンコードエラー
        // 実際にはログ出力などを行う
        fprintf(stderr,
                "[JSON_OUTPUT_ERROR] Failed to format chatMessage JSON or "
                "buffer too small.\n");
        return;
    }

    printf("%s\n", json_buffer);
    fflush(stdout);  // フロントエンドが即座に読み取れるようにフラッシュ
}

// ロック版の実装
// void send_chat_message_received_event(int roomId, int senderColor,
//                                       const char* senderName,
//                                       const char* message, time_t timestamp)
//                                       {
//     pthread_mutex_lock(&g_stdout_mutex);
//     send_chat_message_received_event_unsafe(roomId, senderColor, senderName,
//                                             message, timestamp);
//     pthread_mutex_unlock(&g_stdout_mutex);
// }