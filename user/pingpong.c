#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    if (argc != 1) {
        fprintf(2, "Usage pingpong:fail...\n");
        exit(1);
    }
    int p2c[2], c2p[2];
    pipe(p2c), pipe(c2p);

    if (fork() == 0) {
        // 子进程到父进程
        char buf;
        read(p2c[0], &buf, sizeof(char));
        close(p2c[0]);

        printf("%d: received ping\n", getpid());
        write(c2p[1], "!", sizeof(char));
        close(c2p[1]);
    } else {
        // 父进程到子进程
        write(p2c[1], "!", sizeof(char));
        close(p2c[1]);

        char buf;
        read(c2p[0], &buf, sizeof(char));
        close(c2p[0]);
        printf("%d: received pong\n", getpid());
    }
    exit(0);
}