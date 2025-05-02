#include "client_management.h"

// --- グローバル変数定義 ---
ClientInfo clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// --- クライアントリスト初期化 ---
void initialize_clients() {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        clients[i].sockfd = -1;  // -1は空きスロットを示す
        clients[i].roomId = -1;
        clients[i].playerColor = 0;
    }
    pthread_mutex_unlock(&clients_mutex);
    printf("Client list initialized.\n");
}

// --- クライアント管理 ---
int find_client_index(int sockfd) {
    // clients_mutex は呼び出し元でロック/アンロックされる想定が多いが、
    // この関数単体で使われる可能性も考慮し、ここではロックしない。
    // 必要なら呼び出し側でロックする。
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].sockfd == sockfd) {
            return i;
        }
    }
    return -1;  // 見つからない
}

// sockfdからClientInfoポインタを取得 (clients_mutexで保護)
ClientInfo* get_client_info(int sockfd) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].sockfd == sockfd) {
            // 見つかった場合、mutexをアンロックせずにポインタを返す
            // 呼び出し元が使い終わったらアンロックする必要があるか、
            // またはコピーを返すか、設計による。
            // ここでは単純化のため、ロックしたままポインタを返す。
            // ただし、デッドロックのリスクがあるので注意。
            // より安全なのはインデックスを返し、呼び出し元で再度ロックしてアクセス。
            // もしくは、必要な情報をコピーして返す。
            // ここではインデックスを返す find_client_index を使うことを推奨。
            // この関数は例として残すが、利用は慎重に。
            //
            // find_client_index を使う実装に変更:
            int index = find_client_index(sockfd);
            if (index != -1) {
                pthread_mutex_unlock(&clients_mutex);  // アンロックしてから返す
                return &clients[index];  // ポインタを返す場合は注意が必要
            } else {
                pthread_mutex_unlock(&clients_mutex);
                return NULL;
            }
            // 元の実装（ロックしたまま返すのは危険）
            // pthread_mutex_unlock(&clients_mutex); // これは間違い
            // return &clients[i];
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return NULL;  // 見つからない
}

int add_client(int sockfd, struct sockaddr_in addr) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].sockfd == -1) {
            clients[i].sockfd = sockfd;
            clients[i].addr = addr;
            clients[i].roomId = -1;  // 初期状態はロビー
            clients[i].playerColor = 0;
            clients[i].thread_id =
                pthread_self();  // スレッドIDを記録（オプション）
            pthread_mutex_unlock(&clients_mutex);
            printf("Client %d added (sockfd: %d).\n", i, sockfd);
            return i;  // 追加したインデックスを返す
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    fprintf(stderr, "Failed to add client: server full.\n");
    return -1;  // 満員
}

void remove_client(int sockfd) {
    pthread_mutex_lock(&clients_mutex);
    int index = find_client_index(sockfd);  // mutex内で呼ぶ
    if (index != -1) {
        printf("Removing client %d (sockfd: %d).\n", index,
               clients[index].sockfd);
        clients[index].sockfd = -1;  // スロットを空ける
        clients[index].roomId = -1;
        clients[index].playerColor = 0;
        // 必要なら他の情報もクリア
    } else {
        fprintf(stderr,
                "Attempted to remove non-existent client (sockfd: %d).\n",
                sockfd);
    }
    pthread_mutex_unlock(&clients_mutex);
}