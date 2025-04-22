#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "othello.h"

typedef struct players{
    int sock_black;
    msg user_black;
    int sock_white;
    msg user_white;
} players;

int make_socket(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("ERROR opening socket");
        exit(1);
    }
    int one=1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
    struct sockaddr_in srv_addr;
    memset((char *)&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = INADDR_ANY;
    srv_addr.sin_port = htons(PORT);
    if (bind(sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }
    return sock;
}

void wating_players(players *p, int sock){
    struct sockaddr_in client_addr;
    socklen_t clilen = sizeof(client_addr);

    printf("wating for a player black\n");
    p->sock_black = accept(sock, (struct sockaddr *)&client_addr, &clilen);
    if (p->sock_black < 0) {
        perror("ERROR on accept");
        exit(1);
    }
    msg m;
    memset(&m,0,MESSAGE_LENGTH);
    int n=0;
    n = recv(p->sock_black, &m, MESSAGE_LENGTH, 0);
    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }
    if(m.type!=CONN_REQ){
        printf("from the player %s\n",m.name);
        printf("a received message is not the type CONN_REQ\n");
        exit(1);
    }
    memcpy(&(p->user_black),&m,MESSAGE_LENGTH);
    printf("the player %s joined, color is %c\n",p->user_black.name,BLACK);

    p->sock_white = accept(sock, (struct sockaddr *)&client_addr, &clilen);
    if (p->sock_white < 0) {
        perror("ERROR on accept");
        exit(1);
    }
    memset(&m,0,MESSAGE_LENGTH);
    n = recv(p->sock_white, &m, MESSAGE_LENGTH, 0);
    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }
    if(m.type!=CONN_REQ){
        printf("from the player %s\n",m.name);
        printf("a received message is not the type CONN_REQ\n");
        exit(1);
    }
    memcpy(&(p->user_white),&m,MESSAGE_LENGTH);
    printf("the player %s joined, color is %c\n",p->user_white.name,WHITE);

    memcpy(&m,&(p->sock_black),MESSAGE_LENGTH);
    m.type=CONN_RES;
    m.color=WHITE;
    n=send(p->sock_white, &m, MESSAGE_LENGTH, 0);

    memcpy(&m,&(p->sock_white),MESSAGE_LENGTH);
    m.type=CONN_RES;
    m.color=BLACK;
    n=send(p->sock_black, &m, MESSAGE_LENGTH, 0);

}
int main(int argc, char** argv){
    
    printf("start server\n");
    players p; 
    int sock=make_socket();
    listen(sock, 2);
    printf("an acceptance socket is ready\n");
    wating_players(&p,sock);
    msg m;
    int n=0;
    int player_sock=p.sock_black;
    while(1){
        n = recv(p.sock_black, &m, MESSAGE_LENGTH, 0);
        if (n < 0) {
            perror("ERROR reading from socket");
            exit(1);
        }
        if(m.type!=PUT_MY_STONE){
            printf("from the player %s\n",p.user_black.name);
            printf("a received message is not the type PUT_MY_STONE\n");
            exit(1);
        }
        m.type=PUT_OPP_STONE;
        n=send(p.sock_white,&m,MESSAGE_LENGTH,0);

        n = recv(p.sock_white, &m, MESSAGE_LENGTH, 0);
        if (n < 0) {
            perror("ERROR reading from socket");
            exit(1);
        }
        if(m.type!=PUT_MY_STONE){
            printf("from the player %s\n",p.user_white.name);
            printf("a received message is not the type PUT_MY_STONE\n");
            exit(1);
        }
        m.type=PUT_OPP_STONE;
        n=send(p.sock_black,&m,MESSAGE_LENGTH,0);

    }
    
}