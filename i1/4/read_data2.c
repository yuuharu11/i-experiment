#include <unistd.h>
#include <fcntl.h> 
#include <stdlib.h>
#include <stdio.h>
#define N 256
int main(int argc,char **argv){
    if (argc!=2){
        printf("入力の形式が違います\n");
        exit(1);
    }
    int fd =open(argv[1],O_RDONLY);
    short data;
    ssize_t n;
    short i=0;
    while(1){
        n=read(fd,&data,2);
        if (n==-1){
            perror("read");
            exit(1);
        }
        if (n==0) break;
        printf("%d %d\n",i,data);
        i+=1;
    }
    close(fd);
    return 0;
}
