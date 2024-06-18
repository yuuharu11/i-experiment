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
#include <sox.h>

#define N 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int ss = socket(AF_INET, SOCK_STREAM, 0);
    if (ss == -1) {
        perror("socket");
        exit(1);
    }

    // bind
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(ss, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(ss);
        exit(1);
    }

    // listen
    if (listen(ss, 10) == -1) {
        perror("listen");
        close(ss);
        exit(1);
    }

    // accept
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    int s = accept(ss, (struct sockaddr *)&client_addr, &len);
    if (s == -1) {
        perror("accept");
        close(ss);
        exit(1);
    }
    close(ss);

    size_t n = 48000 * 2; // 1秒分のデータ量

    sox_sample_t *buf = malloc(sizeof(sox_sample_t) * n);
    sox_init();
    sox_format_t *ft = sox_open_read("default", NULL, NULL, "pulseaudio");
    if (!ft) {
        fprintf(stderr, "Error opening audio device\n");
        close(s);
        exit(1);
    }

    ssize_t m;
    short *sbuf = NULL; // sbufの初期化をここで行う

    while ((m = sox_read(ft, buf, n)) > 0) {
        sbuf = malloc(sizeof(short) * n/2); 

        for (size_t i = 0; i < n/2; i++) {
            sbuf[i] = (short)(buf[2*i] >> 16); // 32ビットから16ビットにスケーリング
        }

        ssize_t l = write(s, sbuf, m * sizeof(short)); 
        free(sbuf); 

        if (l == -1) {
            perror("write");
            break;
        }
    }

    if (m == -1) {
        perror("sox_read");
    }

    sox_close(ft);
    close(s);
    return 0;
}
