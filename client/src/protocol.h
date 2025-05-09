// protocol.h

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>  // For fixed-width integers like uint8_t
#include <time.h>    // For time_t

#define BOARD_SIZE 8
#define MAX_ROOM_NAME_LEN 32
#define MAX_MESSAGE_LEN 128

// メッセージタイプ定義
typedef enum {
    // Client -> Server Requests
    MSG_CREATE_ROOM_REQUEST,
    MSG_JOIN_ROOM_REQUEST,
    MSG_LIST_ROOMS_REQUEST,
    MSG_START_GAME_REQUEST,
    MSG_PLACE_PIECE_REQUEST,
    MSG_REMATCH_REQUEST,
    MSG_CHAT_MESSAGE_SEND_REQUEST,

    // Server -> Client Responses/Notifications
    MSG_CREATE_ROOM_RESPONSE,
    MSG_JOIN_ROOM_RESPONSE,
    MSG_LIST_ROOMS_RESPONSE,
    MSG_PLAYER_JOINED_NOTICE,
    MSG_GAME_START_NOTICE,
    MSG_UPDATE_BOARD_NOTICE,
    MSG_INVALID_MOVE_NOTICE,
    MSG_YOUR_TURN_NOTICE,
    MSG_GAME_OVER_NOTICE,
    MSG_REMATCH_OFFER_NOTICE,
    MSG_REMATCH_RESULT_NOTICE,
    MSG_ROOM_CLOSED_NOTICE,
    MSG_ERROR_NOTICE,
    MSG_CHAT_MESSAGE_BROADCAST_NOTICE
} MessageType;

// --- データペイロード定義 ---
// !!! Message 構造体定義よりも前に、すべてのペイロード構造体を定義する !!!

// 部屋作成要求 (Client -> Server)
typedef struct {
    char roomName[MAX_ROOM_NAME_LEN];
} CreateRoomRequestData;

// 部屋作成応答 (Server -> Client)
typedef struct {
    int success;
    int roomId;
    char message[MAX_MESSAGE_LEN];
} CreateRoomResponseData;

// 部屋参加要求 (Client -> Server)
typedef struct {
    int roomId;
} JoinRoomRequestData;

// 部屋参加応答 (Server -> Client)
typedef struct {
    int success;
    int roomId;
    char message[MAX_MESSAGE_LEN];
} JoinRoomResponseData;

// 相手参加通知 (Server -> Client)
typedef struct {
    int roomId;
} PlayerJoinedNoticeData;

// ゲーム開始要求 (Client -> Server)
typedef struct {
    int roomId;
} StartGameRequestData;

// ゲーム開始通知 (Server -> Client)
typedef struct {
    int roomId;
    uint8_t yourColor;
    uint8_t board[BOARD_SIZE][BOARD_SIZE];
} GameStartNoticeData;

// コマ配置要求 (Client -> Server)
typedef struct {
    int roomId;
    uint8_t row;
    uint8_t col;
} PlacePieceRequestData;

// 盤面更新通知 (Server -> Client)
typedef struct {
    int roomId;
    uint8_t playerColor;
    uint8_t row;
    uint8_t col;
    uint8_t board[BOARD_SIZE][BOARD_SIZE];
} UpdateBoardNoticeData;

// 無効手通知 (Server -> Client)
typedef struct {
    int roomId;
    char message[MAX_MESSAGE_LEN];
} InvalidMoveNoticeData;

// あなたの手番通知 (Server -> Client)
typedef struct {
    int roomId;
} YourTurnNoticeData;

// ゲーム終了通知 (Server -> Client)
typedef struct {
    int roomId;
    uint8_t winner;  // 0: Draw, 1: Black, 2: White
    char message[MAX_MESSAGE_LEN];
} GameOverNoticeData;

// 再戦要求 (Client -> Server)
typedef struct {
    int roomId;
    uint8_t agree;  // 1: Yes, 0: No
} RematchRequestData;

// 再戦確認通知 (Server -> Client)
typedef struct {
    int roomId;
} RematchOfferNoticeData;

// 再戦結果通知 (Server -> Client)
typedef struct {
    int roomId;
    uint8_t result;  // 0: Disagreed, 1: Agreed, 2: Timeout
} RematchResultNoticeData;

// 部屋閉鎖通知 (Server -> Client)
typedef struct {
    int roomId;
    char reason[MAX_MESSAGE_LEN];
} RoomClosedNoticeData;

// チャットメッセージ送信要求 (Client -> Server)
typedef struct {
    int roomId;
    char message_text[256];
} ChatMessageSendRequestData;

// チャットメッセージ受信通知 (Server -> Client)
typedef struct {
    int roomId;
    int sender_player_color;  // 0: システム, 1: プレイヤー1, 2: プレイヤー2
    char sender_display_name[MAX_ROOM_NAME_LEN];
    char message_text[256];
    time_t timestamp;
} ChatMessageBroadcastNoticeData;

// エラー通知 (Server -> Client)
typedef struct {
    char message[MAX_MESSAGE_LEN];
} ErrorNoticeData;

// --- 通信メッセージ構造体 ---
typedef struct {
    MessageType type;  // メッセージの種類を示すヘッダー
    union {
        // 各メッセージタイプのペイロード (上記で定義された型を使用)
        CreateRoomRequestData createRoomReq;
        CreateRoomResponseData createRoomResp;
        JoinRoomRequestData joinRoomReq;
        JoinRoomResponseData joinRoomResp;
        PlayerJoinedNoticeData playerJoinedNotice;
        StartGameRequestData startGameReq;
        GameStartNoticeData gameStartNotice;
        PlacePieceRequestData placePieceReq;
        UpdateBoardNoticeData updateBoardNotice;
        InvalidMoveNoticeData invalidMoveNotice;
        YourTurnNoticeData yourTurnNotice;
        GameOverNoticeData gameOverNotice;
        RematchRequestData rematchReq;
        RematchOfferNoticeData rematchOfferNotice;
        RematchResultNoticeData rematchResultNotice;
        RoomClosedNoticeData roomClosedNotice;
        ChatMessageSendRequestData chatMessageSendReq;
        ChatMessageBroadcastNoticeData chatMessageBroadcastNotice;
        ErrorNoticeData errorNotice;
    } data;
} Message;

// --- 通信補助関数 (プロトタイプ宣言) ---
int sendMessage(int sockfd, const Message* msg);
int receiveMessage(int sockfd, Message* msg);

#endif  // PROTOCOL_H