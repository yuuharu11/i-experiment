#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <complex.h>
#include <math.h>
#include <string.h>

#define N 4096

typedef short sample_t;

void die(char * s) {
  perror(s);
  exit(1);
}

/* fd から 必ず n バイト読み, bufへ書く.
   n バイト未満でEOFに達したら, 残りは0で埋める.
   fd から読み出されたバイト数を返す */
ssize_t read_n(int fd, ssize_t n, void * buf) {
  ssize_t re = 0;
  while (re < n) {
    ssize_t r = read(fd, buf + re, n - re);
    if (r == -1) die("read");
    if (r == 0) break;
    re += r;
  }
  memset(buf + re, 0, n - re);
  return re;
}

/* fdへ, bufからnバイト書く */
ssize_t write_n(int fd, ssize_t n, void * buf) {
  ssize_t wr = 0;
  while (wr < n) {
    ssize_t w = write(fd, buf + wr, n - wr);
    if (w == -1) die("write");
    wr += w;
  }
  return wr;
}

/* 標本(整数)を複素数へ変換 */
void sample_to_complex(sample_t * s, complex double * X, long n) {
  long i;
  for (i = 0; i < n; i++) X[i] = s[i];
}

/* 複素数を標本(整数)へ変換. 虚数部分は無視 */
void complex_to_sample(complex double * X, sample_t * s, long n) {
  long i;
  for (i = 0; i < n; i++) {
    s[i] = creal(X[i]);
  }
}

void fft_r(complex double * x, complex double * y, long n, complex double w) {
  if (n == 1) {
    y[0] = x[0];
  } else {
    complex double W = 1.0;
    long i;
    for (i = 0; i < n / 2; i++) {
      y[i] = (x[i] + x[i + n / 2]);    // 偶数行
      y[i + n / 2] = W * (x[i] - x[i + n / 2]);    // 奇数行
      W *= w;
    }
    fft_r(y, x, n / 2, w * w);
    fft_r(y + n / 2, x + n / 2, n / 2, w * w);
    for (i = 0; i < n / 2; i++) {
      y[2 * i] = x[i];
      y[2 * i + 1] = x[i + n / 2];
    }
  }
}

void fft(complex double * x, complex double * y, long n) {
  long i;
  double arg = 2.0 * M_PI / n;
  complex double w = cos(arg) - 1.0j * sin(arg);
  fft_r(x, y, n, w);
  for (i = 0; i < n; i++) {
    y[i] /= n;
  }
}

void ifft(complex double * y, complex double * x, long n) {
  double arg = 2.0 * M_PI / n;
  complex double w = cos(arg) + 1.0j * sin(arg);
  fft_r(y, x, n, w);
}

void apply_bandpass_filter(complex double * Y, long n, double start, double end) {
  long i;
  for (i = 0; i < n; i++) {
    double freq = ((double) i / n) * 44100;    // サンプリング周波数を考慮
    // 指定された範囲外の周波数成分をゼロにする
    if (freq < start || freq > end) {
      Y[i] = 0.0;
    }
  }
}

void * send_audio(void * arg) {
  int s = * (int * ) arg;
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

void * receive_audio(void * arg) {
  int s = * (int * ) arg;
  free(arg);
  sample_t buf[N];
  complex double X[N];
  complex double Y[N]; 
  double start = 20000.0; // バンドパスフィルタの開始周波数
  double end = 20.0; // バンドパスフィルタの終了周波数
  ssize_t n;

  while ((n = recv(s, buf, N, 0)) > 0) {
    sample_to_complex(buf,X,N);
    fft(X,Y,N);
    apply_bandpass_filter(Y,N,start,end);
    ifft(Y,X,N);
    complex_to_sample(X,buf,N);

    if (write(1, buf, n) != n) {
      perror("write");
      break;
    }
  }

  if (n == - 1) {
    perror("recv");
  }
  shutdown(s, SHUT_WR);
  close(s);
  pthread_exit(NULL);
}

int main(int argc, char * argv[]) {
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  int ss = socket(PF_INET, SOCK_STREAM, 0);
  pthread_t send_thread, receive_thread;
  if (ss == - 1) {
    perror("socket() error");
    exit(1);
  }

  if (argc == 3) { // クライアント
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(atoi(argv[2]));
    printf("Attempting to connect to %s:%d\n", argv[1], atoi(argv[2]));
    // connect
    if (connect(ss, (struct sockaddr * ) & addr, sizeof(addr)) == - 1) {
      perror("connect() error");
      exit(1);
    }

    long n = N; // FFTのサイズ
    // スレッド作成
    int * send_sock = malloc(sizeof(int));
    * send_sock = ss;
    if (pthread_create( & send_thread, NULL, send_audio, send_sock) != 0) {
      perror("pthread_create(send_thread)");
      exit(1);
    } else {
      puts("send thread");
    }

    int * receive_sock = malloc(sizeof(int));
    * receive_sock = ss;
    if (pthread_create( & receive_thread, NULL, receive_audio, receive_sock) != 0) {
      perror("pthread_create(receive_thread)");
      exit(1);
    } else {
      puts("receive thread");
    }

    pthread_join(receive_thread, NULL);
    puts("receive exit");
    pthread_join(send_thread, NULL);
    puts("send exit");

  } else if (argc == 2) { // サーバ
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

        //accept
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(struct sockaddr_in);
        int s = accept(ss, (struct sockaddr *)&client_addr, &len);
        if (s==-1){
            perror("accept");
            exit(1);
        }

        while(1){
            //send_thread
            int *send_sock=malloc(sizeof(int));
            *send_sock=s;
            if (pthread_create(&send_thread, NULL, send_audio, send_sock) != 0) {
                perror("pthread_create(send_thread)");
                exit(1);
            }else{
                puts("send thread");
            }

            //create_thread
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
