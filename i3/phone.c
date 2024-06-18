#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h> 
#include <pthread.h>

typedef struct {
    int socket;
    struct sockaddr_in address;
    socklen_t addr_len;
} client_t;

client_t *clients[10];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_client(client_t *client) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < 10; ++i) {
        if (clients[i] == NULL) {
            clients[i] = client;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int client_socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < 10; ++i) {
        if (clients[i] != NULL && clients[i]->socket == client_socket) {
            close(clients[i]->socket); // お試し
            free(clients[i]); // お試し
            clients[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void broadcast(int sender_socket, unsigned char *data, ssize_t length) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < 10; ++i) {
        if (clients[i] != NULL && clients[i]->socket != sender_socket) {
            send(clients[i]->socket, data, length, 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *client_handler(void *arg) {
    client_t *client = (client_t *)arg;
    unsigned char buffer[4096];
    ssize_t read_bytes;

    while ((read_bytes = recv(client->socket, buffer, sizeof(buffer), 0)) > 0) {
        broadcast(client->socket, buffer, read_bytes);
    }

    close(client->socket);
    remove_client(client->socket);
    free(client);
    return NULL;
}

void *speaking(void *num) {
    int s = *(int *)num;
    FILE *fp = popen("rec -t raw -b 16 -c 1 -e s -r 44100 -", "r");
    if (fp == NULL) {
        perror("popen error");
        exit(EXIT_FAILURE);
    }
    int fd = fileno(fp);
    unsigned char data[4096];
    ssize_t read_bytes;

    while ((read_bytes = read(fd, data, sizeof(data))) > 0) {
        if (send(s, data, read_bytes, 0) == -1) {
            perror("send error");
            close(s);
            exit(EXIT_FAILURE);
        }
        usleep(10000); //お試し
    }
    pclose(fp);
    shutdown(s, SHUT_WR);
    return NULL;
}

void *listening(void *num) {
    int s = *(int *)num;
    FILE *fp = popen("play -t raw -b 16 -c 1 -e s -r 44100 -", "w");
    if (fp == NULL) {
        perror("popen error");
        exit(EXIT_FAILURE);
    }
    int fd = fileno(fp);
    unsigned char data[4096];
    ssize_t read_bytes;

    while ((read_bytes = recv(s, data, sizeof(data), 0)) > 0) {
        if (write(fd, data, read_bytes) == -1) {
            perror("write error");
            close(s);
            exit(EXIT_FAILURE);
        }
        if (read_bytes == 0) {
            printf("disconnected\n");
            break;
        } else if (read_bytes == -1) {
            perror("recv error");
            break;
        }
    }

    shutdown(s, SHUT_WR);
    pclose(fp);
    return NULL;
}


int main(int argc, char **argv) {
    FILE *fp;
    char* IP;
    int port;
    int check; //0ならサーバー、1ならクライアント
    int threadcheck = 0;


    if (argc == 2){
        port = atoi(argv[1]);
        check = 0;
    }else if (argc == 3){
        IP = argv[1];
        port = atoi(argv[2]);
        check = 1;
    }else {
        perror("commandline error");
        exit(EXIT_FAILURE);
    }

    

    if (check == 0){
        int ss = socket(PF_INET, SOCK_STREAM, 0);
        if (ss == -1) {
            perror("socket error");
            exit(EXIT_FAILURE);
        }

        struct sockaddr_in addr; /* 最終的にbind に渡すアドレス情報 */ 
        addr.sin_family = AF_INET; /* このアドレスはIPv4 アドレスです */ 
        addr.sin_port = htons(port); /* ポート...で待ち受けしたいです */ 
        addr.sin_addr.s_addr = INADDR_ANY;
        
        if (bind(ss, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
            perror("bind error");
            exit(EXIT_FAILURE);
        }

        if (listen(ss, 10) == -1) {
            perror("listen error");
            exit(EXIT_FAILURE);
        }

        while (1) {
            client_t *client = (client_t *)malloc(sizeof(client_t));
            client->addr_len = sizeof(client->address);
            client->socket = accept(ss, (struct sockaddr *)&client->address, &client->addr_len);

            if (client->socket == -1) {
                perror("accept error");
                free(client);
                continue;
            }

            add_client(client);
            pthread_t thread;
            pthread_create(&thread, NULL, client_handler, client);
            pthread_detach(thread);
        }

        close(ss);
    }else if (check == 1) {
        int s = socket(PF_INET, SOCK_STREAM, 0);
        if (s == -1) {
            perror("socket error");
            exit(EXIT_FAILURE);
        }
        struct sockaddr_in addr;
        addr.sin_family = AF_INET; /* これはIPv4 のアドレスです */
        inet_aton(IP, &addr.sin_addr);
        addr.sin_port = htons(port); /* ポートは...です */

        if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
            perror("connect error");
            exit(EXIT_FAILURE);
        }

        pthread_t speaking_thread, listening_thread;
        pthread_create(&speaking_thread, NULL, speaking, &s);
        pthread_create(&listening_thread, NULL, listening, &s);

        threadcheck = pthread_join(speaking_thread, NULL);
        if (threadcheck != 0) {
            perror("speaking error");
            close(s);
            exit(EXIT_FAILURE);
        }
        threadcheck = pthread_join(listening_thread, NULL);
        if (threadcheck != 0) {
            perror("listening error");
            close(s);
            exit(EXIT_FAILURE);
        }
        close(s);
    }

    
    return 0;
}

