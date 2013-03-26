#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <stdlib.h>

void leave_raw_mode(struct termios *tio)
{
    if (tcsetattr(STDIN_FILENO, TCSADRAIN, tio) == -1) {
        perror("leave_raw_mode");
    }
}
void enter_raw_mode(struct termios *saved_tio)
{
    struct termios tio;
    tio = *saved_tio;

    tio.c_iflag |= IGNPAR;
    tio.c_iflag &= ~(ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXANY | IXOFF);
#ifdef IUCLC
    tio.c_iflag &= ~IUCLC;
#endif
    tio.c_lflag &= ~(ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHONL);
#ifdef IEXTEN
    tio.c_lflag &= ~IEXTEN;
#endif
    tio.c_oflag &= ~OPOST;
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSADRAIN, &tio) == -1) {
        perror("enter_raw_mode");
    }
}
int Max(int a, int b)
{
    return a > b ? a : b;
}
int open_socket(const char *port, const char *ip, int *client_fd)
{
    struct sockaddr_in addr;
    int fd;
    if((fd=socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 0;
    }

    memset((void*)&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(53432);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0) {
        perror("bind");
        return 0;
    }

    memset((void*)&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port));
    inet_aton(ip, &addr.sin_addr); 

    if(connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
        perror("connect");
        return 0;
    }

    *client_fd = fd;

    return 1;
}
int main(int argc, char **argv)
{
    int master_fd, sock_fd;
    struct termios tio;

    if(!open_socket(argv[2], argv[1], &sock_fd))
        _exit(1);

    if(-1 == tcgetattr(STDIN_FILENO, &tio)) {
        perror("tcgetattr");
        _exit(1);
    }

    char buf[256];
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    enter_raw_mode(&tio);

    for(;;) {
        int ret, i, size, status = 0;
        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(STDIN_FILENO, &read_set);
        FD_SET(sock_fd, &read_set);

        ret = select(Max(STDIN_FILENO, sock_fd)+1, &read_set, NULL, NULL, &tv);

        if(ret < 0) {
            perror("select");
            break;
        }else if(ret > 0){

            if(FD_ISSET(STDIN_FILENO, &read_set)) {
                // fprintf(stderr, "read STDIN_FILENO\n");

                size = read(STDIN_FILENO, buf, sizeof(buf));
                // fprintf(stderr, "%d:0x%x,", size, buf[0]);
                if(write(sock_fd, buf, size)<0){
                    perror("write socket");
                    break;
                }
            }
            if(FD_ISSET(sock_fd, &read_set)) {
                // fprintf(stderr, "read sock_fd\n");
                
                size = read(sock_fd, buf, sizeof(buf));
                if(size<0){
                    perror("read socket");
                    break;
                }
                write(STDOUT_FILENO, buf, size);
            }
        }
    }
    leave_raw_mode(&tio);
    close(sock_fd);

    return 0;
}