// utils.h
#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <unistd.h>

void die(const char *message);
void *safe_malloc(size_t size);
ssize_t safe_send(int sockfd, const void *buf, size_t len);
ssize_t safe_recv(int sockfd, void *buf, size_t len);

#endif  // UTILS_H
