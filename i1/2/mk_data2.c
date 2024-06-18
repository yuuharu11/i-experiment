#include <unistd.h>
#include <fcntl.h> 
#include <stdlib.h>
#include <stdio.h>
int main(int argc,char **argv){
    if (argc!=2){
        printf("入力の形式が違います\n");
        exit(1);
    }
    int fd =open(argv[1],O_WRONLY|O_CREAT|O_TRUNC,0644);
    if (fd==-1){
        perror("open");
        exit(1);
    }

    unsigned char data[6];
    data[0]=228;
    data[1]=186;
    data[2]=186;
    data[3]=229;
    data[4]=191;
    data[5]=151;
    ssize_t bytes_written = write(fd, data, 6); 
    if (bytes_written == -1) { 
        perror("write"); 
        close(fd);
        exit(1); 
    }
    write(fd,data,6);
    close(fd);
    return 0;
}
