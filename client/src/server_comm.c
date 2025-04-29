// server_comm.c
#include "server_comm.h"

#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

int connect_to_server(const char *addr, uint16_t port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) die("socket");
    struct sockaddr_in srv = {0};
    srv.sin_family = AF_INET;
    srv.sin_port = htons(port);
    if (!inet_aton(addr, &srv.sin_addr)) die("invalid server address");
    if (connect(sock, (struct sockaddr *)&srv, sizeof(srv)) < 0)
        die("connect_to_server");
    return sock;
}

int server_send_packet(int sock, const PacketHeader *hdr, const void *payload) {
    if (safe_send(sock, hdr, sizeof(*hdr)) < 0) return -1;
    if (hdr->length > 0 && safe_send(sock, payload, hdr->length) < 0) return -1;
    return 0;
}

int server_recv_packet(int sock, PacketHeader *hdr, void *payload) {
    if (safe_recv(sock, hdr, sizeof(*hdr)) <= 0) return -1;
    if (hdr->length > 0 && safe_recv(sock, payload, hdr->length) <= 0)
        return -1;
    return 0;
}
