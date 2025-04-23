#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "othello.h"

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

int main(int argc, char** argv){
    
    printf("start server\n");
    int sock=make_socket();
    listen(sock, 1);
    printf("an acceptance socket is ready\n");
    struct sockaddr_in client_addr;
    socklen_t clilen = sizeof(client_addr);
    int c_sock= accept(sock, (struct sockaddr *)&client_addr, &clilen);
    if (c_sock < 0) {
        perror("ERROR on accept");
        exit(1);
    }
    printf("a new client socket opened\n");
    int n;
    int buf_len=5000;
    char buf[buf_len];
    int seq=0;
    int cnt=0;
    while(1){
        n = recv(c_sock, buf,buf_len, 0);
        printf("receive from %s ",inet_ntoa(client_addr.sin_addr));
        printf("received buf: %d\n",n);
        if (n < 0) {
            perror("ERROR reading from socket");
            exit(1);
        }
        cnt++;
        memcpy(&seq,buf,sizeof(int));
        printf("received seq is %d, count is %d\n",seq,cnt);
    }
    
}
