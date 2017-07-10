#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

int main(void){
    char mes[] = "This is a message.\n";
    int fd = open("/dev/stdout", O_RDWR);
    write(fd, mes, sizeof(mes));
    close(fd);
    return 0;
}

