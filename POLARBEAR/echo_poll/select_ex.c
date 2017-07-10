#include <fcntl.h>
#include <unistd.h>
#include <sys/unistd.h>
#include <sys/select.h>

int main(void) 
{
    fd_set fdset;
    FD_ZERO(&fdset);
    int fd = open("/dev/echo", O_RDWR);
    FD_SET(fd, &fdset);
    select(FD_SETSIZE, &fdset, &fdset, NULL, NULL);
    close(fd);
}
