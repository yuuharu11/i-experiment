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

#define N 1024
#define TIMEOUT 20
#define MAX_MESSAGE_LEN 256
#define MAX_USERNAME_LEN 50

typedef struct {
    char sender[MAX_USERNAME_LEN]; // ユーザーネーム
    char message[MAX_MESSAGE_LEN];
} Message;

pthread_mutex_t chat_mutex = PTHREAD_MUTEX_INITIALIZER;
int chat_socket;
char sender[MAX_USERNAME_LEN]; // ユーザーネーム

void *send_audio(void *arg) {
    int s = *(int *)arg;
    free(arg);
    unsigned char data[N];
    ssize_t n;

    FILE *fp_rec = popen("rec -t raw -b 16 -c 1 -e s -r 44100 -", "r");
    if (!fp_rec) {
        perror("popen(rec)");
        close(s);
        pthread_exit(NULL);
    }

    while ((n = fread(data, 1, N, fp_rec)) > 0) {
        if (write(s, data, n) != n) {
            perror("write() error");
            break;
        }
    }

    fclose(fp_rec);
    close(s);
    pthread_exit(NULL);
}

void *receive_audio(void *arg) {
    int s = *(int *)arg;
    free(arg);
    unsigned char data[N];
    ssize_t n;

    FILE *fp = popen("play -t raw -b 16 -c 1 -e s -r 44100 -", "w");
    if (!fp) {
        perror("popen(play)");
        close(s);
        pthread_exit(NULL);
    }

    while ((n = recv(s, data, N, 0)) > 0) {
        if (fwrite(data, 1, n, fp) != n) {
            perror("fwrite(play)");
            break;
        }
    }

    if (n == -1) {
        perror("recv");
    }

    fclose(fp);
    shutdown(s, SHUT_WR);
    close(s);
    pthread_exit(NULL);
}

void *send_chat(void *arg) {
    char input[MAX_MESSAGE_LEN];
    Message msg;

    while (1) {
        if (fgets(input, MAX_MESSAGE_LEN, stdin) == NULL) {
            perror("fgets error");
            break;
        }
        input[strcspn(input, "\n")] = '\0'; // 改行文字を削除

        pthread_mutex_lock(&chat_mutex);
        strcpy(msg.sender, sender); // ユーザーネームをメッセージに追加
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
        printf("[%s] %s\n", msg.sender, msg.message); // 送信者のユーザーネームとメッセージを表示
        pthread_mutex_unlock(&chat_mutex);
    }

    close(chat_socket);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    int ss = socket(PF_INET, SOCK_STREAM, 0);
    pthread_t send_thread, receive_thread;
    pthread_t chat_send_thread, chat_receive_thread;
    char mode[10];
    char mode_1[10];

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

        printf("Select mode (call/chat): ");
        if (fgets(mode_1, sizeof(mode_1), stdin) == NULL) {
            perror("fgets error");
            printf("invalid input");
            exit(1);
        }
        mode_1[strcspn(mode_1, "\n")] = '\0'; // Remove newline character
        
        if (strcmp(mode_1, "call") == 0) {
            // 通話用の送受信スレッドの作成
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

            // スレッドの終了を待つ
            pthread_join(receive_thread, NULL);
            puts("Receive audio thread exited");
            pthread_join(send_thread, NULL);
            puts("Send audio thread exited");

            close(ss);
        } else if (strcmp(mode_1, "chat") == 0) {
            // チャットのみの処理を実行する
            chat_socket = s; // クライアント用のソケットを使用

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

        printf("Select mode (call/chat): ");
        if (fgets(mode_1, sizeof(mode_1), stdin) == NULL) {
            perror("fgets error");
            exit(1);
        }
        mode_1[strcspn(mode_1, "\n")] = '\0'; // Remove newline character

        if (strcmp(mode_1, "call") == 0) {
            // 通話用の送受信スレッドの作成
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

            // スレッドの終了を待つ
            pthread_join(receive_thread, NULL);
            puts("Receive audio thread exited");
            pthread_join(send_thread, NULL);
            puts("Send audio thread exited");

            close(ss);
        } else if (strcmp(mode_1, "chat") == 0) {
            // チャットのみの処理を実行する
            chat_socket = ss; // クライアント用のソケットを使用

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
    } else {
        printf("Invalid mode selected: %s\n", mode);
        exit(1);
    }

    return 0;
}
