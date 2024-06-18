#include <unistd.h>
#include <fcntl.h> 
int main(){
    int fd =open("my_data",O_WRONLY|O_CREAT|O_TRUNC,0644);
    unsigned char data[256];
    int i;
    for (i=0;i<256;i=i+1)
        data[i]=i;
    write(fd,data,256);
    close(fd);
    return 0;
}
