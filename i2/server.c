#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define BUF_SIZE 1024

void *send_audio(void *arg) {
    int s = *(int *)arg;
    unsigned char data[BUF_SIZE];
    FILE *rec_output = popen("rec -t raw -b 16 -c 1 -e s -r 44100 -", "r");
    if (!rec_output) {
        perror("popen");
        close(s);
        pthread_exit(NULL);
    }

    ssize_t n;
    while ((n = fread(data, 1, BUF_SIZE, rec_output)) > 0) {
        if (send(s, data, n, 0) == -1) {
            perror("send");
            break;
        }
    }

    if (n == -1) {
        perror("fread");
    }

    pclose(rec_output);
    close(s);
    pthread_exit(NULL);
}

void *receive_audio(void *arg) {
    int s = *(int *)arg;
    unsigned char data[BUF_SIZE];
    ssize_t n;

    while ((n = recv(s, data, BUF_SIZE, 0)) > 0) {
        if (fwrite(data, 1, n, stdout) != n) {
            perror("fwrite");
            break;
        }
        fflush(stdout);
    }

    if (n == -1) {
        perror("recv");
    }

    close(s);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int ss = socket(PF_INET, SOCK_STREAM, 0);
    if (ss == -1) {
        perror("socket() error");
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(atoi(argv[1]));

    if (bind(ss, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind() error");
        close(ss);
        exit(1);
    }

    if (listen(ss, 1) == -1) {
        perror("listen() error");
        close(ss);
        exit(1);
    }

    printf("Waiting for incoming connection...\n");

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int s = accept(ss, (struct sockaddr *)&client_addr, &client_len);
    if (s == -1) {
        perror("accept() error");
        close(ss);
        exit(1);
    }

    close(ss);

    pthread_t send_thread, receive_thread;
    if (pthread_create(&send_thread, NULL, send_audio, &s) != 0) {
        perror("pthread_create(send_thread)");
        exit(1);
    }
    if (pthread_create(&receive_thread, NULL, receive_audio, &s) != 0) {
        perror("pthread_create(receive_thread)");
        exit(1);
    }

    pthread_join(send_thread, NULL);
    pthread_join(receive_thread, NULL);

    return 0;
}
