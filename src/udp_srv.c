#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "othello.h"

int make_socket(){
    int sock = socket(AF_INET, SOCK_DGRAM,IPPROTO_UDP);
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
    printf("an server socket is ready\n");

    int n;
    int buf_len=5000;
    char buf[buf_len];
    int seq=0;
    int cnt=0;
    struct sockaddr client_addr;
    socklen_t clilen = sizeof(client_addr);
    while(1){
        n = recvfrom(sock, buf,buf_len, 0,&client_addr,&clilen);
        printf("receive from %s ",inet_ntoa(((struct sockaddr_in *)&client_addr)->sin_addr));
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
