#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAGSIZE 16

int main(int argc, char *argv[]) {
    char *xargv[MAGSIZE] = {0};
    int xargc = 0;

    for (int i = 1; i < argc; i++) {
        xargv[xargc] = argv[i];
        xargc++;
    }

    int i = xargc, j = 0;

    char buf = 0;
    while (read(0, &buf, sizeof(char)) == sizeof(char)) {
        if (buf == ' ') {
            xargv[i][j] = '\0';
            if(xargv[i+1]==0)
                xargv[i + 1] = malloc(MAGSIZE * sizeof(char));
            i++;
            j = 0;
        } else if (buf == '\n') {
            xargv[i][j] = '\0';
            xargv[i + 1] = '\0';

            if (fork() == 0) {
                exec(xargv[0], xargv);
            }

            i = xargc;
            j = 0;

        } else {
            if (xargv[i] == 0)
                xargv[i] = malloc(MAGSIZE * sizeof(char));
            xargv[i][j] = buf;
            j++;
        }
    }

    wait(0);
    exit(0);
}