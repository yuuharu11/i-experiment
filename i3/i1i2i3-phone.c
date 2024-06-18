#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define N 1024

void *send_audio(void *arg) {
    int s = *(int *)arg;
    free(arg);
    unsigned char data[N];
    ssize_t n;

    while ((n = read(0, data, N)) > 0) {
        if (write(s, data, n) != n) {
            perror("write() error");
            break;
        }
    }
    close(s);
    return 0;
}

void *receive_audio(void *arg) {
    int s = *(int *)arg;
    free(arg);
    unsigned char data[N];
    ssize_t n;

    while ((n = recv(s, data, N, 0)) > 0) {
        if (write(1, data, n) != n) {
            perror("write");
            break;
        }
    }

    if (n == -1) {
        perror("recv");
    }
    shutdown(s,SHUT_WR);
    close(s);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    int ss = socket(PF_INET, SOCK_STREAM, 0);
    pthread_t send_thread, receive_thread;
    if (ss == -1) {
        perror("socket() error");
        exit(1);
    }

    if (argc == 3) { //クライアント
        addr.sin_addr.s_addr = inet_addr(argv[1]);
        addr.sin_port = htons(atoi(argv[2]));
        printf("Attempting to connect to %s:%d\n", argv[1], atoi(argv[2]));
        //connect
        if (connect(ss, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
            perror("connect() error");
            exit(1);
        }

        
        //send_thread
        int *send_sock=malloc(sizeof(int));
        *send_sock=ss;
        if (pthread_create(&send_thread, NULL, send_audio, send_sock) != 0) {
            perror("pthread_create(send_thread)");
            exit(1);
        }else{
            puts("send thread");
        }

        //receive_thread
        int *receive_sock=malloc(sizeof(int));
        *receive_sock=ss;
        if (pthread_create(&receive_thread, NULL, receive_audio, receive_sock) != 0) {
            perror("pthread_create(receive_thread)");
            exit(1);
        }else{
            puts("receive thread");
        }

        pthread_join(receive_thread, NULL);
        puts("receive exit");
        pthread_join(send_thread, NULL);
        puts("send exit");
    }else if(argc==2){ //サーバ
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(atoi(argv[1]));
        if (bind(ss,(struct sockaddr *)&addr, sizeof(addr))==-1){
            perror("bind");
            exit(1);
        }

        //listen
        if (listen(ss,10)==-1){
            perror("listen");
            exit(1);
        }

        while(1){
            //accept
            struct sockaddr_in client_addr;
            socklen_t len = sizeof(struct sockaddr_in);
            int s = accept(ss, (struct sockaddr *)&client_addr, &len);
            if (s==-1){
                perror("accept");
                exit(1);
            }

            pthread_t send_thread, receive_thread;

            //send_thread
            int *send_sock=malloc(sizeof(int));
            *send_sock=s;
            if (pthread_create(&send_thread, NULL, send_audio, send_sock) != 0) {
                perror("pthread_create(send_thread)");
                exit(1);
            }else{
                puts("send thread");
            }

            //receive_thread
            int *receive_sock=malloc(sizeof(int));
            *receive_sock=s;
            if (pthread_create(&receive_thread, NULL, receive_audio, receive_sock) != 0) {
                perror("pthread_create(receive_thread)");
                exit(1);
            }else{
                puts("receive thread");
            }
        }
        pthread_join(receive_thread, NULL);
        puts("receive exit");
        pthread_join(send_thread, NULL);
        puts("send exit");
        close(ss);
    } else{
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }
    return 0;
}
