#include "audio.h"

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
    } else if (n == 0) {
        printf("disconnected\n");
    }

    fclose(fp);
    shutdown(s, SHUT_WR);
    close(s);
    pthread_exit(NULL);
}
