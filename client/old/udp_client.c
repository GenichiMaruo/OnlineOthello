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
    return sock;
}


int main(int argc, char** argv){
    const char* optstring = "s:";
    int c;
    char *srv_addr;
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

    struct in_addr dest;
    inet_aton(srv_addr, &dest);
    struct sockaddr_in srv_addr_in;
    memset(&srv_addr_in,0,sizeof(srv_addr_in));
    // 接続先アドレスを設定する
    memcpy(&(srv_addr_in.sin_addr.s_addr),&(dest.s_addr),4);
    srv_addr_in.sin_port=htons(PORT);
    srv_addr_in.sin_family=AF_INET;

    int sock=make_socket();
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
        
        sendto(sock, &(buf[0]),buf_len, 0, ((struct sockaddr *)&srv_addr_in), sizeof(srv_addr_in));
        printf("data was sent\n");
    }
}