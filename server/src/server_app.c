#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "packet_handler.h"
#include "room_manager.h"
#include "server_protocol.h"

#define PORT 10000
#define MAX_CLIENTS 100

pthread_mutex_t room_mutex = PTHREAD_MUTEX_INITIALIZER;

// クライアントハンドリングスレッド
void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);

    printf("[+] New client connected: socket %d\n", client_sock);

    if (handle_client_session(client_sock) != 0) {
        printf("[-] Client session ended with error or disconnect: socket %d\n",
               client_sock);
    }

    close(client_sock);
    pthread_exit(NULL);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addrlen = sizeof(client_addr);

    // ルームシステム初期化
    init_rooms();

    // ソケット作成
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // バインド
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // リッスン
    if (listen(server_sock, 5) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr,
                             &client_addrlen);
        if (client_sock < 0) {
            perror("accept failed");
            continue;
        }

        pthread_t tid;
        int *pclient = malloc(sizeof(int));
        *pclient = client_sock;
        pthread_create(&tid, NULL, handle_client, pclient);
        pthread_detach(tid);
    }

    close(server_sock);
    return 0;
}
