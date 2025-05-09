#ifndef JSON_OUTPUT_H
#define JSON_OUTPUT_H

#include <time.h>

#include "client_common.h"

// ログレベル
typedef enum { LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR } LogLevel;

// --- 関数プロトタイプ ---
void send_state_change_event();
void send_board_update_event();
void send_server_message_event(const char* format, ...);
void send_error_event(const char* format, ...);
void send_log_event(LogLevel level, const char* format, ...);
void send_your_turn_event();
void send_game_over_event(uint8_t winner, const char* message);
void send_rematch_offer_event();
void send_rematch_result_event(uint8_t result);
// void send_chat_message_received_event(int roomId, int senderColor,
//                                       const char* senderName,
//                                       const char* message, time_t timestamp);

// --- 内部用 (Mutexロック済みコンテキスト用) ---
void send_state_change_event_unsafe();
void send_board_update_event_unsafe();
void send_server_message_event_unsafe(const char* format, ...);
void send_error_event_unsafe(const char* format, ...);
void send_log_event_unsafe(LogLevel level, const char* format, ...);
void send_your_turn_event_unsafe();
void send_game_over_event_unsafe(uint8_t winner, const char* message);
void send_rematch_offer_event_unsafe();
void send_rematch_result_event_unsafe(uint8_t result);
void send_chat_message_received_event_unsafe(int roomId, int senderColor,
                                             const char* senderName,
                                             const char* message,
                                             time_t timestamp);

#endif  // JSON_OUTPUT_H