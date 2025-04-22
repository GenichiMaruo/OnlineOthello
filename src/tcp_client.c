#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "othello.h"

int make_connection(char* addr, int *sock){
    struct in_addr dest;
    inet_aton(addr, &dest);
    struct sockaddr_in srv_addr;

    // 接続先アドレスを設定する
    memcpy(&(srv_addr.sin_addr.s_addr),(char*)&(dest.s_addr),4);
    srv_addr.sin_port=htons(PORT);
    srv_addr.sin_family=AF_INET;

    *sock = socket(AF_INET, SOCK_STREAM, 0);
    
    
    if (*sock < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    // サーバーに接続する
    if (connect(*sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
        perror("ERROR connecting");
        exit(1);
    }
}


int main(int argc, char** argv){
    const char* optstring = "s:";
    int c;
    char *srv_addr=NULL;
    char *user_name=NULL;

    if(argc!=3){
        printf("args:%d\n",argc);
        printf(" arg -s server_address\n");
        exit(0);
    }
    while ((c=getopt(argc, argv, optstring)) != -1) {
        //printf("opt=%c ", c);
        switch(c){
            case 's':{
                srv_addr=optarg;
                break;
            }
            default:{
                printf(" arg -s server_address\n");
                exit(0);
            }
        }
    }
    printf("server address:%s\n",srv_addr);

    int sock;
    make_connection(srv_addr,&sock);// make socket communicating with a server by using an IP address
    int buf_len=5000;
    char buf[buf_len];

    printf("socket opened\n");
    memset(buf,0,buf_len);
    int seq=0;
    printf("seq num: %d\n",seq);
    while(1){
        sleep(1);
        seq++;
        printf("seq num: %d\n",seq);
        memcpy(buf,&seq,sizeof(int));
        
        send(sock,&(buf[0]),buf_len,0);
        printf("data was sent\n");
    }
}