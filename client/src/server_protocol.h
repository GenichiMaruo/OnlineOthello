#ifndef SERVER_PROTOCOL_H
#define SERVER_PROTOCOL_H

#include <stdint.h>

#define MAX_NAME_LENGTH 64
#define MAX_CHAT_LENGTH 256
#define MAX_WATCHERS 10
#define MAX_CHAT_HISTORY 500
#define MAX_ROOMS 100

// データ種別 (Packet Type)
typedef enum {
    HELLO = 0x01,
    BOARD_STATE = 0x02,
    MOVE = 0x03,
    PLAYER_INFO = 0x04,
    WATCHER_LIST = 0x05,
    CHAT_MESSAGE = 0x06,
    CHAT_HISTORY = 0x07,
    ROOM_SETUP = 0x08,
    ONLINE_STATUS = 0x09,
    GAME_END = 0x0A,
    PING = 0x0B,
    PONG = 0x0C,
    ERROR_MSG = 0x0D
} PacketType;

// ユーザー情報
typedef struct {
    char name[MAX_NAME_LENGTH];
    int online;     // 1=online, 0=offline
    int color;      // 0=未定, 1=黒, 2=白
    int is_player;  // 1=プレイヤー, 0=観戦者
} UserInfo;

// チャットメッセージ
typedef struct {
    char sender[MAX_NAME_LENGTH];
    char message[MAX_CHAT_LENGTH];
} ChatMessage;

// 部屋設定
typedef struct {
    char password[MAX_NAME_LENGTH];
    int black_first;    // 1=ホストが黒, 0=ホストが白
    int allow_rematch;  // 1=連戦許可, 0=しない
} RoomSetting;

// ゲーム部屋
typedef struct {
    int room_id;
    UserInfo player1;
    UserInfo player2;
    UserInfo watchers[MAX_WATCHERS];
    int watcher_count;
    uint8_t board[8][8];  // 2bit x 8x8 →1マス=00(空),01(黒),10(白)
    int move_count;
    ChatMessage chat_log[MAX_CHAT_HISTORY];
    int chat_log_count;
    RoomSetting setting;
    int active;  // 1=部屋使用中, 0=未使用
} Room;

// パケット共通ヘッダー
typedef struct {
    uint8_t type;     // PacketType
    uint16_t length;  // Payload長
} PacketHeader;

// 手の情報
typedef struct {
    int room_id;  // ルームID
    int color;    // 0=未定, 1=黒, 2=白
    int row;
    int col;
} MoveInfo;

// クライアントからサーバーへ送るパケットのタイプ
typedef enum {
    PACKET_JOIN_ROOM = 1,
    PACKET_SPECTATE_ROOM = 2,
    PACKET_MOVE = 3,
    PACKET_CHAT = 4,
    PACKET_DISCONNECT = 5,
} ClientPacketType;

// ルーム参加パケット
typedef struct {
    char room_name[MAX_NAME_LENGTH];
    char password[MAX_NAME_LENGTH];
} JoinRoomPacket;

// プレイヤーの手番パケット
typedef struct {
    int room_id;
    int row;
    int col;
    int color;
} MovePacket;

// チャットパケット
typedef struct {
    int room_id;
    char sender[MAX_NAME_LENGTH];
    char message[MAX_CHAT_LENGTH];
} ChatPacket;

// 1パケット全体をまとめるやつ
typedef struct {
    uint8_t type;  // ClientPacketType
    union {
        JoinRoomPacket join_room;
        MovePacket move;
        ChatPacket chat;
        // 切断(PACKET_DISCONNECT)にはpayload不要だから無し
    } data;
} ProtocolPacket;

#endif  // SERVER_PROTOCOL_H
