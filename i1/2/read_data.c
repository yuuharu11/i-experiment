#include <unistd.h>
#include <fcntl.h> 
#include <stdlib.h>
#include <stdio.h>
int main(int argc,char **argv){
    if (argc!=2){
        printf("入力の形式が違います\n");
        exit(1);
    }
    int fd =open(argv[1],O_RDONLY);
    if (fd==-1){
        perror("open");
        exit(1);
    }

    unsigned char *data = malloc(1);
    if (data==NULL){
        perror("malloc");
        exit(1);
    }

    ssize_t n;
    int i=0;
    while(1){
        n=read(fd,&data[i],1);
        if (n==-1){
            perror("read");
            exit(1);
        }
        if (n==0) break;
        printf("%d %d\n",i,*data);
        i++;
        unsigned char *tmp = realloc(data, i + 1);
        if (tmp == NULL){
            perror("realloc");
            free(data);
            exit(1);
        }
        data=tmp;
    }

    for (int j=0; j < i; j++){
        printf("%d %d\n",j,data[j]);
    }

    close(fd);
    free(data);
    return 0;
}
