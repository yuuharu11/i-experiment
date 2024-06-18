#ifndef COMMON_H
#define COMMON_H

#define N 1024
#define TIMEOUT 20
#define MAX_MESSAGE_LEN 256
#define MAX_USERNAME_LEN 50

typedef struct {
    char sender[MAX_USERNAME_LEN]; // username
    char message[MAX_MESSAGE_LEN];
} Message;

#endif // COMMON_H
