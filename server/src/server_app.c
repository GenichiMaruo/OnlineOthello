#include "client_handler.h"     // クライアントハンドラ
#include "client_management.h"  // クライアント管理
#include "room_management.h"    // 部屋管理
#include "server_common.h"      // 共通定義

// --- main関数 ---
int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // サーバーと部屋の初期化
    initialize_clients();  // client_management.c
    initialize_rooms();    // room_management.c

    // ソケット作成
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // アドレス再利用設定 (任意)
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
        0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        // 致命的ではない場合が多いので続行してもよい
    }

    // サーバーアドレス設定
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    // バインド
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) <
        0) {
        perror("bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // リッスン
    if (listen(server_sock, 5) < 0) {  // backlogを調整可能 (例: 10)
        perror("listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", SERVER_PORT);

    // クライアント接続受付ループ
    while (1) {
        client_sock =
            accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("accept failed");
            // accept のエラーはリカバリ可能かもしれないが、ループを続ける
            continue;
        }

        printf("Client connected from %s:%d (assigned sockfd: %d)\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),
               client_sock);

        // クライアント情報を追加
        int client_index = add_client(client_sock, client_addr);

        if (client_index >= 0) {
            // スレッドに渡す引数として sockfd のポインタを作成
            // ClientInfo構造体全体を渡すより、変更される可能性が少ないsockfdを渡す方が安全
            int* client_sock_ptr = malloc(sizeof(int));
            if (client_sock_ptr == NULL) {
                perror("Failed to allocate memory for client socket argument");
                remove_client(client_sock);  // 追加したクライアント情報を削除
                close(client_sock);
                continue;  // 次の接続を待つ
            }
            *client_sock_ptr = client_sock;

            // ハンドラースレッドを作成 (client_handler.c の関数を呼び出し)
            pthread_t thread_id;
            if (pthread_create(&thread_id, NULL, handle_client,
                               (void*)client_sock_ptr) != 0) {
                perror("pthread_create failed");
                free(client_sock_ptr);       // メモリ解放
                remove_client(client_sock);  // 追加したクライアント情報を削除
                close(client_sock);
            } else {
                // スレッドIDをClientInfoに保存する場合
                // (add_client内で行う方が良いかも)
                // pthread_mutex_lock(&clients_mutex);
                // clients[client_index].thread_id = thread_id;
                // pthread_mutex_unlock(&clients_mutex);

                // スレッドはデタッチされるので join しない
                printf("Handler thread created for client sockfd %d\n",
                       client_sock);
            }

        } else {
            fprintf(
                stderr,
                "Failed to add client (server full?): closing connection %d\n",
                client_sock);
            // TODO: サーバー満員通知をクライアントに送信する (オプション)
            // Message full_msg; full_msg.type = MSG_ERROR_NOTICE; ...
            // sendMessage(...);
            close(client_sock);
        }
    }  // end while(1)

    // 通常はここに到達しないが、終了処理
    printf("Shutting down server...\n");
    close(server_sock);
    // TODO: 残っているクライアントへの通知、スレッドの終了待ち、リソース解放
    // (例: 全ての部屋を閉鎖、ミューテックスの破棄など)
    // for (int i=0; i<MAX_ROOMS; ++i)
    // pthread_mutex_destroy(&rooms[i].room_mutex);
    // pthread_mutex_destroy(&rooms_mutex);
    // pthread_mutex_destroy(&clients_mutex);

    return 0;
}