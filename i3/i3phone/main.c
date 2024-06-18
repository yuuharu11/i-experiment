#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include "audio.h"
#include "chat.h"
#include "common.h"

pthread_mutex_t chat_mutex = PTHREAD_MUTEX_INITIALIZER;
int chat_socket;
char sender[MAX_USERNAME_LEN]; 

int main(int argc, char *argv[]) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    int ss = socket(PF_INET, SOCK_STREAM, 0);
    pthread_t send_thread, receive_thread;
    pthread_t chat_send_thread, chat_receive_thread;
    char mode[10];
    char mode_1[10];
    char client_username[MAX_USERNAME_LEN];

    if (ss == -1) {
        perror("socket() error");
        exit(1);
    }

    if (argc != 1) {
        printf("Usage: %s\n", argv[0]);
        exit(1);
    }
    printf("Enter username: ");
    if (fgets(sender, sizeof(sender), stdin) == NULL) {
        perror("fgets error");
        exit(1);
    }
    sender[strcspn(sender, "\n")] = '\0'; // Remove newline character

    printf("Select mode (server/client): ");
    if (fgets(mode, sizeof(mode), stdin) == NULL) {
        perror("fgets error");
        exit(1);
    }
    mode[strcspn(mode, "\n")] = '\0'; // Remove newline character

    if (strcmp(mode, "server") == 0) {
        if (argc != 1) {
            printf("Usage: %s\n", argv[0]);
            exit(1);
        }
        
        char input_port[10];

        printf("Select server port: ");
        if (fgets(input_port, sizeof(input_port), stdin) == NULL) {
            perror("fgets error");
            exit(1);
        }
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(atoi(input_port)); 

        // Select mode before accepting connection
        printf("Select mode (call/chat): ");
        if (fgets(mode_1, sizeof(mode_1), stdin) == NULL) {
            perror("fgets error");
            exit(1);
        }
        mode_1[strcspn(mode_1, "\n")] = '\0'; // Remove newline character

        printf("Server listening on port %d\n",atoi(input_port));

        // Bind to port
        if (bind(ss, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
            perror("bind");
            exit(1);
        }

        // Listen for incoming connections
        if (listen(ss, 10) == -1) {
            perror("listen");
            exit(1);
        }

        // Accept with timeout
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(struct sockaddr_in);
        int s;
        fd_set readfds;
        struct timeval tv;

        FD_ZERO(&readfds);
        FD_SET(ss, &readfds);

        tv.tv_sec = TIMEOUT;
        tv.tv_usec = 0;

        int ret = select(ss + 1, &readfds, NULL, NULL, &tv);
        if (ret == -1) {
            perror("select");
            close(ss);
            exit(1);
        } else if (ret == 0) {
            printf("Timeout occurred! No connections within %d seconds.\n", TIMEOUT);
            close(ss);
            exit(0);
        } else {
            s = accept(ss, (struct sockaddr *)&client_addr, &len);
            if (s == -1) {
                perror("accept");
                close(ss);
                exit(1);
            }
        }
        
        // Send the selected mode to the client
        if (send(s, mode_1, sizeof(mode_1), 0) == -1) {
            perror("send mode to client\n");
            close(s);
            close(ss);
            exit(1);
        }

        // Receive the username from client
        if (recv(s, client_username, sizeof(client_username), 0) == -1) {
            perror("recv username from client");
            close(s);
            close(ss);
            exit(1);
        }

        printf("success connect with %s!\n", client_username); 

        if (strcmp(mode_1, "call") == 0) {
            // create thread
            int *receive_sock = malloc(sizeof(int));
            *receive_sock = s;
            if (pthread_create(&receive_thread, NULL, receive_audio, receive_sock) != 0) {
                perror("pthread_create(receive_thread)");
                close(ss);
                exit(1);
            } else {
                puts("Receive audio thread created");
            }

            int *send_sock = malloc(sizeof(int));
            *send_sock = s;
            if (pthread_create(&send_thread, NULL, send_audio, send_sock) != 0) {
                perror("pthread_create(send_thread)");
                close(ss);
                exit(1);
            } else {
                puts("Send audio thread created");
            }

            // wait terminating thread
            pthread_join(receive_thread, NULL);
            puts("Receive audio thread exited");
            pthread_join(send_thread, NULL);
            puts("Send audio thread exited");

            close(ss);
        } else if (strcmp(mode_1, "chat") == 0) {
            // chat
            chat_socket = s; 

            if (pthread_create(&chat_send_thread, NULL, send_chat, NULL) != 0) {
                perror("pthread_create(chat_send_thread)");
                exit(1);
            }

            if (pthread_create(&chat_receive_thread, NULL, receive_chat, NULL) != 0) {
                perror("pthread_create(chat_receive_thread)");
                exit(1);
            }

            pthread_join(chat_send_thread, NULL);
            puts("chat_send thread exited");
            pthread_join(chat_receive_thread, NULL);
            puts("chat_receive thread exited");
        } else {
            printf("Invalid mode selected: %s\n", mode);
            exit(1);
        }
    } else if (strcmp(mode, "client") == 0) {
        char input_addr[20];
        char input_port[10];

        printf("Enter server IP address: ");
        if (fgets(input_addr, sizeof(input_addr), stdin) == NULL) {
            perror("fgets error");
            exit(1);
        }
        input_addr[strcspn(input_addr, "\n")] = '\0'; // Remove newline character

        printf("Enter server port: ");
        if (fgets(input_port, sizeof(input_port), stdin) == NULL) {
            perror("fgets error");
            exit(1);
        }
        input_port[strcspn(input_port, "\n")] = '\0'; // Remove newline character

        // Connect to server
        addr.sin_addr.s_addr = inet_addr(input_addr);
        addr.sin_port = htons(atoi(input_port));

        if (connect(ss, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
            perror("connect");
            exit(1);
        }
        // Send username to server
        if (send(ss, sender, sizeof(sender), 0) == -1) {
            perror("send username to server\n");
            close(ss);
            exit(1);
        }

        // Receive the selected mode from the server
        if (recv(ss, mode_1, sizeof(mode_1), 0) == -1) {
            perror("recv mode from server");
            close(ss);
            exit(1);
        }

        printf("success connect! \n");

        if (strcmp(mode_1, "call") == 0) {
            // create audio thread
            int *receive_sock = malloc(sizeof(int));
            *receive_sock = ss;
            if (pthread_create(&receive_thread, NULL, receive_audio, receive_sock) != 0) {
                perror("pthread_create(receive_thread)");
                close(ss);
                exit(1);
            } else {
                puts("Receive audio thread created");
            }

            int *send_sock = malloc(sizeof(int));
            *send_sock = ss;
            if (pthread_create(&send_thread, NULL, send_audio, send_sock) != 0) {
                perror("pthread_create(send_thread)");
                close(ss);
                exit(1);
            } else {
                puts("Send audio thread created");
            }

            // wait terminating thread
            pthread_join(receive_thread, NULL);
            puts("Receive audio thread exited");
            pthread_join(send_thread, NULL);
            puts("Send audio thread exited");

            close(ss);
        } else if (strcmp(mode_1, "chat") == 0) {
            // chat
            chat_socket = ss; 

            if (pthread_create(&chat_send_thread, NULL, send_chat, NULL) != 0) {
                perror("pthread_create(chat_send_thread)");
                exit(1);
            }

            if (pthread_create(&chat_receive_thread, NULL, receive_chat, NULL) != 0) {
                perror("pthread_create(chat_receive_thread)");
                exit(1);
            }

            pthread_join(chat_send_thread, NULL);
            puts("chat_send thread exited");
            pthread_join(chat_receive_thread, NULL);
            puts("chat_receive thread exited");
        } else {
            printf("Invalid mode received from server: %s\n", mode_1);
            exit(1);
        }
    } else {
        printf("Invalid mode selected: %s\n", mode);
        exit(1);
    }

    printf("Connection closed. Program terminating.\n");
    return 0;
}
