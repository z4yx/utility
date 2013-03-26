#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>

int open_ptym(char **slave_name, int *master_fd)
{
    int fd;
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
    // *slave_name = "/dev/ttys002";
    if(NULL == *slave_name) {
        perror("ptsname");
        close(fd);
        return 0;
    }
    *master_fd = fd;

    fprintf(stderr, "slave_name: %s\n", *slave_name);

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
int main()
{
    char *slave_name = NULL;
    int master_fd;
    pid_t child;
    struct termios tio;

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

        execlp("bash", "bash", NULL);
        _exit(1);
    }else{
        char buf[256];
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        fprintf(stderr, "child: %d\n", child);

        enter_raw_mode(&tio);

        for(;;) {
            int ret, i, size, status = 0;
            fd_set read_set;
            FD_ZERO(&read_set);
            FD_SET(STDIN_FILENO, &read_set);
            FD_SET(master_fd, &read_set);

            ret = select(Max(STDIN_FILENO, master_fd)+1, &read_set, NULL, NULL, &tv);

            if(ret < 0) {
                perror("select");
                break;
            }else if(ret > 0){
                int status;
                if(waitpid(child, &status, WNOHANG) == child){
                    // fprintf(stderr, "status %d\n", status);
                    if(WIFSTOPPED(status) || WIFSIGNALED(status))
                        kill(child, 9);
                    break;
                }

                if(FD_ISSET(STDIN_FILENO, &read_set)) {
                    // fprintf(stderr, "read STDIN_FILENO\n");

                    size = read(STDIN_FILENO, buf, sizeof(buf));
                    // fprintf(stderr, "%d:0x%x,", size, buf[0]);
                    write(master_fd, buf, size);
                }
                if(FD_ISSET(master_fd, &read_set)) {
                    // fprintf(stderr, "read master_fd\n");
                    
                    size = read(master_fd, buf, sizeof(buf));
                    write(STDOUT_FILENO, buf, size);
                }
            }
        }
        leave_raw_mode(&tio);
    }
    return 0;
}