#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#define N 1024

int main(int argc, char *argv[]){
    if (argc!=3){
        printf("Usage: %s <IP> <port>\n",argv[0]);
        exit(1);
    }

    int s=socket(PF_INET,SOCK_STREAM,0);
    if (s==-1){
        perror("socket() error");
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family=AF_INET; //IPv4
    addr.sin_addr.s_addr=inet_addr(argv[1]); //IPaddr
    addr.sin_port=htons(atoi(argv[2])); //port

    int ret;

    ret=connect(s,(struct sockaddr *)&addr, sizeof(addr));
    if (ret==-1){
        perror("connect() error");
        exit(1);
    }

    char data[N];
    ssize_t n;

    while ((n=recv(s,data,N,0))>0){
        if (write(1, data, n) != n) {
            perror("write() error");
            break;
        }
    }

    shutdown(s,SHUT_WR);
    close(s);
    return 0;
}
