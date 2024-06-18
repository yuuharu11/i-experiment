#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define SAMPLING_RATE 44100
#define QUANTIZATION_BITS 16
#define NOTE_DURATION_SAMPLES 13230 // 0.3秒分のサンプル数


double frequency= 440.00;//ラの周波数



int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <number of notes>\n", argv[0]);
        return 1;
    }
    int A = atoi(argv[1]);
    int n = atoi(argv[2]); // 音符の数を取得

    for (int i = 0; i < n; i++) {
        if (i==0){
            frequency = frequency*pow(2.0,(1.0/4));
        }else if (i%7==3 || i%7==0){
            frequency = frequency*pow(2.0,(1.0/12));
        }else{
            frequency = frequency*pow(2.0,(1.0/6));
        }

        // 0から1の範囲でサイン波を生成
        for (int j = 0; j < NOTE_DURATION_SAMPLES; j++) {
            double t = (double)j / SAMPLING_RATE;
            double x_t = A*sin(2 * M_PI * frequency * t);
            int16_t sample = (int16_t)(x_t * 32767); // 16ビットで量子化
            fwrite(&sample, sizeof(sample), 1, stdout); // 標準出力に書き込み
        }
    }

    return 0;
}
