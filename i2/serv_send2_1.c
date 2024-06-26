#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#define N 1024
int main(int argc, char *argv[]){
    if (argc!=2){
        printf("Usage: %s <IP> <port>\n",argv[0]);
        exit(1);
    }
    unsigned char data[N];
    int ss=socket(AF_INET,SOCK_STREAM,0);
    if (ss==-1){
        perror("socket");
        exit(1);
    }

    //bind
    struct sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_port=htons(atoi(argv[1]));
    addr.sin_addr.s_addr=INADDR_ANY;
    if (bind(ss,(struct sockaddr *)&addr, sizeof(addr))==-1){
        perror("bind");
        close(ss);
        exit(1);
    }

    //listen
    if (listen(ss,10)==-1){
        perror("listen");
        close(ss);
        exit(1);
    }

    //accept
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    int s = accept(ss, (struct sockaddr *)&client_addr, &len);
    if (s==-1){
        perror("accept");
        close(ss);
        exit(1);
    }
    close(ss);
    ssize_t n;

    FILE *rec_output = popen("rec -t raw -b 16 -c 1 -e s -r 44100 -", "r");
    if (!rec_output) {
        perror("popen");
        close(s);
        exit(1);
    }

    while (1) {
        n = fread(data, 1, 1, rec_output);
        if (n == -1){ 
            perror("read"); 
            exit(1); 
        }
        if (n == 0) break; 
        if (write(s, data, 1) == -1) {
            perror("write");
            break;
        }
    }

    if (n == -1) {
        perror("fread");
    }
    
    pclose(rec_output);
    close(s);
    return 0;
}