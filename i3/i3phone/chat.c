#include "chat.h"

extern pthread_mutex_t chat_mutex;
extern int chat_socket;
extern char sender[MAX_USERNAME_LEN];

void *send_chat(void *arg) {
    char input[MAX_MESSAGE_LEN];
    Message msg;

    while (1) {
        if (fgets(input, MAX_MESSAGE_LEN, stdin) == NULL) {
            perror("fgets error");
            break;
        }
        input[strcspn(input, "\n")] = '\0'; // remove 

        pthread_mutex_lock(&chat_mutex);
        strcpy(msg.sender, sender); 
        strcpy(msg.message, input);
        if (send(chat_socket, &msg, sizeof(Message), 0) == -1) {
            perror("チャット送信エラー");
            pthread_mutex_unlock(&chat_mutex);
            break;
        }
        pthread_mutex_unlock(&chat_mutex);
    }

    close(chat_socket);
    pthread_exit(NULL);
}

void *receive_chat(void *arg) {
    Message msg;

    while (recv(chat_socket, &msg, sizeof(Message), 0) > 0) {
        pthread_mutex_lock(&chat_mutex);
        printf("[%s] %s\n", msg.sender, msg.message); // display user_name,message
        pthread_mutex_unlock(&chat_mutex);
    }

    printf("disconnected\n");

    close(chat_socket);
    pthread_exit(NULL);
}
