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
#include <string.h>

int open_ptym(char **slave_name, int *master_fd)
{
    int fd;
    char *str;
    fd = posix_openpt(O_RDWR);
    if(fd<0) {
        perror("posix_openpt");
        return 0;
    }

    if(grantpt(fd) < 0) {
        perror("grantpt");
        close(fd);
        return 0;
    }
    if(unlockpt(fd) < 0) {
        perror("unlockpt");
        close(fd);
        return 0;
    }

    *slave_name = ptsname(fd);
    str = malloc(strlen(*slave_name) + 1);
    strcpy(str, *slave_name);
    *slave_name = str;
    // *slave_name = "/dev/ttys002";
    if(NULL == *slave_name) {
        perror("ptsname");
        close(fd);
        return 0;
    }
    *master_fd = fd;

    fprintf(stderr, "master: %d, slave_name: %s\n", fd, *slave_name);

    return 1;
}
int open_ptys(char *slave_name, int *slave_fd)
{
    int fd;
    fd = open(slave_name, O_RDWR);
    if(fd < 0) {
        perror("open slave");
        return 0;
    }
    *slave_fd = fd;

    // fprintf(stderr, "slave: %d\n", fd);

    return 1;
}
int open_socket(const char *port, int *server_fd)
{
    struct sockaddr_in server_addr;
    int fd;
    if((fd=socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 0;
    }

    memset((void*)&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(port));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("bind");
        return 0;
    }

    if(listen(fd, 5) < 0) {
        perror("listen");
        return 0;
    }

    *server_fd = fd;

    return 1;
}

int Max(int a, int b)
{
    return a > b ? a : b;
}
void client_loop(int sock_fd)
{
    pid_t child;
    struct termios tio;
    char *slave_name = NULL;
    int master_fd; 

    if(!open_ptym(&slave_name, &master_fd))
        _exit(1);
    if(-1 == tcgetattr(STDIN_FILENO, &tio)) {
        perror("tcgetattr");
        _exit(1);
    }

    child = fork();
    if(child < 0) {
        perror("fork");
        _exit(1);
    }
    if(child == 0){
        int slave_fd;

        setsid();

        close(sock_fd);
        close(master_fd);

        if(!open_ptys(slave_name, &slave_fd))
            _exit(1);

        if(dup2(slave_fd, STDIN_FILENO) != STDIN_FILENO)
            _exit(1);
        if(dup2(slave_fd, STDOUT_FILENO) != STDOUT_FILENO)
            _exit(1);
        if(dup2(slave_fd, STDERR_FILENO) != STDERR_FILENO)
            _exit(1);
        if(-1 == tcsetattr(STDIN_FILENO, TCSADRAIN, &tio))
            _exit(1);

        close(slave_fd);

        chdir("/");
        execlp("bash", "bash", NULL);
        _exit(1);
    }else{
        char buf[256];
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        fprintf(stderr, "child: %d\n", child);

        for(;;) {
            int ret, i, size, status = 0;
            fd_set read_set, write_set;
            FD_ZERO(&read_set);
            FD_ZERO(&write_set);
            FD_SET(sock_fd, &read_set);
            FD_SET(master_fd, &read_set);

            ret = select(Max(sock_fd, master_fd)+1, &read_set, NULL, NULL, &tv);

            if(ret < 0) {
                perror("select");
                break;
            }else if(ret > 0){
                int status;
                if(waitpid(child, &status, WNOHANG) == child){
                    // fprintf(stderr, "status %d\n", status);
                    break;
                }

                if(FD_ISSET(sock_fd, &read_set)) {
                    // fprintf(stderr, "read sock_fd\n");

                    size = read(sock_fd, buf, sizeof(buf));
                    if(size<0){
                        perror("read socket");
                        break;
                    }
                    // fprintf(stderr, "%d:0x%x,", size, buf[0]);
                    write(master_fd, buf, size);
                }
                if(FD_ISSET(master_fd, &read_set)) {
                    // fprintf(stderr, "read master_fd\n");
                    
                    size = read(master_fd, buf, sizeof(buf));
                    if(write(sock_fd, buf, size)<0){
                        perror("write socket");
                        break;
                    }
                }
            }
        }
        kill(child, 9);
    }
    close(master_fd);
    free(slave_name);
}
int main(int argc, char **argv)
{
    int server_fd, client_fd;

    if(argc < 2) {
        printf("usage: simple_ssh_server <port>\n");
        return 1;
    }

    if(!open_socket(argv[1], &server_fd))
        _exit(1);

    for(;;) {
        struct sockaddr_in client_addr;
        socklen_t length = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &length);

        if(client_fd < 0) {
            perror("accept");
            _exit(1);
        }
        fprintf(stderr, "from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        client_loop(client_fd);

        fprintf(stderr, "connection closed\n");

        close(client_fd);
    }

    close(server_fd);

    return 0;
}