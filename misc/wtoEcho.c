#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    char mes[] = "message";
    int fd = open("/dev/echo", O_RDWR);
    write(fd, mes, sizeof(mes));
    close(fd);
    return 0;
}
