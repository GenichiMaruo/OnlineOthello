// utils.c
#include "utils.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

void die(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

void *safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) die("safe_malloc failed");
    return ptr;
}

ssize_t safe_send(int sockfd, const void *buf, size_t len) {
    ssize_t total = 0;
    const char *p = buf;
    while (total < (ssize_t)len) {
        ssize_t n = send(sockfd, p + total, len - total, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        total += n;
    }
    return total;
}

ssize_t safe_recv(int sockfd, void *buf, size_t len) {
    ssize_t total = 0;
    char *p = buf;
    while (total < (ssize_t)len) {
        ssize_t n = recv(sockfd, p + total, len - total, 0);
        if (n <= 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        total += n;
    }
    return total;
}
