#include <stdio.h>
#include <math.h>
#include <stdlib.h>
int main(int argc,char **argv){
    int A = atoi(argv[1]); 
    double f = atof(argv[2]);
    double n = atof(argv[3]);
    double sample_rate=44100;
    for (int i=0;i<n;i++){
        double t=(double)i/sample_rate;
        short x_t=(short)A*sin(2.0*M_PI*f*t);
        fwrite(&x_t, sizeof(x_t), 1, stdout); 
    }
    return 0;
}
