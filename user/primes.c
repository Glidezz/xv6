#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void primes(int lp[2]) {
    int num;
    if (read(lp[0], &num, sizeof(int)) == sizeof(int)) {
        fprintf(1, "prime %d\n", num);

        int p[2];
        pipe(p);

        int n;
        while (read(lp[0], &n, sizeof(int)) == sizeof(int))
            if (n % num)
                write(p[1], &n, sizeof(int));
        close(lp[0]);
        close(p[1]);

        if (fork() == 0) {
            primes(p);
        } else {
            wait(0);
        }
    } else
        close(lp[0]);
}

int main(int argc, char *argv[]) {
    if (argc != 1) {
        fprintf(2, "Usage primes:fail...\n");
        exit(1);
    }

    int p[2];
    pipe(p);
    for (int i = 2; i <= 35; i++)
        write(p[1], &i, sizeof(int));
    close(p[1]);

    if (fork() == 0) {
        primes(p);
    } else {
        wait(0);
    }
    exit(0);
}