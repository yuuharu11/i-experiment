#include <unistd.h>
#include <fcntl.h> 
#include <stdlib.h>
#include <stdio.h>
int main(int argc,char **argv){
    if (argc!=3){
        printf("入力の形式が違います\n");
        exit(1);
    }
    int fd =open(argv[1],O_RDONLY);
    if (fd==-1){
        perror("open");
        exit(1);
    }

    short data;
    ssize_t n;
    int i=0;
    while(1){
        n=read(fd,&data,2);
        if (n==-1){
            perror("read");
            exit(1);
        }
        if (n==0) break;
        if (i%atoi(argv[2])==0){
            fwrite(&data, sizeof(data), 1, stdout);
        }
        i+=1;
    }
    close(fd);
    return 0;
}