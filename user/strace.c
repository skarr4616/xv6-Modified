// To trace the system calls in a user function

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int mask;

    if (argc <= 1) {
        fprintf(2, "Usage: strace mask...\n");
        exit(1);
    }

    mask = atoi(argv[1]);
    trace(mask);

    if (argc <= 2) {
        fprintf(2, "Usage: strace program...\n");
        exit(1);
    }

    if (exec(argv[2], argv + 2) < 1) {
        exit(1);
    }

    exit(0);
}