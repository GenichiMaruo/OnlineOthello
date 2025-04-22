#ifndef OTHELLO_H
#define OTHELLO_H

#define BLACK '*'
#define WHITE 'o'
#define NO_STONE ' '

#define PORT 10000
#define OTHELLO_ROW 8
#define OTHELLO_COL 8

#define CONN_REQ 1
#define CONN_RES 2
#define ACCEPT 3
#define REJECT 4
#define PUT_MY_STONE 5
#define PUT_OPP_STONE 6

#define NAME_LENGTH 20
#define MESSAGE_LENGTH 24

typedef struct message{
    unsigned char type; // 1 byte
    char name[NAME_LENGTH]; // 20 byte
    char color; // 1 byte
    unsigned char row; // 1 byte
    unsigned char col; // 1 byte
} msg; // 24 byte

#endif
